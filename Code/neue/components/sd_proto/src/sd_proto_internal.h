#pragma once

// sd_proto iç başlığı — protokol implementasyonları arası paylaşılan
// yardımcılar. Dış dünya sd_proto.h kullanır.

#include <stdbool.h>
#include "esp_err.h"
#include "cJSON.h"

#include "sd_util.h"
#include "sd_proto.h"

// Hangi implementasyonlar derlemede var (CMake SRCS listesiyle senkron).
#define SD_PROTO_HAS_UDP   1
#define SD_PROTO_HAS_MQTT  1

// Tipli cJSON erişimcileri sd_util'de (sd_jsonu_*) — burada yalnız eşleme.
int sd_proto_mapped_value(const cJSON *cmd_node, int value);

esp_err_t sd_proto_http_execute(const cJSON *profile_root, const cJSON *cmd_node,
                                const sd_target_t *tgt, int value, bool toggle,
                                int *out_status);
esp_err_t sd_proto_udp_execute(const cJSON *profile_root, const cJSON *cmd_node,
                               const sd_target_t *tgt, int value, bool toggle);
esp_err_t sd_proto_mqtt_execute(const cJSON *profile_root, const cJSON *cmd_node,
                                const sd_target_t *tgt, int value, bool toggle);
esp_err_t sd_proto_mqtt_module_init(void);
