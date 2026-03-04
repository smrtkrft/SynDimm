/**
 * SynDimm - Device Driver
 * Profil tabanli cihaz kontrolu: Dimmer, Shutter, Safe
 *
 * MIMARI:
 *   input_task (hizli, bloklama yok)
 *     → RAM'de deger guncelle + kuyruga komut at
 *
 *   command_task (ayri task, bloklayabilir)
 *     → Kuyruktan al, throttle uygula, HTTP/UDP/MQTT gonder
 *     → NVS kaydi 2sn bosta kalinca yapilir (flash ömrü koruma)
 *     → SSE event tetikle
 *
 * THREAD SAFETY:
 *   Tum shared state mutex ile korunur.
 *   input_task ve web_server ayni state'e guvenle erisir.
 */

#include "device_driver.h"
#include "ext_flash.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/timers.h"

static const char *TAG = "DEV_DRV";
#define NVS_NAMESPACE "sd_device"

// ============================================================
// Komut kuyrugu tipleri
// ============================================================

typedef enum {
    CMD_SET_VALUE = 0,
    CMD_TOGGLE,
    CMD_STOP,
    CMD_SAFE_EVENT,
    CMD_SAFE_TIMEOUT,
} dd_cmd_type_t;

typedef struct {
    dd_cmd_type_t type;
    int           value;       // CMD_SET_VALUE icin hedef deger
    bool          toggle_val;  // CMD_TOGGLE icin hedef state
    char          safe_event;  // CMD_SAFE_EVENT icin 'R','L','B'
} dd_cmd_t;

#define CMD_QUEUE_LEN    16
#define CMD_TASK_STACK   4096
#define CMD_TASK_PRIO    4
#define THROTTLE_MS      50    // Ayni tip komutlari birlestirme penceresi
#define NVS_SAVE_DELAY   2000  // 2sn bosta kalinca NVS'e yaz

static QueueHandle_t     s_cmd_queue     = NULL;
static TaskHandle_t      s_cmd_task      = NULL;
static SemaphoreHandle_t s_mutex         = NULL;
static TimerHandle_t     s_nvs_timer     = NULL;
static bool              s_nvs_dirty     = false;  // NVS yazilmasi gereken deger var mi

// SSE bildirim callback'i (web_server tarafindan kaydedilir)
static void (*s_notify_cb)(void) = NULL;

// ============================================================
// Shared state (mutex korunmali)
// ============================================================

static dd_mode_t  s_mode          = DD_MODE_DIMMER;
static int        s_current_value = 0;
static bool       s_state         = false;
static bool       s_connected     = false;

static char s_mode_str[12]     = "dimmer";
static char s_profile_id[32]   = "";
static char s_host[64]         = "";
static char s_device_id[16]    = "";
static char s_auth_key[128]    = "";
static char s_mqtt_broker[64]  = "";
static int  s_mqtt_port        = 1883;

// Yuklenen profil (cJSON agaci) - sadece command_task ve init'te erisilebilir
static cJSON *s_profile = NULL;

static char s_device_name[48]  = "";
static char s_proto_str[8]     = "";
static dd_proto_t s_proto      = DD_PROTO_NONE;
static int  s_port             = 80;

static cJSON *s_cmd_set_value  = NULL;
static cJSON *s_cmd_toggle     = NULL;
static cJSON *s_cmd_stop       = NULL;
static cJSON *s_cmd_status     = NULL;

static int  s_val_min   = 0;
static int  s_val_max   = 100;
static int  s_val_step  = 1;

static char s_status_text[32] = "off";

// ============================================================
// Mutex helpers
// ============================================================

static inline void state_lock(void)
{
    if (s_mutex) xSemaphoreTake(s_mutex, portMAX_DELAY);
}

static inline void state_unlock(void)
{
    if (s_mutex) xSemaphoreGive(s_mutex);
}

// ============================================================
// NVS yardimci fonksiyonlar
// ============================================================

static esp_err_t nvs_read_str(const char *key, char *out, size_t max_len)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t len = max_len;
    err = nvs_get_str(h, key, out, &len);
    nvs_close(h);
    return err;
}

static esp_err_t nvs_write_str(const char *key, const char *val)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, key, val);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

static esp_err_t nvs_read_i32(const char *key, int32_t *out)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    err = nvs_get_i32(h, key, out);
    nvs_close(h);
    return err;
}

static esp_err_t nvs_write_i32(const char *key, int32_t val)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(h, key, val);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

// NVS kaydetme timer callback (2sn bosta kalinca tetiklenir)
static void nvs_save_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    if (!s_nvs_dirty) return;

    state_lock();
    int val = s_current_value;
    state_unlock();

    nvs_write_i32("cur_value", (int32_t)val);
    s_nvs_dirty = false;
    ESP_LOGD(TAG, "NVS kaydedildi: cur_value=%d", val);
}

// NVS kaydetme timer'ini resetle (her deger degisiminde)
static void nvs_save_schedule(void)
{
    s_nvs_dirty = true;
    if (s_nvs_timer) {
        xTimerReset(s_nvs_timer, 0);
    }
}

// ============================================================
// Profil yukleyici
// ============================================================

static dd_mode_t mode_from_str(const char *s)
{
    if (strcmp(s, "shutter") == 0) return DD_MODE_SHUTTER;
    if (strcmp(s, "safe") == 0)    return DD_MODE_SAFE;
    return DD_MODE_DIMMER;
}

static const char *mode_to_str(dd_mode_t m)
{
    switch (m) {
        case DD_MODE_SHUTTER: return "shutter";
        case DD_MODE_SAFE:    return "safe";
        default:              return "dimmer";
    }
}

static dd_proto_t proto_from_str(const char *s)
{
    if (strcmp(s, "http") == 0) return DD_PROTO_HTTP;
    if (strcmp(s, "udp") == 0)  return DD_PROTO_UDP;
    if (strcmp(s, "mqtt") == 0) return DD_PROTO_MQTT;
    return DD_PROTO_NONE;
}

static void clear_profile(void)
{
    if (s_profile) {
        cJSON_Delete(s_profile);
        s_profile = NULL;
    }
    s_cmd_set_value = NULL;
    s_cmd_toggle    = NULL;
    s_cmd_stop      = NULL;
    s_cmd_status    = NULL;
    s_proto         = DD_PROTO_NONE;
    s_device_name[0] = '\0';
    s_proto_str[0]  = '\0';
    s_val_min       = 0;
    s_val_max       = 100;
    s_val_step      = 1;
    s_port          = 80;
    s_connected     = false;
}

static esp_err_t parse_profile(cJSON *root)
{
    cJSON *name = cJSON_GetObjectItem(root, "name");
    if (name && cJSON_IsString(name)) {
        strncpy(s_device_name, name->valuestring, sizeof(s_device_name) - 1);
    }

    cJSON *proto = cJSON_GetObjectItem(root, "protocol");
    if (proto && cJSON_IsString(proto)) {
        strncpy(s_proto_str, proto->valuestring, sizeof(s_proto_str) - 1);
        s_proto = proto_from_str(s_proto_str);
    }

    cJSON *port_j = cJSON_GetObjectItem(root, "port");
    if (port_j && cJSON_IsNumber(port_j)) {
        s_port = port_j->valueint;
    }

    cJSON *broker = cJSON_GetObjectItem(root, "broker");
    if (broker && cJSON_IsString(broker) && strlen(broker->valuestring) > 0) {
        strncpy(s_mqtt_broker, broker->valuestring, sizeof(s_mqtt_broker) - 1);
    }
    cJSON *broker_port = cJSON_GetObjectItem(root, "broker_port");
    if (broker_port && cJSON_IsNumber(broker_port)) {
        s_mqtt_port = broker_port->valueint;
    }

    cJSON *cmds = cJSON_GetObjectItem(root, "commands");
    if (!cmds) {
        if (s_mode != DD_MODE_SAFE) {
            ESP_LOGW(TAG, "Profilde 'commands' objesi yok");
        }
        return ESP_OK;
    }

    s_cmd_set_value = cJSON_GetObjectItem(cmds, "set_value");
    s_cmd_toggle    = cJSON_GetObjectItem(cmds, "toggle");
    s_cmd_stop      = cJSON_GetObjectItem(cmds, "stop");
    s_cmd_status    = cJSON_GetObjectItem(cmds, "status");

    if (s_cmd_set_value) {
        cJSON *mn = cJSON_GetObjectItem(s_cmd_set_value, "min");
        cJSON *mx = cJSON_GetObjectItem(s_cmd_set_value, "max");
        cJSON *st = cJSON_GetObjectItem(s_cmd_set_value, "step");
        if (mn && cJSON_IsNumber(mn)) s_val_min  = mn->valueint;
        if (mx && cJSON_IsNumber(mx)) s_val_max  = mx->valueint;
        if (st && cJSON_IsNumber(st)) s_val_step = st->valueint;
    }

    ESP_LOGI(TAG, "Profil: %s [%s] min=%d max=%d step=%d",
             s_device_name, s_proto_str, s_val_min, s_val_max, s_val_step);

    return ESP_OK;
}

// ============================================================
// Komut calistirma (command_task icinden cagirilir, bloklayabilir)
// ============================================================

static int map_value(int value, int from_min, int from_max, int to_min, int to_max)
{
    if (from_max == from_min) return to_min;
    return to_min + (value - from_min) * (to_max - to_min) / (from_max - from_min);
}

static esp_err_t execute_set_value_network(int value)
{
    if (!s_cmd_set_value) return ESP_ERR_INVALID_STATE;

    // Deger mapping
    int mapped_value = value;
    cJSON *mmin = cJSON_GetObjectItem(s_cmd_set_value, "map_min");
    cJSON *mmax = cJSON_GetObjectItem(s_cmd_set_value, "map_max");
    if (mmin && mmax && cJSON_IsNumber(mmin) && cJSON_IsNumber(mmax)) {
        mapped_value = map_value(value, s_val_min, s_val_max,
                                 mmin->valueint, mmax->valueint);
    }

    ESP_LOGI(TAG, "-> set_value: %d (mapped: %d)", value, mapped_value);

    switch (s_proto) {
        case DD_PROTO_HTTP: {
            cJSON *method = cJSON_GetObjectItem(s_cmd_set_value, "method");
            cJSON *path   = cJSON_GetObjectItem(s_cmd_set_value, "path");
            cJSON *body   = cJSON_GetObjectItem(s_cmd_set_value, "body");
            if (!method || !path) return ESP_ERR_INVALID_ARG;
            return proto_http_send_command(
                s_host, s_port,
                method->valuestring,
                path->valuestring,
                body ? body->valuestring : NULL,
                s_auth_key, s_device_id,
                mapped_value, false);
        }
        case DD_PROTO_UDP: {
            cJSON *packet = cJSON_GetObjectItem(s_cmd_set_value, "packet");
            if (!packet) return ESP_ERR_INVALID_ARG;
            return proto_udp_send_command(s_host, s_port,
                                           packet->valuestring,
                                           mapped_value, false);
        }
        case DD_PROTO_MQTT: {
            cJSON *topic   = cJSON_GetObjectItem(s_cmd_set_value, "topic");
            cJSON *payload = cJSON_GetObjectItem(s_cmd_set_value, "payload");
            if (!topic || !payload) return ESP_ERR_INVALID_ARG;
            cJSON *prefix  = cJSON_GetObjectItem(s_profile, "topic_prefix");
            cJSON *state_t = cJSON_GetObjectItem(s_profile, "state_topic");
            return proto_mqtt_send_command(
                topic->valuestring,
                payload->valuestring,
                prefix ? prefix->valuestring : "",
                state_t ? state_t->valuestring : "",
                mapped_value, false);
        }
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

static esp_err_t execute_toggle_network(bool toggle_val)
{
    if (!s_cmd_toggle) return ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "-> toggle: %s", toggle_val ? "on" : "off");

    state_lock();
    int cur_val = s_current_value;
    state_unlock();

    switch (s_proto) {
        case DD_PROTO_HTTP: {
            cJSON *method = cJSON_GetObjectItem(s_cmd_toggle, "method");
            cJSON *path   = cJSON_GetObjectItem(s_cmd_toggle, "path");
            cJSON *body   = cJSON_GetObjectItem(s_cmd_toggle, "body");
            if (!method || !path) return ESP_ERR_INVALID_ARG;
            return proto_http_send_command(
                s_host, s_port,
                method->valuestring,
                path->valuestring,
                body ? body->valuestring : NULL,
                s_auth_key, s_device_id,
                cur_val, toggle_val);
        }
        case DD_PROTO_UDP: {
            cJSON *packet = cJSON_GetObjectItem(s_cmd_toggle, "packet");
            if (!packet) return ESP_ERR_INVALID_ARG;
            return proto_udp_send_command(s_host, s_port,
                                           packet->valuestring,
                                           cur_val, toggle_val);
        }
        case DD_PROTO_MQTT: {
            cJSON *topic   = cJSON_GetObjectItem(s_cmd_toggle, "topic");
            cJSON *payload = cJSON_GetObjectItem(s_cmd_toggle, "payload");
            if (!topic || !payload) return ESP_ERR_INVALID_ARG;
            cJSON *prefix  = cJSON_GetObjectItem(s_profile, "topic_prefix");
            cJSON *state_t = cJSON_GetObjectItem(s_profile, "state_topic");
            return proto_mqtt_send_command(
                topic->valuestring,
                payload->valuestring,
                prefix ? prefix->valuestring : "",
                state_t ? state_t->valuestring : "",
                cur_val, toggle_val);
        }
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

static esp_err_t execute_stop_network(void)
{
    if (!s_cmd_stop) return ESP_ERR_INVALID_STATE;

    ESP_LOGI(TAG, "-> stop");

    switch (s_proto) {
        case DD_PROTO_HTTP: {
            cJSON *method = cJSON_GetObjectItem(s_cmd_stop, "method");
            cJSON *path   = cJSON_GetObjectItem(s_cmd_stop, "path");
            if (!method || !path) return ESP_ERR_INVALID_ARG;
            return proto_http_send_command(
                s_host, s_port,
                method->valuestring,
                path->valuestring,
                NULL, s_auth_key, s_device_id, 0, false);
        }
        default:
            return ESP_ERR_NOT_SUPPORTED;
    }
}

static void update_status_text(void)
{
    if (s_mode == DD_MODE_SAFE) {
        if (safe_lock_is_active()) {
            snprintf(s_status_text, sizeof(s_status_text), "Sifre Giriliyor");
        } else {
            snprintf(s_status_text, sizeof(s_status_text), "Sifre Bekleniyor");
        }
    } else {
        snprintf(s_status_text, sizeof(s_status_text), "%s", s_state ? "on" : "off");
    }
}

// ============================================================
// Command Task - ayri task'ta calisir, ag islemleri burada
// ============================================================

static void command_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Command task baslatildi");

    dd_cmd_t cmd;
    while (1) {
        // Kuyruktan komut bekle
        if (xQueueReceive(s_cmd_queue, &cmd, portMAX_DELAY) != pdTRUE) {
            continue;
        }

        // SET_VALUE throttle: kuyrukta daha yeni degerler varsa onlari al
        if (cmd.type == CMD_SET_VALUE) {
            dd_cmd_t peek;
            while (xQueueReceive(s_cmd_queue, &peek, pdMS_TO_TICKS(THROTTLE_MS)) == pdTRUE) {
                if (peek.type == CMD_SET_VALUE) {
                    // Daha yeni deger geldi, eskisini atla
                    cmd = peek;
                } else {
                    // Farkli komut tipi - once SET_VALUE'yu gonder, sonra bunu isle
                    execute_set_value_network(cmd.value);
                    if (s_notify_cb) s_notify_cb();
                    cmd = peek;
                    break;
                }
            }
        }

        // Komutu calistir
        switch (cmd.type) {
            case CMD_SET_VALUE:
                execute_set_value_network(cmd.value);
                break;
            case CMD_TOGGLE:
                execute_toggle_network(cmd.toggle_val);
                break;
            case CMD_STOP:
                execute_stop_network();
                break;
            case CMD_SAFE_EVENT:
                safe_lock_handle_event(cmd.safe_event);
                break;
            case CMD_SAFE_TIMEOUT:
                safe_lock_process_timeout();
                break;
        }

        // SSE event tetikle - GUI güncelleme
        if (s_notify_cb) s_notify_cb();
    }
}

// ============================================================
// Event isleme (input_task'tan cagirilir - HIZLI, bloklama yok)
// ============================================================

static void enqueue_cmd(const dd_cmd_t *cmd)
{
    if (s_cmd_queue) {
        // Overwrite modda degil, kuyruk doluysa en eski komutu at
        if (xQueueSend(s_cmd_queue, cmd, 0) != pdTRUE) {
            dd_cmd_t discard;
            xQueueReceive(s_cmd_queue, &discard, 0);
            xQueueSend(s_cmd_queue, cmd, 0);
            ESP_LOGW(TAG, "Komut kuyrugu tasmasi, eski komut atildi");
        }
    }
}

// Safe lock timeout callback'i - timer daemon'dan cagirilir,
// sadece kuyruga komut ekler (shared state'e dokunmaz)
static void safe_timeout_enqueue(void)
{
    dd_cmd_t cmd = { .type = CMD_SAFE_TIMEOUT };
    enqueue_cmd(&cmd);
}

static void handle_dimmer_event(char event)
{
    dd_cmd_t cmd = {0};

    switch (event) {
        case 'R':
        case 'L': {
            state_lock();
            int new_val = s_current_value + ((event == 'R') ? s_val_step : -s_val_step);
            if (new_val < s_val_min) new_val = s_val_min;
            if (new_val > s_val_max) new_val = s_val_max;
            s_current_value = new_val;
            s_state = (new_val > s_val_min);
            update_status_text();
            state_unlock();

            cmd.type = CMD_SET_VALUE;
            cmd.value = new_val;
            enqueue_cmd(&cmd);
            nvs_save_schedule();
            break;
        }
        case 'B': {
            state_lock();
            s_state = !s_state;
            bool new_state = s_state;
            update_status_text();
            state_unlock();

            cmd.type = CMD_TOGGLE;
            cmd.toggle_val = new_state;
            enqueue_cmd(&cmd);
            break;
        }
        default:
            break;
    }
}

static void handle_shutter_event(char event)
{
    dd_cmd_t cmd = {0};

    switch (event) {
        case 'R':
        case 'L': {
            state_lock();
            int new_val = s_current_value + ((event == 'R') ? s_val_step : -s_val_step);
            if (new_val < s_val_min) new_val = s_val_min;
            if (new_val > s_val_max) new_val = s_val_max;
            s_current_value = new_val;
            update_status_text();
            state_unlock();

            cmd.type = CMD_SET_VALUE;
            cmd.value = new_val;
            enqueue_cmd(&cmd);
            nvs_save_schedule();
            break;
        }
        case 'B': {
            cmd.type = CMD_STOP;
            enqueue_cmd(&cmd);
            break;
        }
        default:
            break;
    }
}

static void handle_safe_event(char event)
{
    dd_cmd_t cmd = {0};
    cmd.type = CMD_SAFE_EVENT;
    cmd.safe_event = event;
    enqueue_cmd(&cmd);

    state_lock();
    update_status_text();
    state_unlock();
}

// ============================================================
// Public API
// ============================================================

esp_err_t device_driver_init(void)
{
    ESP_LOGI(TAG, "Device driver baslatiliyor...");

    // Mutex olustur
    s_mutex = xSemaphoreCreateMutex();
    if (!s_mutex) {
        ESP_LOGE(TAG, "Mutex olusturulamadi!");
        return ESP_ERR_NO_MEM;
    }

    // Komut kuyrugu olustur
    s_cmd_queue = xQueueCreate(CMD_QUEUE_LEN, sizeof(dd_cmd_t));
    if (!s_cmd_queue) {
        ESP_LOGE(TAG, "Komut kuyrugu olusturulamadi!");
        return ESP_ERR_NO_MEM;
    }

    // NVS kaydetme timer'i (2sn, tek seferlik, idle'da tetiklenir)
    s_nvs_timer = xTimerCreate("nvs_save", pdMS_TO_TICKS(NVS_SAVE_DELAY),
                                pdFALSE, NULL, nvs_save_timer_cb);

    // NVS'ten ayarlari oku
    if (nvs_read_str("mode", s_mode_str, sizeof(s_mode_str)) == ESP_OK) {
        s_mode = mode_from_str(s_mode_str);
    } else {
        strcpy(s_mode_str, "dimmer");
        s_mode = DD_MODE_DIMMER;
        nvs_write_str("mode", s_mode_str);
    }

    nvs_read_str("profile", s_profile_id, sizeof(s_profile_id));
    nvs_read_str("host", s_host, sizeof(s_host));
    nvs_read_str("device_id", s_device_id, sizeof(s_device_id));
    nvs_read_str("auth_key", s_auth_key, sizeof(s_auth_key));
    nvs_read_str("mqtt_broker", s_mqtt_broker, sizeof(s_mqtt_broker));

    int32_t mqtt_port_val = 1883;
    if (nvs_read_i32("mqtt_port", &mqtt_port_val) == ESP_OK) {
        s_mqtt_port = (int)mqtt_port_val;
    }

    int32_t cur_val = 0;
    if (nvs_read_i32("cur_value", &cur_val) == ESP_OK) {
        s_current_value = (int)cur_val;
    }

    ESP_LOGI(TAG, "Mod: %s, Profil: %s, Host: %s", s_mode_str, s_profile_id, s_host);

    // Safe lock baslatma
    if (s_mode == DD_MODE_SAFE) {
        safe_lock_init();
        safe_lock_set_timeout_cb(safe_timeout_enqueue);
    }

    // Profil yukle (varsa)
    if (strlen(s_profile_id) > 0) {
        esp_err_t err = device_driver_load_profile(s_profile_id);
        if (err != ESP_OK) {
            ESP_LOGW(TAG, "Profil yuklenemedi: %s", s_profile_id);
        }
    }

    // Profil dizinlerini olustur ve varsayilan profilleri yaz
    ext_flash_write_file("/slot_a/profiles/dimmer/.keep", "", 0);
    ext_flash_write_file("/slot_a/profiles/shutter/.keep", "", 0);
    ext_flash_write_file("/slot_a/profiles/safe/.keep", "", 0);

    update_status_text();

    // Command task baslat
    xTaskCreate(command_task, "cmd_task", CMD_TASK_STACK, NULL, CMD_TASK_PRIO, &s_cmd_task);

    ESP_LOGI(TAG, "Device driver hazir (kuyruk: %d slot, throttle: %dms)",
             CMD_QUEUE_LEN, THROTTLE_MS);
    return ESP_OK;
}

esp_err_t device_driver_set_mode(const char *mode)
{
    if (!mode) return ESP_ERR_INVALID_ARG;

    dd_mode_t new_mode = mode_from_str(mode);

    state_lock();
    if (new_mode == s_mode) {
        state_unlock();
        return ESP_OK;
    }

    // MQTT baglantisini kapat
    if (s_proto == DD_PROTO_MQTT) {
        proto_mqtt_stop();
    }

    s_mode = new_mode;
    strncpy(s_mode_str, mode_to_str(s_mode), sizeof(s_mode_str) - 1);

    clear_profile();
    s_profile_id[0] = '\0';
    s_current_value = 0;
    s_state = false;
    update_status_text();
    state_unlock();

    nvs_write_str("mode", s_mode_str);
    nvs_write_str("profile", "");

    if (new_mode == DD_MODE_SAFE) {
        safe_lock_init();
        safe_lock_set_timeout_cb(safe_timeout_enqueue);
    }

    ESP_LOGI(TAG, "Mod degistirildi: %s", s_mode_str);

    // SSE bildirim: GUI'nin mod degisikligini gormesi icin
    if (s_notify_cb) s_notify_cb();

    return ESP_OK;
}

const char *device_driver_get_mode(void)
{
    return s_mode_str;
}

esp_err_t device_driver_load_profile(const char *profile_id)
{
    if (!profile_id || strlen(profile_id) == 0) return ESP_ERR_INVALID_ARG;

    char path[128];
    snprintf(path, sizeof(path), "/slot_a/profiles/%s/%s.json",
             s_mode_str, profile_id);

    ESP_LOGI(TAG, "Profil yukleniyor: %s", path);

    char *buf = malloc(4096);
    if (!buf) return ESP_ERR_NO_MEM;

    int len = ext_flash_read_file(path, buf, 4095);
    if (len <= 0) {
        ESP_LOGE(TAG, "Profil dosyasi okunamadi: %s", path);
        free(buf);
        return ESP_ERR_NOT_FOUND;
    }
    buf[len] = '\0';

    state_lock();
    clear_profile();
    s_profile = cJSON_Parse(buf);
    free(buf);

    if (!s_profile) {
        state_unlock();
        ESP_LOGE(TAG, "JSON parse hatasi: %s", profile_id);
        return ESP_FAIL;
    }

    esp_err_t err = parse_profile(s_profile);
    if (err != ESP_OK) {
        clear_profile();
        state_unlock();
        return err;
    }

    strncpy(s_profile_id, profile_id, sizeof(s_profile_id) - 1);
    s_connected = (strlen(s_host) > 0);
    update_status_text();
    state_unlock();

    nvs_write_str("profile", s_profile_id);

    // MQTT ise broker'a baglan
    if (s_proto == DD_PROTO_MQTT && strlen(s_mqtt_broker) > 0) {
        proto_mqtt_init(s_mqtt_broker, s_mqtt_port);

        if (s_cmd_status) {
            cJSON *sub = cJSON_GetObjectItem(s_cmd_status, "subscribe");
            if (sub && cJSON_IsString(sub)) {
                cJSON *prefix  = cJSON_GetObjectItem(s_profile, "topic_prefix");
                cJSON *state_t = cJSON_GetObjectItem(s_profile, "state_topic");
                proto_mqtt_subscribe_status(
                    sub->valuestring,
                    prefix ? prefix->valuestring : "",
                    state_t ? state_t->valuestring : "");
            }
        }
    }

    ESP_LOGI(TAG, "Profil yuklendi: %s (%s) [%s]",
             s_device_name, s_profile_id, s_proto_str);
    return ESP_OK;
}

const char *device_driver_get_active_profile(void)
{
    return s_profile_id;
}

esp_err_t device_driver_list_profiles(const char *mode, char *json_out, size_t max_len)
{
    if (!mode || !json_out) return ESP_ERR_INVALID_ARG;

    char dir_path[64];
    snprintf(dir_path, sizeof(dir_path), "/slot_a/profiles/%s", mode);

    DIR *dir = opendir(dir_path);
    if (!dir) {
        snprintf(json_out, max_len, "[]");
        return ESP_OK;
    }

    cJSON *arr = cJSON_CreateArray();
    struct dirent *ent;
    char file_path[320];
    char *buf = malloc(4096);
    if (!buf) {
        closedir(dir);
        cJSON_Delete(arr);
        return ESP_ERR_NO_MEM;
    }

    while ((ent = readdir(dir)) != NULL) {
        size_t nlen = strlen(ent->d_name);
        if (nlen < 6) continue;
        if (strcmp(ent->d_name + nlen - 5, ".json") != 0) continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, ent->d_name);
        int len = ext_flash_read_file(file_path, buf, 4095);
        if (len <= 0) continue;
        buf[len] = '\0';

        cJSON *prof = cJSON_Parse(buf);
        if (!prof) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON *id_j   = cJSON_GetObjectItem(prof, "id");
        cJSON *name_j = cJSON_GetObjectItem(prof, "name");
        cJSON *mfr_j  = cJSON_GetObjectItem(prof, "manufacturer");
        cJSON *pro_j  = cJSON_GetObjectItem(prof, "protocol");

        if (id_j)   cJSON_AddStringToObject(item, "id", id_j->valuestring);
        if (name_j) cJSON_AddStringToObject(item, "name", name_j->valuestring);
        if (mfr_j)  cJSON_AddStringToObject(item, "manufacturer", mfr_j->valuestring);
        if (pro_j)  cJSON_AddStringToObject(item, "protocol", pro_j->valuestring);

        cJSON_AddItemToArray(arr, item);
        cJSON_Delete(prof);
    }

    free(buf);
    closedir(dir);

    char *json_str = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    if (json_str) {
        strncpy(json_out, json_str, max_len - 1);
        json_out[max_len - 1] = '\0';
        free(json_str);
    } else {
        snprintf(json_out, max_len, "[]");
    }

    return ESP_OK;
}

esp_err_t device_driver_delete_profile(const char *profile_id)
{
    if (!profile_id) return ESP_ERR_INVALID_ARG;

    const char *modes[] = {"dimmer", "shutter", "safe"};
    for (int i = 0; i < 3; i++) {
        char path[128];
        snprintf(path, sizeof(path), "/slot_a/profiles/%s/%s.json", modes[i], profile_id);
        if (ext_flash_delete_file(path) == ESP_OK) {
            ESP_LOGI(TAG, "Profil silindi: %s", path);
            state_lock();
            if (strcmp(s_profile_id, profile_id) == 0) {
                clear_profile();
                s_profile_id[0] = '\0';
            }
            state_unlock();
            nvs_write_str("profile", "");
            return ESP_OK;
        }
    }

    return ESP_ERR_NOT_FOUND;
}

esp_err_t device_driver_select_profile(const char *profile_id,
                                        const char *host,
                                        const char *device_id,
                                        const char *auth_key)
{
    if (!profile_id) return ESP_ERR_INVALID_ARG;

    state_lock();
    if (host) {
        strncpy(s_host, host, sizeof(s_host) - 1);
    }
    if (device_id) {
        strncpy(s_device_id, device_id, sizeof(s_device_id) - 1);
    }
    if (auth_key) {
        strncpy(s_auth_key, auth_key, sizeof(s_auth_key) - 1);
    }
    state_unlock();

    if (host)      nvs_write_str("host", s_host);
    if (device_id) nvs_write_str("device_id", s_device_id);
    if (auth_key)  nvs_write_str("auth_key", s_auth_key);

    return device_driver_load_profile(profile_id);
}

esp_err_t device_driver_handle_event(char event)
{
    // Bu fonksiyon input_task'tan cagirilir - HIZLI olmali
    // Ag islemleri command_task'a kuyruk ile gonderilir

    // 'N' = sonraki mod (sinir kontrollu, dolanma yok)
    if (event == 'N') {
        state_lock();
        dd_mode_t cur = s_mode;
        dd_mode_t target = cur;
        switch (cur) {
            case DD_MODE_DIMMER:  target = DD_MODE_SHUTTER; break;
            case DD_MODE_SHUTTER: target = DD_MODE_SAFE;    break;
            default: break; // DD_MODE_SAFE - zaten sonda
        }
        state_unlock();
        if (target != cur) {
            device_driver_set_mode(mode_to_str(target));
            ESP_LOGI(TAG, "Mod ileri: %s", device_driver_get_mode());
        }
        return ESP_OK;
    }

    // 'P' = onceki mod (sinir kontrollu, dolanma yok)
    if (event == 'P') {
        state_lock();
        dd_mode_t cur = s_mode;
        dd_mode_t target = cur;
        switch (cur) {
            case DD_MODE_SAFE:    target = DD_MODE_SHUTTER; break;
            case DD_MODE_SHUTTER: target = DD_MODE_DIMMER;  break;
            default: break; // DD_MODE_DIMMER - zaten basta
        }
        state_unlock();
        if (target != cur) {
            device_driver_set_mode(mode_to_str(target));
            ESP_LOGI(TAG, "Mod geri: %s", device_driver_get_mode());
        }
        return ESP_OK;
    }

    // 'M' eventi: mod degistir (dimmer → shutter → safe → dimmer)
    if (event == 'M') {
        state_lock();
        dd_mode_t next;
        switch (s_mode) {
            case DD_MODE_DIMMER:  next = DD_MODE_SHUTTER; break;
            case DD_MODE_SHUTTER: next = DD_MODE_SAFE;    break;
            default:              next = DD_MODE_DIMMER;   break;
        }
        state_unlock();

        device_driver_set_mode(mode_to_str(next));
        ESP_LOGI(TAG, "Mod degistirildi: %s", device_driver_get_mode());
        return ESP_OK;
    }

    // Modu mutex altinda oku
    state_lock();
    dd_mode_t mode = s_mode;
    state_unlock();

    switch (mode) {
        case DD_MODE_DIMMER:
            handle_dimmer_event(event);
            break;
        case DD_MODE_SHUTTER:
            handle_shutter_event(event);
            break;
        case DD_MODE_SAFE:
            handle_safe_event(event);
            break;
    }

    return ESP_OK;
}

bool device_driver_handle_mode_change(char direction)
{
    state_lock();
    dd_mode_t cur = s_mode;
    state_unlock();

    device_driver_handle_event(direction);

    state_lock();
    dd_mode_t after = s_mode;
    state_unlock();

    return (cur != after);
}

int device_driver_get_value(void)
{
    state_lock();
    int v = s_current_value;
    state_unlock();
    return v;
}

const char *device_driver_get_device_name(void)
{
    static char buf[48];
    state_lock();
    strncpy(buf, s_device_name, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    state_unlock();
    return buf;
}

const char *device_driver_get_status_text(void)
{
    static char buf[32];
    state_lock();
    strncpy(buf, s_status_text, sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';
    state_unlock();
    return buf;
}

bool device_driver_is_connected(void)
{
    if (s_proto == DD_PROTO_MQTT) {
        return proto_mqtt_is_connected();
    }
    return s_connected;
}

void device_driver_get_status(dd_status_t *out)
{
    if (!out) return;

    state_lock();
    out->mode      = s_mode;
    out->value     = s_current_value;
    out->state     = s_state;
    out->connected = device_driver_is_connected();
    strncpy(out->device_name, s_device_name, sizeof(out->device_name) - 1);
    strncpy(out->profile_id, s_profile_id, sizeof(out->profile_id) - 1);
    strncpy(out->mode_str, s_mode_str, sizeof(out->mode_str) - 1);
    state_unlock();
}

esp_err_t device_driver_set_mqtt_config(const char *broker, int port)
{
    state_lock();
    if (broker) {
        strncpy(s_mqtt_broker, broker, sizeof(s_mqtt_broker) - 1);
    }
    s_mqtt_port = port;
    state_unlock();

    if (broker) nvs_write_str("mqtt_broker", s_mqtt_broker);
    nvs_write_i32("mqtt_port", (int32_t)port);
    return ESP_OK;
}

void device_driver_set_notify_cb(void (*cb)(void))
{
    s_notify_cb = cb;
}

esp_err_t device_driver_add_safe_profile(const char *json_str)
{
    if (!json_str) return ESP_ERR_INVALID_ARG;

    cJSON *root = cJSON_Parse(json_str);
    if (!root) return ESP_FAIL;

    cJSON *id_j = cJSON_GetObjectItem(root, "id");
    if (!id_j || !cJSON_IsString(id_j)) {
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    cJSON_DeleteItemFromObject(root, "mode");
    cJSON_AddStringToObject(root, "mode", "safe");

    char *str = cJSON_PrintUnformatted(root);
    if (!str) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    char path[128];
    snprintf(path, sizeof(path), "/slot_a/profiles/safe/%s.json", id_j->valuestring);
    esp_err_t err = ext_flash_write_file(path, str, strlen(str));

    free(str);
    cJSON_Delete(root);

    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Safe profil kaydedildi: %s", path);
    }
    return err;
}

esp_err_t device_driver_list_safe_profiles(char *json_out, size_t max_len)
{
    const char *dir_path = "/slot_a/profiles/safe";
    DIR *dir = opendir(dir_path);
    if (!dir) {
        snprintf(json_out, max_len, "[]");
        return ESP_OK;
    }

    cJSON *arr = cJSON_CreateArray();
    struct dirent *ent;
    char file_path[320];
    char *buf = malloc(2048);
    if (!buf) {
        closedir(dir);
        cJSON_Delete(arr);
        return ESP_ERR_NO_MEM;
    }

    while ((ent = readdir(dir)) != NULL) {
        size_t nlen = strlen(ent->d_name);
        if (nlen < 6) continue;
        if (strcmp(ent->d_name + nlen - 5, ".json") != 0) continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, ent->d_name);
        int len = ext_flash_read_file(file_path, buf, 2047);
        if (len <= 0) continue;
        buf[len] = '\0';

        cJSON *prof = cJSON_Parse(buf);
        if (!prof) continue;

        cJSON *item = cJSON_CreateObject();
        cJSON *id_j   = cJSON_GetObjectItem(prof, "id");
        cJSON *name_j = cJSON_GetObjectItem(prof, "name");
        cJSON *pw_j   = cJSON_GetObjectItem(prof, "password");

        if (id_j)   cJSON_AddStringToObject(item, "id", id_j->valuestring);
        if (name_j) cJSON_AddStringToObject(item, "name", name_j->valuestring);
        if (pw_j && cJSON_IsArray(pw_j)) {
            cJSON_AddNumberToObject(item, "password_length", cJSON_GetArraySize(pw_j));
        }

        cJSON_AddItemToArray(arr, item);
        cJSON_Delete(prof);
    }

    free(buf);
    closedir(dir);

    char *json_str = cJSON_PrintUnformatted(arr);
    cJSON_Delete(arr);

    if (json_str) {
        strncpy(json_out, json_str, max_len - 1);
        json_out[max_len - 1] = '\0';
        free(json_str);
    } else {
        snprintf(json_out, max_len, "[]");
    }

    return ESP_OK;
}
