/**
 * SynDimm - Safe Lock (Celik Kasa Sifre Mantigi)
 *
 * Encoder tiklari ile kombinasyon girilir:
 * 1. R/L cevirme → tik sayaci artar
 * 2. B (buton) → mevcut yon kaydedilir, sonraki yone gec
 * 3. 2 saniye islem olmazsa → sifre dizisi dogrulanir
 * 4. Eslesme → on_unlock API cagirilir + 3 bip
 * 5. Hata → 1 uzun bip
 *
 * Password dizisi: pozitif = saga tik, negatif = sola tik
 * Ornek: [4, -2, 3, -1] = 4 saga, 2 sola, 3 saga, 1 sola
 */

#include "device_driver.h"
#include "ext_flash.h"
#include "buzzer.h"
#include "esp_log.h"
#include "cJSON.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"

static const char *TAG = "SAFE_LOCK";

// Kombinasyon girisi
#define MAX_COMBO_LEN 16
static int    s_combo[MAX_COMBO_LEN];   // Girilen kombinasyon
static int    s_combo_len = 0;          // Girilen adim sayisi
static int    s_current_ticks = 0;      // Mevcut yondeki tik sayisi
static char   s_current_dir = 0;        // 'R' veya 'L' veya 0
static bool   s_active = false;         // Sifre girisi baslamis mi

// Zamanlayici (2 saniye timeout)
static TimerHandle_t s_timeout_timer = NULL;

// Timeout callback'i (device_driver tarafindan kaydedilir,
// command queue'ye CMD_SAFE_TIMEOUT ekler)
static void (*s_timeout_cb)(void) = NULL;

// ============================================================
// Timeout zamanlayicisi
// ============================================================

static void verify_combination(void);

// Timer daemon context'inde calisir - shared state'e dokunmaz,
// sadece command_task'a bildirim gonderir
static void timeout_callback(TimerHandle_t timer)
{
    (void)timer;
    if (s_timeout_cb) {
        s_timeout_cb();
    }
}

// ============================================================
// Sifre dogrulama
// ============================================================

static bool match_password(const cJSON *password, const int *combo, int combo_len)
{
    if (!cJSON_IsArray(password)) return false;

    int pw_len = cJSON_GetArraySize(password);
    if (pw_len != combo_len) return false;

    for (int i = 0; i < pw_len; i++) {
        cJSON *item = cJSON_GetArrayItem(password, i);
        if (!cJSON_IsNumber(item)) return false;
        if (item->valueint != combo[i]) return false;
    }

    return true;
}

static esp_err_t execute_unlock_api(const cJSON *on_unlock)
{
    if (!on_unlock) return ESP_ERR_INVALID_ARG;

    cJSON *protocol = cJSON_GetObjectItem(on_unlock, "protocol");
    cJSON *method   = cJSON_GetObjectItem(on_unlock, "method");
    cJSON *url      = cJSON_GetObjectItem(on_unlock, "url");
    cJSON *body     = cJSON_GetObjectItem(on_unlock, "body");

    if (!url || !cJSON_IsString(url)) {
        ESP_LOGE(TAG, "on_unlock: URL eksik");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Unlock API cagriliyor: %s", url->valuestring);

    // URL'den host ve path ayir
    // http://192.168.1.100/api/unlock -> host=192.168.1.100, path=/api/unlock
    const char *url_str = url->valuestring;
    if (strncmp(url_str, "http://", 7) == 0) url_str += 7;

    char host[64] = "";
    char path[128] = "/";
    int port = 80;

    // Host:port/path ayirma
    const char *slash = strchr(url_str, '/');
    if (slash) {
        size_t host_len = slash - url_str;
        if (host_len >= sizeof(host)) host_len = sizeof(host) - 1;
        strncpy(host, url_str, host_len);
        host[host_len] = '\0';
        strncpy(path, slash, sizeof(path) - 1);
    } else {
        strncpy(host, url_str, sizeof(host) - 1);
    }

    // Port ayir (host:port)
    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port = atoi(colon + 1);
    }

    const char *method_str = "POST";
    if (method && cJSON_IsString(method)) {
        method_str = method->valuestring;
    }

    return proto_http_send_command(
        host, port, method_str, path,
        body ? body->valuestring : NULL,
        NULL, NULL, 0, false);
}

static void verify_combination(void)
{
    if (s_combo_len == 0) return;

    // Log
    ESP_LOGI(TAG, "Girilen kombinasyon (%d adim):", s_combo_len);
    for (int i = 0; i < s_combo_len; i++) {
        ESP_LOGI(TAG, "  [%d] = %d", i, s_combo[i]);
    }

    // Tum safe profillerini tara
    const char *dir_path = "/slot_a/profiles/safe";
    DIR *dir = opendir(dir_path);
    if (!dir) {
        ESP_LOGW(TAG, "Safe dizini acilamadi");
        buzzer_play_dits(1);  // Hata sesi
        return;
    }

    bool matched = false;
    struct dirent *ent;
    char file_path[320];
    char *buf = malloc(2048);
    if (!buf) {
        closedir(dir);
        return;
    }

    while ((ent = readdir(dir)) != NULL) {
        size_t nlen = strlen(ent->d_name);
        if (nlen < 6) continue;
        if (strcmp(ent->d_name + nlen - 5, ".json") != 0) continue;

        snprintf(file_path, sizeof(file_path), "%s/%s", dir_path, ent->d_name);
        int len = ext_flash_read_file(file_path, buf, 2047);
        if (len <= 0) continue;
        buf[len] = '\0';

        cJSON *prof = cJSON_Parse(buf);
        if (!prof) continue;

        cJSON *password = cJSON_GetObjectItem(prof, "password");
        if (match_password(password, s_combo, s_combo_len)) {
            cJSON *name = cJSON_GetObjectItem(prof, "name");
            ESP_LOGI(TAG, "ESLESME BULUNDU: %s",
                     name ? name->valuestring : "?");

            // on_unlock API cagirilir
            cJSON *on_unlock = cJSON_GetObjectItem(prof, "on_unlock");
            if (on_unlock) {
                execute_unlock_api(on_unlock);
            }

            matched = true;
            cJSON_Delete(prof);
            break;
        }

        cJSON_Delete(prof);
    }

    free(buf);
    closedir(dir);

    if (matched) {
        buzzer_play_dits(3);  // Basari: 3 bip
        ESP_LOGI(TAG, "Kilit acildi!");
    } else {
        buzzer_play_dits(1);  // Hata: 1 bip
        ESP_LOGW(TAG, "Sifre eslesmedi");
    }
}

// ============================================================
// Timer baslat / yeniden baslat
// ============================================================

static void reset_timeout(void)
{
    if (s_timeout_timer) {
        xTimerReset(s_timeout_timer, 0);
    }
}

// ============================================================
// Public API
// ============================================================

esp_err_t safe_lock_init(void)
{
    s_combo_len = 0;
    s_current_ticks = 0;
    s_current_dir = 0;
    s_active = false;

    if (!s_timeout_timer) {
        s_timeout_timer = xTimerCreate("safe_tmr",
                                        pdMS_TO_TICKS(2000),  // 2 saniye
                                        pdFALSE,              // tek seferlik
                                        NULL,
                                        timeout_callback);
    }

    ESP_LOGI(TAG, "Safe lock baslatildi");
    return ESP_OK;
}

esp_err_t safe_lock_handle_event(char event)
{
    switch (event) {
        case 'R':
            if (!s_active) {
                s_active = true;
                s_combo_len = 0;
                s_current_ticks = 0;
                s_current_dir = 0;
            }

            if (s_current_dir == 'L' && s_current_ticks > 0) {
                // Yon degisti, onceki yonu kaydet
                if (s_combo_len < MAX_COMBO_LEN) {
                    s_combo[s_combo_len] = -s_current_ticks;
                    s_combo_len++;
                }
                s_current_ticks = 0;
            }
            s_current_dir = 'R';
            s_current_ticks++;
            reset_timeout();
            ESP_LOGD(TAG, "R tik: %d", s_current_ticks);
            break;

        case 'L':
            if (!s_active) {
                s_active = true;
                s_combo_len = 0;
                s_current_ticks = 0;
                s_current_dir = 0;
            }

            if (s_current_dir == 'R' && s_current_ticks > 0) {
                // Yon degisti, onceki yonu kaydet
                if (s_combo_len < MAX_COMBO_LEN) {
                    s_combo[s_combo_len] = s_current_ticks;
                    s_combo_len++;
                }
                s_current_ticks = 0;
            }
            s_current_dir = 'L';
            s_current_ticks++;
            reset_timeout();
            ESP_LOGD(TAG, "L tik: %d", s_current_ticks);
            break;

        case 'B':
            // Buton: mevcut yonu kaydet ve sonraki yone gec
            if (s_active && s_current_ticks > 0 && s_current_dir != 0) {
                if (s_combo_len < MAX_COMBO_LEN) {
                    s_combo[s_combo_len] = (s_current_dir == 'R') ?
                                            s_current_ticks : -s_current_ticks;
                    s_combo_len++;
                }
                s_current_ticks = 0;
                s_current_dir = 0;
                reset_timeout();
                ESP_LOGI(TAG, "Yon kaydedildi (toplam %d adim)", s_combo_len);
            }
            break;

        default:
            break;
    }

    return ESP_OK;
}

void safe_lock_reset(void)
{
    s_combo_len = 0;
    s_current_ticks = 0;
    s_current_dir = 0;
    s_active = false;

    if (s_timeout_timer) {
        xTimerStop(s_timeout_timer, 0);
    }
}

bool safe_lock_is_active(void)
{
    return s_active;
}

void safe_lock_set_timeout_cb(void (*cb)(void))
{
    s_timeout_cb = cb;
}

// command_task context'inde cagirilir - guvenle shared state'e erisir
void safe_lock_process_timeout(void)
{
    ESP_LOGI(TAG, "Timeout - sifre dogrulaniyor (%d adim)", s_combo_len);

    // Son yonu kaydet (bitirilmemisse)
    if (s_current_ticks > 0 && s_current_dir != 0) {
        if (s_combo_len < MAX_COMBO_LEN) {
            s_combo[s_combo_len] = (s_current_dir == 'R') ?
                                    s_current_ticks : -s_current_ticks;
            s_combo_len++;
        }
    }

    verify_combination();

    // Sifirla
    s_combo_len = 0;
    s_current_ticks = 0;
    s_current_dir = 0;
    s_active = false;
}
