#pragma once

// sd_prefs — seçimlik özellik anahtarları (kilitli karar: her seçimlik
// özellik CLI+GUI'den kapatılabilir; pref anahtarı olmayan özellik gemiye
// binemez). NVS ns "sd_prefs", RAM önbellekli.
//
// Anahtarlar (whitelist):
//   gestures  bool   varsayılan 1 — çift tık / uzun basış jestleri
//   buzzer    bool   varsayılan 1 — tüm sesler
//   quiet     str    varsayılan "" (kapalı) — "23:00-07:00" biçimi
//
// CLI: prefs.list / prefs.set <key> <value>
// Olay: prefs.changed {"key":"...","value":"..."}

#include <stdbool.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// NVS'ten yükler, prefs.* CLI komutlarını ve factory-reset silme kancasını
// kaydeder. sk_core_init sonrasında çağrılmalı. Idempotent değil — bir kez.
esp_err_t sd_prefs_init(void);

// Bool anahtar oku (gestures, buzzer). Bilinmeyen anahtar → def.
bool sd_prefs_get_bool(const char *key, bool def);

// String anahtar oku (quiet). out'a NUL'lu kopyalar; bilinmeyen anahtar →
// ESP_ERR_NOT_FOUND, kesme olursa ESP_ERR_INVALID_SIZE (yine NUL'lu).
esp_err_t sd_prefs_get_str(const char *key, char *out, size_t cap);

// Değişiklik bildirimi — prefs.set başarılı olduğunda çağrılır (CLI task
// bağlamında; kısa tut). En fazla 4 abone.
typedef void (*sd_prefs_change_cb_t)(const char *key, void *user);
esp_err_t sd_prefs_on_change(sd_prefs_change_cb_t cb, void *user);

#ifdef __cplusplus
}
#endif
