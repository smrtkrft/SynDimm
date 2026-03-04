/**
 * SynDimm - HTTP Protokol Motoru
 * esp_http_client ile GET/PUT/POST destegi
 * Template genisleme: {value}, {toggle}, {auth_key}, {device_id}
 *
 * KEEP-ALIVE: Ayni host:port'a tekrar eden isteklerde TCP baglantisi
 * acik tutulur. Host degisirse eski baglanti kapatilip yenisi acilir.
 */

#include "device_driver.h"
#include "esp_http_client.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "PROTO_HTTP";

// Keep-alive baglanti
static esp_http_client_handle_t s_keep_alive_client = NULL;
static char s_ka_host[64] = "";
static int  s_ka_port = 0;

// ============================================================
// Keep-alive yonetimi
// ============================================================

static void ka_close(void)
{
    if (s_keep_alive_client) {
        esp_http_client_cleanup(s_keep_alive_client);
        s_keep_alive_client = NULL;
        s_ka_host[0] = '\0';
        s_ka_port = 0;
    }
}

/**
 * Host:port ayni ise mevcut client'i dondur, degilse yenisini ac.
 * Donen client kullanildiktan sonra cleanup YAPILMAZ (keep-alive).
 */
static esp_http_client_handle_t ka_get_client(const char *host, int port,
                                                const char *url,
                                                esp_http_client_method_t method)
{
    // Host veya port degisti mi?
    if (s_keep_alive_client &&
        (strcmp(s_ka_host, host) != 0 || s_ka_port != port)) {
        ESP_LOGD(TAG, "Host degisti, baglanti yenileniyor");
        ka_close();
    }

    if (!s_keep_alive_client) {
        esp_http_client_config_t config = {
            .url = url,
            .method = method,
            .timeout_ms = 3000,
            .keep_alive_enable = true,
        };

        s_keep_alive_client = esp_http_client_init(&config);
        if (!s_keep_alive_client) return NULL;

        strncpy(s_ka_host, host, sizeof(s_ka_host) - 1);
        s_ka_port = port;
    } else {
        // Mevcut client'in URL ve method'unu guncelle
        esp_http_client_set_url(s_keep_alive_client, url);
        esp_http_client_set_method(s_keep_alive_client, method);
    }

    return s_keep_alive_client;
}

// ============================================================
// Template genisleme
// ============================================================

static void expand_template(const char *tmpl, char *out, size_t out_size,
                             int value, bool toggle_val,
                             const char *auth_key, const char *device_id)
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
                                 toggle_val ? "true" : "false");
                oi += n;
                ti += 8;
                continue;
            }
            if (strncmp(tmpl + ti, "{auth_key}", 10) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 auth_key ? auth_key : "");
                oi += n;
                ti += 10;
                continue;
            }
            if (strncmp(tmpl + ti, "{device_id}", 11) == 0) {
                int n = snprintf(out + oi, out_size - oi, "%s",
                                 device_id ? device_id : "");
                oi += n;
                ti += 11;
                continue;
            }
        }
        out[oi++] = tmpl[ti++];
    }
    out[oi] = '\0';
}

// ============================================================
// HTTP istek gonderme (keep-alive ile)
// ============================================================

esp_err_t proto_http_send_command(const char *host, int port,
                                  const char *method, const char *path,
                                  const char *body, const char *auth_key,
                                  const char *device_id, int value, bool toggle_val)
{
    if (!host || !method || !path) return ESP_ERR_INVALID_ARG;
    if (strlen(host) == 0) {
        ESP_LOGW(TAG, "Host ayarlanmamis");
        return ESP_ERR_INVALID_STATE;
    }

    // Path template genislet
    char expanded_path[256];
    expand_template(path, expanded_path, sizeof(expanded_path),
                    value, toggle_val, auth_key, device_id);

    // URL olustur
    char url[384];
    snprintf(url, sizeof(url), "http://%s:%d%s", host, port, expanded_path);

    // HTTP method belirle
    esp_http_client_method_t http_method = HTTP_METHOD_GET;
    if (strcasecmp(method, "POST") == 0) http_method = HTTP_METHOD_POST;
    else if (strcasecmp(method, "PUT") == 0) http_method = HTTP_METHOD_PUT;

    // Keep-alive client al
    esp_http_client_handle_t client = ka_get_client(host, port, url, http_method);
    if (!client) return ESP_FAIL;

    // Body varsa template genislet ve ayarla
    char expanded_body[512];
    if (body && strlen(body) > 0) {
        expand_template(body, expanded_body, sizeof(expanded_body),
                        value, toggle_val, auth_key, device_id);
        esp_http_client_set_header(client, "Content-Type", "application/json");
        esp_http_client_set_post_field(client, expanded_body, strlen(expanded_body));
    } else {
        // Onceki body'yi temizle
        esp_http_client_set_post_field(client, NULL, 0);
    }

    ESP_LOGI(TAG, "%s %s", method, url);

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        ESP_LOGD(TAG, "HTTP %d", status);
    } else {
        ESP_LOGE(TAG, "HTTP hata: %s - baglanti yenileniyor", esp_err_to_name(err));
        // Baglanti kopmussa temizle, sonraki sefere yeniden acilir
        ka_close();
    }

    return err;
}

// ============================================================
// HTTP durum sorgulama (keep-alive kullanmaz - tek seferlik)
// ============================================================

esp_err_t proto_http_get_status(const char *host, int port,
                                 const char *method, const char *path,
                                 const char *auth_key, const char *device_id,
                                 const char *value_key, const char *state_key,
                                 int *out_value, bool *out_state,
                                 int map_min, int map_max)
{
    if (!host || !path) return ESP_ERR_INVALID_ARG;
    if (strlen(host) == 0) return ESP_ERR_INVALID_STATE;

    char expanded_path[256];
    expand_template(path, expanded_path, sizeof(expanded_path),
                    0, false, auth_key, device_id);

    char url[384];
    snprintf(url, sizeof(url), "http://%s:%d%s", host, port, expanded_path);

    char *response_buf = malloc(2048);
    if (!response_buf) return ESP_ERR_NO_MEM;

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 3000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) {
        free(response_buf);
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        free(response_buf);
        return err;
    }

    esp_http_client_fetch_headers(client);
    int response_len = esp_http_client_read_response(client, response_buf, 2047);

    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (response_len <= 0) {
        free(response_buf);
        return ESP_FAIL;
    }
    response_buf[response_len] = '\0';

    cJSON *root = cJSON_Parse(response_buf);
    free(response_buf);

    if (!root) {
        ESP_LOGE(TAG, "Status JSON parse hatasi");
        return ESP_FAIL;
    }

    if (value_key && out_value) {
        cJSON *val = cJSON_GetObjectItem(root, value_key);
        if (val && cJSON_IsNumber(val)) {
            int raw = val->valueint;
            if (map_max > 0 && map_max != 100) {
                *out_value = raw * 100 / map_max;
            } else {
                *out_value = raw;
            }
        }
    }

    if (state_key && out_state) {
        cJSON *st = cJSON_GetObjectItem(root, state_key);
        if (st) {
            if (cJSON_IsBool(st)) {
                *out_state = cJSON_IsTrue(st);
            } else if (cJSON_IsString(st)) {
                *out_state = (strcasecmp(st->valuestring, "on") == 0 ||
                              strcmp(st->valuestring, "ON") == 0);
            }
        }
    }

    cJSON_Delete(root);
    return ESP_OK;
}
