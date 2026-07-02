#pragma once

// sd_binding — bağlama (mod slotu ataması) JSON v2 doğrulayıcı.
// PURE modül (libc + cJSON; host testi: test/host/test_binding.c).
// Şema: ARCHITECTURE.md §4.1 + plan "Veri Şemaları".
//
// Zorunlu alanlar : v==2, behavior (string, 1..31)
// Seçimlik        : enabled(bool, vars. true), name(string ≤47),
//                   profile(string 1..15 — NVS key sınırı),
//                   targets(dizi 0..4; eleman: host zorunlu ≤63,
//                     port 1..65535 vars. 80, device_id ≤31, auth_key ≤127),
//                   params(nesne: step 1..25, accel bool, gestures_enabled
//                     bool, presets{double_click 0..100, long_press 0..100},
//                     topic ≤127, payload_value ≤255, payload_gesture ≤255)
// Bilinmeyen alanlar TOLERE edilir (ileri uyumluluk, contract §5 ruhu).
// Davranışa-özgü zorunluluklar (örn. dimmer'ın profile istemesi) motorda —
// burada yalnız tip/aralık/uzunluk denetimi yapılır.

#include <stdbool.h>
#include <stddef.h>

#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_BINDING_JSON_MAX   1024   // NVS'e yazılacak ham metin üst sınırı
#define SD_BINDING_TARGETS_MAX   4

// root'u doğrular. Hata durumunda false döner ve err'e kısa neden yazar
// (İngilizce, makine zarflarında "reason" alanı olarak kullanılabilir).
bool sd_binding_validate(const cJSON *root, char *err, size_t err_cap);

#ifdef __cplusplus
}
#endif
