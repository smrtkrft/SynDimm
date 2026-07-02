// sd_proto_http — HTTP yürütücü. Port kaynağı: eski proto_http.c
// (keep-alive :29-74, gönderim :127-181). Ölü proto_http_get_status
// (:187-272) bilinçli olarak PORTE EDİLMEDİ.
//
// ⚠️ Yalnız sd_cmd task'ından çağrılır (+ gizli tezgah komutu istisnası) —
// keep-alive istemcisi tekildir ve kilitsizdir.

#include <stdio.h>
#include <string.h>

#include "esp_http_client.h"
#include "esp_log.h"

#include "sd_template.h"
#include "sd_proto_internal.h"

static const char *TAG = "sd_http";

#define HTTP_TIMEOUT_MS  3000

// Keep-alive: aynı host:port'a bağlantı açık tutulur (eski :21-23).
static esp_http_client_handle_t s_ka_client;
static char s_ka_host[64];
static int  s_ka_port;

static void ka_close(void)
{
    if (s_ka_client) {
        esp_http_client_cleanup(s_ka_client);
        s_ka_client = NULL;
        s_ka_host[0] = '\0';
        s_ka_port = 0;
    }
}

static esp_http_client_handle_t ka_get(const char *host, int port,
                                       const char *url,
                                       esp_http_client_method_t method)
{
    if (s_ka_client && (strcmp(s_ka_host, host) != 0 || s_ka_port != port)) {
        ka_close();
    }
    if (!s_ka_client) {
        esp_http_client_config_t cfg = {
            .url               = url,
            .method            = method,
            .timeout_ms        = HTTP_TIMEOUT_MS,
            .keep_alive_enable = true,
        };
        s_ka_client = esp_http_client_init(&cfg);
        if (!s_ka_client) return NULL;
        strlcpy(s_ka_host, host, sizeof(s_ka_host));
        s_ka_port = port;
    } else {
        esp_http_client_set_url(s_ka_client, url);
        esp_http_client_set_method(s_ka_client, method);
    }
    return s_ka_client;
}

esp_err_t sd_proto_http_execute(const cJSON *profile_root, const cJSON *cmd_node,
                                const sd_target_t *tgt, int value, bool toggle,
                                int *out_status)
{
    (void)profile_root;
    if (out_status) *out_status = 0;
    if (!tgt->host[0]) return ESP_ERR_INVALID_STATE;

    const char *path = sd_proto_node_str(cmd_node, "path");
    if (!path) return ESP_ERR_INVALID_ARG;
    const char *method = sd_proto_node_str(cmd_node, "method");
    const char *body   = sd_proto_node_str(cmd_node, "body");

    sd_tmpl_vars_t vars = {
        .value     = sd_proto_mapped_value(cmd_node, value),
        .toggle    = toggle,
        .auth_key  = tgt->auth_key[0]  ? tgt->auth_key  : NULL,
        .device_id = tgt->device_id[0] ? tgt->device_id : NULL,
        // HTTP'de toggle literalleri varsayılan "true"/"false"
    };

    char exp_path[256];
    sd_template_expand(path, exp_path, sizeof(exp_path), &vars);

    char url[384];
    snprintf(url, sizeof(url), "http://%s:%d%s",
             tgt->host, tgt->port > 0 ? tgt->port : 80, exp_path);

    esp_http_client_method_t m = HTTP_METHOD_GET;
    if (method) {
        if      (strcasecmp(method, "POST")   == 0) m = HTTP_METHOD_POST;
        else if (strcasecmp(method, "PUT")    == 0) m = HTTP_METHOD_PUT;
        else if (strcasecmp(method, "DELETE") == 0) m = HTTP_METHOD_DELETE;
    }

    esp_http_client_handle_t client = ka_get(tgt->host,
                                             tgt->port > 0 ? tgt->port : 80,
                                             url, m);
    if (!client) return ESP_FAIL;

    char exp_body[512];
    if (body && body[0]) {
        sd_template_expand(body, exp_body, sizeof(exp_body), &vars);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, exp_body, strlen(exp_body));
    } else {
        esp_http_client_set_post_field(client, NULL, 0);
    }

    ESP_LOGI(TAG, "%s %s", method ? method : "GET", url);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (out_status) *out_status = status;
        ESP_LOGD(TAG, "HTTP %d", status);
    } else {
        ESP_LOGW(TAG, "HTTP hata: %s — keep-alive kapatildi", esp_err_to_name(err));
        ka_close();   // kopan bağlantı sonraki denemede tazelenir
    }
    return err;
}
