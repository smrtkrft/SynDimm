#pragma once

// sd_feedback — sistem olayı → buzzer deseni eşleyicisi. Tablo güdümlü;
// bileşenler buzzer'ı doğrudan çağırmak yerine event yayınlar, ses
// politikası tek yerde yaşar. Abonelikler ileriye dönüktür: mode.changed /
// safe.* olayları Faz 2-4'te yayınlanmaya başlayınca bipler kendiliğinden
// devreye girer.

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Event-bus aboneliklerini kurar ve açılış onay bipini (DIT1) çalar.
// sd_buzzer_init'ten SONRA çağrılmalı.
esp_err_t sd_feedback_init(void);

#ifdef __cplusplus
}
#endif
