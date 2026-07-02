#pragma once

// sd_mode_engine iç başlığı — engine çekirdeği ile CLI dosyası arasında
// paylaşılan yapılar. Dış dünya sd_mode_engine.h kullanır.

#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "cJSON.h"

#include "sd_behavior.h"
#include "sd_proto.h"
#include "sd_mode_engine.h"

// Slot bağlamı — sd_behavior_ctx opak tipinin gerçek gövdesi.
struct sd_behavior_ctx {
    int                  slot;        // 1..3
    cJSON               *binding;     // sahipli; NULL = slot boş
    cJSON               *profile;     // sahipli; NULL olabilir
    const sd_behavior_t *drv;
    void                *priv;
    sd_target_t          target;      // targets[0] çözülmüş
    int                  value;
    bool                 state;
    bool                 enabled;
    bool                 gestures_enabled;  // binding params (vars. true)
    bool                 error;       // ardışık hata eşiği aşıldı
    int                  err_count;
    bool                 nvs_dirty;   // değer debounce bekliyor
    bool                 mqtt_held;   // bu slot MQTT oturumunu refcount'ladı
};

typedef struct sd_behavior_ctx slot_ctx_t;

#define SD_ENGINE_ERR_LIMIT   5       // ardışık hata → slot error
#define SD_ENGINE_NVS_NS      "sd_mode"
#define SD_ENGINE_QUEUE_LEN   16
#define SD_ENGINE_COALESCE_MS 100     // drain-keep-latest penceresi (≤10 Hz)
#define SD_ENGINE_NVS_DEBOUNCE_MS 2000

// Kuyruk işi — HAFİF (kod incelemesi: cJSON kopyası detent başına heap
// fırtınasıydı; ağaçlar artık yürütme anında, kilit altında üretilir).
typedef enum { ENG_JOB_SEND, ENG_JOB_TEST, ENG_JOB_NVS_SAVE,
               ENG_JOB_TIMEOUT } eng_job_type_t;

typedef struct {
    eng_job_type_t     type;
    uint8_t            slot;
    sd_send_kind_t     kind;
    int                value;
    bool               toggle;
    // ENG_JOB_TEST sonuç alanları (iş bloğuyla taşınır — tek tahsis):
    SemaphoreHandle_t  done;      // cmd task verir; CLI bekler+temizler
    esp_err_t          result;
    int                status;
} eng_job_t;

// Çekirdek (sd_mode_engine.c) — CLI dosyasının kullandıkları:
extern slot_ctx_t s_slots[SD_MODE_SLOTS];
extern bool       s_recovery;

void  eng_lock(void);
void  eng_unlock(void);
int   eng_active_slot(void);
void  eng_set_active(int slot, bool persist);
esp_err_t eng_store_and_reload(int slot, const char *compact_json);
esp_err_t eng_clear_slot(int slot);
esp_err_t eng_set_value_direct(int slot, int value);
esp_err_t eng_toggle_direct(int slot);
esp_err_t eng_run_test(int slot, int *out_status, int timeout_ms);
void      eng_cli_register(void);
void      eng_config_register(void);   // config.export/import (sd_config_io.c)
