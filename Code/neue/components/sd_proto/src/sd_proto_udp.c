// sd_proto_udp — UDP yürütücü (WiZ tarzı cihazlar). Port kaynağı: eski
// proto_udp.c:61-141. Yalnız SAYISAL IP (inet_aton — DNS yok, dokümante
// sınırlama); gönderim-sadece (durum sorgusu ölü koddu, porte edilmedi).
// ⚠️ Yalnız sd_cmd task'ından çağrılır.

#include <string.h>

#include "esp_log.h"
#include "lwip/sockets.h"

#include "sd_template.h"
#include "sd_proto_internal.h"

static const char *TAG = "sd_udp";

esp_err_t sd_proto_udp_execute(const cJSON *profile_root, const cJSON *cmd_node,
                               const sd_target_t *tgt, int value, bool toggle)
{
    (void)profile_root;
    if (!tgt->host[0]) return ESP_ERR_INVALID_STATE;

    const char *packet_tmpl = sd_jsonu_str(cmd_node, "packet");
    if (!packet_tmpl) return ESP_ERR_INVALID_ARG;

    sd_tmpl_vars_t vars = {
        .value     = sd_proto_mapped_value(cmd_node, value),
        .toggle    = toggle,
        .auth_key  = tgt->auth_key[0]  ? tgt->auth_key  : NULL,
        .device_id = tgt->device_id[0] ? tgt->device_id : NULL,
    };
    char packet[512];
    size_t plen = sd_template_expand(packet_tmpl, packet, sizeof(packet), &vars);

    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(tgt->port > 0 ? tgt->port : 38899);   // WiZ vars.
    if (inet_aton(tgt->host, &dest.sin_addr) == 0) {
        ESP_LOGE(TAG, "gecersiz IP (DNS desteklenmez): %s", tgt->host);
        return ESP_ERR_INVALID_ARG;
    }

    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) return ESP_FAIL;

    int sent = sendto(sock, packet, plen, 0,
                      (struct sockaddr *)&dest, sizeof(dest));
    close(sock);

    if (sent < 0) {
        ESP_LOGW(TAG, "sendto hatasi: errno %d", errno);
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "UDP -> %s:%d (%d bayt)", tgt->host, ntohs(dest.sin_port), sent);
    return ESP_OK;
    // Dürüstlük notu (plan): yerel sendto başarısı ≠ teslimat — UDP'de
    // offline tespiti fiilen WiFi seviyesindedir.
}
