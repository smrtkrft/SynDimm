/**
 * SynDimm - MQTT Protokol Motoru
 * esp_mqtt ile broker baglantisi, publish/subscribe
 * Template genisleme: {prefix}, {state}, {value}
 */

#include "device_driver.h"
#include "sys_log.h"
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

/* snprintf kesilmis olsa bile "yazilacak" uzunlugu doner; ilerlemeyi kalan alana kirp.
 * Aksi halde oi > out_size olur ve out[oi]='\0' tampon disina yazar (yigin bozulmasi). */
static inline size_t tmpl_advance(int n, size_t avail)
{
    if (n <= 0) return 0;
    if ((size_t)n >= avail) return avail > 0 ? avail - 1 : 0;
    return (size_t)n;
}

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
                oi += tmpl_advance(n, out_size - oi);
                ti += 8;
                continue;
            }
            if (strncmp(tmpl + ti, "{state}", 7) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 state_prefix ? state_prefix : "");
                oi += tmpl_advance(n, out_size - oi);
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
                oi += tmpl_advance(n, out_size - oi);
                ti += 7;
                continue;
            }
            if (strncmp(tmpl + ti, "{toggle}", 8) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 toggle_val ? "ON" : "OFF");
                oi += tmpl_advance(n, out_size - oi);
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
    if (!topic || !*topic) return ESP_ERR_INVALID_ARG;
    expand_topic(topic, s_sub_topic, sizeof(s_sub_topic), prefix, state_prefix);
    s_sub_pending = true;

    if (!s_mqtt_client || !s_mqtt_connected) {
        ESP_LOGI(TAG, "MQTT henuz bagli degil, subscribe baglaninca uygulanacak: %s", s_sub_topic);
        return ESP_OK;
    }
    apply_pending_subscribe();
    return ESP_OK;
}

