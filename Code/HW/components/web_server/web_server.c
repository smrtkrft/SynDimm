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
#include "sys_log.h"
#include "sd_util.h"
#include "esp_timer.h"
#include "mbedtls/sha256.h"
#include "esp_crt_bundle.h"
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
/* index/style/app gomulu olarak gzip'li tutulur (Content-Encoding: gzip ile sunulur) */
extern const char gui_index_html_start[] asm("_binary_index_html_gz_start");
extern const char gui_index_html_end[]   asm("_binary_index_html_gz_end");
extern const char gui_style_css_start[]  asm("_binary_style_css_gz_start");
extern const char gui_style_css_end[]    asm("_binary_style_css_gz_end");
extern const char gui_app_js_start[]     asm("_binary_app_js_gz_start");
extern const char gui_app_js_end[]       asm("_binary_app_js_gz_end");
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
    bool gzip;              /* icerik gzip'li: Content-Encoding: gzip basligi gerekir */
} embedded_file_t;

static const embedded_file_t s_embedded_files[] = {
    { "/index.html",   NULL, NULL, "text/html",              true  },
    { "/style.css",    NULL, NULL, "text/css",               true  },
    { "/app.js",       NULL, NULL, "application/javascript", true  },
    { "/config.json",  NULL, NULL, "application/json",       false },
    { "/pic/logo.png",     NULL, NULL, "image/png",          false },
    { "/pic/darklogo.png", NULL, NULL, "image/png",          false },
    { "/lang/en.json",    NULL, NULL, "application/json",    false },
    { "/lang/de.json",    NULL, NULL, "application/json",    false },
    { "/lang/es.json",    NULL, NULL, "application/json",    false },
    { "/lang/fr.json",    NULL, NULL, "application/json",    false },
    { "/lang/it.json",    NULL, NULL, "application/json",    false },
    { "/lang/tr.json",    NULL, NULL, "application/json",    false },
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

/* JSON string alani oku -> ortak sd_json_get_str (cJSON) */
#define parse_json_string(json, key, out, size) sd_json_get_str((json), (key), (out), (size))

/* JSON govde gonder (tek satir: tip + govde) */
static esp_err_t send_json(httpd_req_t *req, const char *body)
{
    httpd_resp_set_type(req, "application/json");
    return httpd_resp_send(req, body, HTTPD_RESP_USE_STRLEN);
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
// Kimlik Dogrulama (cihaz parolasi + cerez oturumu)
//
// - Parola NVS "sd_auth"/"hash" icinde tuzlu SHA-256 (tuz = cihaz MAC) olarak durur.
// - Parola YOKSA her sey eskisi gibi aciktir (GUI uyari gosterir).
// - Parola VARSA /api/... (auth uclari haric) oturum cerezi "sd_sess" ister.
//   Statik GUI dosyalari aciktir; GUI 401 alinca giris ekrani gosterir.
// - Cerez SameSite=Strict + HttpOnly: baska sitelerden CSRF istekleri cerezsiz gider.
// - 5 hatali giris -> 30 sn kilit; denemeler sys_log'a yazilir.
// ============================================================
#define AUTH_NVS_NS        "sd_auth"
#define AUTH_MAX_SESSIONS  4
#define AUTH_TOKEN_LEN     32                      /* hex karakter */
#define AUTH_SESSION_TTL_S (30 * 24 * 3600)        /* bos oturum: 30 gun */
#define AUTH_FAIL_MAX      5
#define AUTH_LOCK_S        30

typedef struct {
    char token[AUTH_TOKEN_LEN + 1];
    uint32_t last_seen_s;
} auth_session_t;

static bool s_auth_enabled = false;
static char s_auth_hash[65] = {0};                 /* hex sha256 */
static auth_session_t s_sessions[AUTH_MAX_SESSIONS];
static int s_auth_fail = 0;
static uint32_t s_auth_lock_until = 0;

static uint32_t auth_now_s(void) { return (uint32_t)(esp_timer_get_time() / 1000000ULL); }

/* Zamanlama sizintisi olmadan karsilastirma */
static bool ct_equal(const char *a, const char *b, size_t n)
{
    unsigned char d = 0;
    for (size_t i = 0; i < n; i++) d |= (unsigned char)(a[i] ^ b[i]);
    return d == 0;
}

static void auth_hash_password(const char *pass, char out_hex[65])
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);
    char salted[160];
    int n = snprintf(salted, sizeof(salted), "%02x%02x%02x%02x%02x%02x:%s",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], pass);
    uint8_t dig[32];
    mbedtls_sha256((const unsigned char *)salted, (size_t)n, dig, 0);
    for (int i = 0; i < 32; i++) sprintf(out_hex + i * 2, "%02x", dig[i]);
    out_hex[64] = '\0';
}

static void auth_load(void)
{
    nvs_handle_t nvs;
    s_auth_enabled = false;
    if (nvs_open(AUTH_NVS_NS, NVS_READONLY, &nvs) == ESP_OK) {
        size_t len = sizeof(s_auth_hash);
        if (nvs_get_str(nvs, "hash", s_auth_hash, &len) == ESP_OK && strlen(s_auth_hash) == 64) {
            s_auth_enabled = true;
        }
        nvs_close(nvs);
    }
    memset(s_sessions, 0, sizeof(s_sessions));
    ESP_LOGI(TAG, "Kimlik dogrulama: %s", s_auth_enabled ? "ACIK (parola ayarli)" : "KAPALI (parola yok)");
}

static esp_err_t auth_store(const char *pass_or_null)
{
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(AUTH_NVS_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    if (pass_or_null && *pass_or_null) {
        auth_hash_password(pass_or_null, s_auth_hash);
        err = nvs_set_str(nvs, "hash", s_auth_hash);
        s_auth_enabled = (err == ESP_OK);
    } else {
        nvs_erase_key(nvs, "hash");
        s_auth_enabled = false;
        s_auth_hash[0] = '\0';
    }
    nvs_commit(nvs);
    nvs_close(nvs);
    memset(s_sessions, 0, sizeof(s_sessions));      /* tum oturumlar dusur */
    return err;
}

static void auth_new_token(char out[AUTH_TOKEN_LEN + 1])
{
    uint8_t rnd[AUTH_TOKEN_LEN / 2];
    esp_fill_random(rnd, sizeof(rnd));
    for (size_t i = 0; i < sizeof(rnd); i++) sprintf(out + i * 2, "%02x", rnd[i]);
    out[AUTH_TOKEN_LEN] = '\0';
}

static auth_session_t *auth_session_create(void)
{
    uint32_t now = auth_now_s();
    auth_session_t *victim = &s_sessions[0];
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (s_sessions[i].token[0] == '\0') { victim = &s_sessions[i]; break; }
        if (s_sessions[i].last_seen_s < victim->last_seen_s) victim = &s_sessions[i];
    }
    auth_new_token(victim->token);
    victim->last_seen_s = now;
    return victim;
}

/* Istekte gecerli oturum var mi? Cerez "sd_sess" veya "X-Auth-Token" basligi */
static bool auth_request_ok(httpd_req_t *req)
{
    char tok[AUTH_TOKEN_LEN + 1] = {0};
    char hdr[256];
    if (httpd_req_get_hdr_value_str(req, "X-Auth-Token", hdr, sizeof(hdr)) == ESP_OK) {
        strncpy(tok, hdr, AUTH_TOKEN_LEN);
    } else if (httpd_req_get_hdr_value_str(req, "Cookie", hdr, sizeof(hdr)) == ESP_OK) {
        const char *c = strstr(hdr, "sd_sess=");
        if (c) strncpy(tok, c + 8, AUTH_TOKEN_LEN);
    }
    if (strlen(tok) != AUTH_TOKEN_LEN) return false;

    uint32_t now = auth_now_s();
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        auth_session_t *se = &s_sessions[i];
        if (se->token[0] == '\0') continue;
        if (now - se->last_seen_s > AUTH_SESSION_TTL_S) { se->token[0] = '\0'; continue; }
        if (ct_equal(se->token, tok, AUTH_TOKEN_LEN)) {
            se->last_seen_s = now;
            return true;
        }
    }
    return false;
}

static esp_err_t auth_send_401(httpd_req_t *req)
{
    httpd_resp_set_status(req, "401 Unauthorized");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, "{\"status\":\"error\",\"code\":\"auth\",\"message\":\"Giris gerekli\"}",
                           HTTPD_RESP_USE_STRLEN);
}

/* Tum /api/... kayitlari bu kapidan gecer; gercek handler user_ctx icindedir */
static esp_err_t auth_gate_handler(httpd_req_t *req)
{
    esp_err_t (*real)(httpd_req_t *) = (esp_err_t (*)(httpd_req_t *))req->user_ctx;
    if (s_auth_enabled && !auth_request_ok(req)) {
        return auth_send_401(req);
    }
    return real(req);
}

static int read_body(httpd_req_t *req, char *buf, size_t size)
{
    if (req->content_len <= 0 || req->content_len >= size) return -1;
    int total = 0;
    while (total < (int)req->content_len) {
        int r = httpd_req_recv(req, buf + total, req->content_len - total);
        if (r <= 0) { if (r == HTTPD_SOCK_ERR_TIMEOUT) continue; return -1; }
        total += r;
    }
    buf[total] = '\0';
    return total;
}

// GET /api/auth/status - acik: {enabled, authed}
static esp_err_t api_auth_status_handler(httpd_req_t *req)
{
    char resp[96];
    snprintf(resp, sizeof(resp), "{\"enabled\":%s,\"authed\":%s}",
             s_auth_enabled ? "true" : "false",
             (!s_auth_enabled || auth_request_ok(req)) ? "true" : "false");
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/auth/login {"password":"..."} -> Set-Cookie sd_sess
static esp_err_t api_auth_login_handler(httpd_req_t *req)
{
    if (!s_auth_enabled) {
        return send_json(req, "{\"status\":\"ok\",\"enabled\":false}");
    }
    uint32_t now = auth_now_s();
    if (s_auth_lock_until && now < s_auth_lock_until) {
        httpd_resp_set_status(req, "429 Too Many Requests");
        httpd_resp_set_type(req, "application/json");
        char resp[96];
        snprintf(resp, sizeof(resp), "{\"status\":\"error\",\"message\":\"Cok fazla deneme, %lu sn bekleyin\"}",
                 (unsigned long)(s_auth_lock_until - now));
        return httpd_resp_send(req, resp, strlen(resp));
    }

    char body[256];
    if (read_body(req, body, sizeof(body)) <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    char pass[128] = {0};
    cJSON *root = cJSON_Parse(body);
    if (root) {
        cJSON *pj = cJSON_GetObjectItem(root, "password");
        if (pj && cJSON_IsString(pj)) strncpy(pass, pj->valuestring, sizeof(pass) - 1);
        cJSON_Delete(root);
    }

    char hex[65];
    auth_hash_password(pass, hex);
    if (!ct_equal(hex, s_auth_hash, 64)) {
        s_auth_fail++;
        sys_log_write("WARN", "auth", "Hatali parola denemesi (%d/%d)", s_auth_fail, AUTH_FAIL_MAX);
        if (s_auth_fail >= AUTH_FAIL_MAX) {
            s_auth_fail = 0;
            s_auth_lock_until = now + AUTH_LOCK_S;
            sys_log_write("WARN", "auth", "Giris %d sn kilitlendi", AUTH_LOCK_S);
        }
        vTaskDelay(pdMS_TO_TICKS(300));   /* kaba kuvvet yavaslatma */
        return auth_send_401(req);
    }

    s_auth_fail = 0;
    auth_session_t *se = auth_session_create();
    char cookie[160];
    snprintf(cookie, sizeof(cookie), "sd_sess=%s; Path=/; Max-Age=%d; HttpOnly; SameSite=Strict",
             se->token, AUTH_SESSION_TTL_S);
    httpd_resp_set_hdr(req, "Set-Cookie", cookie);
    httpd_resp_set_hdr(req, "Cache-Control", "no-store");
    httpd_resp_set_type(req, "application/json");
    char resp[96];
    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"token\":\"%s\"}", se->token);
    sys_log_write("INFO", "auth", "Giris basarili");
    return httpd_resp_send(req, resp, strlen(resp));
}

// POST /api/auth/logout
static esp_err_t api_auth_logout_handler(httpd_req_t *req)
{
    char hdr[256], tok[AUTH_TOKEN_LEN + 1] = {0};
    if (httpd_req_get_hdr_value_str(req, "Cookie", hdr, sizeof(hdr)) == ESP_OK) {
        const char *c = strstr(hdr, "sd_sess=");
        if (c) strncpy(tok, c + 8, AUTH_TOKEN_LEN);
    }
    for (int i = 0; i < AUTH_MAX_SESSIONS; i++) {
        if (s_sessions[i].token[0] && ct_equal(s_sessions[i].token, tok, AUTH_TOKEN_LEN)) {
            s_sessions[i].token[0] = '\0';
        }
    }
    httpd_resp_set_hdr(req, "Set-Cookie", "sd_sess=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
    return send_json(req, "{\"status\":\"ok\"}");
}

// POST /api/auth/set {"current":"...","password":"..."}  (password bos = kaldir)
// Parola ayarliyken mevcut oturum + mevcut parola gerekir.
static esp_err_t api_auth_set_handler(httpd_req_t *req)
{
    if (s_auth_enabled && !auth_request_ok(req)) return auth_send_401(req);

    char body[320];
    if (read_body(req, body, sizeof(body)) <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    char cur[128] = {0}, pass[128] = {0};
    cJSON *root = cJSON_Parse(body);
    if (root) {
        cJSON *cj = cJSON_GetObjectItem(root, "current");
        cJSON *pj = cJSON_GetObjectItem(root, "password");
        if (cj && cJSON_IsString(cj)) strncpy(cur, cj->valuestring, sizeof(cur) - 1);
        if (pj && cJSON_IsString(pj)) strncpy(pass, pj->valuestring, sizeof(pass) - 1);
        cJSON_Delete(root);
    }
    httpd_resp_set_type(req, "application/json");

    if (s_auth_enabled) {
        char hex[65];
        auth_hash_password(cur, hex);
        if (!ct_equal(hex, s_auth_hash, 64)) {
            httpd_resp_set_status(req, "403 Forbidden");
            return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Mevcut parola yanlis\"}",
                                   HTTPD_RESP_USE_STRLEN);
        }
    }
    if (*pass && (strlen(pass) < 4 || strlen(pass) > 64)) {
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Parola 4-64 karakter olmali\"}",
                               HTTPD_RESP_USE_STRLEN);
    }

    esp_err_t err = auth_store(*pass ? pass : NULL);
    if (err != ESP_OK) {
        return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"NVS yazilamadi\"}",
                               HTTPD_RESP_USE_STRLEN);
    }
    sys_log_write("WARN", "auth", *pass ? "Cihaz parolasi ayarlandi/degistirildi" : "Cihaz parolasi kaldirildi");

    if (*pass) {
        /* Yeni parola: cagirana hemen oturum ver (tekrar giris istemesin) */
        auth_session_t *se = auth_session_create();
        char cookie[160];
        snprintf(cookie, sizeof(cookie), "sd_sess=%s; Path=/; Max-Age=%d; HttpOnly; SameSite=Strict",
                 se->token, AUTH_SESSION_TTL_S);
        httpd_resp_set_hdr(req, "Set-Cookie", cookie);
    } else {
        httpd_resp_set_hdr(req, "Set-Cookie", "sd_sess=; Path=/; Max-Age=0; HttpOnly; SameSite=Strict");
    }
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"enabled\":%s}", s_auth_enabled ? "true" : "false");
    return httpd_resp_send(req, resp, strlen(resp));
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
    if (file_size >= 8192) {
        /* Yogun TX penceresi: brownout bu sirada olursa "GUI_SERVE" olarak gorunur */
        sys_log_mark_event(SL_EV_GUI_SERVE, (int32_t)file_size);
    }

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

/* Uzantiya gore Content-Type ve cache politikasi. clean_path uzantisi .gz OLMAMALI. */
static void set_static_headers(httpd_req_t *req, const char *clean_path, bool gzip)
{
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
    httpd_resp_set_hdr(req, "Cache-Control",
                       cacheable ? "public, max-age=31536000, immutable" : "no-cache");
    if (gzip) {
        httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
        httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
    }
}

/*
 * Statik dosya sun: once aktif slot'ta "<yol>.gz", sonra "<yol>", en son gomulu.
 * Gzip: TX suresini ~3x kisaltir (guc). GUI OTA ile indirilen duz dosyalar da
 * (gz yoksa) aynen sunulur; gui_ota hedef slotu tamamen temizledigi icin eski
 * .gz dosyalari yeni duz dosyalari golgeleyemez.
 */
/* Statik servis beyaz listesi: aktif slotta GUI disinda kullanici verileri de durur
 * (profiles altindaki json dosyalarinda kasa kombinasyonu ve auth basliklari, modes.json). Yalnizca
 * GUI dosyalari disari verilir; ".." iceren yollar reddedilir. */
static bool static_path_allowed(const char *p)
{
    if (!p || p[0] != '/' || strstr(p, "..")) return false;
    static const char *exact[] = {
        "/index.html", "/style.css", "/app.js", "/config.json", "/favicon.ico", "/setup.html", NULL
    };
    for (int i = 0; exact[i]; i++) if (strcmp(p, exact[i]) == 0) return true;
    if (strncmp(p, "/pic/", 5) == 0 || strncmp(p, "/lang/", 6) == 0) {
        return strchr(p + 1, '/') == strrchr(p, '/');   /* tek seviye alt dizin */
    }
    return false;
}

static esp_err_t serve_static_path(httpd_req_t *req, const char *clean_path)
{
    if (!static_path_allowed(clean_path)) {
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Dosya bulunamadi");
        return ESP_FAIL;
    }
    if (ext_flash_is_slot_a_mounted() || ext_flash_is_slot_b_mounted()) {
        char gz_path[140];
        snprintf(gz_path, sizeof(gz_path), "%s.gz", clean_path);
        if (ext_flash_active_get_file_size(gz_path) > 0) {
            set_static_headers(req, clean_path, true);
            return send_active_slot_file_chunked(req, gz_path);
        }
        if (ext_flash_active_get_file_size(clean_path) > 0) {
            set_static_headers(req, clean_path, false);
            return send_active_slot_file_chunked(req, clean_path);
        }
    }

    // Gomulu dosyalardan ara
    for (int i = 0; i < (int)EMBEDDED_FILES_COUNT; i++) {
        if (strcmp(clean_path, s_embedded_files[i].path) == 0) {
            const char *start = get_embedded_start(i);
            const char *end = get_embedded_end(i);
            size_t len = end - start;
            ESP_LOGI(TAG, "%s gomulu veriden sunuluyor (%zu byte%s)", clean_path, len,
                     s_embedded_files[i].gzip ? ", gzip" : "");
            set_static_headers(req, clean_path, s_embedded_files[i].gzip);
            httpd_resp_set_type(req, s_embedded_files[i].content_type);
            return httpd_resp_send(req, start, len);
        }
    }

    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "Dosya bulunamadi");
    return ESP_FAIL;
}

// Root-level statik dosya handler (style.css, app.js, pic/*, lang/* vb.)
// /ext/* handler'indan farki: URI'yi oldugu gibi dosya yolu olarak kullanir
static esp_err_t static_file_handler(httpd_req_t *req)
{
    // Query string'i ayir
    char clean_path[128];
    strncpy(clean_path, req->uri, sizeof(clean_path) - 1);
    clean_path[sizeof(clean_path) - 1] = '\0';
    char *qmark = strchr(clean_path, '?');
    if (qmark) *qmark = '\0';
    return serve_static_path(req, clean_path);
}

// GET / - Ana sayfa (aktif slot veya gomulu)
static esp_err_t root_handler(httpd_req_t *req)
{
    // 1. Harici flash aktif slot'tan index.html dene (once gzip'li, sonra duz)
    if (ext_flash_is_slot_a_mounted() || ext_flash_is_slot_b_mounted()) {
        if (ext_flash_active_get_file_size("/index.html.gz") > 0) {
            set_static_headers(req, "/index.html", true);
            if (send_active_slot_file_chunked(req, "/index.html.gz") == ESP_OK) return ESP_OK;
        } else if (ext_flash_active_get_file_size("/index.html") > 0) {
            set_static_headers(req, "/index.html", false);
            if (send_active_slot_file_chunked(req, "/index.html") == ESP_OK) return ESP_OK;
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

    // 3. Son care: embedded index.html (gzip'li)
    size_t len = gui_index_html_end - gui_index_html_start;
    ESP_LOGI(TAG, "index.html gomulu veriden sunuluyor (%zu byte, gzip)", len);
    set_static_headers(req, "/index.html", true);
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

    return send_json(req, resp);
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
        return send_json(req, "{\"status\":\"ok\",\"message\":\"WiFi ayarlari silindi\"}");
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

    return send_json(req, resp);
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
        return send_json(req, "{\"status\":\"ok\",\"message\":\"Yedek WiFi ayarlari silindi\"}");
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

    return send_json(req, resp);
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
             "\"rssi\":%d,"
             "\"tx_max\":%d,"
             "\"tx_now\":%d,"
             "\"tx_adapt\":%s,"
             "\"ap_clients\":%d"
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
             rssi,
             wifi_manager_get_tx_power_max(),
             wifi_manager_get_tx_power_now(),
             wifi_manager_get_tx_adaptive() ? "true" : "false",
             wifi_manager_get_ap_client_count());

    return send_json(req, resp);
}

// POST /api/wifi/txpower - TX gucu tavani + uyarlanabilir mod {"max":17,"adaptive":true}
static esp_err_t api_wifi_txpower_handler(httpd_req_t *req)
{
    char buf[96];
    int ret = httpd_req_recv(req, buf, sizeof(buf) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Veri alinamadi");
        return ESP_FAIL;
    }
    buf[ret] = '\0';

    int max_dbm = wifi_manager_get_tx_power_max();
    bool adaptive = wifi_manager_get_tx_adaptive();
    const char *p = strstr(buf, "\"max\":");
    if (p) max_dbm = atoi(p + 6);
    if (strstr(buf, "\"adaptive\":true"))  adaptive = true;
    if (strstr(buf, "\"adaptive\":false")) adaptive = false;

    esp_err_t err = wifi_manager_set_tx_power(max_dbm, adaptive);
    char resp[160];
    if (err == ESP_OK) {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"ok\",\"tx_max\":%d,\"tx_now\":%d,\"tx_adapt\":%s}",
                 wifi_manager_get_tx_power_max(), wifi_manager_get_tx_power_now(),
                 wifi_manager_get_tx_adaptive() ? "true" : "false");
    } else {
        snprintf(resp, sizeof(resp),
                 "{\"status\":\"error\",\"message\":\"Gecersiz deger (8-20 dBm)\"}");
    }
    return send_json(req, resp);
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

    return send_json(req, resp);
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
    return send_json(req, resp);
}

// GET /api/modes - Mod yapilandirmasini oku
#define MODES_JSON_MAX  8192   /* modes.json ust siniri (onceden 1024B sabit tampon: kesilip bozuluyordu) */

static esp_err_t api_modes_get_handler(httpd_req_t *req)
{
    char *buf = malloc(MODES_JSON_MAX);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }
    int len = ext_flash_read_file("/slot_a/modes.json", buf, MODES_JSON_MAX);
    httpd_resp_set_type(req, "application/json");
    esp_err_t r;
    if (len <= 0) {
        r = httpd_resp_send(req, "{\"multiMode\":true,\"modes\":[]}", HTTPD_RESP_USE_STRLEN);
    } else {
        r = httpd_resp_send(req, buf, len);
    }
    free(buf);
    return r;
}

// POST /api/modes - Mod yapilandirmasini kaydet
static esp_err_t api_modes_post_handler(httpd_req_t *req)
{
    /* Govde tek recv'de gelmeyebilir; kesik JSON yazilirsa GUI "bos durum"a duser
     * ve kullanici modlari kaybolur. content_len kadar dongude oku, JSON dogrula. */
    if (req->content_len <= 0 || req->content_len >= MODES_JSON_MAX) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Gecersiz boyut");
        return ESP_FAIL;
    }
    char *buf = malloc(req->content_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }
    int total = 0;
    while (total < (int)req->content_len) {
        int r = httpd_req_recv(req, buf + total, req->content_len - total);
        if (r <= 0) {
            if (r == HTTPD_SOCK_ERR_TIMEOUT) continue;
            free(buf);
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Govde alinamadi");
            return ESP_FAIL;
        }
        total += r;
    }
    buf[total] = '\0';

    cJSON *root = cJSON_Parse(buf);
    if (!root || !cJSON_IsObject(root) || !cJSON_IsArray(cJSON_GetObjectItem(root, "modes"))) {
        if (root) cJSON_Delete(root);
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Gecersiz modes JSON");
        return ESP_FAIL;
    }
    cJSON_Delete(root);

    esp_err_t ret = ext_flash_write_file("/slot_a/modes.json", buf, total);
    free(buf);
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

    char resp[1152];
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
             "\"user_free\":%zu,"
             "\"boot_count\":%lu,"
             "\"short_boots\":%lu"
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
             user_free / 1024,
             (unsigned long)sys_log_boot_count(),
             (unsigned long)sys_log_short_boots());

    return send_json(req, resp);
}

// POST /api/device/restart - Cihazi yeniden baslat
static esp_err_t api_device_restart_handler(httpd_req_t *req)
{
    ESP_LOGW(TAG, "Yeniden baslatma istegi alindi (web)");
    sys_log_mark_event(SL_EV_USER_RESTART, 0);
    sys_log_set_restart_reason(SL_RST_USER_WEB);
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
    sys_log_set_restart_reason(SL_RST_FACTORY);
    sys_log_write("WARN", "sys", "Fabrika sifirlama (web)");
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

    /* Basliklar HAM gonderilir. httpd_resp_send_chunk kullanilsaydi yanit
     * "Transfer-Encoding: chunked" olarak acilir, sonraki httpd_socket_send
     * yazimlari chunk cercevesiz gider ve tarayici akisi gecersiz sayar
     * (surekli reconnect). Content-Length olmadan baglanti acik kalir. */
    static const char hdr[] =
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/event-stream\r\n"
        "Cache-Control: no-cache\r\n"
        "Connection: keep-alive\r\n"
        "Access-Control-Allow-Origin: *\r\n"
        "\r\n"
        "data: connected\n\n";
    if (httpd_socket_send(req->handle, fd, hdr, sizeof(hdr) - 1, 0) < 0) {
        s_sse_fds[slot] = -1;
        return ESP_FAIL;
    }
    httpd_sess_update_lru_counter(req->handle, fd);

    ESP_LOGI(TAG, "SSE istemci baglandi (fd=%d, slot=%d)", fd, slot);
    return ESP_OK;
}

/* Oturum kapanisi: SSE slotunu bosalt. close_fn verildiginde soketi kapatmak
 * bize duser; aksi halde fd numarasi yeni bir baglantiya verilip SSE verisi
 * o yanita karisirdi. */
static void sse_session_close(httpd_handle_t hd, int sockfd)
{
    (void)hd;
    for (int i = 0; i < SSE_MAX_CLIENTS; i++) {
        if (s_sse_fds[i] == sockfd) {
            s_sse_fds[i] = -1;
            ESP_LOGI(TAG, "SSE istemci ayrildi (fd=%d)", sockfd);
        }
    }
    close(sockfd);
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
            } else {
                /* Aktif akis LRU temizliginde kurban secilmesin */
                httpd_sess_update_lru_counter(s_server, s_sse_fds[i]);
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
    return serve_static_path(req, clean_path);
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

    return send_json(req, resp);
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
        return send_json(req, "{\"status\":\"error\",\"message\":\"NVS hatasi\"}");
    }

    char val[8] = {0};
    if (parse_json_string(buf, "trigger_api", val, sizeof(val)) >= 0) {
        nvs_set_u8(nvs, "trig_api", (strcmp(val, "true") == 0) ? 1 : 0);
    }

    nvs_commit(nvs);
    nvs_close(nvs);

    ESP_LOGI(TAG, "Action durumlar kaydedildi");
    return send_json(req, "{\"status\":\"ok\"}");
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

    return send_json(req, resp);
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
    return send_json(req, resp);
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
        return send_json(req, "{\"status\":\"error\",\"message\":\"NVS hatasi\"}");
    }

    nvs_set_str(nvs, "api_key", api_key);
    nvs_set_u8(nvs, "enabled", 1);
    nvs_commit(nvs);
    nvs_close(nvs);

    char resp[64];
    snprintf(resp, sizeof(resp), "{\"status\":\"ok\",\"api_key\":\"%s\"}", api_key);
    return send_json(req, resp);
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
        return send_json(req, "{\"status\":\"error\",\"message\":\"Uzaktan sifirlama devre disi\"}");
    }

    // Query param: key
    char query[64] = {0};
    char key_param[16] = {0};
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "key", key_param, sizeof(key_param));
    }

    if (strlen(key_param) == 0 || strcmp(key_param, stored_key) != 0) {
        httpd_resp_set_status(req, "401 Unauthorized");
        return send_json(req, "{\"status\":\"error\",\"message\":\"Gecersiz API anahtari\"}");
    }

    return send_json(req, "{\"status\":\"ok\",\"message\":\"API triggered\"}");
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

    return send_json(req, resp);
}

// POST /api/gui-ota/start
static esp_err_t api_gui_ota_start_handler(httpd_req_t *req)
{
    sys_log_mark_event(SL_EV_OTA_GUI, 0);
    sys_log_write("INFO", "ota", "GUI guncellemesi baslatildi");
    if (gui_ota_is_busy()) {
        return send_json(req, "{\"status\":\"error\",\"message\":\"OTA zaten calisiyor\"}");
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
}

// POST /api/fw-ota/check
static esp_err_t api_fw_ota_check_handler(httpd_req_t *req)
{
    if (fw_ota_is_busy()) {
        const char *resp = "{\"status\":\"error\",\"message\":\"OTA zaten calisiyor\"}";
        return send_json(req, resp);
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

    return send_json(req, resp);
}

// POST /api/fw-ota/start
static esp_err_t api_fw_ota_start_handler(httpd_req_t *req)
{
    sys_log_mark_event(SL_EV_OTA_FW, 0);
    sys_log_set_restart_reason(SL_RST_OTA_FW);   // OTA sonrasi restart bu nedenle etiketlenir
    sys_log_write("INFO", "ota", "Firmware guncellemesi baslatildi");
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
}

// ============================================================
// Device Driver API
// ============================================================

// GET /api/mode
static esp_err_t api_mode_get_handler(httpd_req_t *req)
{
    char resp[64];
    snprintf(resp, sizeof(resp), "{\"mode\":\"%s\"}", device_driver_get_mode());
    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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
    if (!device_driver_valid_name(id_j->valuestring) || !device_driver_valid_name(mode_j->valuestring)) {
        cJSON_Delete(root);
        free(buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id/mode sadece harf, rakam, _ ve - icerebilir");
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
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

    return send_json(req, resp);
}

// DELETE /api/safe?id=xxx
static esp_err_t api_safe_delete_handler(httpd_req_t *req)
{
    char query[64] = {0};
    char id[32] = {0};

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "id", id, sizeof(id));
    }

    if (!device_driver_valid_name(id)) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "id parametresi gecersiz");
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

    return send_json(req, resp);
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
    return send_json(req, empty);
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
        .crt_bundle_attach = esp_crt_bundle_attach,   /* HTTPS: sertifika paketi olmadan TLS her zaman basarisizdi */
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
        return send_json(req, resp);
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
        return send_json(req, resp);
    }
    dl_buf[dl_len] = '\0';

    // Dosyayi kaydet (mode/id yol suzmesinden gecmis olmali)
    if (!device_driver_valid_name(mode) || !device_driver_valid_name(id)) {
        free(dl_buf);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "mode/id gecersiz");
        return ESP_FAIL;
    }
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

    return send_json(req, resp);
}

// ============================================================
// System Logs API
// ============================================================

// GET /api/system/logs - Olay kayitlari (en yeni once) + ozet
static esp_err_t api_system_logs_handler(httpd_req_t *req)
{
    char *entries = sys_log_read_tail_json(150);
    if (!entries) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Bellek yetersiz");
        return ESP_FAIL;
    }
    char head[160];
    int hl = snprintf(head, sizeof(head),
                      "{\"boot\":%lu,\"short\":%lu,\"epoch\":%lu,\"uptime\":%lld,\"entries\":",
                      (unsigned long)sys_log_boot_count(), (unsigned long)sys_log_short_boots(),
                      (unsigned long)sys_log_boot_epoch(),
                      (long long)(esp_timer_get_time() / 1000000));

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Cache-Control", "no-cache");
    httpd_resp_send_chunk(req, head, hl);
    httpd_resp_send_chunk(req, entries, strlen(entries));
    httpd_resp_send_chunk(req, "}", 1);
    httpd_resp_send_chunk(req, NULL, 0);
    free(entries);
    return ESP_OK;
}

// DELETE /api/system/logs - Kayitlari temizle
static esp_err_t api_system_logs_clear_handler(httpd_req_t *req)
{
    esp_err_t err = sys_log_clear();
    httpd_resp_set_type(req, "application/json");
    if (err == ESP_OK) {
        return httpd_resp_send(req, "{\"status\":\"ok\"}", HTTPD_RESP_USE_STRLEN);
    }
    return httpd_resp_send(req, "{\"status\":\"error\",\"message\":\"Log deposu hazir degil\"}",
                           HTTPD_RESP_USE_STRLEN);
}

// ============================================================
// Server Yonetim
// ============================================================

// ============================================================
// Rota tablosu
// Sira onemli: "/ext/*" ve "/*" joker kayitlari EN SONDA olmali.
// gated=true olanlar auth_gate_handler uzerinden gecer (parola ayarliysa 401).
// ============================================================
typedef struct {
    const char *uri;
    httpd_method_t method;
    esp_err_t (*handler)(httpd_req_t *req);
    bool gated;
} route_t;

static const route_t s_routes[] = {
    { "/", HTTP_GET, root_handler, false },
    { "/api/system/info", HTTP_GET, api_system_info_handler, true },
    { "/api/auth/status", HTTP_GET, api_auth_status_handler, false },
    { "/api/auth/login", HTTP_POST, api_auth_login_handler, false },
    { "/api/auth/logout", HTTP_POST, api_auth_logout_handler, false },
    { "/api/auth/set", HTTP_POST, api_auth_set_handler, false },
    { "/api/system/detail", HTTP_GET, api_system_detail_handler, true },
    { "/api/system/logs", HTTP_GET, api_system_logs_handler, true },
    { "/api/system/logs", HTTP_DELETE, api_system_logs_clear_handler, true },
    { "/api/wifi/connect", HTTP_POST, api_wifi_connect_handler, true },
    { "/api/wifi/backup/save", HTTP_POST, api_wifi_backup_save_handler, true },
    { "/api/wifi/config", HTTP_GET, api_wifi_config_handler, true },
    { "/api/wifi/ap/set", HTTP_POST, api_wifi_ap_set_handler, true },
    { "/api/wifi/mdns", HTTP_POST, api_wifi_mdns_handler, true },
    { "/api/wifi/txpower", HTTP_POST, api_wifi_txpower_handler, true },
    { "/api/modes", HTTP_GET, api_modes_get_handler, true },
    { "/api/modes", HTTP_POST, api_modes_post_handler, true },
    { "/api/device/restart", HTTP_POST, api_device_restart_handler, true },
    { "/api/device/factory-reset", HTTP_POST, api_device_factory_reset_handler, true },
    { "/api/actions/get", HTTP_GET, api_actions_get_handler, true },
    { "/api/actions/save", HTTP_POST, api_actions_save_handler, true },
    { "/api/reset/config", HTTP_GET, api_reset_config_get_handler, true },
    { "/api/reset/config", HTTP_POST, api_reset_config_save_handler, true },
    { "/api/reset/generate", HTTP_POST, api_reset_generate_handler, true },
    { "/api/reset", HTTP_GET, api_reset_handler, true },
    { "/api/gui-ota/info", HTTP_GET, api_gui_ota_info_handler, true },
    { "/api/gui-ota/start", HTTP_POST, api_gui_ota_start_handler, true },
    { "/api/gui-ota/status", HTTP_GET, api_gui_ota_status_handler, true },
    { "/api/gui-ota/rollback", HTTP_POST, api_gui_ota_rollback_handler, true },
    { "/api/fw-ota/info", HTTP_GET, api_fw_ota_info_handler, true },
    { "/api/fw-ota/check", HTTP_POST, api_fw_ota_check_handler, true },
    { "/api/fw-ota/start", HTTP_POST, api_fw_ota_start_handler, true },
    { "/api/fw-ota/status", HTTP_GET, api_fw_ota_status_handler, true },
    { "/api/fw-ota/rollback", HTTP_POST, api_fw_ota_rollback_handler, true },
    { "/api/mode", HTTP_GET, api_mode_get_handler, true },
    { "/api/mode", HTTP_POST, api_mode_set_handler, true },
    { "/api/profiles", HTTP_GET, api_profiles_list_handler, true },
    { "/api/profiles/active", HTTP_GET, api_profiles_active_handler, true },
    { "/api/profiles/select", HTTP_POST, api_profiles_select_handler, true },
    { "/api/profiles/upload", HTTP_POST, api_profiles_upload_handler, true },
    { "/api/profiles", HTTP_DELETE, api_profiles_delete_handler, true },
    { "/api/status", HTTP_GET, api_device_status_handler, true },
    { "/api/safe/list", HTTP_GET, api_safe_list_handler, true },
    { "/api/safe/add", HTTP_POST, api_safe_add_handler, true },
    { "/api/safe", HTTP_DELETE, api_safe_delete_handler, true },
    { "/api/catalog/list", HTTP_GET, api_catalog_list_handler, true },
    { "/api/catalog/download", HTTP_POST, api_catalog_download_handler, true },
    { "/api/events", HTTP_GET, api_events_handler, true },
    { "/ext/*", HTTP_GET, ext_file_handler, false },
    { "/*", HTTP_GET, static_file_handler, false },
};
#define ROUTE_COUNT (sizeof(s_routes) / sizeof(s_routes[0]))

esp_err_t web_server_start(void)
{
    if (s_server) {
        ESP_LOGW(TAG, "Server zaten calisiyor");
        return ESP_OK;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.uri_match_fn = httpd_uri_match_wildcard;
    config.max_uri_handlers = ROUTE_COUNT + 4;
    /* Katalog indirme bu gorevde TLS el sikismasi yapar; 8 KB yetersiz kalabiliyordu */
    config.stack_size = 12288;
    /* Tarayici 6 paralel istek + 3 kalici SSE: 7 soket yetmiyordu, LRU temizligi
     * SSE'yi kesiyordu. LWIP_MAX_SOCKETS=16 -> httpd icin 12 (+3 dahili) uygun. */
    config.max_open_sockets = 12;
    config.lru_purge_enable = true;
    config.close_fn = sse_session_close;

    auth_load();

    esp_err_t ret = httpd_start(&s_server, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Server baslatilamadi: %s", esp_err_to_name(ret));
        return ret;
    }

    // --- Rotalar (tablo) ---
    for (size_t i = 0; i < ROUTE_COUNT; i++) {
        const route_t *r = &s_routes[i];
        httpd_uri_t u = {
            .uri = r->uri,
            .method = r->method,
            .handler = r->gated ? auth_gate_handler : r->handler,
            .user_ctx = r->gated ? (void *)r->handler : NULL,
        };
        esp_err_t rr = httpd_register_uri_handler(s_server, &u);
        if (rr != ESP_OK) {
            ESP_LOGE(TAG, "Rota kaydi basarisiz: %s %s (%s)", r->uri,
                     r->method == HTTP_GET ? "GET" : r->method == HTTP_POST ? "POST" : "DELETE",
                     esp_err_to_name(rr));
        }
    }

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
