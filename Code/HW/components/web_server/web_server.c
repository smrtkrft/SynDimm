/**
 * SynDimm - Web Server
 * Dahili flash: API endpointleri (ayarlar)
 * Harici flash: Statik dosyalar (HTML/CSS/JS/JSON)
 * Harici flash erisilemezse: Gomulu (embedded) dosyalardan sun
 */

#include "web_server.h"
#include "ext_flash.h"
#include "wifi_manager.h"
#include "system_info.h"
#include "device_driver.h"
#include "esp_http_server.h"
#include "esp_http_client.h"
#include "cJSON.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_random.h"
#include "esp_wifi.h"
#include "esp_mac.h"
#include "gui_ota.h"
#include "fw_ota.h"
#include <stdio.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

static const char *TAG = "WEB_SRV";
static httpd_handle_t s_server = NULL;

// SSE bagli istemciler (max 3 eszamanli baglanti)
#define SSE_MAX_CLIENTS 3
static int s_sse_fds[SSE_MAX_CLIENTS] = {-1, -1, -1};

// SSE gonderim task'i icin flag
static TaskHandle_t s_sse_task_handle = NULL;

// ============================================================
// Gomulu (embedded) GUI dosyalari - firmware icinden
// ============================================================
extern const char gui_index_html_start[] asm("_binary_index_html_start");
extern const char gui_index_html_end[]   asm("_binary_index_html_end");
extern const char gui_style_css_start[]  asm("_binary_style_css_start");
extern const char gui_style_css_end[]    asm("_binary_style_css_end");
extern const char gui_app_js_start[]     asm("_binary_app_js_start");
extern const char gui_app_js_end[]       asm("_binary_app_js_end");
extern const char gui_config_json_start[] asm("_binary_config_json_start");
extern const char gui_config_json_end[]   asm("_binary_config_json_end");
extern const char gui_logo_png_start[]      asm("_binary_logo_png_start");
extern const char gui_logo_png_end[]        asm("_binary_logo_png_end");
extern const char gui_darklogo_png_start[]  asm("_binary_darklogo_png_start");
extern const char gui_darklogo_png_end[]    asm("_binary_darklogo_png_end");
extern const char gui_lang_en_json_start[]  asm("_binary_en_json_start");
extern const char gui_lang_en_json_end[]    asm("_binary_en_json_end");
extern const char gui_lang_de_json_start[]  asm("_binary_de_json_start");
extern const char gui_lang_de_json_end[]    asm("_binary_de_json_end");
extern const char gui_lang_es_json_start[]  asm("_binary_es_json_start");
extern const char gui_lang_es_json_end[]    asm("_binary_es_json_end");
extern const char gui_lang_fr_json_start[]  asm("_binary_fr_json_start");
extern const char gui_lang_fr_json_end[]    asm("_binary_fr_json_end");
extern const char gui_lang_it_json_start[]  asm("_binary_it_json_start");
extern const char gui_lang_it_json_end[]    asm("_binary_it_json_end");
extern const char gui_lang_tr_json_start[]  asm("_binary_tr_json_start");
extern const char gui_lang_tr_json_end[]    asm("_binary_tr_json_end");
extern const char gui_setup_html_start[] asm("_binary_setup_html_start");
extern const char gui_setup_html_end[]   asm("_binary_setup_html_end");

// Gomulu dosya arama tablosu
typedef struct {
    const char *path;
    const char *start;
    const char *end;
    const char *content_type;
} embedded_file_t;

static const embedded_file_t s_embedded_files[] = {
    { "/index.html",   NULL, NULL, "text/html" },
    { "/style.css",    NULL, NULL, "text/css" },
    { "/app.js",       NULL, NULL, "application/javascript" },
    { "/config.json",  NULL, NULL, "application/json" },
    { "/pic/logo.png",     NULL, NULL, "image/png" },
    { "/pic/darklogo.png", NULL, NULL, "image/png" },
    { "/lang/en.json",    NULL, NULL, "application/json" },
    { "/lang/de.json",    NULL, NULL, "application/json" },
    { "/lang/es.json",    NULL, NULL, "application/json" },
    { "/lang/fr.json",    NULL, NULL, "application/json" },
    { "/lang/it.json",    NULL, NULL, "application/json" },
    { "/lang/tr.json",    NULL, NULL, "application/json" },
};
#define EMBEDDED_FILES_COUNT (sizeof(s_embedded_files) / sizeof(s_embedded_files[0]))

// Gomulu dosya bul - runtime init yapilmasi gerekiyor
static const char *get_embedded_start(int idx) {
    switch (idx) {
        case 0: return gui_index_html_start;
        case 1: return gui_style_css_start;
        case 2: return gui_app_js_start;
        case 3: return gui_config_json_start;
        case 4: return gui_logo_png_start;
        case 5: return gui_darklogo_png_start;
        case 6: return gui_lang_en_json_start;
        case 7: return gui_lang_de_json_start;
        case 8: return gui_lang_es_json_start;
        case 9: return gui_lang_fr_json_start;
        case 10: return gui_lang_it_json_start;
        case 11: return gui_lang_tr_json_start;
        default: return NULL;
    }
}

static const char *get_embedded_end(int idx) {
    switch (idx) {
        case 0: return gui_index_html_end;
        case 1: return gui_style_css_end;
        case 2: return gui_app_js_end;
        case 3: return gui_config_json_end;
        case 4: return gui_logo_png_end;
        case 5: return gui_darklogo_png_end;
        case 6: return gui_lang_en_json_end;
        case 7: return gui_lang_de_json_end;
        case 8: return gui_lang_es_json_end;
        case 9: return gui_lang_fr_json_end;
        case 10: return gui_lang_it_json_end;
        case 11: return gui_lang_tr_json_end;
        default: return NULL;
    }
}

// Basit JSON string parse helper
static int parse_json_string(const char *json, const char *key, char *out, size_t out_size)
{
    char search[64];
    snprintf(search, sizeof(search), "\"%s\":\"", key);
    char *start = strstr(json, search);
    if (!start) return -1;
    start += strlen(search);
    char *end = strchr(start, '"');
    if (!end) return -1;
    size_t len = end - start;
    if (len >= out_size) len = out_size - 1;
    strncpy(out, start, len);
    out[len] = '\0';
    return (int)len;
}

/* JSON string icindeki ozel karakterleri escape'le (" \ \n \r \t) */
static size_t json_escape(const char *src, char *dst, size_t dst_size)
{
    size_t si = 0, di = 0;
    while (src[si] && di < dst_size - 1) {
        char c = src[si++];
        const char *esc = NULL;
        switch (c) {
            case '"':  esc = "\\\""; break;
            case '\\': esc = "\\\\"; break;
            case '\n': esc = "\\n";  break;
            case '\r': esc = "\\r";  break;
            case '\t': esc = "\\t";  break;
            default: break;
        }
        if (esc) {
            size_t el = strlen(esc);
            if (di + el > dst_size - 1) break;
            memcpy(&dst[di], esc, el);
            di += el;
        } else if ((unsigned char)c >= 0x20) {
            dst[di++] = c;
        }
    }
    dst[di] = '\0';
    return di;
}

// ============================================================
// HTTP Handler'lar
// ============================================================

// Chunked dosya gonderim boyutu
#define FILE_CHUNK_SIZE 4096

// Aktif slot'tan dosyayi chunked olarak gonder (tek fopen/fclose)
static esp_err_t send_active_slot_file_chunked(httpd_req_t *req, const char *path)
{
    long file_size = 0;
    ext_flash_file_t f = ext_flash_active_fopen(path, &file_size);
    if (!f || file_size <= 0) return ESP_FAIL;

    char *chunk = malloc(FILE_CHUNK_SIZE);
    if (!chunk) {
        ext_flash_active_fclose(f);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "%s aktif slot'tan sunuluyor (%ld byte, chunked)", path, file_size);

    esp_err_t result = ESP_OK;
    int read_len;
    while ((read_len = ext_flash_active_fread(f, chunk, FILE_CHUNK_SIZE)) > 0) {
        esp_err_t err = httpd_resp_send_chunk(req, chunk, read_len);
        if (err != ESP_OK) {
            result = err;
            break;
        }
    }

    free(chunk);
    ext_flash_active_fclose(f);

    if (result == ESP_OK) {
        httpd_resp_send_chunk(req, NULL, 0); // Son chunk (bitis)
    }
    return result;
}

// Root-level statik dosya handler (style.css, app.js, pic/*, lang/* vb.)
// /ext/* handler'indan farki: URI'yi oldugu gibi dosya yolu olarak kullanir
static esp_err_t static_file_handler(httpd_req_t *req)
{
    const char *uri = req->uri;

    // Query string'i ayir
    char clean_path[128];
    strncpy(clean_path, uri, sizeof(clean_path) - 1);
    clean_path[sizeof(clean_path) - 1] = '\0';
    char *qmark = strchr(clean_path, '?');
    if (qmark) *qmark = '\0';

    // Once aktif slot'tan dene
    if (ext_flash_is_slot_a_mounted() || ext_flash_is_slot_b_mounted()) {
        long fsize = ext_flash_active_get_file_size(clean_path);
        if (fsize > 0) {
            const char *ext = strrchr(clean_path, '.');
            bool cacheable = false;
            if (ext) {
                if (strcmp(ext, ".html") == 0) httpd_resp_set_type(req, "text/html");
                else if (strcmp(ext, ".css") == 0)  { httpd_resp_set_type(req, "text/css"); cacheable = true; }
                else if (strcmp(ext, ".js") == 0)   { httpd_resp_set_type(req, "application/javascript"); cacheable = true; }
                else if (strcmp(ext, ".json") == 0) httpd_resp_set_type(req, "application/json");
                else if (strcmp(ext, ".png") == 0)  { httpd_resp_set_type(req, "image/png"); cacheable = true; }
                else if (strcmp(ext, ".ico") == 0)  { httpd_resp_set_type(req, "image/x-icon"); cacheable = true; }
                else httpd_resp_set_type(req, "text/plain");
            }
            if (cacheable) {
                httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000, immutable");
            } else {
                httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
            }
            return send_active_slot_file_chunked(req, clean_path);
        }
    }

    // Gomulu dosyalardan ara
    for (int i = 0; i < (int)EMBEDDED_FILES_COUNT; i++) {
        if (strcmp(clean_path, s_embedded_files[i].path) == 0) {
            const char *start = get_embedded_start(i);
            const char *end = get_embedded_end(i);
            size_t len = end - start;
            ESP_LOGI(TAG, "%s gomulu veriden sunuluyor (%zu byte)", clean_path, len);
            httpd_resp_set_type(req, s_embedded_files[i].content_type);
            return httpd_resp_send(req, start, len);
        }
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Dosya bulunamadi");
    return ESP_FAIL;
}

// GET / - Ana sayfa (aktif slot veya gomulu)
static esp_err_t root_handler(httpd_req_t *req)
{
    // 1. Harici flash aktif slot'tan index.html dene
    if (ext_flash_is_slot_a_mounted() || ext_flash_is_slot_b_mounted()) {
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
        if (send_active_slot_file_chunked(req, "/index.html") == ESP_OK) {
            return ESP_OK;  // Harici flash'ta GUI var → normal arayuz
        }
    }

    // 2. Harici flash'ta GUI YOK → setup.html goster (embedded fallback)
    size_t setup_len = gui_setup_html_end - gui_setup_html_start;
    if (setup_len > 0) {
        ESP_LOGI(TAG, "setup.html gomulu veriden sunuluyor (%zu byte)", setup_len);
        httpd_resp_set_type(req, "text/html");
        httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
        return httpd_resp_send(req, gui_setup_html_start, setup_len);
    }

    // 3. Son care: embedded index.html
    size_t len = gui_index_html_end - gui_index_html_start;
    ESP_LOGI(TAG, "index.html gomulu veriden sunuluyor (%zu byte)", len);
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    return httpd_resp_send(req, gui_index_html_start, len);
}

// GET /api/system/info - Sistem bilgisi (cihaz adi, AP, STA, mDNS, flash)
static esp_err_t api_system_info_handler(httpd_req_t *req)
{
    char resp[640];
    size_t slot_a_total = 0, slot_a_free = 0;
    size_t user_total = 0, user_free = 0;

    if (ext_flash_is_slot_a_mounted()) {
        ext_flash_get_slot_a_info(&slot_a_total, &slot_a_free);
    }
    if (ext_flash_is_user_data_mounted()) {
        ext_flash_get_info(&user_total, &user_free);
    }

    snprintf(resp, sizeof(resp),
             "{"
             "\"device_name\":\"%s\","
             "\"ip\":\"%s\","
             "\"ap_ssid\":\"%s\","
             "\"ap_ip\":\"192.168.4.1\","
             "\"sta_ssid\":\"%s\","
             "\"connected\":%s,"
             "\"mdns\":\"%s.local\","
             "\"ext_flash\":%s,"
             "\"slot_a_total\":%zu,"
             "\"slot_a_free\":%zu,"
             "\"user_data_total\":%zu,"
             "\"user_data_free\":%zu"
             "}",
             wifi_manager_get_device_name(),
             wifi_manager_get_ip(),
             wifi_manager_get_ap_ssid(),
             wifi_manager_get_sta_ssid(),
             wifi_manager_is_connected() ? "true" : "false",
             wifi_manager_get_device_name(),
             ext_flash_is_mounted() ? "true" : "false",
             slot_a_total / 1024,
             slot_a_free / 1024,
             user_total / 1024,
             user_free / 1024);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/wifi/connect - WiFi baglanti (ssid, password, static_ip)
static esp_err_t api_wifi_connect_handler(httpd_req_t *req)
{
    char buf[384];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};
    char static_ip[16] = {0};

    parse_json_string(buf, "ssid", ssid, sizeof(ssid));
    parse_json_string(buf, "password", password, sizeof(password));
    parse_json_string(buf, "static_ip", static_ip, sizeof(static_ip));

    ESP_LOGI("WEB_SRV", "WiFi connect - SSID:'%s' pass_len:%d ip:'%s'",
             ssid, (int)strlen(password), static_ip);

    if (strlen(ssid) == 0) {
        wifi_manager_clear_primary();
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"WiFi ayarlari silindi\"}", HTTPD_RESP_USE_STRLEN);
    }

    esp_err_t err = wifi_manager_connect_sta(ssid, password, static_ip);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"ok\",\"message\":\"Baglaniliyor: %s\"}", ssid);
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Hata: %s\"}", esp_err_to_name(err));
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/wifi/backup/save - Yedek WiFi kaydet
static esp_err_t api_wifi_backup_save_handler(httpd_req_t *req)
{
    char buf[384];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char ssid[33] = {0};
    char password[65] = {0};
    char static_ip[16] = {0};

    parse_json_string(buf, "ssid", ssid, sizeof(ssid));
    parse_json_string(buf, "password", password, sizeof(password));
    parse_json_string(buf, "static_ip", static_ip, sizeof(static_ip));

    ESP_LOGI("WEB_SRV", "Backup WiFi save - SSID:'%s' pass_len:%d ip:'%s'",
             ssid, (int)strlen(password), static_ip);

    if (strlen(ssid) == 0) {
        wifi_manager_clear_backup();
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"Yedek WiFi ayarlari silindi\"}", HTTPD_RESP_USE_STRLEN);
    }

    esp_err_t err = wifi_manager_save_backup(ssid, password, static_ip);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"ok\",\"message\":\"Yedek WiFi kaydedildi: %s\"}", ssid);
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Hata: %s\"}", esp_err_to_name(err));
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/wifi/config - WiFi ayarlarini getir (primary + backup)
static esp_err_t api_wifi_config_handler(httpd_req_t *req)
{
    // RSSI al (bagli ise)
    int rssi = 0;
    wifi_ap_record_t ap_rec;
    if (esp_wifi_sta_get_ap_info(&ap_rec) == ESP_OK) {
        rssi = ap_rec.rssi;
    }

    char resp[512];
    snprintf(resp, sizeof(resp),
             "{"
             "\"connected\":%s,"
             "\"ip\":\"%s\","
             "\"ssid\":\"%s\","
             "\"sta_ssid\":\"%s\","
             "\"backup_ssid\":\"%s\","
             "\"has_backup\":%s,"
             "\"ap_mode\":%s,"
             "\"ap_enabled\":%s,"
             "\"ap_ssid\":\"%s\","
             "\"mdns\":\"%s\","
             "\"rssi\":%d"
             "}",
             wifi_manager_is_connected() ? "true" : "false",
             wifi_manager_get_ip(),
             wifi_manager_get_sta_ssid(),
             wifi_manager_get_sta_ssid(),
             wifi_manager_get_backup_ssid(),
             wifi_manager_has_backup_config() ? "true" : "false",
             wifi_manager_is_ap_enabled() ? "true" : "false",
             wifi_manager_is_ap_enabled() ? "true" : "false",
             wifi_manager_get_ap_ssid(),
             wifi_manager_get_mdns_hostname(),
             rssi);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/wifi/ap/set - AP modunu ac/kapat
static esp_err_t api_wifi_ap_set_handler(httpd_req_t *req)
{
    char buf[64];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    // Boolean veya string olarak gelen "enabled" degerini parse et
    // JSON boolean: {"enabled":true} veya string: {"enabled":"true"}
    bool enabled = false;
    char enabled_str[8] = {0};
    if (parse_json_string(buf, "enabled", enabled_str, sizeof(enabled_str)) >= 0) {
        enabled = (strcmp(enabled_str, "true") == 0 || strcmp(enabled_str, "1") == 0);
    } else {
        // JSON boolean parse (tirnak isareti olmadan)
        enabled = (strstr(buf, "\"enabled\":true") != NULL);
    }

    esp_err_t err = wifi_manager_set_ap_enabled(enabled);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"ok\",\"ap_enabled\":%s}",
                 wifi_manager_is_ap_enabled() ? "true" : "false");
    } else if (err == ESP_ERR_INVALID_STATE) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"WiFi bagli degil, AP kapatilamaz\"}");
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"%s\"}", esp_err_to_name(err));
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/wifi/mdns - mDNS hostname ayarla
static esp_err_t api_wifi_mdns_handler(httpd_req_t *req)
{
    char buf[128];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[len] = '\0';

    char hostname[32] = {0};
    parse_json_string(buf, "hostname", hostname, sizeof(hostname));

    esp_err_t ret = wifi_manager_set_mdns_hostname(hostname);
    char resp[128];
    if (ret == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"ok\":true,\"mdns\":\"%s\"}", wifi_manager_get_mdns_hostname());
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"ok\":false,\"error\":\"%s\"}", esp_err_to_name(ret));
    }
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/modes - Mod yapilandirmasini oku
static esp_err_t api_modes_get_handler(httpd_req_t *req)
{
    char buf[1024];
    int len = ext_flash_read_file("/slot_a/modes.json", buf, sizeof(buf) - 1);
    if (len <= 0) {
        const char *empty = "{\"multiMode\":true,\"modes\":[]}";
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, empty, HTTPD_RESP_USE_STRLEN);
    }
    buf[len] = '\0';
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, buf, len);
}

// POST /api/modes - Mod yapilandirmasini kaydet
static esp_err_t api_modes_post_handler(httpd_req_t *req)
{
    char buf[1024];
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty body");
        return ESP_FAIL;
    }
    buf[len] = '\0';
    esp_err_t ret = ext_flash_write_file("/slot_a/modes.json", buf, len);
    httpd_resp_set_type(req, "application/json");
    if (ret == ESP_OK) {
        return httpd_resp_send(req, "{\"ok\":true}", HTTPD_RESP_USE_STRLEN);
    }
    httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Write failed");
    return ESP_FAIL;
}

// GET /api/system/detail - Detayli sistem bilgisi
static esp_err_t api_system_detail_handler(httpd_req_t *req)
{
    system_info_t info;
    system_info_get(&info);

    size_t slot_a_total = 0, slot_a_free = 0;
    size_t slot_b_total = 0, slot_b_free = 0;
    size_t reserve_total = 0, reserve_free = 0;
    size_t user_total = 0, user_free = 0;
    if (ext_flash_is_slot_a_mounted()) {
        ext_flash_get_slot_a_info(&slot_a_total, &slot_a_free);
    }
    if (ext_flash_is_slot_b_mounted()) {
        ext_flash_get_slot_b_info(&slot_b_total, &slot_b_free);
    }
    ext_flash_get_reserve_info(&reserve_total, &reserve_free);
    if (ext_flash_is_user_data_mounted()) {
        ext_flash_get_info(&user_total, &user_free);
    }

    // MAC adresi
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

    char resp[1024];
    snprintf(resp, sizeof(resp),
             "{"
             "\"chip\":\"%s\","
             "\"cores\":%u,"
             "\"cpu_mhz\":%lu,"
             "\"flash_kb\":%lu,"
             "\"flash_used_kb\":%lu,"
             "\"heap_total\":%lu,"
             "\"heap_free\":%lu,"
             "\"heap_min\":%lu,"
             "\"uptime\":%lld,"
             "\"rssi\":%d,"
             "\"reset_reason\":\"%s\","
             "\"fw_version\":\"%s\","
             "\"idf_version\":\"%s\","
             "\"mac\":\"%s\","
             "\"mdns\":\"%s.local\","
             "\"slot_a_total\":%zu,"
             "\"slot_a_free\":%zu,"
             "\"slot_b_total\":%zu,"
             "\"slot_b_free\":%zu,"
             "\"reserve_total\":%zu,"
             "\"reserve_free\":%zu,"
             "\"user_total\":%zu,"
             "\"user_free\":%zu"
             "}",
             info.chip_model,
             (unsigned)info.cores,
             (unsigned long)info.cpu_freq_mhz,
             (unsigned long)info.flash_size_kb,
             (unsigned long)info.flash_used_kb,
             (unsigned long)info.heap_total,
             (unsigned long)info.heap_free,
             (unsigned long)info.heap_min_free,
             (long long)info.uptime_seconds,
             info.wifi_rssi,
             info.reset_reason,
             info.fw_version,
             esp_get_idf_version(),
             mac_str,
             wifi_manager_get_device_name(),
             slot_a_total / 1024,
             slot_a_free / 1024,
             slot_b_total / 1024,
             slot_b_free / 1024,
             reserve_total / 1024,
             reserve_free / 1024,
             user_total / 1024,
             user_free / 1024);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/device/restart - Cihazi yeniden baslat
static esp_err_t api_device_restart_handler(httpd_req_t *req)
{
    ESP_LOGW(TAG, "Yeniden baslatma istegi alindi (web)");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

// POST /api/device/factory-reset - Fabrika ayarlarina don
static esp_err_t api_device_factory_reset_handler(httpd_req_t *req)
{
    ESP_LOGW(TAG, "Factory Reset istegi alindi (web)");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);

    nvs_flash_erase();
    ext_flash_format_user_data();

    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
    return ESP_OK;
}

// ============================================================
// SSE (Server-Sent Events) - Heartbeat
// ============================================================

// GET /api/events - SSE stream
static esp_err_t api_events_handler(httpd_req_t *req)
{
    int fd = httpd_req_to_sockfd(req);
    if (fd < 0) return ESP_FAIL;

    // Bos slot bul
    int slot = -1;
    for (int i = 0; i < SSE_MAX_CLIENTS; i++) {
        if (s_sse_fds[i] < 0) {
            slot = i;
            break;
        }
    }
    if (slot < 0) {
        httpd_sess_trigger_close(req->handle, s_sse_fds[0]);
        s_sse_fds[0] = -1;
        slot = 0;
    }

    s_sse_fds[slot] = fd;

    // SSE header'lari gonder
    httpd_resp_set_status(req, "200 OK");
    httpd_resp_set_type(req, "text/event-stream");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_set_hdr(req, "Connection", "keep-alive");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");

    httpd_resp_send_chunk(req, "data: connected\n\n", 17);

    ESP_LOGI(TAG, "SSE istemci baglandi (fd=%d, slot=%d)", fd, slot);
    return ESP_OK;
}

// SSE gonderim - cihaz durumu ile
static void sse_do_send(void)
{
    if (!s_server) return;

    dd_status_t st;
    device_driver_get_status(&st);

    char buf[256];
    int len = snprintf(buf, sizeof(buf),
        "data: {\"type\":\"state\",\"mode\":\"%s\",\"value\":%d,"
        "\"state\":\"%s\",\"connected\":%s,\"device\":\"%s\"}\n\n",
        st.mode_str, st.value,
        st.state ? "on" : "off",
        st.connected ? "true" : "false",
        st.device_name);

    for (int i = 0; i < SSE_MAX_CLIENTS; i++) {
        if (s_sse_fds[i] >= 0) {
            int ret = httpd_socket_send(s_server, s_sse_fds[i], buf, len, 0);
            if (ret < 0) {
                ESP_LOGW(TAG, "SSE gonderim basarisiz (fd=%d), kapatiliyor", s_sse_fds[i]);
                httpd_sess_trigger_close(s_server, s_sse_fds[i]);
                s_sse_fds[i] = -1;
            }
        }
    }
}

// SSE gonderim task'i
static void sse_send_task(void *arg)
{
    (void)arg;
    while (1) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        sse_do_send();
    }
}

// SSE event gonder
void web_server_send_sse_event(void)
{
    if (!s_server || !s_sse_task_handle) return;
    xTaskNotifyGive(s_sse_task_handle);
}

// GET /ext/* - Statik dosya sun (Slot A veya gomulu fallback)
static esp_err_t ext_file_handler(httpd_req_t *req)
{
    const char *uri = req->uri;
    const char *file_path = uri + 4;
    if (strlen(file_path) == 0 || strcmp(file_path, "/") == 0) {
        file_path = "/index.html";
    }

    // Query string'i ayir
    char clean_path[128];
    strncpy(clean_path, file_path, sizeof(clean_path) - 1);
    clean_path[sizeof(clean_path) - 1] = '\0';
    char *qmark = strchr(clean_path, '?');
    if (qmark) *qmark = '\0';

    // Once aktif slot'tan dene
    if (ext_flash_is_slot_a_mounted() || ext_flash_is_slot_b_mounted()) {
        long fsize = ext_flash_active_get_file_size(clean_path);
        if (fsize > 0) {
            const char *ext = strrchr(clean_path, '.');
            bool cacheable = false;
            if (ext) {
                if (strcmp(ext, ".html") == 0) httpd_resp_set_type(req, "text/html");
                else if (strcmp(ext, ".css") == 0)  { httpd_resp_set_type(req, "text/css"); cacheable = true; }
                else if (strcmp(ext, ".js") == 0)   { httpd_resp_set_type(req, "application/javascript"); cacheable = true; }
                else if (strcmp(ext, ".json") == 0) httpd_resp_set_type(req, "application/json");
                else if (strcmp(ext, ".png") == 0)  { httpd_resp_set_type(req, "image/png"); cacheable = true; }
                else if (strcmp(ext, ".ico") == 0)  { httpd_resp_set_type(req, "image/x-icon"); cacheable = true; }
                else httpd_resp_set_type(req, "text/plain");
            }
            if (cacheable) {
                httpd_resp_set_hdr(req, "Cache-Control", "public, max-age=31536000, immutable");
            } else {
                httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
            }
            return send_active_slot_file_chunked(req, clean_path);
        }
    }

    // Gomulu dosyalardan ara
    for (int i = 0; i < (int)EMBEDDED_FILES_COUNT; i++) {
        if (strcmp(clean_path, s_embedded_files[i].path) == 0) {
            const char *start = get_embedded_start(i);
            const char *end = get_embedded_end(i);
            size_t len = end - start;
            ESP_LOGI(TAG, "%s gomulu veriden sunuluyor (%zu byte)", file_path, len);
            httpd_resp_set_type(req, s_embedded_files[i].content_type);
            return httpd_resp_send(req, start, len);
        }
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Dosya bulunamadi");
    return ESP_FAIL;
}

// ============================================================
// Action Card API (NVS namespace: sd_actions) - Sadece trigger_api
// ============================================================

// GET /api/actions/get
static esp_err_t api_actions_get_handler(httpd_req_t *req)
{
    nvs_handle_t nvs;
    uint8_t trig_api = 0;

    if (nvs_open("sd_actions", NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u8(nvs, "trig_api", &trig_api);
        nvs_close(nvs);
    }

    char resp[64];
    snprintf(resp, sizeof(resp),
        "{\"trigger_api\":%s}",
        trig_api ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/actions/save
static esp_err_t api_actions_save_handler(httpd_req_t *req)
{
    char buf[128];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("sd_actions", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"NVS hatasi\"}", HTTPD_RESP_USE_STRLEN);
    }

    char val[8] = {0};
    if (parse_json_string(buf, "trigger_api", val, sizeof(val)) >= 0) {
        nvs_set_u8(nvs, "trig_api", (strcmp(val, "true") == 0) ? 1 : 0);
    }

    nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGI(TAG, "Action durumlar kaydedildi");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
}

// ============================================================
// Uzaktan Sifirlama API (NVS namespace: sd_reset)
// ============================================================

// GET /api/reset/config
static esp_err_t api_reset_config_get_handler(httpd_req_t *req)
{
    nvs_handle_t nvs;
    char api_key[16] = {0};
    uint8_t enabled = 0;
    bool has_key = false;
    char masked[16] = {0};

    if (nvs_open("sd_reset", NVS_READONLY, &nvs) == ESP_OK) {
        size_t len = sizeof(api_key);
        if (nvs_get_str(nvs, "api_key", api_key, &len) == ESP_OK && strlen(api_key) > 0) {
            has_key = true;
            // Maskeleme: sd_a3f7****
            size_t kl = strlen(api_key);
            if (kl > 4) {
                memcpy(masked, api_key, kl - 4);
                memcpy(masked + kl - 4, "****", 4);
                masked[kl] = '\0';
            } else {
                strcpy(masked, "****");
            }
        }
        nvs_get_u8(nvs, "enabled", &enabled);
        nvs_close(nvs);
    }

    char resp[128];
    snprintf(resp, sizeof(resp),
             "{\"enabled\":%s,\"has_key\":%s,\"masked_key\":\"%s\"}",
             enabled ? "true" : "false",
             has_key ? "true" : "false",
             masked);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/reset/config
static esp_err_t api_reset_config_save_handler(httpd_req_t *req)
{
    char buf[64];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    char enabled_str[8] = {0};
    parse_json_string(buf, "enabled", enabled_str, sizeof(enabled_str));
    uint8_t enabled = (strcmp(enabled_str, "true") == 0 || strcmp(enabled_str, "1") == 0) ? 1 : 0;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("sd_reset", NVS_READWRITE, &nvs);
    if (err == ESP_OK) {
        nvs_set_u8(nvs, "enabled", enabled);
        nvs_commit(nvs);
        nvs_close(nvs);
    }

    char resp[64];
    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"enabled\":%s}", enabled ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/reset/generate - Yeni API key uret
static esp_err_t api_reset_generate_handler(httpd_req_t *req)
{
    // sd_ + 8 hex = 11 karakter
    uint32_t rnd = esp_random();
    char api_key[16];
    snprintf(api_key, sizeof(api_key), "sd_%08lx", (unsigned long)rnd);

    nvs_handle_t nvs;
    esp_err_t err = nvs_open("sd_reset", NVS_READWRITE, &nvs);
    if (err != ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"NVS hatasi\"}", HTTPD_RESP_USE_STRLEN);
    }

    nvs_set_str(nvs, "api_key", api_key);
    nvs_set_u8(nvs, "enabled", 1);
    nvs_commit(nvs);
    nvs_close(nvs);

    char resp[64];
    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"api_key\":\"%s\"}", api_key);
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/reset?key=xxx - API tetiklemesi (dis erisim)
static esp_err_t api_reset_handler(httpd_req_t *req)
{
    nvs_handle_t nvs;
    uint8_t enabled = 0;
    char stored_key[16] = {0};

    if (nvs_open("sd_reset", NVS_READONLY, &nvs) == ESP_OK) {
        nvs_get_u8(nvs, "enabled", &enabled);
        size_t len = sizeof(stored_key);
        nvs_get_str(nvs, "api_key", stored_key, &len);
        nvs_close(nvs);
    }

    if (!enabled) {
        httpd_resp_set_status(req, "403 Forbidden");
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Uzaktan sifirlama devre disi\"}", HTTPD_RESP_USE_STRLEN);
    }

    // Query param: key
    char query[64] = {0};
    char key_param[16] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "key", key_param, sizeof(key_param));
    }

    if (strlen(key_param) == 0 || strcmp(key_param, stored_key) != 0) {
        httpd_resp_set_status(req, "401 Unauthorized");
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Gecersiz API anahtari\"}", HTTPD_RESP_USE_STRLEN);
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, "{\"status\":\"ok\",\"message\":\"API triggered\"}", HTTPD_RESP_USE_STRLEN);
}

// ============================================================
// GUI OTA API
// ============================================================

// GET /api/gui-ota/info
static esp_err_t api_gui_ota_info_handler(httpd_req_t *req)
{
    char active_ver[16] = {0};
    char inactive_ver[16] = {0};

    const char *active_mount = ext_flash_get_active_mount();
    {
        char path[64];
        snprintf(path, sizeof(path), "%s/config.json", active_mount);
        FILE *f = fopen(path, "r");
        if (f) {
            char buf[256];
            size_t len = fread(buf, 1, sizeof(buf) - 1, f);
            buf[len] = '\0';
            fclose(f);
            parse_json_string(buf, "version", active_ver, sizeof(active_ver));
        }
    }

    const char *inactive_mount = ext_flash_get_inactive_mount();
    {
        char path[64];
        snprintf(path, sizeof(path), "%s/config.json", inactive_mount);
        FILE *f = fopen(path, "r");
        if (f) {
            char buf[256];
            size_t len = fread(buf, 1, sizeof(buf) - 1, f);
            buf[len] = '\0';
            fclose(f);
            parse_json_string(buf, "version", inactive_ver, sizeof(inactive_ver));
        }
    }

    char slot_char = ext_flash_get_active_slot();
    bool slot_b_has_data = (strlen(inactive_ver) > 0);

    char resp[384];
    snprintf(resp, sizeof(resp),
             "{\"active_slot\":\"%c\","
             "\"active_version\":\"%s\","
             "\"inactive_version\":\"%s\","
             "\"inactive_has_data\":%s,"
             "\"slot_b_mounted\":%s,"
             "\"busy\":%s}",
             slot_char,
             active_ver,
             inactive_ver,
             slot_b_has_data ? "true" : "false",
             ext_flash_is_slot_b_mounted() ? "true" : "false",
             gui_ota_is_busy() ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/gui-ota/start
static esp_err_t api_gui_ota_start_handler(httpd_req_t *req)
{
    if (gui_ota_is_busy()) {
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"OTA zaten calisiyor\"}", HTTPD_RESP_USE_STRLEN);
    }

    gui_ota_params_t params;
    memset(&params, 0, sizeof(params));

    char buf[256] = {0};
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret > 0) {
        buf[ret] = '\0';
        char source[16] = {0};
        parse_json_string(buf, "source", source, sizeof(source));
        if (strcmp(source, "custom") == 0) {
            params.use_custom_source = true;
            parse_json_string(buf, "repo_url", params.repo_url, sizeof(params.repo_url));
            parse_json_string(buf, "branch", params.branch, sizeof(params.branch));
        }
    }

    esp_err_t err = gui_ota_start(&params);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"message\":\"OTA baslatildi\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"OTA baslatilamadi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/gui-ota/status
static esp_err_t api_gui_ota_status_handler(httpd_req_t *req)
{
    gui_ota_status_t st;
    gui_ota_get_status(&st);

    char esc_msg[256];
    json_escape(st.message, esc_msg, sizeof(esc_msg));

    char resp[512];
    snprintf(resp, sizeof(resp),
             "{\"state\":%d,"
             "\"file_index\":%d,"
             "\"file_count\":%d,"
             "\"progress_pct\":%d,"
             "\"message\":\"%s\","
             "\"remote_version\":\"%s\","
             "\"current_version\":\"%s\"}",
             (int)st.state,
             st.file_index,
             st.file_count,
             st.progress_pct,
             esc_msg,
             st.remote_version,
             st.current_version);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/gui-ota/rollback
static esp_err_t api_gui_ota_rollback_handler(httpd_req_t *req)
{
    char buf[32];
    httpd_req_recv(req, buf, sizeof(buf) - 1);

    esp_err_t err = gui_ota_rollback();
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"ok\",\"active_slot\":\"%c\"}",
                 ext_flash_get_active_slot());
    } else if (err == ESP_ERR_NOT_FOUND) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Diger slot'ta veri yok\"}");
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Rollback basarisiz\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// ============================================================
// Firmware OTA API
// ============================================================

// GET /api/fw-ota/info
static esp_err_t api_fw_ota_info_handler(httpd_req_t *req)
{
    const char *running = fw_ota_get_running_label();
    const char *rollback_ver = fw_ota_get_rollback_version();
    bool can_rollback = fw_ota_can_rollback();
    bool busy = fw_ota_is_busy();

    fw_ota_status_t st;
    fw_ota_get_status(&st);

    char resp[384];
    snprintf(resp, sizeof(resp),
             "{\"running_partition\":\"%s\","
             "\"current_version\":\"%s\","
             "\"can_rollback\":%s,"
             "\"rollback_version\":\"%s\","
             "\"busy\":%s}",
             running,
             st.current_version,
             can_rollback ? "true" : "false",
             rollback_ver ? rollback_ver : "",
             busy ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/fw-ota/check
static esp_err_t api_fw_ota_check_handler(httpd_req_t *req)
{
    if (fw_ota_is_busy()) {
        const char *resp = "{\"status\":\"error\",\"message\":\"OTA zaten calisiyor\"}";
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, resp, strlen(resp));
    }

    char body[512] = {0};
    int len = httpd_req_recv(req, body, sizeof(body) - 1);
    if (len > 0) body[len] = '\0';

    const char *custom_url = NULL;
    char url_buf[300] = {0};
    if (len > 0) {
        char source[16] = {0};
        parse_json_string(body, "source", source, sizeof(source));
        if (strcmp(source, "custom") == 0) {
            parse_json_string(body, "url", url_buf, sizeof(url_buf));
            if (strlen(url_buf) > 0) {
                custom_url = url_buf;
            }
        }
    }

    esp_err_t err = fw_ota_check(custom_url);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"message\":\"Kontrol basladi\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Baslatilamadi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/fw-ota/start
static esp_err_t api_fw_ota_start_handler(httpd_req_t *req)
{
    esp_err_t err = fw_ota_start();
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"message\":\"Indirme basladi\"}");
    } else if (err == ESP_ERR_INVALID_STATE) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Once versiyon kontrolu yapiniz\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Baslatilamadi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/fw-ota/status
static esp_err_t api_fw_ota_status_handler(httpd_req_t *req)
{
    fw_ota_status_t st;
    fw_ota_get_status(&st);

    char safe_msg[256];
    json_escape(st.message, safe_msg, sizeof(safe_msg));

    char resp[512];
    snprintf(resp, sizeof(resp),
             "{\"state\":%d,"
             "\"progress_pct\":%d,"
             "\"message\":\"%s\","
             "\"remote_version\":\"%s\","
             "\"current_version\":\"%s\"}",
             (int)st.state,
             st.progress_pct,
             safe_msg,
             st.remote_version,
             st.current_version);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/fw-ota/rollback
static esp_err_t api_fw_ota_rollback_handler(httpd_req_t *req)
{
    esp_err_t err = fw_ota_rollback();
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"ok\",\"message\":\"Rollback ayarlandi, restart gerekli\"}");
    } else if (err == ESP_ERR_NOT_FOUND) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Rollback partition bulunamadi\"}");
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Rollback hatasi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// ============================================================
// Device Driver API
// ============================================================

// GET /api/mode
static esp_err_t api_mode_get_handler(httpd_req_t *req)
{
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"mode\":\"%s\"}", device_driver_get_mode());
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/mode
static esp_err_t api_mode_set_handler(httpd_req_t *req)
{
    char buf[64] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body gerekli");
        return ESP_FAIL;
    }
    buf[len] = '\0';

    char mode[12] = {0};
    parse_json_string(buf, "mode", mode, sizeof(mode));
    if (strlen(mode) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "mode alani gerekli");
        return ESP_FAIL;
    }

    esp_err_t err = device_driver_set_mode(mode);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"mode\":\"%s\"}", device_driver_get_mode());
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Mod degistirilemedi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/profiles?mode=dimmer
static esp_err_t api_profiles_list_handler(httpd_req_t *req)
{
    // Query string'den mode parametresi
    char query[64] = {0};
    char mode[12] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "mode", mode, sizeof(mode));
    }

    // Mode belirtilmediyse aktif modu kullan
    if (strlen(mode) == 0) {
        strncpy(mode, device_driver_get_mode(), sizeof(mode) - 1);
    }

    char *json_out = malloc(4096);
    if (!json_out) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }

    device_driver_list_profiles(mode, json_out, 4096);

    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, json_out, strlen(json_out));
    free(json_out);
    return ret;
}

// GET /api/profiles/active
static esp_err_t api_profiles_active_handler(httpd_req_t *req)
{
    dd_status_t st;
    device_driver_get_status(&st);

    char resp[256];
    snprintf(resp, sizeof(resp),
             "{\"id\":\"%s\",\"name\":\"%s\",\"mode\":\"%s\",\"connected\":%s}",
             st.profile_id, st.device_name, st.mode_str,
             st.connected ? "true" : "false");

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/profiles/select
static esp_err_t api_profiles_select_handler(httpd_req_t *req)
{
    char buf[512] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body gerekli");
        return ESP_FAIL;
    }
    buf[len] = '\0';

    char id[32] = {0}, host[64] = {0}, device_id[16] = {0}, auth_key[128] = {0};
    parse_json_string(buf, "id", id, sizeof(id));
    parse_json_string(buf, "host", host, sizeof(host));
    parse_json_string(buf, "device_id", device_id, sizeof(device_id));
    parse_json_string(buf, "auth_key", auth_key, sizeof(auth_key));

    if (strlen(id) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id alani gerekli");
        return ESP_FAIL;
    }

    esp_err_t err = device_driver_select_profile(
        id,
        strlen(host) > 0 ? host : NULL,
        strlen(device_id) > 0 ? device_id : NULL,
        strlen(auth_key) > 0 ? auth_key : NULL);

    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"profile\":\"%s\"}", id);
    } else if (err == ESP_ERR_NOT_FOUND) {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Profil bulunamadi\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Profil yuklenemedi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/profiles/upload
static esp_err_t api_profiles_upload_handler(httpd_req_t *req)
{
    // Buyuk JSON dosyasi icin dinamik buffer
    int total = req->content_len;
    if (total <= 0 || total > 8192) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Gecersiz boyut (max 8KB)");
        return ESP_FAIL;
    }

    char *buf = malloc(total + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }

    int received = 0;
    while (received < total) {
        int ret = httpd_req_recv(req, buf + received, total - received);
        if (ret <= 0) {
            free(buf);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Okuma hatasi");
            return ESP_FAIL;
        }
        received += ret;
    }
    buf[received] = '\0';

    // JSON dogrulama ve id/mode bilgisi cikartma
    cJSON *root = cJSON_Parse(buf);
    if (!root) {
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Gecersiz JSON");
        return ESP_FAIL;
    }

    cJSON *id_j   = cJSON_GetObjectItem(root, "id");
    cJSON *mode_j = cJSON_GetObjectItem(root, "mode");

    if (!id_j || !cJSON_IsString(id_j) || !mode_j || !cJSON_IsString(mode_j)) {
        cJSON_Delete(root);
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id ve mode alanlari gerekli");
        return ESP_FAIL;
    }

    // Dosya yolunu olustur
    char path[128];
    snprintf(path, sizeof(path), "/slot_a/profiles/%s/%s.json",
             mode_j->valuestring, id_j->valuestring);

    esp_err_t err = ext_flash_write_file(path, buf, received);

    cJSON_Delete(root);
    free(buf);

    char resp[256];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"path\":\"%s\"}", path);
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Yazma hatasi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// DELETE /api/profiles?id=xxx
static esp_err_t api_profiles_delete_handler(httpd_req_t *req)
{
    char query[64] = {0};
    char id[32] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "id", id, sizeof(id));
    }

    if (strlen(id) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id parametresi gerekli");
        return ESP_FAIL;
    }

    esp_err_t err = device_driver_delete_profile(id);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Profil bulunamadi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/status (device status)
static esp_err_t api_device_status_handler(httpd_req_t *req)
{
    dd_status_t st;
    device_driver_get_status(&st);

    char esc_name[96];
    json_escape(st.device_name, esc_name, sizeof(esc_name));

    char resp[256];
    snprintf(resp, sizeof(resp),
             "{\"mode\":\"%s\",\"value\":%d,\"state\":\"%s\","
             "\"connected\":%s,\"device\":\"%s\",\"profile\":\"%s\"}",
             st.mode_str, st.value,
             st.state ? "on" : "off",
             st.connected ? "true" : "false",
             esc_name, st.profile_id);

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/safe/list
static esp_err_t api_safe_list_handler(httpd_req_t *req)
{
    char *json_out = malloc(4096);
    if (!json_out) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }

    device_driver_list_safe_profiles(json_out, 4096);

    httpd_resp_set_type(req, "application/json");
    esp_err_t ret = httpd_resp_send(req, json_out, strlen(json_out));
    free(json_out);
    return ret;
}

// POST /api/safe/add
static esp_err_t api_safe_add_handler(httpd_req_t *req)
{
    char buf[1024] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body gerekli");
        return ESP_FAIL;
    }
    buf[len] = '\0';

    esp_err_t err = device_driver_add_safe_profile(buf);
    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Kayit basarisiz\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// DELETE /api/safe?id=xxx
static esp_err_t api_safe_delete_handler(httpd_req_t *req)
{
    char query[64] = {0};
    char id[32] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "id", id, sizeof(id));
    }

    if (strlen(id) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id parametresi gerekli");
        return ESP_FAIL;
    }

    char path[128];
    snprintf(path, sizeof(path), "/slot_a/profiles/safe/%s.json", id);
    esp_err_t err = ext_flash_delete_file(path);

    char resp[128];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\"}");
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Profil bulunamadi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// GET /api/catalog/list
static esp_err_t api_catalog_list_handler(httpd_req_t *req)
{
    // Onbellekten oku
    char *buf = malloc(4096);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }

    int len = ext_flash_read_file("/ext/catalog_cache.json", buf, 4095);
    if (len > 0) {
        buf[len] = '\0';
        httpd_resp_set_type(req, "application/json");
        esp_err_t ret = httpd_resp_send(req, buf, len);
        free(buf);
        return ret;
    }

    free(buf);

    // Onbellek bos - bos dizi dondur
    const char *empty = "{\"profiles\":[]}";
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, empty, strlen(empty));
}

// POST /api/catalog/download  ← {"id":"hue_bulb","mode":"dimmer"}
static esp_err_t api_catalog_download_handler(httpd_req_t *req)
{
    char buf[128] = {0};
    int len = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Body gerekli");
        return ESP_FAIL;
    }
    buf[len] = '\0';

    char id[32] = {0}, mode[12] = {0};
    parse_json_string(buf, "id", id, sizeof(id));
    parse_json_string(buf, "mode", mode, sizeof(mode));

    if (strlen(id) == 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id alani gerekli");
        return ESP_FAIL;
    }
    if (strlen(mode) == 0) {
        strncpy(mode, device_driver_get_mode(), sizeof(mode) - 1);
    }

    // GitHub'dan indir
    // URL: https://raw.githubusercontent.com/smrtkrft/SynDimm/main/profiles/{mode}/{id}.json
    char url[256];
    snprintf(url, sizeof(url),
             "https://raw.githubusercontent.com/smrtkrft/SynDimm/main/profiles/%s/%s.json",
             mode, id);

    ESP_LOGI(TAG, "Katalog indirme: %s", url);

    // esp_http_client ile indir
    esp_http_client_config_t cfg = {
        .url = url,
        .timeout_ms = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "HTTP client olusturulamadi");
        return ESP_FAIL;
    }

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        esp_http_client_cleanup(client);
        char resp[128];
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Baglanti hatasi\"}");
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, resp, strlen(resp));
    }

    esp_http_client_fetch_headers(client);
    char *dl_buf = malloc(8192);
    if (!dl_buf) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }

    int dl_len = esp_http_client_read_response(client, dl_buf, 8191);
    int status_code = esp_http_client_get_status_code(client);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);

    if (dl_len <= 0 || status_code != 200) {
        free(dl_buf);
        char resp[128];
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Indirme basarisiz (HTTP %d)\"}", status_code);
        httpd_resp_set_type(req, "application/json");
        return httpd_resp_send(req, resp, strlen(resp));
    }
    dl_buf[dl_len] = '\0';

    // Dosyayi kaydet
    char path[128];
    snprintf(path, sizeof(path), "/slot_a/profiles/%s/%s.json", mode, id);
    err = ext_flash_write_file(path, dl_buf, dl_len);
    free(dl_buf);

    char resp[256];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"id\":\"%s\",\"path\":\"%s\"}", id, path);
    } else {
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Dosya yazma hatasi\"}");
    }

    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, resp, strlen(resp));
}

// ============================================================
// System Logs API
// ============================================================

static esp_err_t api_system_logs_handler(httpd_req_t *req) {
    char *buf = malloc(4096);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }
    int len = error_log_get_recent(buf, 4096);
    httpd_resp_set_type(req, "application/json");
    httpd_resp_send(req, buf, len);
    free(buf);
    return ESP_OK;
}

// ============================================================
// Server Yonetim
// ============================================================

esp_err_t web_server_start(void)
{
    if (s_server) {
        ESP_LOGW(TAG, "Server zaten calisiyor");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = 47;
    config.stack_size = 8192;
    config.max_open_sockets = 7;
    config.lru_purge_enable = true;

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Server baslatilamadi: %s", esp_err_to_name(ret));
        return ret;
    }

    // --- Ana sayfa ---
    const httpd_uri_t root = {
        .uri = "/", .method = HTTP_GET, .handler = root_handler
    };
    httpd_register_uri_handler(s_server, &root);

    // --- System API ---
    const httpd_uri_t api_info = {
        .uri = "/api/system/info", .method = HTTP_GET, .handler = api_system_info_handler
    };
    httpd_register_uri_handler(s_server, &api_info);

    const httpd_uri_t api_sys_detail = {
        .uri = "/api/system/detail", .method = HTTP_GET, .handler = api_system_detail_handler
    };
    httpd_register_uri_handler(s_server, &api_sys_detail);

    const httpd_uri_t api_logs = {
        .uri = "/api/system/logs", .method = HTTP_GET, .handler = api_system_logs_handler
    };
    httpd_register_uri_handler(s_server, &api_logs);

    // --- WiFi API ---
    const httpd_uri_t api_wifi = {
        .uri = "/api/wifi/connect", .method = HTTP_POST, .handler = api_wifi_connect_handler
    };
    httpd_register_uri_handler(s_server, &api_wifi);

    const httpd_uri_t api_wifi_backup = {
        .uri = "/api/wifi/backup/save", .method = HTTP_POST, .handler = api_wifi_backup_save_handler
    };
    httpd_register_uri_handler(s_server, &api_wifi_backup);

    const httpd_uri_t api_wifi_cfg = {
        .uri = "/api/wifi/config", .method = HTTP_GET, .handler = api_wifi_config_handler
    };
    httpd_register_uri_handler(s_server, &api_wifi_cfg);

    const httpd_uri_t api_wifi_ap = {
        .uri = "/api/wifi/ap/set", .method = HTTP_POST, .handler = api_wifi_ap_set_handler
    };
    httpd_register_uri_handler(s_server, &api_wifi_ap);

    const httpd_uri_t api_wifi_mdns = {
        .uri = "/api/wifi/mdns", .method = HTTP_POST, .handler = api_wifi_mdns_handler
    };
    httpd_register_uri_handler(s_server, &api_wifi_mdns);

    // --- Modes API ---
    const httpd_uri_t api_modes_get = {
        .uri = "/api/modes", .method = HTTP_GET, .handler = api_modes_get_handler
    };
    httpd_register_uri_handler(s_server, &api_modes_get);

    const httpd_uri_t api_modes_post = {
        .uri = "/api/modes", .method = HTTP_POST, .handler = api_modes_post_handler
    };
    httpd_register_uri_handler(s_server, &api_modes_post);

    // --- Device Control API ---
    const httpd_uri_t api_restart = {
        .uri = "/api/device/restart", .method = HTTP_POST, .handler = api_device_restart_handler
    };
    httpd_register_uri_handler(s_server, &api_restart);

    const httpd_uri_t api_factory_reset = {
        .uri = "/api/device/factory-reset", .method = HTTP_POST, .handler = api_device_factory_reset_handler
    };
    httpd_register_uri_handler(s_server, &api_factory_reset);

    // --- Actions API ---
    const httpd_uri_t api_actions_get = {
        .uri = "/api/actions/get", .method = HTTP_GET, .handler = api_actions_get_handler
    };
    httpd_register_uri_handler(s_server, &api_actions_get);

    const httpd_uri_t api_actions_save = {
        .uri = "/api/actions/save", .method = HTTP_POST, .handler = api_actions_save_handler
    };
    httpd_register_uri_handler(s_server, &api_actions_save);

    // --- Reset API ---
    const httpd_uri_t api_reset_cfg_get = {
        .uri = "/api/reset/config", .method = HTTP_GET, .handler = api_reset_config_get_handler
    };
    httpd_register_uri_handler(s_server, &api_reset_cfg_get);

    const httpd_uri_t api_reset_cfg_save = {
        .uri = "/api/reset/config", .method = HTTP_POST, .handler = api_reset_config_save_handler
    };
    httpd_register_uri_handler(s_server, &api_reset_cfg_save);

    const httpd_uri_t api_reset_generate = {
        .uri = "/api/reset/generate", .method = HTTP_POST, .handler = api_reset_generate_handler
    };
    httpd_register_uri_handler(s_server, &api_reset_generate);

    const httpd_uri_t api_reset = {
        .uri = "/api/reset", .method = HTTP_GET, .handler = api_reset_handler
    };
    httpd_register_uri_handler(s_server, &api_reset);

    // --- GUI OTA API ---
    const httpd_uri_t api_gui_ota_info = {
        .uri = "/api/gui-ota/info", .method = HTTP_GET, .handler = api_gui_ota_info_handler
    };
    httpd_register_uri_handler(s_server, &api_gui_ota_info);

    const httpd_uri_t api_gui_ota_start = {
        .uri = "/api/gui-ota/start", .method = HTTP_POST, .handler = api_gui_ota_start_handler
    };
    httpd_register_uri_handler(s_server, &api_gui_ota_start);

    const httpd_uri_t api_gui_ota_status = {
        .uri = "/api/gui-ota/status", .method = HTTP_GET, .handler = api_gui_ota_status_handler
    };
    httpd_register_uri_handler(s_server, &api_gui_ota_status);

    const httpd_uri_t api_gui_ota_rollback = {
        .uri = "/api/gui-ota/rollback", .method = HTTP_POST, .handler = api_gui_ota_rollback_handler
    };
    httpd_register_uri_handler(s_server, &api_gui_ota_rollback);

    // --- Firmware OTA API ---
    const httpd_uri_t api_fw_ota_info = {
        .uri = "/api/fw-ota/info", .method = HTTP_GET, .handler = api_fw_ota_info_handler
    };
    httpd_register_uri_handler(s_server, &api_fw_ota_info);

    const httpd_uri_t api_fw_ota_check = {
        .uri = "/api/fw-ota/check", .method = HTTP_POST, .handler = api_fw_ota_check_handler
    };
    httpd_register_uri_handler(s_server, &api_fw_ota_check);

    const httpd_uri_t api_fw_ota_start = {
        .uri = "/api/fw-ota/start", .method = HTTP_POST, .handler = api_fw_ota_start_handler
    };
    httpd_register_uri_handler(s_server, &api_fw_ota_start);

    const httpd_uri_t api_fw_ota_status = {
        .uri = "/api/fw-ota/status", .method = HTTP_GET, .handler = api_fw_ota_status_handler
    };
    httpd_register_uri_handler(s_server, &api_fw_ota_status);

    const httpd_uri_t api_fw_ota_rollback = {
        .uri = "/api/fw-ota/rollback", .method = HTTP_POST, .handler = api_fw_ota_rollback_handler
    };
    httpd_register_uri_handler(s_server, &api_fw_ota_rollback);

    // --- Device Driver API ---
    const httpd_uri_t api_mode_get = {
        .uri = "/api/mode", .method = HTTP_GET, .handler = api_mode_get_handler
    };
    httpd_register_uri_handler(s_server, &api_mode_get);

    const httpd_uri_t api_mode_set = {
        .uri = "/api/mode", .method = HTTP_POST, .handler = api_mode_set_handler
    };
    httpd_register_uri_handler(s_server, &api_mode_set);

    const httpd_uri_t api_profiles = {
        .uri = "/api/profiles", .method = HTTP_GET, .handler = api_profiles_list_handler
    };
    httpd_register_uri_handler(s_server, &api_profiles);

    const httpd_uri_t api_profiles_active = {
        .uri = "/api/profiles/active", .method = HTTP_GET, .handler = api_profiles_active_handler
    };
    httpd_register_uri_handler(s_server, &api_profiles_active);

    const httpd_uri_t api_profiles_select = {
        .uri = "/api/profiles/select", .method = HTTP_POST, .handler = api_profiles_select_handler
    };
    httpd_register_uri_handler(s_server, &api_profiles_select);

    const httpd_uri_t api_profiles_upload = {
        .uri = "/api/profiles/upload", .method = HTTP_POST, .handler = api_profiles_upload_handler
    };
    httpd_register_uri_handler(s_server, &api_profiles_upload);

    const httpd_uri_t api_profiles_del = {
        .uri = "/api/profiles", .method = HTTP_DELETE, .handler = api_profiles_delete_handler
    };
    httpd_register_uri_handler(s_server, &api_profiles_del);

    const httpd_uri_t api_dev_status = {
        .uri = "/api/status", .method = HTTP_GET, .handler = api_device_status_handler
    };
    httpd_register_uri_handler(s_server, &api_dev_status);

    const httpd_uri_t api_safe_list = {
        .uri = "/api/safe/list", .method = HTTP_GET, .handler = api_safe_list_handler
    };
    httpd_register_uri_handler(s_server, &api_safe_list);

    const httpd_uri_t api_safe_add = {
        .uri = "/api/safe/add", .method = HTTP_POST, .handler = api_safe_add_handler
    };
    httpd_register_uri_handler(s_server, &api_safe_add);

    const httpd_uri_t api_safe_del = {
        .uri = "/api/safe", .method = HTTP_DELETE, .handler = api_safe_delete_handler
    };
    httpd_register_uri_handler(s_server, &api_safe_del);

    const httpd_uri_t api_catalog = {
        .uri = "/api/catalog/list", .method = HTTP_GET, .handler = api_catalog_list_handler
    };
    httpd_register_uri_handler(s_server, &api_catalog);

    const httpd_uri_t api_catalog_dl = {
        .uri = "/api/catalog/download", .method = HTTP_POST, .handler = api_catalog_download_handler
    };
    httpd_register_uri_handler(s_server, &api_catalog_dl);

    // --- SSE Events ---
    const httpd_uri_t api_events = {
        .uri = "/api/events", .method = HTTP_GET, .handler = api_events_handler
    };
    httpd_register_uri_handler(s_server, &api_events);

    // --- Statik dosyalar (harici flash + gomulu fallback) ---
    const httpd_uri_t ext_files = {
        .uri = "/ext/*", .method = HTTP_GET, .handler = ext_file_handler
    };
    httpd_register_uri_handler(s_server, &ext_files);

    // --- Root-level statik dosyalar (style.css, app.js, pic/*, lang/*) ---
    // EN SON kayit edilmeli - wildcard catch-all, API handler'larindan sonra
    const httpd_uri_t static_files = {
        .uri = "/*", .method = HTTP_GET, .handler = static_file_handler
    };
    httpd_register_uri_handler(s_server, &static_files);

    // SSE gonderim task'i
    if (!s_sse_task_handle) {
        xTaskCreate(sse_send_task, "sse_send", 3072, NULL, 5, &s_sse_task_handle);
    }

    // Device driver state degisiklik callback'i kaydet
    device_driver_set_notify_cb(web_server_send_sse_event);

    ESP_LOGI(TAG, "SynDimm Web server baslatildi (port: %d)", config.server_port);
    return ESP_OK;
}

esp_err_t web_server_stop(void)
{
    if (!s_server) {
        return ESP_OK;
    }

    esp_err_t ret = httpd_stop(s_server);
    s_server = NULL;
    ESP_LOGI(TAG, "Web server durduruldu");
    return ret;
}
