// drv_dimmer — parlaklık/seviye sürücüsü. Mantık kaynağı: eski
// device_driver.c:531-568 (handle_dimmer_event) + yeni eklemeler:
// preset'ler (çift tık / uzun basış) ve isteğe bağlı ivmelenme.
//
// Girdi → eylem:
//   ROTATE ±1        → value ± step (×ivme), [min,max] kırpma → SEND_VALUE
//   CLICK            → state toggle → SEND_TOGGLE
//   DOUBLE_CLICK     → params.presets.double_click değerine atla
//   LONG_PRESS       → params.presets.long_press değerine atla
// min/max/step önceliği: binding params.step > profil set_value.step > 1.

#include <stdlib.h>

#include "esp_log.h"

#include "sd_util.h"
#include "sd_behavior.h"
#include "sd_behaviors.h"

static const char *TAG = "drv_dimmer";

// İvmelenme: ardışık detent aralığına göre adım çarpanı. Eşikler saha
// ayarı gerektirebilir — sabitler tek yerde dursun.
#define ACCEL_FAST_MS    30    // ≤30 ms arayla tik → ×4
#define ACCEL_MED_MS     80    // ≤80 ms → ×2

typedef struct {
    int     min, max, step;
    bool    accel;
    int     preset_double;     // -1 = tanımsız
    int     preset_long;
    int64_t last_rotate_ms;
} dimmer_priv_t;

static esp_err_t dimmer_init(sd_behavior_ctx_t *ctx)
{
    dimmer_priv_t *p = calloc(1, sizeof(*p));
    if (!p) return ESP_ERR_NO_MEM;

    // Profilden aralık (commands.set_value.{min,max,step}), binding'den
    // params.{step,accel,presets} — binding profili ezer.
    const cJSON *profile = sd_ctx_profile(ctx);
    const cJSON *sv = NULL;
    if (profile) {
        const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(profile, "commands");
        sv = cmds ? cJSON_GetObjectItemCaseSensitive(cmds, "set_value") : NULL;
    }
    p->min  = sd_jsonu_int(sv, "min", 0);
    p->max  = sd_jsonu_int(sv, "max", 100);
    p->step = sd_jsonu_int(sv, "step", 1);
    if (p->max <= p->min) { p->min = 0; p->max = 100; }   // bozuk profile karşı

    const cJSON *binding = sd_ctx_binding(ctx);
    const cJSON *params  = binding
        ? cJSON_GetObjectItemCaseSensitive(binding, "params") : NULL;
    p->step  = sd_jsonu_int(params, "step", p->step);
    if (p->step < 1) p->step = 1;
    p->accel = sd_jsonu_bool(params, "accel", false);

    const cJSON *presets = params
        ? cJSON_GetObjectItemCaseSensitive(params, "presets") : NULL;
    p->preset_double = sd_jsonu_int(presets, "double_click", -1);
    p->preset_long   = sd_jsonu_int(presets, "long_press", -1);
    p->last_rotate_ms = 0;

    sd_ctx_set_priv(ctx, p);
    ESP_LOGI(TAG, "slot %d: aralik [%d,%d] adim %d accel=%d presets(%d,%d)",
             sd_ctx_slot(ctx), p->min, p->max, p->step, p->accel,
             p->preset_double, p->preset_long);
    return ESP_OK;
}

static void dimmer_deinit(sd_behavior_ctx_t *ctx)
{
    free(sd_ctx_priv(ctx));
    sd_ctx_set_priv(ctx, NULL);
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static esp_err_t jump_to(sd_behavior_ctx_t *ctx, dimmer_priv_t *p, int target)
{
    int value = clamp(target, p->min, p->max);
    bool state = value > p->min;
    sd_ctx_set_value(ctx, value, state);
    return sd_ctx_send(ctx, SD_SEND_VALUE, value, state);
}

static esp_err_t dimmer_on_input(sd_behavior_ctx_t *ctx, const sd_input_event_t *evt)
{
    dimmer_priv_t *p = sd_ctx_priv(ctx);
    if (!p) return ESP_ERR_INVALID_STATE;

    switch (evt->type) {
        case SD_INPUT_ROTATE: {
            int factor = 1;
            if (p->accel && p->last_rotate_ms > 0) {
                int64_t dt = evt->ts_ms - p->last_rotate_ms;
                if      (dt <= ACCEL_FAST_MS) factor = 4;
                else if (dt <= ACCEL_MED_MS)  factor = 2;
            }
            p->last_rotate_ms = evt->ts_ms;

            int value = clamp(sd_ctx_get_value(ctx) + evt->delta * p->step * factor,
                              p->min, p->max);
            bool state = value > p->min;                 // eski :545 semantiği
            sd_ctx_set_value(ctx, value, state);
            return sd_ctx_send(ctx, SD_SEND_VALUE, value, state);
        }
        case SD_INPUT_CLICK: {
            bool state = !sd_ctx_get_state(ctx);          // eski :556 toggle
            sd_ctx_set_value(ctx, sd_ctx_get_value(ctx), state);
            return sd_ctx_send(ctx, SD_SEND_TOGGLE, sd_ctx_get_value(ctx), state);
        }
        case SD_INPUT_DOUBLE_CLICK:
            if (p->preset_double >= 0) return jump_to(ctx, p, p->preset_double);
            return ESP_OK;
        case SD_INPUT_LONG_PRESS:
            if (p->preset_long >= 0) return jump_to(ctx, p, p->preset_long);
            return ESP_OK;
        default:
            return ESP_OK;
    }
}

static esp_err_t dimmer_get_state(sd_behavior_ctx_t *ctx, char *json, size_t cap)
{
    snprintf(json, cap, "{\"value\":%d,\"state\":%s}",
             sd_ctx_get_value(ctx), sd_ctx_get_state(ctx) ? "true" : "false");
    return ESP_OK;
}

static esp_err_t dimmer_test(sd_behavior_ctx_t *ctx)
{
    // Test ateşi: mevcut değeri hedefe bir kez gönder (cmd task'ta koşar).
    return sd_ctx_send(ctx, SD_SEND_VALUE,
                       sd_ctx_get_value(ctx), sd_ctx_get_state(ctx));
}

static const sd_behavior_t DIMMER = {
    .id          = "dimmer",
    .api_version = SD_BEHAVIOR_API_V1,
    .init        = dimmer_init,
    .deinit      = dimmer_deinit,
    .on_input    = dimmer_on_input,
    .get_state   = dimmer_get_state,
    .test        = dimmer_test,
};

const sd_behavior_t *sd_behavior_dimmer(void) { return &DIMMER; }
