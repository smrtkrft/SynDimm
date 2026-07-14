// drv_mqtt_remote — SynDimm'i evrensel MQTT giriş cihazı yapar: düğme
// hareketleri doğrudan bir topic'e basılır, mantığı Home Assistant /
// Node-RED kurar (kilitli karar: V1 seçimlik özelliği).
//
// Profil YOK — her şey bağlama parametrelerinde:
//   params.broker / broker_port   → motor yüklerken MQTT oturumu alır
//   params.topic                  → yayın topic'i
//   params.payload_value          → ROTATE şablonu ({value} 0-100)
//   params.payload_gesture        → CLICK şablonu ({toggle} → ON/OFF)
//   params.step                   → detent başına adım (vars. 1)
// Motor materialize() bu parametrelerden mqtt komutu sentezler; sürücü
// yalnız yerel değer/durum tutar.

#include <stdlib.h>

#include "esp_log.h"

#include "sd_util.h"
#include "sd_behavior.h"
#include "sd_behaviors.h"

static const char *TAG = "drv_mqtt_remote";

typedef struct { int step; } remote_priv_t;

static esp_err_t remote_init(sd_behavior_ctx_t *ctx)
{
    const cJSON *binding = sd_ctx_binding(ctx);
    const cJSON *params  = binding
        ? cJSON_GetObjectItemCaseSensitive(binding, "params") : NULL;
    if (!sd_jsonu_str(params, "topic")) {
        ESP_LOGE(TAG, "slot %d: params.topic yok", sd_ctx_slot(ctx));
        return ESP_ERR_INVALID_ARG;   // motor slot'u error'a düşürür
    }

    remote_priv_t *p = calloc(1, sizeof(*p));
    if (!p) return ESP_ERR_NO_MEM;
    p->step = sd_jsonu_int(params, "step", 1);
    if (p->step < 1) p->step = 1;
    sd_ctx_set_priv(ctx, p);

    ESP_LOGI(TAG, "slot %d: topic=%s adim=%d", sd_ctx_slot(ctx),
             sd_jsonu_str(params, "topic"), p->step);
    return ESP_OK;
}

static void remote_deinit(sd_behavior_ctx_t *ctx)
{
    free(sd_ctx_priv(ctx));
    sd_ctx_set_priv(ctx, NULL);
}

static esp_err_t remote_on_input(sd_behavior_ctx_t *ctx, const sd_input_event_t *evt)
{
    remote_priv_t *p = sd_ctx_priv(ctx);
    if (!p) return ESP_ERR_INVALID_STATE;

    switch (evt->type) {
        case SD_INPUT_ROTATE: {
            int value = sd_ctx_get_value(ctx) + evt->delta * p->step;
            if (value < 0)   value = 0;
            if (value > 100) value = 100;
            sd_ctx_set_value(ctx, value, value > 0);
            return sd_ctx_send(ctx, SD_SEND_VALUE, value, value > 0);
        }
        case SD_INPUT_CLICK: {
            bool state = !sd_ctx_get_state(ctx);
            sd_ctx_set_value(ctx, sd_ctx_get_value(ctx), state);
            return sd_ctx_send(ctx, SD_SEND_TOGGLE, sd_ctx_get_value(ctx), state);
        }
        default:
            // DOUBLE/LONG V1'de yayınlanmaz (input.gesture olayı zaten
            // event-bus'ta — SKAPP/system slotları oradan görebilir).
            return ESP_OK;
    }
}

static esp_err_t remote_get_state(sd_behavior_ctx_t *ctx, char *json, size_t cap)
{
    snprintf(json, cap, "{\"value\":%d,\"state\":%s}",
             sd_ctx_get_value(ctx), sd_ctx_get_state(ctx) ? "true" : "false");
    return ESP_OK;
}

static esp_err_t remote_test(sd_behavior_ctx_t *ctx)
{
    return sd_ctx_send(ctx, SD_SEND_VALUE,
                       sd_ctx_get_value(ctx), sd_ctx_get_state(ctx));
}

static const sd_behavior_t MQTT_REMOTE = {
    .id          = "mqtt_remote",
    .api_version = SD_BEHAVIOR_API_V1,
    .init        = remote_init,
    .deinit      = remote_deinit,
    .on_input    = remote_on_input,
    .get_state   = remote_get_state,
    .test        = remote_test,
};

const sd_behavior_t *sd_behavior_mqtt_remote(void) { return &MQTT_REMOTE; }
