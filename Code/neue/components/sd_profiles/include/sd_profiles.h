#pragma once

// sd_profiles — hedef cihaz profil deposu (NVS blob, ns "sd_profile",
// key = profil id). Şema v2 = eski şablon şeması (name/protocol/port/
// commands{set_value,toggle,stop}) + "v":2 + "id" + "behaviors":[].
// Katalog SKAPP/repoda yaşar; cihaz YALNIZ atanmış profilleri saklar.
//
// Kısıtlar: id 1..15 karakter (NVS key sınırı), ham JSON ≤ 2048 bayt,
// protocol ∈ {http, udp, mqtt}.
//
// CLI: profile.list / profile.get <id> / profile.add {json} /
//      profile.remove <id> (etkin bağlama referansı varsa ERR_BUSY).

#include <stdbool.h>
#include "esp_err.h"
#include "cJSON.h"

#ifdef __cplusplus
extern "C" {
#endif

#define SD_PROFILE_ID_MAX     15
#define SD_PROFILE_JSON_MAX   2048

esp_err_t sd_profiles_init(void);

// Profili yükleyip parse eder. Dönen ağacı çağıran cJSON_Delete ile bırakır.
// Yoksa/parse edilemezse NULL.
cJSON *sd_profiles_load(const char *id);

bool sd_profiles_exists(const char *id);

// profile.remove koruması: id'yi referanslayan ETKİN bağlama sayısı.
// Mode engine (T2.5) kendi sayacını kaydeder; kayıtlı değilken 0 döner.
typedef int (*sd_profiles_ref_checker_t)(const char *id);
void sd_profiles_set_ref_checker(sd_profiles_ref_checker_t fn);

#ifdef __cplusplus
}
#endif
