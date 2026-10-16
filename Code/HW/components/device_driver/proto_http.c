/**
 * SynDimm - HTTP Protokol Motoru
 * esp_http_client ile GET/PUT/POST destegi
 * Template genisleme: {value}, {toggle}, {auth_key}, {device_id}
 *
 * KEEP-ALIVE: Ayni host:port'a tekrar eden isteklerde TCP baglantisi
 * acik tutulur. Host degisirse eski baglanti kapatilip yenisi acilir.
 */

#include "device_driver.h"
#include "sys_log.h"
#include "sd_util.h"
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


// ============================================================
// HTTP istek gonderme (keep-alive ile)
// ============================================================

esp_err_t proto_http_send_command(const char *host, int port,
                                  const char *method, const char *path,
                                  const char *body, const char *auth_key,
                                  const char *device_id, int value, bool toggle_val)
{
    sys_log_mark_event(SL_EV_HTTP_CMD, 0);   /* hedef cihaza komut: TX piki baglami */
    if (!host || !method || !path) return ESP_ERR_INVALID_ARG;
    if (strlen(host) == 0) {
        ESP_LOGW(TAG, "Host ayarlanmamis");
        return ESP_ERR_INVALID_STATE;
    }

    // Path template genislet
    char expanded_path[256];
    const sd_tmpl_vars_t vars = {
        .value = value, .toggle_str = toggle_val ? "true" : "false",
        .auth_key = auth_key, .device_id = device_id };
    sd_template_expand(path, expanded_path, sizeof(expanded_path), &vars);

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
        sd_template_expand(body, expanded_body, sizeof(expanded_body), &vars);
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

