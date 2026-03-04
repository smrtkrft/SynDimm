#pragma once

#include "esp_err.h"
#include <stdbool.h>

/**
 * SynDimm - Buzzer (ESP-IDF)
 * GPIO17, 2kHz PWM, non-blocking dit patterns
 */

/**
 * Buzzer baslatma - LEDC PWM yapilandirmasi
 */
esp_err_t buzzer_init(void);

/**
 * Non-blocking state machine guncelleme
 * Her loop iterasyonunda cagirilmali
 */
void buzzer_update(void);

/**
 * Dit pattern cal (1, 2 veya 3 bip)
 * @param count 1-3 arasi dit sayisi
 */
void buzzer_play_dits(int count);

/**
 * Buzzer su an caliyor mu
 */
bool buzzer_is_playing(void);

/**
 * Buzzer'i durdur
 */
void buzzer_stop(void);
