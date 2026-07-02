#pragma once

// SynDimm PCB rev B pin haritası — TEK doğruluk kaynağı.
// ⚠️ Rev B (harici flash çıkarılmış kart) üretimden gelince bu değerler
// donanımla DOĞRULANACAK (plan: Risk #9). Değerler rev A ile aynı varsayıldı
// (eski kod: encoder.c:20-22, buzzer.c:14).

#define SD_PIN_ENC_CLK      19   // rotary encoder CLK (quadrature A)
#define SD_PIN_ENC_DT       20   // rotary encoder DT  (quadrature B)
#define SD_PIN_ENC_SW       18   // rotary encoder buton (aktif-low, dahili pull-up)
#define SD_PIN_BUZZER       17   // pasif buzzer, LEDC PWM 2 kHz

#define SD_PIN_BOOT_RESERVED 9   // ESP32-C6 BOOT pini — kullanılamaz
