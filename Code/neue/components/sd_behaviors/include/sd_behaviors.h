#pragma once

// sd_behaviors — yerleşik sürücülerin kaydı. sd_behavior.h sözleşmedir;
// bu başlık yalnız init + yerleşik sürücü fabrikalarını verir.

#include "esp_err.h"
#include "sd_behavior.h"

#ifdef __cplusplus
extern "C" {
#endif

// Yerleşik sürücüleri registry'ye kaydeder. Motor init'inden ÖNCE çağrılır.
esp_err_t sd_behaviors_init(void);

// Yerleşik sürücü tanımları (statik ömürlü).
const sd_behavior_t *sd_behavior_dimmer(void);
// T3.3: sd_behavior_shutter(), sd_behavior_mqtt_remote(); T4.2: sd_behavior_safe()

#ifdef __cplusplus
}
#endif
