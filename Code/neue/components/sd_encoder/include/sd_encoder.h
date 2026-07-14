#pragma once

// sd_encoder — EC11 döner enkoder sürücüsü (quadrature ISR + buton).
// Port kaynağı: eski encoder.c (tablo :37-42, ISR+ring :68-99, GPIO :122-180,
// buton debounce :182-260). ESKİDEN FARKLI: bu katman SADECE ham girdi üretir —
// reboot/factory-reset/mod-onayı gibi kontrol kararları burada YOK
// (encoder.c:102-118 ve :232-243 bilinçli olarak porte edilmedi; o işler
// jest katmanı [T1.4] üzerinden sk_control'e akar).
//
// GPIO sahipliği: CLK/DT/SW pinlerinin tek sahibi bu bileşendir
// (sk_button_init ÇAĞRILMAZ — plan: Buton bantları).

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// GPIO + ISR kurulumu ve sd_input task'ının (10 ms döngü, prio 5) başlatılması.
esp_err_t sd_encoder_init(void);

#ifdef __cplusplus
}
#endif
