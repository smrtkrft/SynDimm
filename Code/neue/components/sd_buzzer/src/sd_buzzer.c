// sd_buzzer — desen çalıcı. Port kaynağı: eski buzzer.c (LEDC konfigürasyonu
// buzzer.c:55-85, zamanlamalar :21-22). Fark: poll'lü durum makinesi yerine
// esp_timer one-shot zinciri; desen seti genişledi (OK/ERR/LONG).

#include <string.h>
#include <time.h>

#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "sk_capabilities.h"
#include "sk_cli.h"
#include "sk_errors.h"

#include "sd_pins.h"
#include "sd_prefs.h"
#include "sd_quiet.h"
#include "sd_buzzer.h"

static const char *TAG = "sd_buzzer";

#define BUZZER_FREQ_HZ      2000
#define BUZZER_LEDC_TIMER   LEDC_TIMER_0
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_DUTY_ON      128            // 8-bit'in %50'si

#define DIT_MS              80
#define GAP_MS              100
#define LONG_MS             400

// Desen = {süre_ms, çıkış} adım dizisi. Son adımdan sonra çıkış kapanır.
typedef struct { uint16_t ms; bool on; } bz_step_t;

static const bz_step_t STEPS_OK[]   = { {DIT_MS, true}, {GAP_MS, false}, {240, true} };
static const bz_step_t STEPS_ERR[]  = { {LONG_MS, true}, {150, false}, {LONG_MS, true} };
static const bz_step_t STEPS_DIT1[] = { {DIT_MS, true} };
static const bz_step_t STEPS_DIT2[] = { {DIT_MS, true}, {GAP_MS, false}, {DIT_MS, true} };
static const bz_step_t STEPS_DIT3[] = { {DIT_MS, true}, {GAP_MS, false}, {DIT_MS, true},
                                        {GAP_MS, false}, {DIT_MS, true} };
static const bz_step_t STEPS_LONG[] = { {LONG_MS, true} };

static const struct { const bz_step_t *steps; int len; const char *name; }
PATTERNS[] = {
    [SD_BEEP_OK]   = { STEPS_OK,   3, "ok"   },
    [SD_BEEP_ERR]  = { STEPS_ERR,  3, "err"  },
    [SD_BEEP_DIT1] = { STEPS_DIT1, 1, "dit1" },
    [SD_BEEP_DIT2] = { STEPS_DIT2, 3, "dit2" },
    [SD_BEEP_DIT3] = { STEPS_DIT3, 5, "dit3" },
    [SD_BEEP_LONG] = { STEPS_LONG, 1, "long" },
};
#define PATTERN_COUNT (sizeof(PATTERNS) / sizeof(PATTERNS[0]))

static esp_timer_handle_t s_timer;
static const bz_step_t   *s_steps;          // NULL = idle (yalnız timer task + play yazär)
static volatile int       s_len, s_idx;

// Prefs önbelleği — prefs.changed callback'i günceller, play yolu okur.
static bool     s_quiet_valid;
static uint16_t s_quiet_from, s_quiet_to;
static int16_t  s_tz_offset_min;

static void out_set(bool on)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, on ? BUZZER_DUTY_ON : 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

static void apply_step(void)
{
    out_set(s_steps[s_idx].on);
    esp_timer_start_once(s_timer, (uint64_t)s_steps[s_idx].ms * 1000);
}

static void timer_cb(void *arg)
{
    (void)arg;
    if (!s_steps) return;
    s_idx++;
    if (s_idx >= s_len) {
        out_set(false);
        s_steps = NULL;
        return;
    }
    apply_step();
}

// --- Sessiz saat kapısı ---------------------------------------------------

static void reload_prefs_cache(void)
{
    char buf[24];
    s_quiet_valid = (sd_prefs_get_str("quiet", buf, sizeof(buf)) == ESP_OK) &&
                    sd_quiet_parse(buf, &s_quiet_from, &s_quiet_to);
    s_tz_offset_min = 0;
    if (sd_prefs_get_str("tz", buf, sizeof(buf)) == ESP_OK) {
        int16_t off;
        if (sd_tz_parse(buf, &off)) s_tz_offset_min = off;
    }
}

static void on_prefs_changed(const char *key, void *user)
{
    (void)user;
    if (!strcmp(key, "quiet") || !strcmp(key, "tz")) reload_prefs_cache();
}

static bool quiet_now(void)
{
    if (!s_quiet_valid) return false;
    time_t now = time(NULL);
    if (now < 1700000000) return false;   // duvar saati yok → fail-open (bkz. .h)
    // UTC + tz ofseti → yerel gün-içi dakika.
    int day_min = (int)(((now + (time_t)s_tz_offset_min * 60) % 86400) / 60);
    if (day_min < 0) day_min += 1440;
    return sd_quiet_active(s_quiet_from, s_quiet_to, (uint16_t)day_min);
}

// --- Public API -------------------------------------------------------------

void sd_buzzer_play(sd_beep_t pattern)
{
    if ((unsigned)pattern >= PATTERN_COUNT) return;
    if (!sd_prefs_get_bool("buzzer", true)) return;
    if (quiet_now()) return;
    if (s_steps) return;                  // meşgul → yoksay (port semantiği)

    s_len   = PATTERNS[pattern].len;
    s_idx   = 0;
    s_steps = PATTERNS[pattern].steps;
    apply_step();
}

void sd_buzzer_stop(void)
{
    esp_timer_stop(s_timer);
    out_set(false);
    s_steps = NULL;
}

bool sd_buzzer_is_playing(void)
{
    return s_steps != NULL;
}

// --- CLI (gizli bench komutu) ------------------------------------------------

static sk_err_t cmd_buzzer_play(sk_cli_ctx_t *ctx)
{
    const char *name = sk_cli_arg_named(ctx, "pattern");
    if (!name) name = sk_cli_arg(ctx, 0);
    if (!name) {
        sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"pattern\"}");
        return SK_OK;
    }
    for (unsigned i = 0; i < PATTERN_COUNT; i++) {
        if (!strcmp(name, PATTERNS[i].name)) {
            sd_buzzer_play((sd_beep_t)i);
            char buf[64];
            snprintf(buf, sizeof(buf), "{\"pattern\":\"%s\",\"played\":%s}",
                     name, sd_buzzer_is_playing() ? "true" : "false");
            sk_cli_ok(ctx, buf);   // played:false → gate yuttu (prefs/quiet/meşgul)
            return SK_OK;
        }
    }
    sk_cli_err(ctx, SK_ERR_INVALID_ARG,
               "{\"known\":[\"ok\",\"err\",\"dit1\",\"dit2\",\"dit3\",\"long\"]}");
    return SK_OK;
}

static const sk_cli_command_t s_cmd_buzzer_play = {
    .name    = "buzzer.play",
    .summary = "Play a beep pattern (bench test)",
    .usage   = "buzzer play <ok|err|dit1|dit2|dit3|long>",
    .hidden  = true,   // tezgah/tanılama komutu — help'te görünmez
    .handler = cmd_buzzer_play,
};

// --- Init ---------------------------------------------------------------------

esp_err_t sd_buzzer_init(void)
{
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num       = BUZZER_LEDC_TIMER,
        .freq_hz         = BUZZER_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t err = ledc_timer_config(&timer_conf);
    if (err != ESP_OK) return err;

    ledc_channel_config_t ch_conf = {
        .gpio_num   = SD_PIN_BUZZER,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BUZZER_LEDC_CHANNEL,
        .timer_sel  = BUZZER_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    err = ledc_channel_config(&ch_conf);
    if (err != ESP_OK) return err;

    const esp_timer_create_args_t targs = {
        .callback = timer_cb,
        .name     = "sd_buzzer",
    };
    err = esp_timer_create(&targs, &s_timer);
    if (err != ESP_OK) return err;

    reload_prefs_cache();
    sd_prefs_on_change(on_prefs_changed, NULL);

    sk_cli_register(&s_cmd_buzzer_play);
    sk_capabilities_register_book("sd_buzzer", "1.0.0");

    ESP_LOGI(TAG, "ready (GPIO%d, %dHz, quiet=%s)",
             SD_PIN_BUZZER, BUZZER_FREQ_HZ, s_quiet_valid ? "set" : "off");
    return ESP_OK;
}
