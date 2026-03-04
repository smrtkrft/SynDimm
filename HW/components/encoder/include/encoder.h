#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * SynDimm - EC11 Rotary Encoder (ESP-IDF)
 * CLK=GPIO19, DT=GPIO20, SW=GPIO18
 *
 * Event turleri:
 *   'L' = Sola cevirme
 *   'R' = Saga cevirme
 *   'B' = Kisa buton basisi
 *   'M' = Mod degisikligi onaylandi (basili tut + cevir + birak)
 *   'P' = Uzun basma (5sn reboot, 10sn factory reset)
 */

/**
 * Encoder baslatma - GPIO, ISR ve dahili state'i hazirlar
 */
esp_err_t encoder_init(void);

/**
 * Buton durumunu yoklar ve event buffer'i kontrol eder
 * Her loop iterasyonunda cagirilmali
 * @return true ise encoder_read() ile event okunabilir
 */
bool encoder_available(void);

/**
 * Siradaki event'i oku ve buffer'dan cikar
 * @return 'L','R','B','M','P' veya event yoksa 0
 */
char encoder_read(void);

/**
 * Event buffer'i temizle
 */
void encoder_clear(void);

/**
 * Buton su an basili mi
 */
bool encoder_is_button_held(void);

/**
 * Buton mod degisikligi icin yeterince uzun basildi mi (1sn)
 */
bool encoder_is_held_long_enough(void);

/**
 * Basili tutarken cevirme yapildigini bildir (mod degisikligi)
 */
void encoder_set_mode_change_triggered(void);

/**
 * Son yon: 'L', 'R' veya 0
 */
char encoder_get_last_direction(void);

/**
 * Buton basili tutulma suresi (ms). Basili degilse 0 doner.
 */
int64_t encoder_get_hold_duration_ms(void);
