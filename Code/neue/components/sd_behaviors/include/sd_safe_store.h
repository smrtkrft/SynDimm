#pragma once

// sd_safe_store — safe modu kayıt deposu (NVS ns "sd_safe", e1..e5).
// Kayıt şeması (plan §4.3):
//   {"v":2,"enabled":true,"sequence":"L3-R5-L2","endpoint":"<sk_api adı>",
//    "lockout":{"enabled":true,"after":5,"seconds":30}}
// Diziler SIR: safe.* komutları requires_auth (yalnız kimlikli BLE/TCP);
// safe.list sequence'i GÖSTERMEZ (yalnız uzunluk).
// Lockout politikası SLOT geneli uygulanır (yanlış giriş hiçbir kayda
// eşleşmez, tek sayaç mantıklıdır) — etkin kayıtlardan lockout'u açık ilk
// kaydın after/seconds değerleri kullanılır.

#include <stdbool.h>
#include "esp_err.h"

#include "sd_sequence.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_SAFE_ENTRIES       5
#define SD_SAFE_ENDPOINT_MAX  31

esp_err_t sd_safe_store_init(void);   // NVS yükle + safe.* CLI + wipe hook

// Diziyi etkin kayıtlarla karşılaştır. Eşleşme: kayıt no (1..5) döner ve
// endpoint_out doldurulur; yoksa 0.
int sd_safe_store_match(const sd_seq_t *seq,
                        char *endpoint_out, size_t endpoint_cap);

// Slot-geneli lockout politikası. Politika yoksa false.
bool sd_safe_store_lockout_policy(int *out_after, int *out_seconds);

// İlk etkin kaydın endpoint'i (mode.test için). Yoksa false.
bool sd_safe_store_first_endpoint(char *endpoint_out, size_t endpoint_cap);

// config.import için programatik yol. validate endpoint VARLIĞINI denetlemez
// (import sırasında sk_api envanteri farklı olabilir) — biçim denetler.
bool      sd_safe_store_validate_json(const cJSON *root, const char **reason);
esp_err_t sd_safe_store_set_json(int n, const cJSON *root);

#ifdef __cplusplus
}
#endif
