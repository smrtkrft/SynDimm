// sd_behaviors — sürücü kayıt defteri (registry). Yeni davranış eklemek =
// drv_*.c yazıp buradaki register çağrısına bir satır eklemek; başka hiçbir
// dosyaya dokunulmaz (ARCHITECTURE §1 "ekleme, değiştirme değil").

#include <string.h>

#include "esp_log.h"

#include "sk_capabilities.h"

#include "sd_behavior.h"
#include "sd_behaviors.h"
#include "sd_safe_store.h"

static const char *TAG = "sd_behaviors";

#define REGISTRY_MAX 8

static const sd_behavior_t *s_registry[REGISTRY_MAX];
static int s_count;

esp_err_t sd_behavior_register(const sd_behavior_t *drv)
{
    if (!drv || !drv->id || !drv->init || !drv->on_input || !drv->get_state) {
        return ESP_ERR_INVALID_ARG;
    }
    if (drv->api_version != SD_BEHAVIOR_API_V1) {
        ESP_LOGE(TAG, "'%s': desteklenmeyen API v%u", drv->id, drv->api_version);
        return ESP_ERR_NOT_SUPPORTED;
    }
    if (s_count >= REGISTRY_MAX) return ESP_ERR_NO_MEM;
    if (sd_behavior_find(drv->id)) return ESP_ERR_INVALID_STATE;   // çift kayıt

    s_registry[s_count++] = drv;
    ESP_LOGI(TAG, "kayit: %s (API v%u)", drv->id, drv->api_version);
    return ESP_OK;
}

const sd_behavior_t *sd_behavior_find(const char *id)
{
    if (!id) return NULL;
    for (int i = 0; i < s_count; i++) {
        if (strcmp(s_registry[i]->id, id) == 0) return s_registry[i];
    }
    return NULL;
}

int sd_behavior_count(void) { return s_count; }

const sd_behavior_t *sd_behavior_at(int idx)
{
    return (idx >= 0 && idx < s_count) ? s_registry[idx] : NULL;
}

esp_err_t sd_behaviors_init(void)
{
    ESP_ERROR_CHECK(sd_safe_store_init());   // safe.* CLI + NVS deposu

    // Yerleşik sürücüler.
    ESP_ERROR_CHECK(sd_behavior_register(sd_behavior_dimmer()));
    ESP_ERROR_CHECK(sd_behavior_register(sd_behavior_shutter()));
    ESP_ERROR_CHECK(sd_behavior_register(sd_behavior_mqtt_remote()));
    ESP_ERROR_CHECK(sd_behavior_register(sd_behavior_safe()));

    sk_capabilities_register_book("sd_behaviors", "1.2.0");
    ESP_LOGI(TAG, "ready (%d surucu)", s_count);
    return ESP_OK;
}
