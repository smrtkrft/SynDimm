// sd_prefs — seçimlik özellik anahtarları. Desen: bf_vibration (NVS'li
// bool switch + CLI + event) + sk_api (factory-reset silme kancası).

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "sk_capabilities.h"
#include "sk_cli.h"
#include "sk_errors.h"
#include "sk_event_bus.h"

#include "sd_quiet.h"
#include "sd_prefs.h"

static const char *TAG = "sd_prefs";

#define NVS_NS            "sd_prefs"
#define QUIET_MAX         24            // "23:00-07:00" + pay
#define CHANGE_CB_MAX     4

// RAM önbelleği — bool'lar tek-kelime okunur (atomik yeter); quiet string'i
// mutex'le korunur (CLI task yazar, sd_buzzer başka task'tan okuyacak).
static bool  s_gestures = true;
static bool  s_buzzer   = true;
static char  s_quiet[QUIET_MAX] = "";
static char  s_tz[8] = "+00:00";
static SemaphoreHandle_t s_lock;

static struct { sd_prefs_change_cb_t cb; void *user; } s_change_cb[CHANGE_CB_MAX];
static int s_change_cb_count;

// --- NVS yardımcıları -------------------------------------------------

static void nvs_save_u8(const char *key, uint8_t v)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_u8(h, key, v);
    nvs_commit(h);
    nvs_close(h);
}

static void nvs_save_str(const char *key, const char *v)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) != ESP_OK) return;
    nvs_set_str(h, key, v);
    nvs_commit(h);
    nvs_close(h);
}

static void load_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;  // ilk boot: defaults
    uint8_t u = 1;
    if (nvs_get_u8(h, "gestures", &u) == ESP_OK) s_gestures = (u != 0);
    u = 1;
    if (nvs_get_u8(h, "buzzer", &u) == ESP_OK)   s_buzzer   = (u != 0);
    size_t len = sizeof(s_quiet);
    if (nvs_get_str(h, "quiet", s_quiet, &len) != ESP_OK) s_quiet[0] = '\0';
    len = sizeof(s_tz);
    if (nvs_get_str(h, "tz", s_tz, &len) != ESP_OK) strcpy(s_tz, "+00:00");
    nvs_close(h);
}

// --- Değer doğrulama ---------------------------------------------------

static bool parse_bool(const char *v, bool *out)
{
    if (!v) return false;
    if (!strcasecmp(v, "on")  || !strcasecmp(v, "true")  || !strcmp(v, "1")) { *out = true;  return true; }
    if (!strcasecmp(v, "off") || !strcasecmp(v, "false") || !strcmp(v, "0")) { *out = false; return true; }
    return false;
}

// Değer doğrulama = yetkili çözümleyicinin kendisi (sd_quiet, artık bu
// bileşende). Kod incelemesi bulgusu: eski sscanf bekçisi "9:00-17:00"
// gibi değerleri kabul edip kaydediyordu ama sd_quiet_parse reddediyordu —
// sessiz saatler sessizce hiç çalışmıyordu. Tek gramer, tek fonksiyon.
static bool quiet_format_valid(const char *v)
{
    uint16_t f, t;
    return sd_quiet_parse(v, &f, &t);
}

static bool tz_format_valid(const char *v)
{
    int16_t off;
    return sd_tz_parse(v, &off);
}

static void notify_change(const char *key)
{
    for (int i = 0; i < s_change_cb_count; i++) {
        s_change_cb[i].cb(key, s_change_cb[i].user);
    }
}

// --- Public API ---------------------------------------------------------

bool sd_prefs_get_bool(const char *key, bool def)
{
    if (!strcmp(key, "gestures")) return s_gestures;
    if (!strcmp(key, "buzzer"))   return s_buzzer;
    return def;
}

esp_err_t sd_prefs_get_str(const char *key, char *out, size_t cap)
{
    if (!out || cap == 0) return ESP_ERR_NOT_FOUND;
    const char *src;
    if      (!strcmp(key, "quiet")) src = s_quiet;
    else if (!strcmp(key, "tz"))    src = s_tz;
    else return ESP_ERR_NOT_FOUND;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    size_t n = strlcpy(out, src, cap);
    xSemaphoreGive(s_lock);
    return (n >= cap) ? ESP_ERR_INVALID_SIZE : ESP_OK;
}

esp_err_t sd_prefs_on_change(sd_prefs_change_cb_t cb, void *user)
{
    if (!cb) return ESP_ERR_INVALID_ARG;
    if (s_change_cb_count >= CHANGE_CB_MAX) return ESP_ERR_NO_MEM;
    s_change_cb[s_change_cb_count].cb   = cb;
    s_change_cb[s_change_cb_count].user = user;
    s_change_cb_count++;
    return ESP_OK;
}

// --- CLI ---------------------------------------------------------------

static void render_list(char *buf, size_t cap)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    snprintf(buf, cap,
             "{\"gestures\":%s,\"buzzer\":%s,\"quiet\":\"%s\",\"tz\":\"%s\"}",
             s_gestures ? "true" : "false",
             s_buzzer   ? "true" : "false",
             s_quiet[0] ? s_quiet : "off",
             s_tz);
    xSemaphoreGive(s_lock);
}

static sk_err_t cmd_prefs_list(sk_cli_ctx_t *ctx)
{
    char buf[96];
    render_list(buf, sizeof(buf));
    sk_cli_ok(ctx, buf);
    return SK_OK;
}

esp_err_t sd_prefs_set(const char *key, const char *value, bool dry_run)
{
    if (!key || !value) return ESP_ERR_INVALID_ARG;

    if (!strcmp(key, "gestures") || !strcmp(key, "buzzer")) {
        bool on;
        if (!parse_bool(value, &on)) return ESP_ERR_INVALID_ARG;
        if (dry_run) return ESP_OK;
        if (!strcmp(key, "gestures")) s_gestures = on; else s_buzzer = on;
        nvs_save_u8(key, on ? 1 : 0);
        sk_event_bus_publishf("prefs.changed", "{\"key\":\"%s\",\"value\":%s}",
                              key, on ? "true" : "false");
    } else if (!strcmp(key, "quiet")) {
        char norm[QUIET_MAX] = "";
        if (strcasecmp(value, "off") != 0) {   // "off" → boş string (kapalı)
            if (!quiet_format_valid(value) || strlen(value) >= QUIET_MAX) {
                return ESP_ERR_INVALID_ARG;
            }
            strlcpy(norm, value, sizeof(norm));
        }
        if (dry_run) return ESP_OK;
        xSemaphoreTake(s_lock, portMAX_DELAY);
        strlcpy(s_quiet, norm, sizeof(s_quiet));
        xSemaphoreGive(s_lock);
        nvs_save_str("quiet", norm);
        sk_event_bus_publishf("prefs.changed", "{\"key\":\"quiet\",\"value\":\"%s\"}",
                              norm[0] ? norm : "off");
    } else if (!strcmp(key, "tz")) {
        if (!tz_format_valid(value) || strlen(value) >= sizeof(s_tz)) {
            return ESP_ERR_INVALID_ARG;
        }
        if (dry_run) return ESP_OK;
        xSemaphoreTake(s_lock, portMAX_DELAY);
        strlcpy(s_tz, value, sizeof(s_tz));
        xSemaphoreGive(s_lock);
        nvs_save_str("tz", value);
        sk_event_bus_publishf("prefs.changed", "{\"key\":\"tz\",\"value\":\"%s\"}",
                              value);
    } else {
        return ESP_ERR_NOT_FOUND;
    }

    notify_change(key);
    return ESP_OK;
}

static sk_err_t cmd_prefs_set(sk_cli_ctx_t *ctx)
{
    // Makine modu: {"key":"gestures","value":"off"}; insan modu:
    // `prefs set gestures off` (pozisyonel) — ikisini de kabul et.
    const char *key = sk_cli_arg_named(ctx, "key");
    const char *val = sk_cli_arg_named(ctx, "value");
    if (!key) key = sk_cli_arg(ctx, 0);
    if (!val) val = sk_cli_arg(ctx, 1);
    if (!key || !val) {
        sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"expected\":\"key value\"}");
        return SK_OK;
    }

    esp_err_t err = sd_prefs_set(key, val, false);
    if (err == ESP_ERR_NOT_FOUND) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG,
                   "{\"known\":[\"gestures\",\"buzzer\",\"quiet\",\"tz\"]}");
        return SK_OK;
    }
    if (err != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG,
                   "{\"expected\":\"on|off | HH:MM-HH:MM | +HH:MM\"}");
        return SK_OK;
    }
    char buf[96];
    render_list(buf, sizeof(buf));
    sk_cli_ok(ctx, buf);
    return SK_OK;
}

static const sk_cli_command_t s_cmd_prefs_list = {
    .name    = "prefs.list",
    .summary = "Show feature switches (gestures / buzzer / quiet)",
    .usage   = "prefs list",
    .handler = cmd_prefs_list,
};
static const sk_cli_command_t s_cmd_prefs_set = {
    .name    = "prefs.set",
    .summary = "Set a feature switch",
    .usage   = "prefs set <gestures|buzzer|quiet|tz> <value>",
    .help_block =
        "prefs set gestures off     cift tik/uzun basis jestlerini kapat\n"
        "prefs set buzzer off       tum sesleri kapat\n"
        "prefs set quiet 23:00-07:00  sessiz saat araligi (gece sarmali ok)\n"
        "prefs set quiet off        sessiz saatleri kapat\n"
        "prefs set tz +02:00        yerel saat dilimi (quiet icin; cihaz UTC)",
    .handler = cmd_prefs_set,
};

// --- Factory reset -------------------------------------------------------

static void on_factory_reset(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    s_gestures = true;
    s_buzzer   = true;
    s_quiet[0] = '\0';
    strcpy(s_tz, "+00:00");
    ESP_LOGW(TAG, "factory reset: prefs varsayilanlara dondu");
}

// --- Init ----------------------------------------------------------------

esp_err_t sd_prefs_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    load_from_nvs();

    sk_cli_register(&s_cmd_prefs_list);
    sk_cli_register(&s_cmd_prefs_set);

    int sub;
    sk_event_bus_subscribe("device.factory-reset.requested",
                           on_factory_reset, NULL, &sub);

    sk_capabilities_register_book("sd_prefs", "1.0.0");

    ESP_LOGI(TAG, "ready (gestures=%s buzzer=%s quiet=%s)",
             s_gestures ? "on" : "off",
             s_buzzer   ? "on" : "off",
             s_quiet[0] ? s_quiet : "off");
    return ESP_OK;
}
