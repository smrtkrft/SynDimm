#pragma once

// sd_util_str — sd_util'in PURE kısmı (libc + cJSON; ESP başlığı yok).
// Host testleri bu başlığı ve sd_util_str.c'yi doğrudan derler.

#include <stdbool.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- cJSON tipli erişimciler (yoksa/yanlış tipse varsayılan) ---------------
int         sd_jsonu_int (const cJSON *obj, const char *key, int def);
bool        sd_jsonu_bool(const cJSON *obj, const char *key, bool def);
const char *sd_jsonu_str (const cJSON *obj, const char *key);   // NULL olabilir

// --- String denetimleri -------------------------------------------------------
// Çıktı JSON'una kaçışsız gömülen alanlar: '"', '\\' ve kontrol karakteri
// REDDEDİLİR. Boş string güvenlidir.
bool sd_stru_json_safe(const char *s);

// Kimlik alanları (NVS anahtarı olanlar): [A-Za-z0-9_-], boş olamaz.
bool sd_stru_ident_ok(const char *s);

#ifdef __cplusplus
}
#endif
