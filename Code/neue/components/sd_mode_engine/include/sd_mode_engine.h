#pragma once

// sd_mode_engine — mod slotu motoru. 3 sabit slot × bağlama
// {davranış + profil + hedef + params}; aktif slot; girdi yönlendirme;
// coalescing'li tek ağ task'ı (sd_cmd); NVS değer debounce'u; mode.* CLI.
//
// Görev/kuyruk topolojisi (plan): sd_input → engine input (hızlı, kilitli)
// → drv->on_input → sd_ctx_send (kuyruk) → sd_cmd task (drain-keep-latest
// 100 ms coalesce) → sd_proto_execute. Kuyruk işleri KENDİNE YETERLİdir
// (cmd düğümü kopyalanır) — slot hot-reload'u ağ çağrısıyla yarışmaz.
//
// Kilitleme: tek recursive mutex; ctx servisleri her bağlamdan güvenli.
// mode.value olayı detent başına DEĞİL, gönderim yolunda (≤10 Hz) yayınlanır.

#include <stdbool.h>
#include "esp_err.h"

#include "sd_behavior.h"   // sd_input_event_t

#ifdef __cplusplus
extern "C" {
#endif

#define SD_MODE_SLOTS 3

// CLI kaydı + NVS şeması. HER boot'ta çağrılır (recovery dahil) — recovery'de
// slotlar YÜKLENMEZ, mode.list {"recovery":true} bildirir; mode.set/select
// yalnız kalıcılaştırır (bozuk konfig CLI'dan düzeltilebilir kalsın).
esp_err_t sd_mode_engine_init(bool recovery);

// Slotları yükler + sd_cmd task'ını başlatır. YALNIZ normal boot'ta,
// sd_behaviors_init + sd_proto_init SONRASI çağrılır.
esp_err_t sd_mode_engine_start(void);

// Enkoder hızlı yolu (sd_input task): aktif slota girdi.
void sd_mode_engine_input(const sd_input_event_t *evt);

// Basılı+çevirme slot adımı: 1..3 sınırlı, sarmasız (eski 'N'/'P' semantiği).
void sd_mode_engine_slot_step(int dir);

int  sd_mode_engine_active_slot(void);

#ifdef __cplusplus
}
#endif
