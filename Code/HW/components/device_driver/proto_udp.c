/**
 * SynDimm - UDP Protokol Motoru
 * UDP soket ile komut gonderme/alma (WiZ gibi cihazlar)
 * Template genisleme: {value}, {toggle}
 */

#include "device_driver.h"
#include "esp_log.h"
#include "cJSON.h"
#include "lwip/sockets.h"
#include "lwip/netdb.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *TAG = "PROTO_UDP";

#define UDP_TIMEOUT_MS  2000
#define UDP_RX_BUF_SIZE 1024

// ============================================================
// Template genisleme
// ============================================================

static void expand_template(const char *tmpl, char *out, size_t out_size,
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
                                 toggle_val ? "true" : "false");
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
// UDP gonderim
// ============================================================

/**
 * UDP paketi gonder ve opsiyonel yanit al
 * @return yanit uzunlugu veya hata durumunda negatif
 */
static int udp_send_recv(const char *host, int port,
                          const char *data, size_t data_len,
                          char *rx_buf, size_t rx_buf_size)
{
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(port);

    if (inet_aton(host, &dest.sin_addr) == 0) {
        ESP_LOGE(TAG, "Gecersiz IP: %s", host);
        return -1;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        ESP_LOGE(TAG, "Socket olusturulamadi: errno %d", errno);
        return -1;
    }

    // Timeout ayarla
    struct timeval tv;
    tv.tv_sec = UDP_TIMEOUT_MS / 1000;
    tv.tv_usec = (UDP_TIMEOUT_MS % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    // Gonder
    int sent = sendto(sock, data, data_len, 0,
                      (struct sockaddr *)&dest, sizeof(dest));
    if (sent < 0) {
        ESP_LOGE(TAG, "sendto hatasi: errno %d", errno);
        close(sock);
        return -1;
    }

    ESP_LOGD(TAG, "UDP gonderildi %s:%d (%d bytes)", host, port, sent);

    // Yanit bekleniyor mu?
    if (!rx_buf || rx_buf_size == 0) {
        close(sock);
        return 0;
    }

    // Yanit al
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int rx_len = recvfrom(sock, rx_buf, rx_buf_size - 1, 0,
                          (struct sockaddr *)&from, &fromlen);

    close(sock);

    if (rx_len > 0) {
        rx_buf[rx_len] = '\0';
        ESP_LOGD(TAG, "UDP yanit: %s", rx_buf);
    }

    return rx_len;
}

// ============================================================
// Public API
// ============================================================

esp_err_t proto_udp_send_command(const char *host, int port,
                                  const char *packet_template,
                                  int value, bool toggle_val)
{
    if (!host || !packet_template) return ESP_ERR_INVALID_ARG;
    if (strlen(host) == 0) {
        ESP_LOGW(TAG, "Host ayarlanmamis");
        return ESP_ERR_INVALID_STATE;
    }

    char packet[512];
    expand_template(packet_template, packet, sizeof(packet), value, toggle_val);

    ESP_LOGI(TAG, "UDP -> %s:%d %s", host, port, packet);

    int ret = udp_send_recv(host, port, packet, strlen(packet), NULL, 0);
    return (ret >= 0) ? ESP_OK : ESP_FAIL;
}

esp_err_t proto_udp_get_status(const char *host, int port,
                                const char *packet_template,
                                const char *value_key, const char *state_key,
                                int *out_value, bool *out_state)
{
    if (!host || !packet_template) return ESP_ERR_INVALID_ARG;
    if (strlen(host) == 0) return ESP_ERR_INVALID_STATE;

    char packet[512];
    expand_template(packet_template, packet, sizeof(packet), 0, false);

    char rx_buf[UDP_RX_BUF_SIZE];
    int rx_len = udp_send_recv(host, port, packet, strlen(packet),
                                rx_buf, sizeof(rx_buf));

    if (rx_len <= 0) {
        ESP_LOGW(TAG, "UDP yanit alinamadi");
        return ESP_FAIL;
    }

    // JSON parse
    cJSON *root = cJSON_Parse(rx_buf);
    if (!root) {
        ESP_LOGE(TAG, "UDP yanit JSON parse hatasi");
        return ESP_FAIL;
    }

    // WiZ yanit formati: {"method":"getPilot","result":{"dimming":72,"state":true}}
    cJSON *result = cJSON_GetObjectItem(root, "result");
    cJSON *search = result ? result : root;

    if (value_key && out_value) {
        cJSON *val = cJSON_GetObjectItem(search, value_key);
        if (val && cJSON_IsNumber(val)) {
            *out_value = val->valueint;
        }
    }

    if (state_key && out_state) {
        cJSON *st = cJSON_GetObjectItem(search, state_key);
        if (st) {
            if (cJSON_IsBool(st)) {
                *out_state = cJSON_IsTrue(st);
            } else if (cJSON_IsString(st)) {
                *out_state = (strcasecmp(st->valuestring, "true") == 0);
            }
        }
    }

    cJSON_Delete(root);
    return ESP_OK;
}
