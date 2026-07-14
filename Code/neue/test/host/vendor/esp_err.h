// Host-test stub — sd_profiles.h gibi esp_err_t bildiren başlıkların host
// derlemesi için asgari tanım. Gerçek IDF başlığının yerine YALNIZ test/host
// altında geçer (include sırası: -Ivendor). Fonksiyon gövdeleri değil,
// yalnız tipler/sabitler: saf modüller esp API'si ÇAĞIRMAZ.
#pragma once

typedef int esp_err_t;

#define ESP_OK    0
#define ESP_FAIL -1
