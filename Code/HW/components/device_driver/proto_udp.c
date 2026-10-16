/**
 * SynDimm - UDP Protokol Motoru
 * UDP soket ile komut gonderme/alma (WiZ gibi cihazlar)
 * Template genisleme: {value}, {toggle}
 */

#include "device_driver.h"
#include "sys_log.h"
#include "sd_util.h"
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
    sys_log_mark_event(SL_EV_HTTP_CMD, 0);   /* hedef cihaza komut: TX piki baglami */
    if (!host || !packet_template) return ESP_ERR_INVALID_ARG;
    if (strlen(host) == 0) {
        ESP_LOGW(TAG, "Host ayarlanmamis");
        return ESP_ERR_INVALID_STATE;
    }

    char packet[512];
    const sd_tmpl_vars_t vars = { .value = value, .toggle_str = toggle_val ? "true" : "false" };
    sd_template_expand(packet_template, packet, sizeof(packet), &vars);

    ESP_LOGI(TAG, "UDP -> %s:%d %s", host, port, packet);

    int ret = udp_send_recv(host, port, packet, strlen(packet), NULL, 0);
    return (ret >= 0) ? ESP_OK : ESP_FAIL;
}

