// sd_proto — dispatch + ortak değer eşleme. Protokol implementasyonları:
// sd_proto_http.c (T2.3), sd_proto_udp.c (T3.1), sd_proto_mqtt.c (T3.2).

#include <string.h>

#include "esp_log.h"

#include "sk_capabilities.h"
#include "sk_cli.h"
#include "sk_errors.h"

#include "sd_template.h"
#include "sd_proto.h"
#include "sd_proto_internal.h"

static const char *TAG = "sd_proto";

// cmd_node'dan int alan (yoksa def).
int sd_proto_node_int(const cJSON *node, const char *key, int def)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(node, key);
    return (it && cJSON_IsNumber(it)) ? (int)it->valuedouble : def;
}

const char *sd_proto_node_str(const cJSON *node, const char *key)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(node, key);
    return (it && cJSON_IsString(it)) ? it->valuestring : NULL;
}

// {value} için eşlenmiş değer: min/max kaynak aralığından map_min/map_max
// hedef aralığına (yalnız ikisi de tanımlıysa).
int sd_proto_mapped_value(const cJSON *cmd_node, int value)
{
    const cJSON *mm = cJSON_GetObjectItemCaseSensitive(cmd_node, "map_min");
    const cJSON *mx = cJSON_GetObjectItemCaseSensitive(cmd_node, "map_max");
    if (!cJSON_IsNumber(mm) || !cJSON_IsNumber(mx)) return value;
    int from_min = sd_proto_node_int(cmd_node, "min", 0);
    int from_max = sd_proto_node_int(cmd_node, "max", 100);
    return sd_map_value(value, from_min, from_max,
                        (int)mm->valuedouble, (int)mx->valuedouble);
}

esp_err_t sd_proto_execute(const cJSON *profile_root, const cJSON *cmd_node,
                           const sd_target_t *tgt, int value, bool toggle,
                           int *out_http_status)
{
    if (out_http_status) *out_http_status = 0;
    if (!cJSON_IsObject(profile_root) || !cJSON_IsObject(cmd_node) || !tgt) {
        return ESP_ERR_INVALID_ARG;
    }

    const char *proto = sd_proto_node_str(profile_root, "protocol");
    if (!proto) return ESP_ERR_INVALID_ARG;

    if (strcmp(proto, "http") == 0) {
        return sd_proto_http_execute(profile_root, cmd_node, tgt,
                                     value, toggle, out_http_status);
    }
    if (strcmp(proto, "udp") == 0) {
        return sd_proto_udp_execute(profile_root, cmd_node, tgt, value, toggle);
    }
    if (strcmp(proto, "mqtt") == 0) {
        return sd_proto_mqtt_execute(profile_root, cmd_node, tgt, value, toggle);
    }
    ESP_LOGW(TAG, "bilinmeyen protokol: %s", proto);
    return ESP_ERR_NOT_SUPPORTED;
}

// --- T3.1/T3.2'ye kadar stub'lar ------------------------------------------

#if !SD_PROTO_HAS_UDP
esp_err_t sd_proto_udp_execute(const cJSON *profile_root, const cJSON *cmd_node,
                               const sd_target_t *tgt, int value, bool toggle)
{
    (void)profile_root; (void)cmd_node; (void)tgt; (void)value; (void)toggle;
    return ESP_ERR_NOT_SUPPORTED;   // T3.1
}
#endif

#if !SD_PROTO_HAS_MQTT
esp_err_t sd_proto_mqtt_execute(const cJSON *profile_root, const cJSON *cmd_node,
                                const sd_target_t *tgt, int value, bool toggle)
{
    (void)profile_root; (void)cmd_node; (void)tgt; (void)value; (void)toggle;
    return ESP_ERR_NOT_SUPPORTED;   // T3.2
}
esp_err_t sd_proto_mqtt_acquire(const char *broker_uri, const char *client_id)
{
    (void)broker_uri; (void)client_id;
    return ESP_ERR_NOT_SUPPORTED;
}
void sd_proto_mqtt_release(void) {}
bool sd_proto_mqtt_is_connected(void) { return false; }
esp_err_t sd_proto_mqtt_subscribe(const char *topic, sd_proto_mqtt_rx_t cb, void *user)
{
    (void)topic; (void)cb; (void)user;
    return ESP_ERR_NOT_SUPPORTED;
}
esp_err_t sd_proto_mqtt_publish(const char *topic, const char *payload,
                                int qos, bool retain)
{
    (void)topic; (void)payload; (void)qos; (void)retain;
    return ESP_ERR_NOT_SUPPORTED;
}
#endif

// --- Gizli tezgah komutu -----------------------------------------------------
// ⚠️ CLI task'ında bloklar (≤3 sn) — yalnız geliştirme/donanım doğrulama.

static sk_err_t cmd_proto_http_test(sk_cli_ctx_t *ctx)
{
    const char *host = sk_cli_arg(ctx, 0);
    const char *path = sk_cli_arg(ctx, 2);
    long port = 80;
    if (!sk_cli_arg_long(ctx, "port", &port)) {
        const char *p = sk_cli_arg(ctx, 1);
        if (p) port = strtol(p, NULL, 10);
    }
    if (!host || !path) {
        sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"usage\":\"proto.http.test <host> <port> <path>\"}");
        return SK_OK;
    }

    cJSON *profile = cJSON_Parse("{\"protocol\":\"http\"}");
    cJSON *cmd     = cJSON_CreateObject();
    cJSON_AddStringToObject(cmd, "method", "GET");
    cJSON_AddStringToObject(cmd, "path", path);

    sd_target_t tgt = { .port = (int)port };
    strlcpy(tgt.host, host, sizeof(tgt.host));

    int status = 0;
    esp_err_t err = sd_proto_http_execute(profile, cmd, &tgt, 0, false, &status);

    char buf[96];
    snprintf(buf, sizeof(buf), "{\"err\":\"%s\",\"status\":%d}",
             esp_err_to_name(err), status);
    sk_cli_ok(ctx, buf);

    cJSON_Delete(profile);
    cJSON_Delete(cmd);
    return SK_OK;
}

static const sk_cli_command_t s_cmd_http_test = {
    .name    = "proto.http.test",
    .summary = "Bench: fire a GET at host:port/path",
    .usage   = "proto http test <host> <port> <path>",
    .hidden  = true,
    .handler = cmd_proto_http_test,
};

esp_err_t sd_proto_init(void)
{
    sk_cli_register(&s_cmd_http_test);
    sk_capabilities_register_book("sd_proto", "1.0.0");
    ESP_LOGI(TAG, "ready (http%s%s)",
             SD_PROTO_HAS_UDP ? "+udp" : "", SD_PROTO_HAS_MQTT ? "+mqtt" : "");
    return ESP_OK;
}
