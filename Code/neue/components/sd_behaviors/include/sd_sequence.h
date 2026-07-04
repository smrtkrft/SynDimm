#pragma once

// sd_sequence — safe modu kombinasyon eşleyicisi. PURE modül (libc only;
// host testi: test/host/test_sequence.c). Semantik kaynağı: eski
// safe_lock.c:243-310 (tik birikimi, yön-değişimi commit, buton commit),
// :334-355 (timeout finalize), :65-79 (karşılaştırma).
//
// Gösterim: işaretli segment dizisi — +n = n tik sağ, -n = n tik sol.
// "L3-R5-L2-B" metin biçimi (plan §4.3) codec'le çevrilir; kuyruktaki
// "-B" opsiyoneldir ve saklanmaz (buton zaten segment sonlandırıcıdır).

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define SD_SEQ_MAX_SEGMENTS      16  // parser/dizi sınırı (geriye-uyum: eski kayıtlar okunur)
#define SD_SEQ_MAX_TICKS         50  // segment başına (SAFE_MODE kılavuzu)
#define SD_SEQ_MIN_SEGMENTS      3   // safe.set doğrulaması (kılavuz: en az 3)
// Yeni dizi OLUŞTURMA ve CANLI GİRİŞ tavanı (ürün kuralı: şifre 3-6 eleman).
// Parser sınırı (16) bilinçli olarak ayrı tutulur: NVS'te kalmış eski uzun
// kayıtlar parse edilebilir kalır ama safe.set reddeder, canlı giriş 7.
// segmentte overflow işaretler (deneme başarısız sayılır — drv_safe.c).
#define SD_SEQ_MAX_SET_SEGMENTS  6

typedef struct {
    int8_t seg[SD_SEQ_MAX_SEGMENTS];
    int    len;
} sd_seq_t;

// --- Girdi biriktirici -------------------------------------------------------
typedef struct {
    sd_seq_t cur;
    int      cur_ticks;    // aktif segmentteki tik sayısı
    int      cur_dir;      // +1 / -1 / 0 (henüz yok)
    bool     active;       // giriş başladı mı
    bool     overflow;     // segment limiti aşıldı → eşleşme geçersiz
} sd_seq_input_t;

void sd_seq_input_init(sd_seq_input_t *in);

// Detent: +1 sağ / -1 sol. Yön değişimi önceki segmenti kapatır
// (eski :243-289 semantiği).
void sd_seq_input_rotate(sd_seq_input_t *in, int dir);

// Buton: aktif segmenti kapatır (eski :290-303).
void sd_seq_input_button(sd_seq_input_t *in);

// Deneme taşmış mı? Bekleyen (henüz commit edilmemiş) segment dahil —
// finalize'dan ÖNCE çağrılmalı; finalize durumu sıfırladığı için bayrak
// orada kaybolur. drv_safe taşan denemeyi başarısız saymak için kullanır.
bool sd_seq_input_overflowed(const sd_seq_input_t *in);

// Zaman aşımı: kalan segmenti kapatır; sonuç out'a yazılır.
// Dönüş false = giriş yoktu ya da taşma oldu (eşleşme denenmez).
bool sd_seq_input_finalize(sd_seq_input_t *in, sd_seq_t *out);

bool sd_seq_equal(const sd_seq_t *a, const sd_seq_t *b);

// --- Metin codec'i -----------------------------------------------------------
// "L3-R5-L2" veya "L3-R5-L2-B" → sd_seq_t. Kurallar: 1..16 segment,
// tik 1..50, yön karakteri L/R (büyük harf), sayı zorunlu.
bool sd_seq_parse(const char *s, sd_seq_t *out);

// sd_seq_t → "R4-L2-R3". Dönüş: yazılan bayt (NUL hariç), sığmazsa 0.
size_t sd_seq_format(const sd_seq_t *seq, char *out, size_t cap);

#ifdef __cplusplus
}
#endif
