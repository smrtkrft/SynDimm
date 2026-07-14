#pragma once

// sd_offline — hedef erişilebilirlik izleyicisi. PURE modül (libc only,
// zaman enjekte; host testi: test/host/test_offline.c). Plan "Offline
// Semantiği (V1)":
//   * 3 ardışık transport hatası → OFFLINE (target.offline olayını çağıran
//     yayınlar); yerel değer ilerlemeye devam eder.
//   * OFFLINE iken gönderimler 5 sn'de bir proba düşer (should_send).
//   * İlk başarılı gönderim → ONLINE (target.online + uzlaştırma çağıranda).
//   * WiFi düşünce tüm slotlar anında offline (probe yakma yok);
//     WiFi gelince offline bayrağı KALIR (dürüstlük) ama probe kapısı
//     açılır — uzlaştırma gönderimi başarırsa ONLINE'a döner.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_OFFLINE_FAIL_LIMIT  3
#define SD_OFFLINE_PROBE_MS    5000
#define SD_OFFLINE_MAX_SLOTS   8

typedef struct {
    int     fail;
    bool    offline;
    int64_t last_probe_ms;
} sd_offline_slot_t;

typedef struct {
    sd_offline_slot_t s[SD_OFFLINE_MAX_SLOTS];
    int               n;
} sd_offline_t;

typedef enum {
    SD_OFFLINE_NONE = 0,
    SD_OFFLINE_WENT,    // yeni offline oldu → target.offline yayınla
    SD_OFFLINE_CAME,    // geri geldi → target.online + uzlaştırma
} sd_offline_evt_t;

void sd_offline_init(sd_offline_t *o, int nslots);

// Gönderim sonucu bildir (idx 0-tabanlı).
sd_offline_evt_t sd_offline_feed(sd_offline_t *o, int idx, bool ok, int64_t now_ms);

// Ağa çıkılmalı mı? Online → her zaman true. Offline → 5 sn'de bir true
// (true döndürdüğünde probe zamanını günceller).
bool sd_offline_should_send(sd_offline_t *o, int idx, int64_t now_ms);

// WiFi koptu: tüm slotlar offline. Dönüş: YENİ offline olanların bitmask'i
// (bit idx) — çağıran her biri için target.offline yayınlar.
uint32_t sd_offline_wifi_down(sd_offline_t *o);

// WiFi geldi: sayaçlar sıfır, offline bayrağı kalır, probe kapısı açılır.
void sd_offline_wifi_up(sd_offline_t *o);

bool sd_offline_is_offline(const sd_offline_t *o, int idx);

#ifdef __cplusplus
}
#endif
