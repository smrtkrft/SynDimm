// sd_mode_engine — çekirdek. Port kaynakları: iki-task deseni + coalescing
// device_driver.c:453-529, NVS debounce :174-196. Yenilikler: kendine-yeterli
// kuyruk işleri (hot-reload ↔ ağ yarışı yok), slot-hata politikası,
// recursive kilit, olay yayınları.

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
#include "sk_api.h"

#include "sd_buzzer.h"
#include "sd_profiles.h"
#include "sd_binding.h"
#include "sd_offline.h"
#include "sd_mode_engine_internal.h"

static const char *TAG = "sd_engine";

static sd_offline_t s_offline;   // erişim: yalnız cmd task + wifi handler
                                 //   (feed/should_send cmd task; wifi_down/up
                                 //   handler — alanlar bağımsız, yarış zararsız
                                 //   düzeyde [sayaç sıfırlama], kabul edildi)

slot_ctx_t s_slots[SD_MODE_SLOTS];
bool       s_recovery;

static int                s_active = 1;
static bool               s_started;
static SemaphoreHandle_t  s_lock;          // recursive
static QueueHandle_t      s_queue;         // eng_job_t*
static esp_timer_handle_t s_nvs_timer;

void eng_lock(void)   { xSemaphoreTakeRecursive(s_lock, portMAX_DELAY); }
void eng_unlock(void) { xSemaphoreGiveRecursive(s_lock); }
int  eng_active_slot(void) { return s_active; }
int  sd_mode_engine_active_slot(void) { return s_active; }

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

static void nvs_save_values(void)
{
    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    eng_lock();
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        if (!s_slots[i].nvs_dirty) continue;
        char key[12];
        snprintf(key, sizeof(key), "v%d", i + 1);
        nvs_set_i32(h, key, s_slots[i].value);
        snprintf(key, sizeof(key), "s%d", i + 1);
        nvs_set_u8(h, key, s_slots[i].state ? 1 : 0);
        s_slots[i].nvs_dirty = false;
    }
    eng_unlock();
    nvs_commit(h);
    nvs_close(h);
}

// --- Kuyruk işleri --------------------------------------------------------

static void job_free(eng_job_t *job)
{
    if (!job) return;
    cJSON_Delete(job->mini_profile);
    cJSON_Delete(job->cmd_node);
    free(job);
}

// Doluysa en eskiyi düşürerek gönderir (device_driver.c:510-521 deseni).
static esp_err_t enqueue_job(eng_job_t *job)
{
    if (!s_queue) { job_free(job); return ESP_ERR_INVALID_STATE; }
    if (xQueueSend(s_queue, &job, 0) == pdTRUE) return ESP_OK;
    eng_job_t *old = NULL;
    if (xQueueReceive(s_queue, &old, 0) == pdTRUE) job_free(old);
    if (xQueueSend(s_queue, &job, 0) == pdTRUE) return ESP_OK;
    job_free(job);
    return ESP_ERR_NO_MEM;
}

// NVS debounce zamanlayıcısı: yalnız kuyruğa iş atar (timer-daemon'da NVS yok).
static void nvs_timer_cb(void *arg)
{
    (void)arg;
    eng_job_t *job = calloc(1, sizeof(*job));
    if (!job) return;
    job->type = ENG_JOB_NVS_SAVE;
    enqueue_job(job);
}

static void schedule_nvs_save(void)
{
    esp_timer_stop(s_nvs_timer);
    esp_timer_start_once(s_nvs_timer, (uint64_t)SD_ENGINE_NVS_DEBOUNCE_MS * 1000);
}

// SEND/TEST işi kur: profil komut düğümünü kopyala (kendine yeterli iş).
// Kilit altında çağrılır.
static eng_job_t *build_send_job(slot_ctx_t *ctx, eng_job_type_t type,
                                 sd_send_kind_t kind, int value, bool toggle)
{
    static const char *KIND_NODE[] = {
        [SD_SEND_VALUE]  = "set_value",
        [SD_SEND_TOGGLE] = "toggle",
        [SD_SEND_STOP]   = "stop",
    };
    if (!ctx->profile) return NULL;
    const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(ctx->profile, "commands");
    const cJSON *node = cmds
        ? cJSON_GetObjectItemCaseSensitive(cmds, KIND_NODE[kind]) : NULL;
    if (!cJSON_IsObject(node)) return NULL;

    eng_job_t *job = calloc(1, sizeof(*job));
    if (!job) return NULL;
    job->type   = type;
    job->slot   = (uint8_t)ctx->slot;
    job->kind   = kind;
    job->value  = value;
    job->toggle = toggle;
    job->target = ctx->target;

    job->mini_profile = cJSON_CreateObject();
    const cJSON *proto  = cJSON_GetObjectItemCaseSensitive(ctx->profile, "protocol");
    const cJSON *prefix = cJSON_GetObjectItemCaseSensitive(ctx->profile, "prefix");
    if (cJSON_IsString(proto)) {
        cJSON_AddStringToObject(job->mini_profile, "protocol", proto->valuestring);
    }
    if (cJSON_IsString(prefix)) {
        cJSON_AddStringToObject(job->mini_profile, "prefix", prefix->valuestring);
    }
    job->cmd_node = cJSON_Duplicate(node, true);
    if (!job->mini_profile || !job->cmd_node) {
        job_free(job);
        return NULL;
    }
    return job;
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
    // mode.value olayı burada YAYINLANMAZ (detent seli) — cmd task,
    // coalesce edilmiş gönderim sonrası yayınlar (≤10 Hz).
}

esp_err_t sd_ctx_send(sd_behavior_ctx_t *ctx, sd_send_kind_t kind,
                      int value, bool toggle)
{
    eng_lock();
    eng_job_t *job = build_send_job(ctx, ENG_JOB_SEND, kind, value, toggle);
    eng_unlock();
    if (!job) return ESP_ERR_NOT_FOUND;   // profil/komut düğümü yok
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
    cJSON_Delete(ctx->binding);
    cJSON_Delete(ctx->profile);
    memset(ctx, 0, sizeof(*ctx));
    ctx->slot = idx + 1;
}

static void slot_error(int slot, const char *reason)
{
    ESP_LOGE(TAG, "slot %d hata: %s", slot, reason);
    sk_event_bus_publishf("mode.error",
                          "{\"slot\":%d,\"err\":\"%s\",\"count\":0}", slot, reason);
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

    const cJSON *en = cJSON_GetObjectItemCaseSensitive(binding, "enabled");
    ctx->enabled = cJSON_IsBool(en) ? cJSON_IsTrue(en) : true;
    if (!ctx->enabled) return ESP_OK;   // bağlama saklanır, sürücü yüklenmez

    const cJSON *beh = cJSON_GetObjectItemCaseSensitive(binding, "behavior");
    ctx->drv = sd_behavior_find(beh->valuestring);
    if (!ctx->drv) {
        slot_error(idx + 1, "behavior_unknown");
        ctx->enabled = false;
        return ESP_FAIL;
    }

    const cJSON *prof = cJSON_GetObjectItemCaseSensitive(binding, "profile");
    if (cJSON_IsString(prof)) {
        ctx->profile = sd_profiles_load(prof->valuestring);
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
        const cJSON *host = cJSON_GetObjectItemCaseSensitive(t0, "host");
        const cJSON *port = cJSON_GetObjectItemCaseSensitive(t0, "port");
        const cJSON *did  = cJSON_GetObjectItemCaseSensitive(t0, "device_id");
        const cJSON *ak   = cJSON_GetObjectItemCaseSensitive(t0, "auth_key");
        if (cJSON_IsString(host)) {
            strlcpy(ctx->target.host, host->valuestring, sizeof(ctx->target.host));
        }
        ctx->target.port = cJSON_IsNumber(port) ? port->valueint : 80;
        if (cJSON_IsString(did)) {
            strlcpy(ctx->target.device_id, did->valuestring,
                    sizeof(ctx->target.device_id));
        }
        if (cJSON_IsString(ak)) {
            strlcpy(ctx->target.auth_key, ak->valuestring,
                    sizeof(ctx->target.auth_key));
        }
    }
    // Profildeki varsayılan port'u hedef port belirtilmemişse kullan.
    if (ctx->target.port <= 0 && ctx->profile) {
        const cJSON *pport = cJSON_GetObjectItemCaseSensitive(ctx->profile, "port");
        ctx->target.port = cJSON_IsNumber(pport) ? pport->valueint : 80;
    }

    const cJSON *params = cJSON_GetObjectItemCaseSensitive(binding, "params");
    const cJSON *ge = params
        ? cJSON_GetObjectItemCaseSensitive(params, "gestures_enabled") : NULL;
    ctx->gestures_enabled = cJSON_IsBool(ge) ? cJSON_IsTrue(ge) : true;

    // Kalıcı değer önbelleği + profil aralığına kırpma (eski cur_value
    // bug'ının düzeltmesi — device_driver.c:660 kırpmıyordu).
    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        char vkey[12];
        int32_t v = 0;
        uint8_t st = 0;
        snprintf(vkey, sizeof(vkey), "v%d", idx + 1);
        nvs_get_i32(h, vkey, &v);
        snprintf(vkey, sizeof(vkey), "s%d", idx + 1);
        nvs_get_u8(h, vkey, &st);
        nvs_close(h);
        int lo = 0, hi = 100;
        if (ctx->profile) {
            const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(ctx->profile, "commands");
            const cJSON *sv = cmds
                ? cJSON_GetObjectItemCaseSensitive(cmds, "set_value") : NULL;
            const cJSON *mn = sv ? cJSON_GetObjectItemCaseSensitive(sv, "min") : NULL;
            const cJSON *mx = sv ? cJSON_GetObjectItemCaseSensitive(sv, "max") : NULL;
            if (cJSON_IsNumber(mn)) lo = mn->valueint;
            if (cJSON_IsNumber(mx)) hi = mx->valueint;
            if (hi <= lo) { lo = 0; hi = 100; }
        }
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
             ctx->profile ? " + " : "",
             ctx->profile
                 ? cJSON_GetStringValue(
                       cJSON_GetObjectItemCaseSensitive(ctx->profile, "id"))
                 : "");
    return ESP_OK;
}

static const char *slot_display_name(const slot_ctx_t *ctx)
{
    if (ctx->binding) {
        const cJSON *n = cJSON_GetObjectItemCaseSensitive(ctx->binding, "name");
        if (cJSON_IsString(n) && n->valuestring[0]) return n->valuestring;
        const cJSON *b = cJSON_GetObjectItemCaseSensitive(ctx->binding, "behavior");
        if (cJSON_IsString(b)) return b->valuestring;
    }
    return "empty";
}

void eng_set_active(int slot, bool persist)
{
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
        sk_event_bus_publishf("mode.changed", "{\"slot\":%d,\"name\":\"%s\"}",
                              slot, slot_display_name(&s_slots[slot - 1]));
    }
    eng_unlock();
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
    if ((evt->type == SD_INPUT_DOUBLE_CLICK || evt->type == SD_INPUT_LONG_PRESS) &&
        !ctx->gestures_enabled) {
        eng_unlock();
        return;   // bağlama bazında jest kapalı — yok say
    }
    esp_err_t err = ctx->drv->on_input(ctx, evt);
    if (err != ESP_OK && err != ESP_ERR_NOT_FOUND) {
        if (++ctx->err_count >= SD_ENGINE_ERR_LIMIT) {
            ctx->error = true;
            sk_event_bus_publishf("mode.error",
                                  "{\"slot\":%d,\"err\":\"driver\",\"count\":%d}",
                                  ctx->slot, ctx->err_count);
        }
    } else {
        ctx->err_count = 0;
    }
    eng_unlock();
}

void sd_mode_engine_slot_step(int dir)
{
    if (!s_started) return;
    int target = s_active + (dir > 0 ? 1 : -1);
    if (target < 1 || target > SD_MODE_SLOTS) return;   // sınırlı, sarmasız
    eng_set_active(target, true);
}

// --- CLI yardımcı yolları (sd_mode_cli.c çağırır) ------------------------------

static int profile_range(const slot_ctx_t *ctx, int *out_lo)
{
    int lo = 0, hi = 100;
    if (ctx->profile) {
        const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(ctx->profile, "commands");
        const cJSON *sv = cmds
            ? cJSON_GetObjectItemCaseSensitive(cmds, "set_value") : NULL;
        const cJSON *mn = sv ? cJSON_GetObjectItemCaseSensitive(sv, "min") : NULL;
        const cJSON *mx = sv ? cJSON_GetObjectItemCaseSensitive(sv, "max") : NULL;
        if (cJSON_IsNumber(mn)) lo = mn->valueint;
        if (cJSON_IsNumber(mx)) hi = mx->valueint;
        if (hi <= lo) { lo = 0; hi = 100; }
    }
    if (out_lo) *out_lo = lo;
    return hi;
}

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
    eng_job_t *job = build_send_job(ctx, ENG_JOB_SEND, SD_SEND_VALUE,
                                    value, ctx->state);
    eng_unlock();
    schedule_nvs_save();
    if (!job) return ESP_ERR_NOT_FOUND;
    return enqueue_job(job);
}

esp_err_t eng_toggle_direct(int slot)
{
    eng_lock();
    slot_ctx_t *ctx = &s_slots[slot - 1];
    if (!ctx->drv || !ctx->enabled) { eng_unlock(); return ESP_ERR_INVALID_STATE; }
    ctx->state = !ctx->state;
    ctx->nvs_dirty = true;
    eng_job_t *job = build_send_job(ctx, ENG_JOB_SEND, SD_SEND_TOGGLE,
                                    ctx->value, ctx->state);
    eng_unlock();
    schedule_nvs_save();
    if (!job) return ESP_ERR_NOT_FOUND;
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

esp_err_t eng_run_test(int slot, int *out_status, int timeout_ms)
{
    eng_lock();
    slot_ctx_t *ctx = &s_slots[slot - 1];
    if (!ctx->drv || !ctx->enabled || !ctx->drv->test) {
        eng_unlock();
        return ESP_ERR_NOT_SUPPORTED;
    }
    // Test işi: drv->test cmd task'ta koşsun diye kuyruklanır.
    eng_job_t *job = calloc(1, sizeof(*job));
    if (!job) { eng_unlock(); return ESP_ERR_NO_MEM; }
    job->type = ENG_JOB_TEST;
    job->slot = (uint8_t)slot;
    job->done = xSemaphoreCreateBinary();
    job->out_err    = calloc(1, sizeof(esp_err_t));
    job->out_status = calloc(1, sizeof(int));
    eng_unlock();
    if (!job->done || !job->out_err || !job->out_status) {
        if (job->done) vSemaphoreDelete(job->done);
        free(job->out_err); free(job->out_status); free(job);
        return ESP_ERR_NO_MEM;
    }
    SemaphoreHandle_t done = job->done;
    esp_err_t *perr = job->out_err;
    int       *pst  = job->out_status;

    esp_err_t err = enqueue_job(job);
    if (err != ESP_OK) {
        vSemaphoreDelete(done); free(perr); free(pst);
        return err;
    }
    if (xSemaphoreTake(done, pdMS_TO_TICKS(timeout_ms)) != pdTRUE) {
        // Zaman aşımı: cmd task hâlâ yazabilir — yapıları KASITLI sızdır
        // (nadir yol, ~40B; use-after-free'den iyidir). done'ı cmd task verir.
        ESP_LOGW(TAG, "mode.test %d zaman asimi (yapilar birakildi)", slot);
        return ESP_ERR_TIMEOUT;
    }
    err = *perr;
    if (out_status) *out_status = *pst;
    vSemaphoreDelete(done);
    free(perr); free(pst);
    return err;
}

// --- Offline izleyici entegrasyonu (T2.6) ----------------------------------------

static inline int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

// Uzlaştırma: slotun son değerini bir kez gönder (online dönüşü / got-ip).
static void reconcile_slot(int idx0)
{
    eng_lock();
    slot_ctx_t *ctx = &s_slots[idx0];
    eng_job_t *job = (ctx->drv && ctx->enabled && ctx->profile)
        ? build_send_job(ctx, ENG_JOB_SEND, SD_SEND_VALUE, ctx->value, ctx->state)
        : NULL;
    eng_unlock();
    if (job) enqueue_job(job);
}

void eng_offline_feed(int slot, bool transport_ok)
{
    sd_offline_evt_t evt = sd_offline_feed(&s_offline, slot - 1,
                                           transport_ok, now_ms());
    if (evt == SD_OFFLINE_WENT) {
        sk_event_bus_publishf("target.offline", "{\"slot\":%d}", slot);
    } else if (evt == SD_OFFLINE_CAME) {
        sk_event_bus_publishf("target.online", "{\"slot\":%d}", slot);
        reconcile_slot(slot - 1);   // son değeri hedefe eşitle
    }
}

void eng_offline_wifi(bool connected)
{
    if (!connected) {
        uint32_t newly = sd_offline_wifi_down(&s_offline);
        for (int i = 0; i < SD_MODE_SLOTS; i++) {
            if (newly & (1u << i)) {
                sk_event_bus_publishf("target.offline", "{\"slot\":%d}", i + 1);
            }
        }
    } else {
        sd_offline_wifi_up(&s_offline);
        for (int i = 0; i < SD_MODE_SLOTS; i++) reconcile_slot(i);
    }
}

static void on_wifi_state(const sk_event_t *evt, void *user)
{
    (void)user;
    if (!evt || !evt->payload_json) return;
    if (strstr(evt->payload_json, "\"state\":\"connected\"")) {
        eng_offline_wifi(true);
    } else if (strstr(evt->payload_json, "\"state\":\"disconnected\"")) {
        eng_offline_wifi(false);
    }
}

// --- sd_cmd task: coalescing + yürütme -----------------------------------------

static void execute_send(eng_job_t *job)
{
    // Offline slotta gönderimler 5 sn'de bir proba düşer — düğme yerelde
    // ilerlemeye devam eder, ağ boğulmaz (plan: Offline Semantiği).
    if (!sd_offline_should_send(&s_offline, job->slot - 1, now_ms())) {
        eng_lock();
        slot_ctx_t *ctx = &s_slots[job->slot - 1];
        sk_event_bus_publishf("mode.value",
                              "{\"slot\":%d,\"value\":%d,\"state\":%s}",
                              job->slot, ctx->value, ctx->state ? "true" : "false");
        eng_unlock();
        return;
    }

    int status = 0;
    esp_err_t err = sd_proto_execute(job->mini_profile, job->cmd_node,
                                     &job->target, job->value, job->toggle,
                                     &status);
    eng_offline_feed(job->slot, err == ESP_OK);
    eng_lock();
    slot_ctx_t *ctx = &s_slots[job->slot - 1];
    sk_event_bus_publishf("mode.value",
                          "{\"slot\":%d,\"value\":%d,\"state\":%s}",
                          job->slot, ctx->value, ctx->state ? "true" : "false");
    eng_unlock();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "slot %d gonderim hatasi: %s",
                 job->slot, esp_err_to_name(err));
    }
    (void)status;
}

static void execute_test(eng_job_t *job)
{
    esp_err_t err = ESP_ERR_INVALID_STATE;
    eng_lock();
    slot_ctx_t *ctx = &s_slots[job->slot - 1];
    const sd_behavior_t *drv = ctx->drv;
    eng_unlock();
    if (drv && drv->test) err = drv->test(ctx);
    // drv->test genelde sd_ctx_send kuyruklar — sırayı korumak için
    // kuyruktaki o işi hemen işle: burada bekleyen SEND'leri boşalt.
    eng_job_t *next = NULL;
    while (xQueueReceive(s_queue, &next, 0) == pdTRUE) {
        if (next->type == ENG_JOB_SEND && next->slot == job->slot) {
            int status = 0;
            esp_err_t serr = sd_proto_execute(next->mini_profile, next->cmd_node,
                                              &next->target, next->value,
                                              next->toggle, &status);
            eng_offline_feed(next->slot, serr == ESP_OK);
            if (err == ESP_OK) err = serr;
            if (job->out_status) *job->out_status = status;
            job_free(next);
        } else {
            // Başkasının işi — geri koy (tek eleman geri koymak sırayı bozar
            // ama nadir; test tezgah yoludur).
            xQueueSend(s_queue, &next, 0);
            break;
        }
    }
    *job->out_err = err;
    xSemaphoreGive(job->done);   // bundan sonra job alanlarına DOKUNMA
}

static void cmd_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "sd_cmd task basladi");
    eng_job_t *job = NULL;
    while (xQueueReceive(s_queue, &job, portMAX_DELAY) == pdTRUE) {
        if (job->type == ENG_JOB_NVS_SAVE) {
            nvs_save_values();
            free(job);
            continue;
        }
        if (job->type == ENG_JOB_TEST) {
            execute_test(job);
            free(job);   // done verildi; içerikler CLI'nın (veya sızdırıldı)
            continue;
        }
        // SEND: drain-keep-latest coalescing (device_driver.c:466-480 portu).
        // Aynı slot+kind'ten daha yenisi pencere içinde geldiyse eskisi düşer.
        eng_job_t *next = NULL;
        while (xQueueReceive(s_queue, &next,
                             pdMS_TO_TICKS(SD_ENGINE_COALESCE_MS)) == pdTRUE) {
            if (next->type == ENG_JOB_SEND &&
                next->slot == job->slot && next->kind == job->kind) {
                job_free(job);
                job = next;             // en yenisini tut
            } else {
                break;                  // farklı iş: önce eldekini gönder
            }
        }
        execute_send(job);
        job_free(job);
        if (next && next != job) {
            if (next->type == ENG_JOB_NVS_SAVE) {
                nvs_save_values();
                free(next);
            } else if (next->type == ENG_JOB_TEST) {
                execute_test(next);
                free(next);
            } else {
                execute_send(next);
                job_free(next);
            }
        }
    }
}

// --- Factory reset / ref-checker / init ------------------------------------------

static void on_factory_reset(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    nvs_handle_t h;
    if (nvs_open(SD_ENGINE_NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGW(TAG, "factory reset: mod slotlari silindi");
}

static int profile_ref_count(const char *id)
{
    int refs = 0;
    eng_lock();
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        if (!s_slots[i].binding || !s_slots[i].enabled) continue;
        const cJSON *p = cJSON_GetObjectItemCaseSensitive(s_slots[i].binding,
                                                          "profile");
        if (cJSON_IsString(p) && strcmp(p->valuestring, id) == 0) refs++;
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

    sk_capabilities_register_book("sd_mode_engine", "1.0.0");
    ESP_LOGI(TAG, "init (recovery=%d)", recovery);
    return ESP_OK;
}

esp_err_t sd_mode_engine_start(void)
{
    if (s_recovery) return ESP_ERR_INVALID_STATE;

    s_queue = xQueueCreate(SD_ENGINE_QUEUE_LEN, sizeof(eng_job_t *));
    if (!s_queue) return ESP_ERR_NO_MEM;

    // Aktif slot + slotları yükle.
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
