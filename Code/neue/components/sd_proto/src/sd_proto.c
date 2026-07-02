// sd_proto — dispatch + ortak değer eşleme. Protokol implementasyonları:
// sd_proto_http.c (T2.3), sd_proto_udp.c (T3.1), sd_proto_mqtt.c (T3.2).

#include <string.h>

#include "esp_log.h"

#include "sk_capabilities.h"
#include "sk_cli.h"
#include "sk_errors.h"

#include "sd_util.h"
#include "sd_template.h"
#include "sd_proto.h"
#include "sd_proto_internal.h"

static const char *TAG = "sd_proto";

// {value} için eşlenmiş değer: min/max kaynak aralığından map_min/map_max
// hedef aralığına (yalnız ikisi de tanımlıysa).
int sd_proto_mapped_value(const cJSON *cmd_node, int value)
{
    const cJSON *mm = cJSON_GetObjectItemCaseSensitive(cmd_node, "map_min");
    const cJSON *mx = cJSON_GetObjectItemCaseSensitive(cmd_node, "map_max");
    if (!cJSON_IsNumber(mm) || !cJSON_IsNumber(mx)) return value;
    int from_min = sd_jsonu_int(cmd_node, "min", 0);
    int from_max = sd_jsonu_int(cmd_node, "max", 100);
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

    const char *proto = sd_jsonu_str(profile_root, "protocol");
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
// Kod incelemesi: keep-alive singleton'ı sd_cmd task'ına aittir; bu komut
// kendi TEK SEFERLİK istemcisini kurar, paylaşılan duruma DOKUNMAZ.

#include "esp_http_client.h"

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
        sk_cli_err(ctx, SK_ERR_MISSING_ARG,
                   "{\"usage\":\"proto.http.test <host> <port> <path>\"}");
        return SK_OK;
    }

    char url[384];
    snprintf(url, sizeof(url), "http://%s:%ld%s", host, port, path);
    esp_http_client_config_t cfg = {
        .url        = url,
        .method     = HTTP_METHOD_GET,
        .timeout_ms = 3000,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }

    esp_err_t err = esp_http_client_perform(client);
    int status = (err == ESP_OK) ? esp_http_client_get_status_code(client) : 0;
    esp_http_client_cleanup(client);

    char buf[96];
    snprintf(buf, sizeof(buf), "{\"err\":\"%s\",\"status\":%d}",
             esp_err_to_name(err), status);
    sk_cli_ok(ctx, buf);
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
