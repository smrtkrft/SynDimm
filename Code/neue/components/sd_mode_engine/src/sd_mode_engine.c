// sd_mode_engine — çekirdek. Port kaynakları: iki-task deseni + coalescing
// device_driver.c:453-529, NVS debounce :174-196.
//
// Kod incelemesi (Faz 2) sonrası tasarım notları:
//   * Kuyruk işleri HAFİF: {slot,kind,value,toggle}. cJSON ağaçları yürütme
//     anında kilit altında üretilir (hot-reload'la yarışmaz; detent başına
//     heap fırtınası yok — coalesce'in düşürdüğü işler bedava).
//   * Coalescing YALNIZ SD_SEND_VALUE'da (iki TOGGLE asla birleşmez —
//     birleşse ışık ters durumda kalırdı).
//   * mode.test "yakalama" modeliyle koşar: drv->test kilit altında işi
//     ÜRETİR, ağ çağrısı kilitsiz yapılır; kuyruk karıştırma yok.
//   * Olay yayınları kilit DIŞINDA (abone, yayıncının task'ında koşar —
//     kilit altında yayın tüm motoru bloke ederdi).
//   * s_offline erişimi eng_lock altında (32-bit'te int64 bölünmüş okuma).
//   * Düşürülen işler job_discard'dan geçer: TEST hatayla TAMAMLANIR
//     (sema verilir), NVS_SAVE timer'ı yeniden kurar.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "sk_capabilities.h"
#include "sk_event_bus.h"
#include "sk_identity.h"
#include "sk_api.h"

#include "sd_buzzer.h"
#include "sd_profiles.h"
#include "sd_util.h"
#include "sd_binding.h"
#include "sd_offline.h"
#include "sd_mode_engine_internal.h"

static const char *TAG = "sd_engine";

slot_ctx_t s_slots[SD_MODE_SLOTS];
bool       s_recovery;

static int                s_active = 1;
static bool               s_started;
static SemaphoreHandle_t  s_lock;            // recursive
static QueueHandle_t      s_queue;           // eng_job_t*
static esp_timer_handle_t s_nvs_timer;
static volatile bool      s_nvs_pending;     // timer kurulu mu (churn önleme)
static sd_offline_t       s_offline;         // erişim: eng_lock altında

// mode.test yakalama durumu — yalnız cmd task, kilit altında yazar;
// sd_ctx_send owner karşılaştırmasıyla yanlış task'ı dışlar.
static struct {
    bool         active;
    TaskHandle_t owner;
    eng_job_t   *captured;
} s_capture;

void eng_lock(void)   { xSemaphoreTakeRecursive(s_lock, portMAX_DELAY); }
void eng_unlock(void) { xSemaphoreGiveRecursive(s_lock); }
int  eng_active_slot(void) { return s_active; }
int  sd_mode_engine_active_slot(void) { return s_active; }

static inline int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

// --- NVS yardımcıları ---------------------------------------------------

static char *nvs_load_raw(const char *key)
{
    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READONLY, &h) != ESP_OK) return NULL;
    size_t len = 0;
    char *buf = NULL;
    if (nvs_get_str(h, key, NULL, &len) == ESP_OK && len > 0 &&
        len <= SD_BINDING_JSON_MAX + 1) {
        buf = malloc(len);
        if (buf && nvs_get_str(h, key, buf, &len) != ESP_OK) {
            free(buf);
            buf = NULL;
        }
    }
    nvs_close(h);
    return buf;
}

static esp_err_t nvs_store_str(const char *key, const char *val)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(SD_ENGINE_NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = val ? nvs_set_str(h, key, val) : nvs_erase_key(h, key);
    if (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

// Kirli değerleri kilit altında ANLIK KOPYALA, NVS yazımını kilitsiz yap.
static void nvs_save_values(void)
{
    struct { int32_t v; uint8_t st; bool dirty; } snap[SD_MODE_SLOTS];
    eng_lock();
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        snap[i].dirty = s_slots[i].nvs_dirty;
        snap[i].v     = s_slots[i].value;
        snap[i].st    = s_slots[i].state ? 1 : 0;
        s_slots[i].nvs_dirty = false;
    }
    eng_unlock();

    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        if (!snap[i].dirty) continue;
        char key[16];
        snprintf(key, sizeof(key), "v%d", i + 1);
        nvs_set_i32(h, key, snap[i].v);
        snprintf(key, sizeof(key), "s%d", i + 1);
        nvs_set_u8(h, key, snap[i].st);
    }
    nvs_commit(h);
    nvs_close(h);
}

// --- Kuyruk işleri --------------------------------------------------------

static eng_job_t *make_job(eng_job_type_t type, int slot, sd_send_kind_t kind,
                           int value, bool toggle)
{
    eng_job_t *job = calloc(1, sizeof(*job));
    if (!job) return NULL;
    job->type   = type;
    job->slot   = (uint8_t)slot;
    job->kind   = kind;
    job->value  = value;
    job->toggle = toggle;
    return job;
}

static void schedule_nvs_save(void);

// Düşürülen iş asla sessizce kaybolmaz: TEST hatayla tamamlanır (bekleyen
// CLI 5 sn timeout yemez), NVS_SAVE debounce'u yeniden kurar.
static void job_discard(eng_job_t *job)
{
    if (!job) return;
    if (job->type == ENG_JOB_TEST && job->done) {
        job->result = ESP_ERR_NO_MEM;
        xSemaphoreGive(job->done);   // sahiplik CLI'ya geçti — free YOK
        return;
    }
    if (job->type == ENG_JOB_NVS_SAVE) {
        free(job);
        s_nvs_pending = false;
        schedule_nvs_save();
        return;
    }
    free(job);
}

// Doluysa en eski SEND'i düşürerek gönderir (SEND-dışı işler korunur).
static esp_err_t enqueue_job(eng_job_t *job)
{
    if (!s_queue) { job_discard(job); return ESP_ERR_INVALID_STATE; }
    for (int attempt = 0; attempt < 2; attempt++) {
        if (xQueueSend(s_queue, &job, 0) == pdTRUE) return ESP_OK;
        eng_job_t *old = NULL;
        if (xQueueReceive(s_queue, &old, 0) != pdTRUE) continue;
        if (old->type == ENG_JOB_SEND) {
            job_discard(old);                     // kurban: en eski SEND
        } else {
            // SEND-dışı işi geri koy (az önce yer açtık; üretici yarışında
            // başarısız olursa güvenli tamamla).
            if (xQueueSend(s_queue, &old, 0) != pdTRUE) job_discard(old);
        }
    }
    job_discard(job);
    return ESP_ERR_NO_MEM;
}

// NVS debounce: 2 sn'lik TEK timer; her detent'te stop/start churn'ü yok
// (kod incelemesi) — kurulu değilse kur, kuruluysa dokunma. Semantik:
// İLK değişiklikten 2 sn sonra kaydet (sınırlı bayatlık, daha az churn).
static void nvs_timer_cb(void *arg)
{
    (void)arg;
    eng_job_t *job = make_job(ENG_JOB_NVS_SAVE, 0, 0, 0, false);
    s_nvs_pending = false;
    if (job) enqueue_job(job);
}

static void schedule_nvs_save(void)
{
    if (s_nvs_pending) return;
    s_nvs_pending = true;
    if (esp_timer_start_once(s_nvs_timer,
                             (uint64_t)SD_ENGINE_NVS_DEBOUNCE_MS * 1000) != ESP_OK) {
        s_nvs_pending = false;   // timer zaten koşuyor olabilir — cb toparlar
    }
}

// --- ctx servisleri ----------------------------------------------------------

int          sd_ctx_slot(const sd_behavior_ctx_t *ctx)    { return ctx->slot; }
const cJSON *sd_ctx_binding(const sd_behavior_ctx_t *ctx) { return ctx->binding; }
const cJSON *sd_ctx_profile(const sd_behavior_ctx_t *ctx) { return ctx->profile; }
void        *sd_ctx_priv(const sd_behavior_ctx_t *ctx)    { return ctx->priv; }
void         sd_ctx_set_priv(sd_behavior_ctx_t *ctx, void *priv) { ctx->priv = priv; }

int sd_ctx_get_value(const sd_behavior_ctx_t *ctx)
{
    eng_lock();
    int v = ctx->value;
    eng_unlock();
    return v;
}

bool sd_ctx_get_state(const sd_behavior_ctx_t *ctx)
{
    eng_lock();
    bool s = ctx->state;
    eng_unlock();
    return s;
}

void sd_ctx_set_value(sd_behavior_ctx_t *ctx, int value, bool state)
{
    eng_lock();
    ctx->value     = value;
    ctx->state     = state;
    ctx->nvs_dirty = true;
    eng_unlock();
    schedule_nvs_save();
    // mode.value olayı gönderim yolunda yayınlanır (≤10 Hz) — detent seli yok.
}

esp_err_t sd_ctx_send(sd_behavior_ctx_t *ctx, sd_send_kind_t kind,
                      int value, bool toggle)
{
    eng_job_t *job = make_job(ENG_JOB_SEND, ctx->slot, kind, value, toggle);
    if (!job) return ESP_ERR_NO_MEM;

    // mode.test yakalama: test'in ürettiği gönderim kuyruğa girmez,
    // execute_test onu senkron koşar (yalnız cmd task + kilit altında).
    if (s_capture.active &&
        s_capture.owner == xTaskGetCurrentTaskHandle()) {
        free(s_capture.captured);          // birden çok send → sonuncusu kalır
        s_capture.captured = job;
        return ESP_OK;
    }
    return enqueue_job(job);
}

esp_err_t sd_ctx_api_send(sd_behavior_ctx_t *ctx, const char *endpoint_name,
                          const char *payload)
{
    (void)ctx;
    return sk_api_send(endpoint_name, payload);   // async, kendi worker'ı var
}

void sd_ctx_beep(sd_behavior_ctx_t *ctx, sd_beep_t pattern)
{
    (void)ctx;
    sd_buzzer_play(pattern);   // prefs+quiet kapıları sd_buzzer'da
}

// --- Slot yükleme / boşaltma ---------------------------------------------------

static void unload_slot(int idx)   // 0-based
{
    slot_ctx_t *ctx = &s_slots[idx];
    if (ctx->drv && ctx->drv->deinit) ctx->drv->deinit(ctx);
    if (ctx->mqtt_held) sd_proto_mqtt_release();
    cJSON_Delete(ctx->binding);
    cJSON_Delete(ctx->profile);
    memset(ctx, 0, sizeof(*ctx));
    ctx->slot = idx + 1;
}

// Slot MQTT istiyorsa oturumu refcount'la. Broker KURULUMA özgüdür (cihaz
// tipine değil): önce binding params.broker, yoksa profilin broker alanı.
// (a) profil protocol=="mqtt", (b) profilsiz mqtt_remote tarzı bağlama
// (params.topic). V1 tek broker — çakışmada slot error.
static esp_err_t acquire_mqtt_if_needed(slot_ctx_t *ctx)
{
    const cJSON *params = cJSON_GetObjectItemCaseSensitive(ctx->binding,
                                                           "params");
    const char *proto = sd_jsonu_str(ctx->profile, "protocol");
    bool wants_mqtt = (proto && strcmp(proto, "mqtt") == 0) ||
                      (!ctx->profile && sd_jsonu_str(params, "topic"));
    if (!wants_mqtt) return ESP_OK;

    const char *broker = sd_jsonu_str(params, "broker");
    int bport          = sd_jsonu_int(params, "broker_port", 0);
    if (!broker) {
        broker = sd_jsonu_str(ctx->profile, "broker");
        if (bport <= 0) bport = sd_jsonu_int(ctx->profile, "broker_port", 1883);
    }
    if (bport <= 0) bport = 1883;
    if (!broker || !broker[0]) {
        return ESP_ERR_INVALID_ARG;   // mqtt slotu broker'sız olamaz
    }

    char uri[128];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", broker, bport);
    esp_err_t err = sd_proto_mqtt_acquire(uri, sk_identity_get());
    if (err == ESP_OK) ctx->mqtt_held = true;
    return err;
}

static void slot_error(int slot, const char *reason)
{
    ESP_LOGE(TAG, "slot %d hata: %s", slot, reason);
    sk_event_bus_publishf("mode.error",
                          "{\"slot\":%d,\"err\":\"%s\",\"count\":0}", slot, reason);
}

// Profil set_value aralığı — motor clamp'i ve sürücü aynı kaynağı okur.
static int profile_range(const slot_ctx_t *ctx, int *out_lo)
{
    int lo = 0, hi = 100;
    if (ctx->profile) {
        const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(ctx->profile, "commands");
        const cJSON *sv = cmds
            ? cJSON_GetObjectItemCaseSensitive(cmds, "set_value") : NULL;
        lo = sd_jsonu_int(sv, "min", 0);
        hi = sd_jsonu_int(sv, "max", 100);
        if (hi <= lo) { lo = 0; hi = 100; }
    }
    if (out_lo) *out_lo = lo;
    return hi;
}

// NVS'ten slotu yükle (kilit altında). Boş slot = OK.
static esp_err_t load_slot(int idx)   // 0-based
{
    unload_slot(idx);
    slot_ctx_t *ctx = &s_slots[idx];

    char key[16];
    snprintf(key, sizeof(key), "slot%d", idx + 1);
    char *raw = nvs_load_raw(key);
    if (!raw) return ESP_OK;   // atanmamış

    cJSON *binding = cJSON_Parse(raw);
    free(raw);
    char verr[48];
    if (!binding || !sd_binding_validate(binding, verr, sizeof(verr))) {
        cJSON_Delete(binding);
        slot_error(idx + 1, binding ? verr : "parse");
        return ESP_FAIL;
    }
    ctx->binding = binding;
    ctx->enabled = sd_jsonu_bool(binding, "enabled", true);
    if (!ctx->enabled) return ESP_OK;   // bağlama saklanır, sürücü yüklenmez

    ctx->drv = sd_behavior_find(sd_jsonu_str(binding, "behavior"));
    if (!ctx->drv) {
        slot_error(idx + 1, "behavior_unknown");
        ctx->enabled = false;
        return ESP_FAIL;
    }

    const char *prof_id = sd_jsonu_str(binding, "profile");
    if (prof_id) {
        ctx->profile = sd_profiles_load(prof_id);
        if (!ctx->profile) {
            slot_error(idx + 1, "profile_missing");
            ctx->drv = NULL;
            ctx->enabled = false;
            return ESP_FAIL;
        }
    }

    const cJSON *targets = cJSON_GetObjectItemCaseSensitive(binding, "targets");
    const cJSON *t0 = targets ? cJSON_GetArrayItem(targets, 0) : NULL;
    if (cJSON_IsObject(t0)) {
        const char *host = sd_jsonu_str(t0, "host");
        const char *did  = sd_jsonu_str(t0, "device_id");
        const char *ak   = sd_jsonu_str(t0, "auth_key");
        if (host) strlcpy(ctx->target.host, host, sizeof(ctx->target.host));
        ctx->target.port = sd_jsonu_int(t0, "port", 0);
        if (did) strlcpy(ctx->target.device_id, did, sizeof(ctx->target.device_id));
        if (ak)  strlcpy(ctx->target.auth_key, ak, sizeof(ctx->target.auth_key));
    }
    if (ctx->target.port <= 0) {
        ctx->target.port = sd_jsonu_int(ctx->profile, "port", 80);
    }

    const cJSON *params = cJSON_GetObjectItemCaseSensitive(binding, "params");
    ctx->gestures_enabled = sd_jsonu_bool(params, "gestures_enabled", true);

    // MQTT yaşam döngüsü: slot yüklenirken acquire, boşalırken release.
    esp_err_t merr = acquire_mqtt_if_needed(ctx);
    if (merr != ESP_OK) {
        slot_error(idx + 1, merr == ESP_ERR_NOT_SUPPORTED
                            ? "mqtt_broker_conflict" : "mqtt_broker");
        ctx->drv = NULL;
        ctx->enabled = false;
        return merr;
    }

    // Kalıcı değer + profil aralığına kırpma (eski cur_value bug fix'i).
    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        char vkey[16];
        int32_t v = 0;
        uint8_t st = 0;
        snprintf(vkey, sizeof(vkey), "v%d", idx + 1);
        nvs_get_i32(h, vkey, &v);
        snprintf(vkey, sizeof(vkey), "s%d", idx + 1);
        nvs_get_u8(h, vkey, &st);
        nvs_close(h);
        int lo;
        int hi = profile_range(ctx, &lo);
        ctx->value = (v < lo) ? lo : ((v > hi) ? hi : (int)v);
        ctx->state = st != 0;
    }

    esp_err_t err = ctx->drv->init(ctx);
    if (err != ESP_OK) {
        slot_error(idx + 1, "drv_init");
        ctx->drv = NULL;
        ctx->enabled = false;
        return err;
    }

    ESP_LOGI(TAG, "slot %d yuklendi: %s%s%s", idx + 1, ctx->drv->id,
             prof_id ? " + " : "", prof_id ? prof_id : "");
    return ESP_OK;
}

static const char *slot_display_name(const slot_ctx_t *ctx)
{
    const char *n = sd_jsonu_str(ctx->binding, "name");
    if (n && n[0]) return n;
    const char *b = sd_jsonu_str(ctx->binding, "behavior");
    return b ? b : "empty";
}

void eng_set_active(int slot, bool persist)
{
    char evt[80];
    evt[0] = '\0';
    eng_lock();
    if (slot >= 1 && slot <= SD_MODE_SLOTS && slot != s_active) {
        s_active = slot;
        if (persist) {
            nvs_handle_t h;
            if (nvs_open(SD_ENGINE_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
                nvs_set_u8(h, "active", (uint8_t)slot);
                nvs_commit(h);
                nvs_close(h);
            }
        }
        snprintf(evt, sizeof(evt), "{\"slot\":%d,\"name\":\"%s\"}",
                 slot, slot_display_name(&s_slots[slot - 1]));
    }
    eng_unlock();
    if (evt[0]) sk_event_bus_publish("mode.changed", evt);   // kilit DIŞI
}

// --- Girdi yolu ------------------------------------------------------------------

void sd_mode_engine_input(const sd_input_event_t *evt)
{
    if (!s_started || !evt) return;
    eng_lock();
    slot_ctx_t *ctx = &s_slots[s_active - 1];
    if (!ctx->drv || !ctx->enabled || ctx->error) {
        eng_unlock();
        return;
    }
    // Jest kapılaması sd_encoder'daki TEK etkin bayrakta —
    // burada ikinci kapı yok (kod incelemesi: katman karışıklığı).
    esp_err_t err = ctx->drv->on_input(ctx, evt);
    bool tripped = false;
    int  count = 0, slot = ctx->slot;
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        if (++ctx->err_count >= SD_ENGINE_ERR_LIMIT && !ctx->error) {
            ctx->error = true;
            tripped = true;
            count = ctx->err_count;
        }
    } else {
        ctx->err_count = 0;
    }
    eng_unlock();
    if (tripped) {
        sk_event_bus_publishf("mode.error",
                              "{\"slot\":%d,\"err\":\"driver\",\"count\":%d}",
                              slot, count);
    }
}

void sd_mode_engine_slot_step(int dir)
{
    if (!s_started) return;
    int target = s_active + (dir > 0 ? 1 : -1);
    if (target < 1 || target > SD_MODE_SLOTS) return;   // sınırlı, sarmasız
    eng_set_active(target, true);
}

bool sd_mode_engine_active_gestures(void)
{
    if (!s_started) return true;
    eng_lock();
    bool g = s_slots[s_active - 1].binding
        ? s_slots[s_active - 1].gestures_enabled : true;
    eng_unlock();
    return g;
}

// --- CLI yardımcı yolları ------------------------------------------------------

esp_err_t eng_set_value_direct(int slot, int value)
{
    eng_lock();
    slot_ctx_t *ctx = &s_slots[slot - 1];
    if (!ctx->drv || !ctx->enabled) { eng_unlock(); return ESP_ERR_INVALID_STATE; }
    int lo;
    int hi = profile_range(ctx, &lo);
    if (value < lo) value = lo;
    if (value > hi) value = hi;
    ctx->value = value;
    ctx->state = value > lo;
    ctx->nvs_dirty = true;
    bool state = ctx->state;
    eng_unlock();
    schedule_nvs_save();
    eng_job_t *job = make_job(ENG_JOB_SEND, slot, SD_SEND_VALUE, value, state);
    if (!job) return ESP_ERR_NO_MEM;
    return enqueue_job(job);
}

esp_err_t eng_toggle_direct(int slot)
{
    eng_lock();
    slot_ctx_t *ctx = &s_slots[slot - 1];
    if (!ctx->drv || !ctx->enabled) { eng_unlock(); return ESP_ERR_INVALID_STATE; }
    ctx->state = !ctx->state;
    ctx->nvs_dirty = true;
    int value = ctx->value;
    bool state = ctx->state;
    eng_unlock();
    schedule_nvs_save();
    eng_job_t *job = make_job(ENG_JOB_SEND, slot, SD_SEND_TOGGLE, value, state);
    if (!job) return ESP_ERR_NO_MEM;
    return enqueue_job(job);
}

esp_err_t eng_store_and_reload(int slot, const char *compact_json)
{
    char key[16];
    snprintf(key, sizeof(key), "slot%d", slot);
    esp_err_t err = nvs_store_str(key, compact_json);
    if (err != ESP_OK) return err;
    if (s_recovery || !s_started) return ESP_OK;   // recovery: yalnız kalıcılaştır
    eng_lock();
    err = load_slot(slot - 1);
    eng_unlock();
    return err;
}

esp_err_t eng_clear_slot(int slot)
{
    char key[16];
    snprintf(key, sizeof(key), "slot%d", slot);
    esp_err_t err = nvs_store_str(key, NULL);
    if (s_recovery || !s_started) return err;
    eng_lock();
    unload_slot(slot - 1);
    eng_unlock();
    return err;
}

// --- Yürütme (yalnız cmd task) ----------------------------------------------

// İşi kilit altında somutlaştır: profil komut düğümü kopyası + mini profil +
// hedef. Slot bu arada değiştiyse en GÜNCEL bağlama kullanılır (istenen).
// Profilsiz mqtt_remote tarzı bağlamalar için komut params'tan SENTEZLENİR:
// VALUE → params.payload_value, TOGGLE → params.payload_gesture.
static bool materialize(const eng_job_t *job, cJSON **out_mini,
                        cJSON **out_cmd, sd_target_t *out_tgt)
{
    static const char *KIND_NODE[] = {
        [SD_SEND_VALUE]  = "set_value",
        [SD_SEND_TOGGLE] = "toggle",
        [SD_SEND_STOP]   = "stop",
    };
    bool ok = false;
    eng_lock();
    slot_ctx_t *ctx = &s_slots[job->slot - 1];
    if (ctx->drv && ctx->enabled && ctx->profile) {
        const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(ctx->profile, "commands");
        const cJSON *node = cmds
            ? cJSON_GetObjectItemCaseSensitive(cmds, KIND_NODE[job->kind]) : NULL;
        if (cJSON_IsObject(node)) {
            *out_cmd  = cJSON_Duplicate(node, true);
            *out_mini = cJSON_CreateObject();
            const char *proto  = sd_jsonu_str(ctx->profile, "protocol");
            const char *prefix = sd_jsonu_str(ctx->profile, "prefix");
            const char *stpfx  = sd_jsonu_str(ctx->profile, "state_prefix");
            if (*out_mini && proto)  cJSON_AddStringToObject(*out_mini, "protocol", proto);
            if (*out_mini && prefix) cJSON_AddStringToObject(*out_mini, "prefix", prefix);
            if (*out_mini && stpfx)  cJSON_AddStringToObject(*out_mini, "state_prefix", stpfx);
            *out_tgt = ctx->target;
            ok = (*out_cmd && *out_mini);
        }
    } else if (ctx->drv && ctx->enabled && !ctx->profile) {
        const cJSON *params = cJSON_GetObjectItemCaseSensitive(ctx->binding,
                                                               "params");
        const char *topic   = sd_jsonu_str(params, "topic");
        const char *payload = sd_jsonu_str(params,
            job->kind == SD_SEND_VALUE ? "payload_value" : "payload_gesture");
        if (topic && payload) {
            *out_cmd  = cJSON_CreateObject();
            *out_mini = cJSON_CreateObject();
            if (*out_cmd && *out_mini) {
                cJSON_AddStringToObject(*out_cmd, "topic", topic);
                cJSON_AddStringToObject(*out_cmd, "payload", payload);
                cJSON_AddStringToObject(*out_mini, "protocol", "mqtt");
                *out_tgt = ctx->target;
                ok = true;
            }
        }
    }
    eng_unlock();
    if (!ok) {
        cJSON_Delete(*out_cmd);
        cJSON_Delete(*out_mini);
        *out_cmd = *out_mini = NULL;
    }
    return ok;
}

static void publish_mode_value(int slot)
{
    char buf[80];
    eng_lock();
    slot_ctx_t *ctx = &s_slots[slot - 1];
    snprintf(buf, sizeof(buf), "{\"slot\":%d,\"value\":%d,\"state\":%s}",
             slot, ctx->value, ctx->state ? "true" : "false");
    eng_unlock();
    sk_event_bus_publish("mode.value", buf);   // kilit DIŞI yayın
}

// Uzlaştırma: slotun son değerini bir kez gönder (online dönüşü / got-ip).
static void reconcile_slot(int idx0)
{
    eng_lock();
    slot_ctx_t *ctx = &s_slots[idx0];
    eng_job_t *job = (ctx->drv && ctx->enabled && ctx->profile)
        ? make_job(ENG_JOB_SEND, idx0 + 1, SD_SEND_VALUE, ctx->value, ctx->state)
        : NULL;
    eng_unlock();
    if (job) enqueue_job(job);
}

static void offline_feed(int slot, bool transport_ok)
{
    eng_lock();
    sd_offline_evt_t evt = sd_offline_feed(&s_offline, slot - 1,
                                           transport_ok, now_ms());
    eng_unlock();
    if (evt == SD_OFFLINE_WENT) {
        sk_event_bus_publishf("target.offline", "{\"slot\":%d}", slot);
    } else if (evt == SD_OFFLINE_CAME) {
        sk_event_bus_publishf("target.online", "{\"slot\":%d}", slot);
        reconcile_slot(slot - 1);
    }
}

// Gönderimi yürüt; dönüş = transport sonucu, out_status = HTTP kodu.
static esp_err_t execute_send(const eng_job_t *job, int *out_status)
{
    if (out_status) *out_status = 0;

    eng_lock();
    bool send_now = sd_offline_should_send(&s_offline, job->slot - 1, now_ms());
    eng_unlock();
    if (!send_now) {
        // Offline probe kapısı kapalı: yerel değer zaten güncel, ağ atlanır.
        publish_mode_value(job->slot);
        return ESP_OK;
    }

    cJSON *mini = NULL, *cmd = NULL;
    sd_target_t tgt;
    if (!materialize(job, &mini, &cmd, &tgt)) {
        publish_mode_value(job->slot);
        return ESP_ERR_NOT_FOUND;   // slot bu arada boşaldı/profilsiz
    }

    int status = 0;
    esp_err_t err = sd_proto_execute(mini, cmd, &tgt, job->value, job->toggle,
                                     &status);
    cJSON_Delete(mini);
    cJSON_Delete(cmd);
    if (out_status) *out_status = status;

    offline_feed(job->slot, err == ESP_OK);
    publish_mode_value(job->slot);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "slot %d gonderim hatasi: %s",
                 job->slot, esp_err_to_name(err));
    }
    return err;
}

// mode.test: drv->test kilit altında SADECE iş üretir (yakalama modu),
// ağ çağrısı kilitsiz koşar. Kuyruk karıştırma yok (kod incelemesi).
static void execute_test(eng_job_t *job)
{
    eng_job_t *captured = NULL;
    esp_err_t err;

    eng_lock();
    slot_ctx_t *ctx = &s_slots[job->slot - 1];
    if (!ctx->drv || !ctx->enabled || !ctx->drv->test) {
        err = ESP_ERR_INVALID_STATE;
    } else {
        s_capture.active   = true;
        s_capture.owner    = xTaskGetCurrentTaskHandle();
        s_capture.captured = NULL;
        err = ctx->drv->test(ctx);          // bloklamaz: iş üretir
        captured           = s_capture.captured;
        s_capture.active   = false;
        s_capture.captured = NULL;
    }
    eng_unlock();

    if (err == ESP_OK && captured) {
        err = execute_send(captured, &job->status);   // kilitsiz ağ çağrısı
        free(captured);
    }
    job->result = err;
    xSemaphoreGive(job->done);   // sahiplik CLI'ya geçti — job'a DOKUNMA
}

esp_err_t eng_run_test(int slot, int *out_status, int timeout_ms)
{
    eng_job_t *job = make_job(ENG_JOB_TEST, slot, SD_SEND_VALUE, 0, false);
    if (!job) return ESP_ERR_NO_MEM;
    job->done = xSemaphoreCreateBinary();
    if (!job->done) { free(job); return ESP_ERR_NO_MEM; }

    SemaphoreHandle_t done = job->done;
    esp_err_t enq = enqueue_job(job);   // başarısızsa job_discard sema verdi
    if (enq != ESP_OK && xSemaphoreTake(done, 0) != pdTRUE) {
        // enqueue_job içeride discard etti; sema verilmiş olmalı — değilse
        // güvenli tarafta kal, sızıntıyı kabul et.
        return enq;
    }
    if (enq == ESP_OK && xSemaphoreTake(done, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        // Zaman aşımı: cmd task hâlâ yazabilir — job KASITLI sızdırılır
        // (tek blok; use-after-free'den iyidir). Nadir yol, log'lanır.
        ESP_LOGW(TAG, "mode.test %d zaman asimi (is blogu birakildi)", slot);
        return ESP_ERR_TIMEOUT;
    }
    esp_err_t err = job->result;
    if (out_status) *out_status = job->status;
    vSemaphoreDelete(done);
    free(job);
    return err;
}

// --- sd_cmd task: tek dispatch noktası + VALUE coalescing ---------------------

static void dispatch_job(eng_job_t *job)
{
    switch (job->type) {
        case ENG_JOB_NVS_SAVE:
            nvs_save_values();
            free(job);
            break;
        case ENG_JOB_TEST:
            execute_test(job);   // free CLI tarafında (sema sahipliği)
            break;
        case ENG_JOB_SEND:
        default:
            execute_send(job, NULL);
            free(job);
            break;
    }
}

static void cmd_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "sd_cmd task basladi");
    eng_job_t *job = NULL;
    for (;;) {
        if (!job && xQueueReceive(s_queue, &job, portMAX_DELAY) != pdTRUE) {
            continue;
        }
        eng_job_t *carry = NULL;
        if (job->type == ENG_JOB_SEND && job->kind == SD_SEND_VALUE) {
            // Drain-keep-latest — YALNIZ VALUE (iki TOGGLE asla birleşmez).
            eng_job_t *next = NULL;
            while (xQueueReceive(s_queue, &next,
                                 pdMS_TO_TICKS(SD_ENGINE_COALESCE_MS)) == pdTRUE) {
                if (next->type == ENG_JOB_SEND &&
                    next->kind == SD_SEND_VALUE && next->slot == job->slot) {
                    free(job);
                    job = next;          // en yenisini tut (hafif iş — bedava)
                } else {
                    carry = next;        // farklı iş: sonra işle
                    break;
                }
            }
        }
        dispatch_job(job);
        job = carry;                     // tek dispatch noktası (tekrar yok)
    }
}

// --- WiFi / factory reset / init ------------------------------------------------

static void on_wifi_state(const sk_event_t *evt, void *user)
{
    (void)user;
    if (!evt || !evt->payload_json) return;
    if (strstr(evt->payload_json, "\"state\":\"connected\"")) {
        eng_lock();
        sd_offline_wifi_up(&s_offline);
        eng_unlock();
        for (int i = 0; i < SD_MODE_SLOTS; i++) reconcile_slot(i);
    } else if (strstr(evt->payload_json, "\"state\":\"disconnected\"")) {
        eng_lock();
        uint32_t newly = sd_offline_wifi_down(&s_offline);
        eng_unlock();
        for (int i = 0; i < SD_MODE_SLOTS; i++) {
            if (newly & (1u << i)) {
                sk_event_bus_publishf("target.offline", "{\"slot\":%d}", i + 1);
            }
        }
    }
}

static void on_factory_reset(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    // Debounce'u durdur + kirli bayrakları temizle ki silme SONRASI bir
    // NVS_SAVE değerleri geri yazmasın (cihaz ~500 ms içinde restart olur).
    eng_lock();
    esp_timer_stop(s_nvs_timer);
    s_nvs_pending = false;
    for (int i = 0; i < SD_MODE_SLOTS; i++) s_slots[i].nvs_dirty = false;
    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    eng_unlock();
    ESP_LOGW(TAG, "factory reset: mod slotlari silindi");
}

static int profile_ref_count(const char *id)
{
    int refs = 0;
    eng_lock();
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        if (!s_slots[i].binding || !s_slots[i].enabled) continue;
        const char *p = sd_jsonu_str(s_slots[i].binding, "profile");
        if (p && strcmp(p, id) == 0) refs++;
    }
    eng_unlock();
    return refs;
}

esp_err_t sd_mode_engine_init(bool recovery)
{
    s_recovery = recovery;
    s_lock = xSemaphoreCreateRecursiveMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    for (int i = 0; i < SD_MODE_SLOTS; i++) s_slots[i].slot = i + 1;

    const esp_timer_create_args_t targs = {
        .callback = nvs_timer_cb,
        .name     = "sd_nvs_save",
    };
    esp_err_t err = esp_timer_create(&targs, &s_nvs_timer);
    if (err != ESP_OK) return err;

    eng_cli_register();

    int sub;
    sk_event_bus_subscribe("device.factory-reset.requested",
                           on_factory_reset, NULL, &sub);
    sd_profiles_set_ref_checker(profile_ref_count);

    sk_capabilities_register_book("sd_mode_engine", "1.1.0");
    ESP_LOGI(TAG, "init (recovery=%d)", recovery);
    return ESP_OK;
}

esp_err_t sd_mode_engine_start(void)
{
    if (s_recovery) return ESP_ERR_INVALID_STATE;

    s_queue = xQueueCreate(SD_ENGINE_QUEUE_LEN, sizeof(eng_job_t *));
    if (!s_queue) return ESP_ERR_NO_MEM;

    nvs_handle_t h;
    uint8_t active = 1;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, "active", &active);
        nvs_close(h);
    }
    if (active < 1 || active > SD_MODE_SLOTS) active = 1;
    s_active = active;

    sd_offline_init(&s_offline, SD_MODE_SLOTS);
    int sub;
    sk_event_bus_subscribe("wifi.state", on_wifi_state, NULL, &sub);

    eng_lock();
    for (int i = 0; i < SD_MODE_SLOTS; i++) load_slot(i);
    eng_unlock();

    BaseType_t r = xTaskCreate(cmd_task, "sd_cmd", 6144, NULL, 4, NULL);
    if (r != pdPASS) return ESP_FAIL;

    s_started = true;
    ESP_LOGI(TAG, "start: aktif slot %d", s_active);
    return ESP_OK;
}
