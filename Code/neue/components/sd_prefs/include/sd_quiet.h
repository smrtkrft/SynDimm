#pragma once

// sd_quiet — sessiz saat aralığı çözümleyici. PURE modül: libc dışında
// bağımlılık yok, host testli (test/host/test_quiet.c). sd_prefs'teki
// format bekçisiyle aynı gramer; yetkili çözümleyici burasıdır.

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// "23:00-07:00" → gün içi dakika çifti. Boş/biçimsiz → false.
bool sd_quiet_parse(const char *s, uint16_t *from_min, uint16_t *to_min);

// Aralık aktif mi? Gece sarmalı destekli: from > to ise [from,24h)∪[0,to).
// from == to → hiçbir zaman aktif değil (dejenere aralık, kapalı sayılır).
// Aralık [from, to) — bitiş dakikası dahil değil.
bool sd_quiet_active(uint16_t from_min, uint16_t to_min, uint16_t now_min);

// "+02:00" / "-05:30" → dakika ofseti. Biçimsiz → false.
bool sd_tz_parse(const char *s, int16_t *offset_min);

#ifdef __cplusplus
}
#endif
