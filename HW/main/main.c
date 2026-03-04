/**
 * SynDimm - Ana Program
 * ESP32-C6 | Dahili 4MB Flash + Harici 32MB Flash
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_flash.h"
#include "esp_system.h"
#include "esp_partition.h"
#include "esp_mac.h"

#include "wifi_manager.h"
#include "web_server.h"
#include "ext_flash.h"
#include "flash_sync.h"
#include "gui_ota.h"
#include "fw_ota.h"
#include "esp_ota_ops.h"
#include "encoder.h"
#include "buzzer.h"
#include "device_driver.h"

static const char *TAG = "MAIN";
static char device_name[20] = {0};

/* Encoder + Buzzer periyodik guncelleme task'i */
static bool s_mode_change_active = false;

static void input_task(void *arg)
{
    ESP_LOGI(TAG, "Input task baslatildi");
    while (1) {
        while (encoder_available()) {
            char evt = encoder_read();

            // Mod degistirme: buton basili + dondurme
            // Bir kez aktif olunca sure siniri yok (restart/reset iptal edilir)
            if ((evt == 'L' || evt == 'R') &&
                encoder_is_button_held())
            {
                int64_t hold = encoder_get_hold_duration_ms();
                if (s_mode_change_active || hold < 3000) {
                    if (!s_mode_change_active) {
                        encoder_set_mode_change_triggered();
                        s_mode_change_active = true;
                    }
                    // Mod degistir ve degisim olup olmadigini kontrol et
                    bool changed = device_driver_handle_mode_change(evt == 'R' ? 'N' : 'P');
                    if (changed) {
                        const char *mode = device_driver_get_mode();
                        int dits = 1;
                        if (strcmp(mode, "shutter") == 0) dits = 2;
                        else if (strcmp(mode, "safe") == 0) dits = 3;
                        buzzer_play_dits(dits);
                    }
                    // changed==false → sinirda, buzzer yok
                }
                continue;
            }

            switch (evt) {
                case 'L':
                    ESP_LOGD(TAG, "[ENC] SOL");
                    device_driver_handle_event('L');
                    break;
                case 'R':
                    ESP_LOGD(TAG, "[ENC] SAG");
                    device_driver_handle_event('R');
                    break;
                case 'B':
                    ESP_LOGD(TAG, "[ENC] BUTON");
                    device_driver_handle_event('B');
                    s_mode_change_active = false;
                    break;
                case 'M':
                    if (s_mode_change_active) {
                        ESP_LOGI(TAG, "[ENC] MOD ONAYLANDI");
                        s_mode_change_active = false;
                        // Mod zaten dondurme sirasinda degisti, tekrar degistirme
                    } else {
                        ESP_LOGI(TAG, "[ENC] MOD DEGISIKLIGI");
                        device_driver_handle_event('M');
                    }
                    break;
                default:
                    break;
            }
        }

        buzzer_update();
        vTaskDelay(1);
    }
}

// MAC adresinden FNV-1a hash ile 10 haneli benzersiz cihaz ID olustur
// Ornek: SD-K7M2X9P4HB
static void generate_device_name(void)
{
    uint8_t mac[6];
    esp_efuse_mac_get_default(mac);

    // FNV-1a 64-bit hash (tum 6 byte MAC kullanilir)
    uint64_t hash = 0xcbf29ce484222325ULL;  // FNV offset basis
    for (int i = 0; i < 6; i++) {
        hash ^= mac[i];
        hash *= 0x100000001b3ULL;            // FNV prime
    }

    // 10 haneli alfanumerik ID olustur (A-Z, 0-9 = 36 karakter)
    static const char charset[] = "0123456789ABCDEFGHJKLMNPQRSTUVWXYZ";
    // I ve O cikarildi (1 ve 0 ile karismamasi icin) = 33 karakter
    const int base = sizeof(charset) - 1;  // 33

    char id[11];
    for (int i = 0; i < 10; i++) {
        id[i] = charset[hash % base];
        hash /= base;
    }
    id[10] = '\0';

    snprintf(device_name, sizeof(device_name), "SD-%s", id);
    ESP_LOGI(TAG, "Cihaz ID: %s (MAC: %02X:%02X:%02X:%02X:%02X:%02X)",
             device_name, mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

void app_main(void)
{
    ESP_LOGI(TAG, "=== SynDimm Baslatiliyor ===");

    // WiFi driver log seviyesini azalt (cok fazla detay basiliyordu)
    esp_log_level_set("wifi", ESP_LOG_WARN);
    esp_log_level_set("wifi_init", ESP_LOG_WARN);

    // Benzersiz cihaz adi olustur
    generate_device_name();

    // NVS Flash baslatma
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS flash siliniyor ve yeniden baslatiliyor...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "NVS Flash baslatildi");

    // Harici flash baslatma (SPI - MX25L25645G)
    ret = ext_flash_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Harici flash baslatilamadi! Hata: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Harici flash baslatildi (32MB)");
        ext_flash_create_default_files();
        error_log_init();
    }

    // Flash senkronizasyon baslatma
    ret = flash_sync_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Flash senkronizasyon baslatilamadi: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Flash senkronizasyon hazir");
    }

    // GUI OTA baslatma
    ret = gui_ota_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "GUI OTA baslatilamadi: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "GUI OTA hazir (aktif slot: %c)", ext_flash_get_active_slot());
    }

    // Firmware OTA baslatma
    ret = fw_ota_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "FW OTA baslatilamadi: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "FW OTA hazir (partition: %s)", fw_ota_get_running_label());
    }

    // WiFi AP modda baslat (cihaz adi ile)
    ret = wifi_manager_init(device_name);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "WiFi baslatilamadi! Hata: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "WiFi AP modu baslatildi");
    }

    // Web server baslatma
    ret = web_server_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Web server baslatilamadi! Hata: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Web server baslatildi");
    }

    // Encoder baslatma
    ret = encoder_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Encoder baslatilamadi! Hata: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Encoder baslatildi (CLK=19, DT=20, SW=18)");
    }

    // Buzzer baslatma
    ret = buzzer_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Buzzer baslatilamadi! Hata: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Buzzer baslatildi (GPIO17, 2kHz)");
        buzzer_play_dits(1);  // Boot bildirim sesi
    }

    // Device driver baslatma (profil tabanli cihaz kontrolu)
    ret = device_driver_init();
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Device driver baslatilamadi: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Device driver baslatildi (mod: %s)", device_driver_get_mode());
    }

    // Input task - encoder yoklama + buzzer guncelleme
    xTaskCreate(input_task, "input_task", 4096, NULL, 5, NULL);

    // OTA rollback korumasi: basarili boot'u onayla
    esp_ota_mark_app_valid_cancel_rollback();

    // Flash ozet bilgisi
    {
        size_t sa_total = 0, sa_free = 0, rv_total = 0, rv_free = 0;
        if (ext_flash_is_slot_a_mounted())
            ext_flash_get_slot_a_info(&sa_total, &sa_free);
        if (ext_flash_is_reserve_mounted())
            ext_flash_get_reserve_info(&rv_total, &rv_free);
        ESP_LOGI(TAG, "Flash: SlotA %zuKB bos, Reserve %zuKB bos",
                 sa_free / 1024, rv_free / 1024);
    }

    ESP_LOGI(TAG, "=== SynDimm Hazir ===");
    ESP_LOGI(TAG, "Cihaz: %s | AP: %s | mDNS: %s.local",
             device_name, device_name, device_name);

    // Kayitli WiFi varsa otomatik baglan
    wifi_manager_auto_connect();
}
