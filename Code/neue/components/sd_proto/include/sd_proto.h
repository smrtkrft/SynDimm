#pragma once

// sd_proto — protokol yürütücüleri (HTTP / UDP / MQTT). Profil komut
// düğümünü (commands.set_value / .toggle / .stop) hedefe uygular.
//
// ⚠️ EŞZAMANLILIK SÖZLEŞMESİ: sd_proto_execute BLOKLAR ve YALNIZCA motorun
// sd_cmd task'ından çağrılır (tek-ağ-çağıranı kuralı, plan §topoloji).
// Tek istisna: gizli `proto.http.test` tezgah komutu (CLI task'ında koşar,
// yalnız geliştirme).
//
// Değer eşleme: cmd_node.{min,max} (vars. 0..100) kaynak aralığından
// cmd_node.{map_min,map_max} hedef aralığına (ikisi de varsa) sd_map_value
// ile çevrilir — {value} yer tutucusuna eşlenmiş değer girer.

#include <stdbool.h>
#include "esp_err.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char host[64];
    int  port;
    char device_id[32];
    char auth_key[128];
} sd_target_t;

// Tezgah komutunu ve (T3.2) MQTT oturum yönetimini kaydeder.
esp_err_t sd_proto_init(void);

// Bir profil komut düğümünü yürütür. profile_root'tan protocol/port/prefix
// okunur; cmd_node'dan method/path/body/packet/topic/payload + eşleme.
// out_http_status: HTTP'de cevap kodu (NULL olabilir; diğer protokollerde 0).
esp_err_t sd_proto_execute(const cJSON *profile_root, const cJSON *cmd_node,
                           const sd_target_t *tgt, int value, bool toggle,
                           int *out_http_status);

// --- MQTT oturumu (tek esp-mqtt istemcisi, refcount; V1 TEK broker) --------
// İkinci farklı URI → ESP_ERR_NOT_SUPPORTED (slot error'a düşer).
// T3.2'ye kadar stub: ESP_ERR_NOT_SUPPORTED döner.
esp_err_t sd_proto_mqtt_acquire(const char *broker_uri, const char *client_id);
void      sd_proto_mqtt_release(void);
bool      sd_proto_mqtt_is_connected(void);
typedef void (*sd_proto_mqtt_rx_t)(const char *topic, const char *payload, void *user);
// Ertelemeli abonelik: bağlantı yoksa saklanır, CONNECTED'ta ve her
// reconnect'te uygulanır (eski proto_mqtt.c:219-237 bug'ının düzeltmesi).
esp_err_t sd_proto_mqtt_subscribe(const char *topic, sd_proto_mqtt_rx_t cb, void *user);
esp_err_t sd_proto_mqtt_publish(const char *topic, const char *payload,
                                int qos, bool retain);

#ifdef __cplusplus
}
#endif
