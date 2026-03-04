/**
 * SynDimm - MQTT Protokol Motoru
 * esp_mqtt ile broker baglantisi, publish/subscribe
 * Template genisleme: {prefix}, {state}, {value}
 */

#include "device_driver.h"
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

static void expand_topic(const char *tmpl, char *out, size_t out_size,
                          const char *prefix, const char *state_prefix)
{
    size_t oi = 0;
    size_t ti = 0;
    size_t tlen = strlen(tmpl);

    while (ti < tlen && oi < out_size - 1) {
        if (tmpl[ti] == '{') {
            if (strncmp(tmpl + ti, "{prefix}", 8) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 prefix ? prefix : "");
                oi += n;
                ti += 8;
                continue;
            }
            if (strncmp(tmpl + ti, "{state}", 7) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 state_prefix ? state_prefix : "");
                oi += n;
                ti += 7;
                continue;
            }
        }
        out[oi++] = tmpl[ti++];
    }
    out[oi] = '\0';
}

static void expand_payload(const char *tmpl, char *out, size_t out_size,
                            int value, bool toggle_val)
{
    size_t oi = 0;
    size_t ti = 0;
    size_t tlen = strlen(tmpl);

    while (ti < tlen && oi < out_size - 1) {
        if (tmpl[ti] == '{') {
            if (strncmp(tmpl + ti, "{value}", 7) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%d", value);
                oi += n;
                ti += 7;
                continue;
            }
            if (strncmp(tmpl + ti, "{toggle}", 8) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 toggle_val ? "ON" : "OFF");
                oi += n;
                ti += 8;
                continue;
            }
        }
        out[oi++] = tmpl[ti++];
    }
    out[oi] = '\0';
}

// ============================================================
// MQTT event handler
// ============================================================

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT baglandi");
            s_mqtt_connected = true;
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
    if (!s_mqtt_client || !s_mqtt_connected) {
        ESP_LOGW(TAG, "MQTT bagli degil");
        return ESP_ERR_INVALID_STATE;
    }

    if (!topic || !payload) return ESP_ERR_INVALID_ARG;

    // Topic genislet
    char expanded_topic[128];
    expand_topic(topic, expanded_topic, sizeof(expanded_topic),
                 prefix, state_prefix);

    // Payload genislet
    char expanded_payload[256];
    expand_payload(payload, expanded_payload, sizeof(expanded_payload),
                   value, toggle_val);

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
    if (!s_mqtt_client || !s_mqtt_connected) {
        ESP_LOGW(TAG, "MQTT bagli degil, subscribe bekletiliyor");
        // Baglanti sonrasi otomatik subscribe icin topic'i saklayabiliriz
        // Simdilik sadece hata dondur
        return ESP_ERR_INVALID_STATE;
    }

    char expanded_topic[128];
    expand_topic(topic, expanded_topic, sizeof(expanded_topic),
                 prefix, state_prefix);

    ESP_LOGI(TAG, "MQTT subscribe: %s", expanded_topic);

    int msg_id = esp_mqtt_client_subscribe(s_mqtt_client, expanded_topic, 0);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t proto_mqtt_get_status(const char *value_key, const char *state_key,
                                 int *out_value, bool *out_state)
{
    if (!s_state_received) {
        return ESP_ERR_NOT_FOUND;
    }

    // JSON parse
    cJSON *root = cJSON_Parse(s_last_state_payload);
    if (!root) {
        // Duz string olabilir (ör: "ON", "OFF", "72")
        if (state_key && out_state) {
            *out_state = (strcasecmp(s_last_state_payload, "ON") == 0);
        }
        if (value_key && out_value) {
            int v = atoi(s_last_state_payload);
            if (v > 0) *out_value = v;
        }
        return ESP_OK;
    }

    if (value_key && out_value) {
        cJSON *val = cJSON_GetObjectItem(root, value_key);
        if (val && cJSON_IsNumber(val)) {
            *out_value = val->valueint;
        }
    }

    if (state_key && out_state) {
        cJSON *st = cJSON_GetObjectItem(root, state_key);
        if (st) {
            if (cJSON_IsString(st)) {
                *out_state = (strcasecmp(st->valuestring, "ON") == 0);
            } else if (cJSON_IsBool(st)) {
                *out_state = cJSON_IsTrue(st);
            }
        }
    }

    cJSON_Delete(root);
    return ESP_OK;
}
