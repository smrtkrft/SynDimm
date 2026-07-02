// drv_shutter — panjur/motor sürücüsü. Mantık kaynağı: eski
// device_driver.c:570-599 (handle_shutter_event).
//
// Girdi → eylem:
//   ROTATE ±1     → pozisyon ± step, [min,max] kırpma → SEND_VALUE
//   CLICK         → SEND_STOP (hareketi durdur — panjurun asıl butonu)
//   DOUBLE_CLICK  → params.presets.double_click pozisyonuna git
//   LONG_PRESS    → params.presets.long_press pozisyonuna git

#include <stdlib.h>

#include "esp_log.h"

#include "sd_util.h"
#include "sd_behavior.h"
#include "sd_behaviors.h"

static const char *TAG = "drv_shutter";

typedef struct {
    int min, max, step;
    int preset_double;    // -1 = tanımsız
    int preset_long;
} shutter_priv_t;

static esp_err_t shutter_init(sd_behavior_ctx_t *ctx)
{
    shutter_priv_t *p = calloc(1, sizeof(*p));
    if (!p) return ESP_ERR_NO_MEM;

    const cJSON *profile = sd_ctx_profile(ctx);
    const cJSON *sv = NULL;
    if (profile) {
        const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(profile, "commands");
        sv = cmds ? cJSON_GetObjectItemCaseSensitive(cmds, "set_value") : NULL;
    }
    p->min  = sd_jsonu_int(sv, "min", 0);
    p->max  = sd_jsonu_int(sv, "max", 100);
    p->step = sd_jsonu_int(sv, "step", 1);
    if (p->max <= p->min) { p->min = 0; p->max = 100; }

    const cJSON *binding = sd_ctx_binding(ctx);
    const cJSON *params  = binding
        ? cJSON_GetObjectItemCaseSensitive(binding, "params") : NULL;
    p->step = sd_jsonu_int(params, "step", p->step);
    if (p->step < 1) p->step = 1;
    const cJSON *presets = params
        ? cJSON_GetObjectItemCaseSensitive(params, "presets") : NULL;
    p->preset_double = sd_jsonu_int(presets, "double_click", -1);
    p->preset_long   = sd_jsonu_int(presets, "long_press", -1);

    sd_ctx_set_priv(ctx, p);
    ESP_LOGI(TAG, "slot %d: aralik [%d,%d] adim %d",
             sd_ctx_slot(ctx), p->min, p->max, p->step);
    return ESP_OK;
}

static void shutter_deinit(sd_behavior_ctx_t *ctx)
{
    free(sd_ctx_priv(ctx));
    sd_ctx_set_priv(ctx, NULL);
}

static int clamp(int v, int lo, int hi) { return v < lo ? lo : (v > hi ? hi : v); }

static esp_err_t goto_pos(sd_behavior_ctx_t *ctx, shutter_priv_t *p, int target)
{
    int value = clamp(target, p->min, p->max);
    sd_ctx_set_value(ctx, value, value > p->min);
    return sd_ctx_send(ctx, SD_SEND_VALUE, value, value > p->min);
}

static esp_err_t shutter_on_input(sd_behavior_ctx_t *ctx, const sd_input_event_t *evt)
{
    shutter_priv_t *p = sd_ctx_priv(ctx);
    if (!p) return ESP_ERR_INVALID_STATE;

    switch (evt->type) {
        case SD_INPUT_ROTATE:
            return goto_pos(ctx, p,
                            sd_ctx_get_value(ctx) + evt->delta * p->step);
        case SD_INPUT_CLICK:
            // Panjur kliği = DUR (eski :594 semantiği).
            return sd_ctx_send(ctx, SD_SEND_STOP,
                               sd_ctx_get_value(ctx), sd_ctx_get_state(ctx));
        case SD_INPUT_DOUBLE_CLICK:
            if (p->preset_double >= 0) return goto_pos(ctx, p, p->preset_double);
            return ESP_OK;
        case SD_INPUT_LONG_PRESS:
            if (p->preset_long >= 0) return goto_pos(ctx, p, p->preset_long);
            return ESP_OK;
        default:
            return ESP_OK;
    }
}

static esp_err_t shutter_get_state(sd_behavior_ctx_t *ctx, char *json, size_t cap)
{
    snprintf(json, cap, "{\"value\":%d,\"state\":%s}",
             sd_ctx_get_value(ctx), sd_ctx_get_state(ctx) ? "true" : "false");
    return ESP_OK;
}

static esp_err_t shutter_test(sd_behavior_ctx_t *ctx)
{
    return sd_ctx_send(ctx, SD_SEND_VALUE,
                       sd_ctx_get_value(ctx), sd_ctx_get_state(ctx));
}

static const sd_behavior_t SHUTTER = {
    .id          = "shutter",
    .api_version = SD_BEHAVIOR_API_V1,
    .init        = shutter_init,
    .deinit      = shutter_deinit,
    .on_input    = shutter_on_input,
    .get_state   = shutter_get_state,
    .test        = shutter_test,
};

const sd_behavior_t *sd_behavior_shutter(void) { return &SHUTTER; }
