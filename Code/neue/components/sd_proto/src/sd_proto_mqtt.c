// sd_proto_mqtt — MQTT oturumu + yürütücü. Eski proto_mqtt.c'den publish
// yolu porte edildi (:186-217); YENİDEN TASARIM:
//   * TEK esp-mqtt istemcisi, refcount'lu (V1 tek broker — ikinci farklı
//     URI ESP_ERR_NOT_SUPPORTED, slot error'a düşer; şema çoklu-broker'a
//     hazır, plan Risk #7).
//   * ERTELEMELİ abonelik tablosu: bağlantı yokken istenen abonelikler
//     saklanır, MQTT_EVENT_CONNECTED'ta ve HER reconnect'te uygulanır —
//     eski subscribe-before-connected bug'ının (proto_mqtt.c:219-237)
//     kalıcı düzeltmesi.
// Publish sd_cmd task'ından; acquire/release motor slot yaşam döngüsünden.

#include <stdio.h>
#include <string.h>

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#include "sd_template.h"
#include "sd_proto_internal.h"

static const char *TAG = "sd_mqtt";

#define SUB_MAX       4
#define TOPIC_MAX     128
#define PAYLOAD_MAX   256

static esp_mqtt_client_handle_t s_client;
static SemaphoreHandle_t        s_mtx;          // oturum durumu kilidi
static char                     s_uri[128];
static int                      s_refcount;
static volatile bool            s_connected;

static struct {
    char               topic[TOPIC_MAX];
    sd_proto_mqtt_rx_t cb;
    void              *user;
    bool               in_use;
} s_subs[SUB_MAX];

static void lock(void)   { xSemaphoreTake(s_mtx, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(s_mtx); }

static void apply_subscriptions(void)
{
    for (int i = 0; i < SUB_MAX; i++) {
        if (s_subs[i].in_use) {
            esp_mqtt_client_subscribe(s_client, s_subs[i].topic, 0);
            ESP_LOGI(TAG, "subscribe: %s", s_subs[i].topic);
        }
    }
}

static void mqtt_event_handler(void *args, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    (void)args; (void)base;
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "baglandi");
            s_connected = true;
            lock();
            apply_subscriptions();   // ertelenen + reconnect abonelikleri
            unlock();
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGW(TAG, "baglanti kesildi");
            s_connected = false;
            break;
        case MQTT_EVENT_DATA: {
            char topic[TOPIC_MAX];
            char payload[PAYLOAD_MAX];
            int tlen = event->topic_len < TOPIC_MAX - 1
                     ? event->topic_len : TOPIC_MAX - 1;
            int plen = event->data_len < PAYLOAD_MAX - 1
                     ? event->data_len : PAYLOAD_MAX - 1;
            memcpy(topic, event->topic, tlen);   topic[tlen] = '\0';
            memcpy(payload, event->data, plen);  payload[plen] = '\0';
            lock();
            for (int i = 0; i < SUB_MAX; i++) {
                if (s_subs[i].in_use && strcmp(s_subs[i].topic, topic) == 0) {
                    sd_proto_mqtt_rx_t cb = s_subs[i].cb;
                    void *user = s_subs[i].user;
                    unlock();
                    cb(topic, payload, user);   // kilit DIŞI callback
                    return;
                }
            }
            unlock();
            break;
        }
        case MQTT_EVENT_ERROR:
            ESP_LOGW(TAG, "hata olayi");
            break;
        default:
            break;
    }
}

// --- Oturum API ---------------------------------------------------------------

esp_err_t sd_proto_mqtt_acquire(const char *broker_uri, const char *client_id)
{
    if (!broker_uri || !broker_uri[0]) return ESP_ERR_INVALID_ARG;
    lock();
    if (s_client) {
        if (strcmp(s_uri, broker_uri) != 0) {
            unlock();
            ESP_LOGE(TAG, "V1 tek broker: '%s' aktifken '%s' istendi",
                     s_uri, broker_uri);
            return ESP_ERR_NOT_SUPPORTED;
        }
        s_refcount++;
        unlock();
        return ESP_OK;
    }

    esp_mqtt_client_config_t cfg = {
        .broker.address.uri  = broker_uri,
        .credentials.client_id = client_id,
    };
    s_client = esp_mqtt_client_init(&cfg);
    if (!s_client) {
        unlock();
        return ESP_FAIL;
    }
    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID,
                                   mqtt_event_handler, NULL);
    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK) {
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        unlock();
        return err;
    }
    strlcpy(s_uri, broker_uri, sizeof(s_uri));
    s_refcount = 1;
    unlock();
    ESP_LOGI(TAG, "baslatildi: %s (id=%s)", broker_uri, client_id ? client_id : "-");
    return ESP_OK;
}

void sd_proto_mqtt_release(void)
{
    lock();
    if (s_refcount > 0 && --s_refcount == 0 && s_client) {
        esp_mqtt_client_stop(s_client);
        esp_mqtt_client_destroy(s_client);
        s_client    = NULL;
        s_connected = false;
        s_uri[0]    = '\0';
        memset(s_subs, 0, sizeof(s_subs));
        ESP_LOGI(TAG, "durduruldu (refcount 0)");
    }
    unlock();
}

bool sd_proto_mqtt_is_connected(void) { return s_connected; }

esp_err_t sd_proto_mqtt_subscribe(const char *topic, sd_proto_mqtt_rx_t cb, void *user)
{
    if (!topic || strlen(topic) >= TOPIC_MAX) return ESP_ERR_INVALID_ARG;
    lock();
    int slot = -1;
    for (int i = 0; i < SUB_MAX; i++) {
        if (s_subs[i].in_use && strcmp(s_subs[i].topic, topic) == 0) {
            slot = i;   // aynı topic → cb güncelle
            break;
        }
        if (slot < 0 && !s_subs[i].in_use) slot = i;
    }
    if (slot < 0) {
        unlock();
        return ESP_ERR_NO_MEM;
    }
    strlcpy(s_subs[slot].topic, topic, TOPIC_MAX);
    s_subs[slot].cb     = cb;
    s_subs[slot].user   = user;
    s_subs[slot].in_use = true;
    bool now = s_connected && s_client;
    if (now) esp_mqtt_client_subscribe(s_client, topic, 0);
    unlock();
    ESP_LOGI(TAG, "abonelik %s: %s", now ? "uygulandi" : "ertelendi", topic);
    return ESP_OK;   // bağlantı yoksa CONNECTED'ta uygulanır
}

esp_err_t sd_proto_mqtt_publish(const char *topic, const char *payload,
                                int qos, bool retain)
{
    if (!s_client || !s_connected) return ESP_ERR_INVALID_STATE;
    int msg_id = esp_mqtt_client_publish(s_client, topic, payload,
                                         (int)strlen(payload), qos, retain);
    return (msg_id >= 0) ? ESP_OK : ESP_FAIL;
}

// --- Yürütücü -------------------------------------------------------------------

esp_err_t sd_proto_mqtt_execute(const cJSON *profile_root, const cJSON *cmd_node,
                                const sd_target_t *tgt, int value, bool toggle)
{
    (void)tgt;
    const char *topic_tmpl   = sd_jsonu_str(cmd_node, "topic");
    const char *payload_tmpl = sd_jsonu_str(cmd_node, "payload");
    if (!topic_tmpl || !payload_tmpl) return ESP_ERR_INVALID_ARG;

    sd_tmpl_vars_t vars = {
        .value        = sd_proto_mapped_value(cmd_node, value),
        .toggle       = toggle,
        .prefix       = sd_jsonu_str(profile_root, "prefix"),
        .state_topic  = sd_jsonu_str(profile_root, "state_prefix"),
        .toggle_true  = "ON",       // MQTT literalleri (eski :74-75)
        .toggle_false = "OFF",
    };

    char topic[TOPIC_MAX];
    char payload[PAYLOAD_MAX];
    sd_template_expand(topic_tmpl, topic, sizeof(topic), &vars);
    sd_template_expand(payload_tmpl, payload, sizeof(payload), &vars);

    ESP_LOGI(TAG, "publish: %s = %s", topic, payload);
    return sd_proto_mqtt_publish(topic, payload, 0, false);
}

// Modül init — sd_proto_init'ten çağrılır (mutex kurulumu).
esp_err_t sd_proto_mqtt_module_init(void)
{
    s_mtx = xSemaphoreCreateMutex();
    return s_mtx ? ESP_OK : ESP_ERR_NO_MEM;
}
