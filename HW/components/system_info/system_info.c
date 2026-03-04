#include "system_info.h"
#include <string.h>
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "esp_flash.h"
#include "esp_heap_caps.h"
#include "esp_partition.h"
#include "esp_app_desc.h"
#include "esp_log.h"

esp_err_t system_info_get(system_info_t *info)
{
    if (!info) return ESP_ERR_INVALID_ARG;
    memset(info, 0, sizeof(system_info_t));

    /* Chip */
    esp_chip_info_t chip;
    esp_chip_info(&chip);
    switch (chip.model) {
        case CHIP_ESP32:   strncpy(info->chip_model, "ESP32", sizeof(info->chip_model)-1); break;
        case CHIP_ESP32S2: strncpy(info->chip_model, "ESP32-S2", sizeof(info->chip_model)-1); break;
        case CHIP_ESP32S3: strncpy(info->chip_model, "ESP32-S3", sizeof(info->chip_model)-1); break;
        case CHIP_ESP32C3: strncpy(info->chip_model, "ESP32-C3", sizeof(info->chip_model)-1); break;
        case CHIP_ESP32C6: strncpy(info->chip_model, "ESP32-C6", sizeof(info->chip_model)-1); break;
        default:           strncpy(info->chip_model, "ESP32-xx", sizeof(info->chip_model)-1); break;
    }
    info->cores = chip.cores;

    /* CPU freq */
    info->cpu_freq_mhz = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ;

    /* Internal flash */
    uint32_t flash_bytes = 0;
    esp_flash_get_size(NULL, &flash_bytes);
    info->flash_size_kb = flash_bytes / 1024;

    /* Internal flash kullanimi - sadece dahili flash partition'larini say */
    size_t used = 0;
    esp_partition_iterator_t it = esp_partition_find(ESP_PARTITION_TYPE_ANY, ESP_PARTITION_SUBTYPE_ANY, NULL);
    while (it) {
        const esp_partition_t *p = esp_partition_get(it);
        /* Harici flash partition'larini atla (esp_partition_register_external ile eklenenler) */
        if (p->flash_chip == NULL || p->flash_chip == esp_flash_default_chip) {
            used += p->size;
        }
        it = esp_partition_next(it);
    }
    esp_partition_iterator_release(it);
    info->flash_used_kb = (uint32_t)(used / 1024);

    /* Heap */
    info->heap_total = heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    info->heap_free = esp_get_free_heap_size();
    info->heap_min_free = heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);

    /* Uptime */
    info->uptime_seconds = esp_timer_get_time() / 1000000;

    /* WiFi RSSI */
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        info->wifi_rssi = ap_info.rssi;
    }

    /* Reset reason */
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON:  strncpy(info->reset_reason, "Power on", sizeof(info->reset_reason)-1); break;
        case ESP_RST_SW:       strncpy(info->reset_reason, "Software", sizeof(info->reset_reason)-1); break;
        case ESP_RST_PANIC:    strncpy(info->reset_reason, "Panic", sizeof(info->reset_reason)-1); break;
        case ESP_RST_INT_WDT:
        case ESP_RST_TASK_WDT:
        case ESP_RST_WDT:      strncpy(info->reset_reason, "Watchdog", sizeof(info->reset_reason)-1); break;
        case ESP_RST_DEEPSLEEP:strncpy(info->reset_reason, "Deep sleep", sizeof(info->reset_reason)-1); break;
        case ESP_RST_BROWNOUT: strncpy(info->reset_reason, "Brownout", sizeof(info->reset_reason)-1); break;
        default:               strncpy(info->reset_reason, "Unknown", sizeof(info->reset_reason)-1); break;
    }

    /* FW version — CMakeLists.txt PROJECT_VER'den dinamik al */
    const esp_app_desc_t *app_desc = esp_app_get_description();
    strncpy(info->fw_version, app_desc->version, sizeof(info->fw_version)-1);

    return ESP_OK;
}
