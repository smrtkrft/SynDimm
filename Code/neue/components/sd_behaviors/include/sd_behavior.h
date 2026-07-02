#pragma once

// sd_behavior — davranış sürücüsü SÖZLEŞMESİ (API v1, DONDURULMUŞ).
// Plan §C1'in vücut bulmuş hali; ARCHITECTURE.md §3 kuralları geçerli:
//   * Sürücü kendi task'ını AÇMAZ — motor tek task'tan çağırır.
//   * Donanıma/ağa/NVS'e doğrudan erişim YOK — yalnız sd_ctx_* servisleri.
//   * on_input BLOKLAMAZ; ağ işi sd_ctx_send ile kuyruğa gider.
//   * Yeni sürücü = bu struct'ı doldur + sd_behavior_register. Mevcut koda
//     dokunulmaz (registry deseni = "kütüphane" kilitli kararı).
// Plandan sapmalar (gerekçeli):
//   * manifest() üyesi YOK — sk_baseline manifest'i CLI kayıtlarından
//     türetiyor, SKAPP manifest'i zaten kullanmıyor (keşif bulgusu).
//   * sd_input_event_t'ye ts_ms eklendi — ivmelenme (accel) hesabı için.
//   * sd_beep_t sd_buzzer.h'ta yaşar (donanım bileşeninin tipi).

#include <stdint.h>
#include <stdbool.h>

#include "esp_err.h"
#include "cJSON.h"

#include "sd_buzzer.h"   // sd_beep_t

#ifdef __cplusplus
extern "C" {
#endif

#define SD_BEHAVIOR_API_V1  1

typedef enum {
    SD_INPUT_ROTATE,        // delta = ±1 (detent)
    SD_INPUT_CLICK,
    SD_INPUT_DOUBLE_CLICK,  // yalnız jestler açıkken (global pref VE binding
    SD_INPUT_LONG_PRESS,    //   params.gestures_enabled) sürücüye ulaşır
} sd_input_type_t;

typedef struct {
    sd_input_type_t type;
    int32_t         delta;   // ROTATE için ±1, diğerlerinde 0
    int64_t         ts_ms;   // monotonik zaman (ivmelenme hesabı için)
} sd_input_event_t;

typedef struct sd_behavior_ctx sd_behavior_ctx_t;   // opak — motor sahipli

typedef struct {
    const char *id;              // "dimmer" | "shutter" | "safe" | "mqtt_remote"
    uint16_t    api_version;     // SD_BEHAVIOR_API_V1
    esp_err_t (*init)     (sd_behavior_ctx_t *ctx);
    void      (*deinit)   (sd_behavior_ctx_t *ctx);
    esp_err_t (*on_input) (sd_behavior_ctx_t *ctx, const sd_input_event_t *evt);
    esp_err_t (*get_state)(sd_behavior_ctx_t *ctx, char *json, size_t cap);
    esp_err_t (*test)     (sd_behavior_ctx_t *ctx);   // NULL olabilir;
                                                      // cmd task'ta, ≤5 sn
    // sd_ctx_arm_timeout süresi dolunca cmd task'ta, kilit altında çağrılır
    // (safe'in 2 sn hareketsizlik commit'i). NULL olabilir. BLOKLAMAZ.
    esp_err_t (*on_timeout)(sd_behavior_ctx_t *ctx);
} sd_behavior_t;

// Statik kayıt (boot'ta, drv tabloları statik ömürlü olmalı).
esp_err_t             sd_behavior_register(const sd_behavior_t *drv);
const sd_behavior_t  *sd_behavior_find(const char *id);
// Kayıtlı sürücüleri listele (mode.set doğrulaması + tanılama).
int                   sd_behavior_count(void);
const sd_behavior_t  *sd_behavior_at(int idx);

// --- ctx servisleri (sd_mode_engine implemente eder) ------------------------

typedef enum { SD_SEND_VALUE, SD_SEND_TOGGLE, SD_SEND_STOP } sd_send_kind_t;

int          sd_ctx_slot      (const sd_behavior_ctx_t *ctx);   // 1..3
const cJSON *sd_ctx_binding   (const sd_behavior_ctx_t *ctx);   // bağlama JSON
const cJSON *sd_ctx_profile   (const sd_behavior_ctx_t *ctx);   // NULL olabilir
void        *sd_ctx_priv      (const sd_behavior_ctx_t *ctx);
void         sd_ctx_set_priv  (sd_behavior_ctx_t *ctx, void *priv);
int          sd_ctx_get_value (const sd_behavior_ctx_t *ctx);
bool         sd_ctx_get_state (const sd_behavior_ctx_t *ctx);
// Önbelleği günceller + NVS debounce + mode.value olayı yayınlar.
void         sd_ctx_set_value (sd_behavior_ctx_t *ctx, int value, bool state);
// Ağ gönderimini kuyruğa atar (coalesce edilir); bloklamaz.
esp_err_t    sd_ctx_send      (sd_behavior_ctx_t *ctx, sd_send_kind_t kind,
                               int value, bool toggle);
// sk_api USER endpoint tetikle (safe sürücüsü; async, sk_api_send).
esp_err_t    sd_ctx_api_send  (sd_behavior_ctx_t *ctx, const char *endpoint_name,
                               const char *payload);
// prefs+quiet kapılı bip.
void         sd_ctx_beep      (sd_behavior_ctx_t *ctx, sd_beep_t pattern);
// Slot'un tek-atımlık zaman aşımını (yeniden) kur — dolunca drv->on_timeout
// cmd task'ta çağrılır. ms=0 iptal eder.
void         sd_ctx_arm_timeout(sd_behavior_ctx_t *ctx, uint32_t ms);
// Olay yayınla (motor, kilit bırakıldıktan sonra event-bus'a iletir —
// aboneler motoru kilitleyemez). Yalnız sürücü geri çağrılarından kullan.
void         sd_ctx_publish   (sd_behavior_ctx_t *ctx, const char *event,
                               const char *payload_json);

#ifdef __cplusplus
}
#endif
