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
};

typedef struct sd_behavior_ctx slot_ctx_t;

#define SD_ENGINE_ERR_LIMIT   5       // ardışık hata → slot error
#define SD_ENGINE_NVS_NS      "sd_mode"
#define SD_ENGINE_QUEUE_LEN   16
#define SD_ENGINE_COALESCE_MS 100     // drain-keep-latest penceresi (≤10 Hz)
#define SD_ENGINE_NVS_DEBOUNCE_MS 2000

// Kuyruk işi — kendine yeterli (slot ağaçlarına işaret etmez).
typedef enum { ENG_JOB_SEND, ENG_JOB_TEST, ENG_JOB_NVS_SAVE } eng_job_type_t;

typedef struct {
    eng_job_type_t     type;
    uint8_t            slot;
    sd_send_kind_t     kind;
    int                value;
    bool               toggle;
    cJSON             *mini_profile;   // sahipli: {"protocol","prefix"}
    cJSON             *cmd_node;       // sahipli kopya
    sd_target_t        target;
    // ENG_JOB_TEST alanları:
    SemaphoreHandle_t  done;           // CLI bekletme
    esp_err_t         *out_err;
    int               *out_status;
} eng_job_t;

// Çekirdek (sd_mode_engine.c) — CLI dosyasının kullandıkları:
extern slot_ctx_t s_slots[SD_MODE_SLOTS];
extern bool       s_recovery;

void  eng_lock(void);
void  eng_unlock(void);
int   eng_active_slot(void);
void  eng_set_active(int slot, bool persist);
// Ham bağlama JSON'unu NVS'e yazar + slotu yeniden yükler (recovery'de yalnız
// yazar). compact_json sahipliği çağırandadır.
esp_err_t eng_store_and_reload(int slot, const char *compact_json);
esp_err_t eng_clear_slot(int slot);
// Motor-değeri yolu (mode.value CLI): clamp + cache + kuyruk.
esp_err_t eng_set_value_direct(int slot, int value);
esp_err_t eng_toggle_direct(int slot);
// Test işi: kuyruğa atar, ≤timeout bekler.
esp_err_t eng_run_test(int slot, int *out_status, int timeout_ms);
// CLI kayıtları (sd_mode_cli.c).
void      eng_cli_register(void);

// T2.6 offline izleyici kancaları (sd_offline.c):
void eng_offline_feed(int slot, bool transport_ok);
void eng_offline_wifi(bool connected);
