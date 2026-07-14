#pragma once

// sd_buzzer — pasif buzzer desen çalıcısı (LEDC 2 kHz, esp_timer one-shot
// zinciri; task yok, poll yok — eski buzzer.c'nin poll sözleşmesi kaldırıldı).
//
// Her sd_buzzer_play çağrısı iki kapıdan geçer:
//   1) prefs "buzzer" == off → sessiz no-op
//   2) sessiz saatler aktif (prefs "quiet" + "tz", duvar saati biliniyorsa)
//      → sessiz no-op. Duvar saati yoksa fail-open: bipler çalınır.
// Meşgulken çağrı yoksayılır (eski davranışın portu — kuyruk yok).
//
// sd_beep_t plan §C1'de sd_behavior.h'te tanımlıydı; desen donanıma ait
// olduğu için burada yaşar — sd_behavior.h (T2.4) bu başlığı içerecek.

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    SD_BEEP_OK = 0,    // 80ms + 240ms   — onay (çift ton hissi)
    SD_BEEP_ERR,       // 400ms + 400ms  — hata (uzun-uzun)
    SD_BEEP_DIT1,      // 1 × 80ms       — mod 1 / genel bilgi
    SD_BEEP_DIT2,      // 2 × 80ms       — mod 2
    SD_BEEP_DIT3,      // 3 × 80ms       — mod 3 / safe onay
    SD_BEEP_LONG,      // 400ms          — uyarı / safe yanlış dizi
} sd_beep_t;

// LEDC + esp_timer kurulumu. sd_prefs_init'ten SONRA çağrılmalı
// (gating prefs okur ve prefs.changed'e abone olur).
esp_err_t sd_buzzer_init(void);

// Deseni başlat (bloklamaz). Kapılar için üstteki nota bakın.
void sd_buzzer_play(sd_beep_t pattern);

// Çalan deseni kes, çıkışı sustur.
void sd_buzzer_stop(void);

bool sd_buzzer_is_playing(void);

#ifdef __cplusplus
}
#endif
