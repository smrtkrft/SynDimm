#pragma once

// sd_gesture — ham enkoder girdisini semantik jestlere sınıflandırır.
// PURE modül: yalnız libc, zaman int64_t ms olarak enjekte edilir
// (host testi: test/host/test_gesture.c).
//
// Bant haritası (kilitli kararlar, plan "Buton bantları"):
//   çevirme (buton serbest)      → ROTATE ±1 / detent
//   basılı + çevirme             → MODE_STEP ±1 (bırakma sınıflandırması
//                                  TAMAMEN bastırılır — ≥5 sn kontrol
//                                  bantları dahil; kasıtlı kontrol jesti
//                                  dönmesiz basılı tutmadır)
//   bırakma <400 ms              → jest AÇIK: tık sayacı (2. tık pencere
//                                  içindeyse DOUBLE_CLICK; tek tık pencere
//                                  bitince poll'dan CLICK)
//                                  jest KAPALI: anında CLICK (0 ms gecikme)
//   bırakma 400–1500 ms          → jest AÇIK: LONG_PRESS; KAPALI: CLICK
//   bırakma 1500–5000 ms         → ölü bölge (hiçbir şey)
//   bırakma ≥5000 ms (dönmesiz)  → CONTROL_RELEASE(süre) — çağıran bunu
//                                  "button.released" {duration_ms} olarak
//                                  yayınlar; restart/factory kararını
//                                  sk_control verir (5-10 sn / ≥10 sn)
//
// KRİTİK: <5 sn bırakmalar için CONTROL_RELEASE ASLA üretilmez — aksi her
// tık sk_control'de pairing penceresi açardı (sk_control.c:72-74).

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Zamanlama sabitleri (ms)
#define SD_GESTURE_CLICK_MAX_MS      400
#define SD_GESTURE_DOUBLE_WINDOW_MS  300
#define SD_GESTURE_LONG_MAX_MS       1500
#define SD_GESTURE_CONTROL_MIN_MS    5000

typedef enum {
    SD_GESTURE_ROTATE,           // value = ±1 (detent)
    SD_GESTURE_CLICK,            // value = 0
    SD_GESTURE_DOUBLE_CLICK,     // value = 0
    SD_GESTURE_LONG_PRESS,       // value = 0
    SD_GESTURE_MODE_STEP,        // value = ±1 (slot yönü)
    SD_GESTURE_CONTROL_RELEASE,  // value = basılı tutma süresi (ms)
} sd_gesture_type_t;

typedef void (*sd_gesture_emit_t)(sd_gesture_type_t type, int32_t value, void *user);

typedef struct {
    // yapılandırma
    bool              gestures_enabled;
    sd_gesture_emit_t emit;
    void             *user;
    // durum
    bool    pressed;
    int64_t press_time;
    bool    rotated_while_pressed;
    int     pending_clicks;          // 0 veya 1
    int64_t last_release_time;
} sd_gesture_t;

// g çağıran tarafından tahsis edilir; emit NULL olamaz.
void sd_gesture_init(sd_gesture_t *g, sd_gesture_emit_t emit, void *user,
                     bool gestures_enabled);

// Jest anahtarı (prefs "gestures"). Kapatınca bekleyen tık anında CLICK
// olarak boşaltılır (kaybolmaz).
void sd_gesture_set_enabled(sd_gesture_t *g, bool enabled, int64_t now_ms);

// Ham girdiler — sd_encoder task'ından, debounce SONRASI çağrılır.
void sd_gesture_on_rotate(sd_gesture_t *g, int dir /* +1 sağ / -1 sol */, int64_t now_ms);
void sd_gesture_on_button(sd_gesture_t *g, bool pressed, int64_t now_ms);

// Periyodik çağrı (~10 ms): çift-tık penceresi dolan bekleyen tıkı CLICK
// olarak yayınlar.
void sd_gesture_poll(sd_gesture_t *g, int64_t now_ms);

// Kontrol-uygun basılı tutma süresi: basılıysa ve dönme olmadıysa geçen ms,
// aksi halde -1. Çağıran 5 sn/10 sn eşik geçişlerinde buzzer kademesi çalar.
int64_t sd_gesture_hold_ms(const sd_gesture_t *g, int64_t now_ms);

#ifdef __cplusplus
}
#endif
