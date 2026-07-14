// sd_feedback — bkz. sd_feedback.h. Eşleme tablosu (plan T1.5):
//   mode.changed {slot:1..3} → DIT1/DIT2/DIT3   (slot kimliği bip sayısı)
//   target.offline           → ERR
//   target.online            → OK
//   safe.triggered {ok}      → ok=true: DIT3, ok=false: LONG
//   safe.lockout             → ERR
//   boot (init anı)          → DIT1
// Not: handler'lar yayıncının task'ında senkron koşar (sk_event_bus.h) —
// sd_buzzer_play bloklamaz, doğrudan çağrı güvenli.

#include <stdlib.h>
#include <string.h>

#include "esp_log.h"

#include "sk_capabilities.h"
#include "sk_event_bus.h"

#include "sd_buzzer.h"
#include "sd_feedback.h"

static const char *TAG = "sd_feedback";

// Küçük JSON payload'da anahtarın değer başlangıcını bul (sk_control.c:59-64
// deseni — tam cJSON parse'a değmeyecek kadar ufak yükler). Tek tarayıcı,
// iki tip okuyucu (kod incelemesi: kopyalar ıraksamıştı).
static const char *payload_value(const char *json, const char *key)
{
    if (!json || !key) return NULL;
    char pat[32];
    snprintf(pat, sizeof(pat), "\"%s\"", key);
    const char *p = strstr(json, pat);
    if (!p) return NULL;
    p = strchr(p + strlen(pat), ':');
    if (!p) return NULL;
    p++;
    while (*p == ' ') p++;
    return p;
}

static long payload_long(const char *json, const char *key, long def)
{
    const char *v = payload_value(json, key);
    return v ? strtol(v, NULL, 10) : def;
}

static bool payload_bool(const char *json, const char *key, bool def)
{
    const char *v = payload_value(json, key);
    return v ? (strncmp(v, "true", 4) == 0) : def;
}

static void on_mode_changed(const sk_event_t *evt, void *user)
{
    (void)user;
    long slot = payload_long(evt ? evt->payload_json : NULL, "slot", 1);
    switch (slot) {
        case 2:  sd_buzzer_play(SD_BEEP_DIT2); break;
        case 3:  sd_buzzer_play(SD_BEEP_DIT3); break;
        default: sd_buzzer_play(SD_BEEP_DIT1); break;
    }
}

static void on_target_offline(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    sd_buzzer_play(SD_BEEP_ERR);
}

static void on_target_online(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    sd_buzzer_play(SD_BEEP_OK);
}

static void on_safe_triggered(const sk_event_t *evt, void *user)
{
    (void)user;
    bool ok = payload_bool(evt ? evt->payload_json : NULL, "ok", false);
    sd_buzzer_play(ok ? SD_BEEP_DIT3 : SD_BEEP_LONG);
}

static void on_safe_lockout(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    sd_buzzer_play(SD_BEEP_ERR);
}

esp_err_t sd_feedback_init(void)
{
    int sub;
    sk_event_bus_subscribe("mode.changed",   on_mode_changed,   NULL, &sub);
    sk_event_bus_subscribe("target.offline", on_target_offline, NULL, &sub);
    sk_event_bus_subscribe("target.online",  on_target_online,  NULL, &sub);
    sk_event_bus_subscribe("safe.triggered", on_safe_triggered, NULL, &sub);
    sk_event_bus_subscribe("safe.lockout",   on_safe_lockout,   NULL, &sub);

    sk_capabilities_register_book("sd_feedback", "1.0.0");

    sd_buzzer_play(SD_BEEP_DIT1);   // açılış onayı (eski main.c:217 davranışı)
    ESP_LOGI(TAG, "ready (5 olay aboneligi + boot bip)");
    return ESP_OK;
}
