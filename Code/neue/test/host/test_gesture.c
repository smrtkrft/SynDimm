// sd_gesture birim testleri (host, assert tabanlı, sahte saat).

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sd_gesture.h"

// --- Emit yakalayıcı ---------------------------------------------------

#define CAP_MAX 32
static struct { sd_gesture_type_t type; int32_t value; } cap[CAP_MAX];
static int cap_n;

static void emit_cb(sd_gesture_type_t type, int32_t value, void *user)
{
    (void)user;
    assert(cap_n < CAP_MAX);
    cap[cap_n].type  = type;
    cap[cap_n].value = value;
    cap_n++;
}

static void reset(sd_gesture_t *g, bool gestures_on)
{
    cap_n = 0;
    memset(cap, 0, sizeof(cap));
    sd_gesture_init(g, emit_cb, NULL, gestures_on);
}

#define EXPECT(i, t, v) (cap[i].type == (t) && cap[i].value == (v))

// --- Testler --------------------------------------------------------------

static void test_rotate_plain(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_rotate(&g, +1, 100);
    sd_gesture_on_rotate(&g, -1, 120);
    assert(cap_n == 2);
    assert(EXPECT(0, SD_GESTURE_ROTATE, +1));
    assert(EXPECT(1, SD_GESTURE_ROTATE, -1));
}

static void test_single_click_deferred(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1100);          // 100 ms tık
    assert(cap_n == 0);                             // pencere bekliyor
    sd_gesture_poll(&g, 1100 + 299);
    assert(cap_n == 0);                             // pencere dolmadı
    sd_gesture_poll(&g, 1100 + 300);
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_CLICK, 0));
    sd_gesture_poll(&g, 1100 + 400);
    assert(cap_n == 1);                             // tekrar üretmez
}

static void test_double_click(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1080);
    sd_gesture_on_button(&g, true, 1200);
    sd_gesture_on_button(&g, false, 1280);          // 2. bırakma, pencere içi
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_DOUBLE_CLICK, 0));
    sd_gesture_poll(&g, 5000);
    assert(cap_n == 1);                             // bekleyen tık kalmadı
}

static void test_two_separate_clicks(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1100);
    sd_gesture_poll(&g, 1500);                      // pencere doldu → CLICK
    sd_gesture_on_button(&g, true, 2000);
    sd_gesture_on_button(&g, false, 2100);
    sd_gesture_poll(&g, 2500);
    assert(cap_n == 2);
    assert(EXPECT(0, SD_GESTURE_CLICK, 0) && EXPECT(1, SD_GESTURE_CLICK, 0));
}

static void test_gestures_off_immediate_click(void)
{
    sd_gesture_t g;
    reset(&g, false);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1100);
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_CLICK, 0));   // 0 ms gecikme
    // Çift tık denemesi → iki ayrı CLICK
    sd_gesture_on_button(&g, true, 1200);
    sd_gesture_on_button(&g, false, 1280);
    assert(cap_n == 2 && EXPECT(1, SD_GESTURE_CLICK, 0));
    // Uzun basış bandı da CLICK olur (jest kapalı)
    sd_gesture_on_button(&g, true, 2000);
    sd_gesture_on_button(&g, false, 2800);          // 800 ms
    assert(cap_n == 3 && EXPECT(2, SD_GESTURE_CLICK, 0));
}

static void test_long_press(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1800);          // 800 ms
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_LONG_PRESS, 0));
}

static void test_dead_zone(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1000 + 2000);   // 2 sn → ölü bölge
    assert(cap_n == 0);
    reset(&g, false);                               // jest kapalıyken de ölü
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1000 + 4999);
    assert(cap_n == 0);
}

static void test_control_release(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1000 + 6000);   // 6 sn → restart bandı
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_CONTROL_RELEASE, 6000));
    reset(&g, false);                               // pref'ten bağımsız
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1000 + 11000);  // 11 sn → factory bandı
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_CONTROL_RELEASE, 11000));
}

static void test_hold_rotate_mode_step(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_rotate(&g, +1, 1200);
    sd_gesture_on_rotate(&g, +1, 1300);
    sd_gesture_on_rotate(&g, -1, 1400);
    assert(cap_n == 3);
    assert(EXPECT(0, SD_GESTURE_MODE_STEP, +1));
    assert(EXPECT(1, SD_GESTURE_MODE_STEP, +1));
    assert(EXPECT(2, SD_GESTURE_MODE_STEP, -1));
    // 8 sn sonra bırakma: dönme olduğu için kontrol bandı BASTIRILIR.
    sd_gesture_on_button(&g, false, 1000 + 8000);
    assert(cap_n == 3);
}

static void test_click_then_rotate_flushes(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1100);          // bekleyen tık
    sd_gesture_on_rotate(&g, +1, 1150);             // pencere içinde çevirme
    assert(cap_n == 2);
    assert(EXPECT(0, SD_GESTURE_CLICK, 0));         // önce toggle
    assert(EXPECT(1, SD_GESTURE_ROTATE, +1));       // sonra çevirme
}

static void test_disable_flushes_pending(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 1000);
    sd_gesture_on_button(&g, false, 1100);
    assert(cap_n == 0);
    sd_gesture_set_enabled(&g, false, 1150);        // kapatınca tık kaybolmaz
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_CLICK, 0));
}

static void test_hold_ms(void)
{
    sd_gesture_t g;
    reset(&g, true);
    assert(sd_gesture_hold_ms(&g, 1000) == -1);     // basılı değil
    sd_gesture_on_button(&g, true, 1000);
    assert(sd_gesture_hold_ms(&g, 6500) == 5500);
    sd_gesture_on_rotate(&g, +1, 7000);             // dönme → kontrol-dışı
    assert(sd_gesture_hold_ms(&g, 8000) == -1);
    sd_gesture_on_button(&g, false, 9000);
    assert(sd_gesture_hold_ms(&g, 9100) == -1);
}

static void test_time_warp(void)
{
    sd_gesture_t g;
    reset(&g, true);
    sd_gesture_on_button(&g, true, 5000);
    sd_gesture_on_button(&g, false, 4000);          // saat geri sıçradı
    // Negatif süre 0'a kırpılır → kısa tık muamelesi, çökme yok.
    sd_gesture_poll(&g, 4000 + 300);
    assert(cap_n == 1 && EXPECT(0, SD_GESTURE_CLICK, 0));
}

int main(void)
{
    test_rotate_plain();
    test_single_click_deferred();
    test_double_click();
    test_two_separate_clicks();
    test_gestures_off_immediate_click();
    test_long_press();
    test_dead_zone();
    test_control_release();
    test_hold_rotate_mode_step();
    test_click_then_rotate_flushes();
    test_disable_flushes_pending();
    test_hold_ms();
    test_time_warp();
    printf("test_gesture: OK\n");
    return 0;
}
