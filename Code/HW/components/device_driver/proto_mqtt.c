/**
 * SynDimm - MQTT Protokol Motoru
 * esp_mqtt ile broker baglantisi, publish/subscribe
 * Template genisleme: {prefix}, {state}, {value}
 */

#include "device_driver.h"
#include "sys_log.h"
#include "sd_util.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "PROTO_MQTT";

// MQTT client
static esp_mqtt_client_handle_t s_mqtt_client = NULL;
static bool s_mqtt_connected = false;

// Son alinan state mesaji (subscribe'dan)
static char s_last_state_payload[512] = "";
static bool s_state_received = false;


// ============================================================
// Template genisleme
// ============================================================



// ============================================================
// MQTT event handler
// ============================================================

/* Bekleyen durum aboneligi: subscribe baglanti kurulmadan istenirse burada
 * saklanir ve CONNECTED olayinda (her yeniden baglanmada) uygulanir. Eskiden
 * init'ten hemen sonra cagrilan subscribe her zaman INVALID_STATE donuyordu. */
static char s_sub_topic[128] = {0};
static bool s_sub_pending = false;

static void apply_pending_subscribe(void)
{
    if (!s_sub_pending || !s_mqtt_client || !s_mqtt_connected) return;
    int msg_id = esp_mqtt_client_subscribe(s_mqtt_client, s_sub_topic, 0);
    ESP_LOGI(TAG, "MQTT subscribe: %s (%s)", s_sub_topic, msg_id >= 0 ? "ok" : "hata");
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT baglandi");
            s_mqtt_connected = true;
            apply_pending_subscribe();
            break;

        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "MQTT baglanti kesildi");
            s_mqtt_connected = false;
            break;

        case MQTT_EVENT_DATA:
            // Gelen mesaji sakla
            if (event->data_len > 0 && event->data_len < (int)sizeof(s_last_state_payload)) {
                memcpy(s_last_state_payload, event->data, event->data_len);
                s_last_state_payload[event->data_len] = '\0';
                s_state_received = true;
                ESP_LOGD(TAG, "MQTT data: topic=%.*s payload=%s",
                         event->topic_len, event->topic, s_last_state_payload);
            }
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT hata");
            break;

        default:
            break;
    }
}

// ============================================================
// Public API
// ============================================================

esp_err_t proto_mqtt_init(const char *broker, int port)
{
    if (!broker || strlen(broker) == 0) {
        ESP_LOGW(TAG, "MQTT broker ayarlanmamis");
        return ESP_ERR_INVALID_ARG;
    }

    // Onceki baglanti varsa kapat
    if (s_mqtt_client) {
        proto_mqtt_stop();
    }

    char uri[128];
    snprintf(uri, sizeof(uri), "mqtt://%s:%d", broker, port);

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = uri,
    };

    s_mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    if (!s_mqtt_client) {
        ESP_LOGE(TAG, "MQTT client olusturulamadi");
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_mqtt_client, ESP_EVENT_ANY_ID,
                                    mqtt_event_handler, NULL);

    esp_err_t err = esp_mqtt_client_start(s_mqtt_client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "MQTT baslatilamadi: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
    } else {
        ESP_LOGI(TAG, "MQTT baslatildi: %s", uri);
    }

    return err;
}

void proto_mqtt_stop(void)
{
    s_sub_pending = false;
    if (s_mqtt_client) {
        esp_mqtt_client_stop(s_mqtt_client);
        esp_mqtt_client_destroy(s_mqtt_client);
        s_mqtt_client = NULL;
        s_mqtt_connected = false;
        ESP_LOGI(TAG, "MQTT durduruldu");
    }
}

bool proto_mqtt_is_connected(void)
{
    return s_mqtt_connected;
}

esp_err_t proto_mqtt_send_command(const char *topic, const char *payload,
                                   const char *prefix, const char *state_prefix,
                                   int value, bool toggle_val)
{
    sys_log_mark_event(SL_EV_HTTP_CMD, 0);   /* hedef cihaza komut: TX piki baglami */
    if (!s_mqtt_client || !s_mqtt_connected) {
        ESP_LOGW(TAG, "MQTT bagli degil");
        return ESP_ERR_INVALID_STATE;
    }

    if (!topic || !payload) return ESP_ERR_INVALID_ARG;

    // Topic genislet
    char expanded_topic[128];
    const sd_tmpl_vars_t tv = { .prefix = prefix, .state = state_prefix };
    sd_template_expand(topic, expanded_topic, sizeof(expanded_topic), &tv);

    // Payload genislet
    char expanded_payload[256];
    const sd_tmpl_vars_t pv = { .value = value, .toggle_str = toggle_val ? "ON" : "OFF" };
    sd_template_expand(payload, expanded_payload, sizeof(expanded_payload), &pv);

    ESP_LOGI(TAG, "MQTT publish: %s = %s", expanded_topic, expanded_payload);

    int msg_id = esp_mqtt_client_publish(s_mqtt_client,
                                          expanded_topic,
                                          expanded_payload,
                                          strlen(expanded_payload),
                                          0,   // QoS 0
                                          0);  // retain = false

    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t proto_mqtt_subscribe_status(const char *topic, const char *prefix,
                                       const char *state_prefix)
{
    if (!topic || !*topic) return ESP_ERR_INVALID_ARG;
    const sd_tmpl_vars_t tv = { .prefix = prefix, .state = state_prefix };
    sd_template_expand(topic, s_sub_topic, sizeof(s_sub_topic), &tv);
    s_sub_pending = true;

    if (!s_mqtt_client || !s_mqtt_connected) {
        ESP_LOGI(TAG, "MQTT henuz bagli degil, subscribe baglaninca uygulanacak: %s", s_sub_topic);
        return ESP_OK;
    }
    apply_pending_subscribe();
    return ESP_OK;
}

