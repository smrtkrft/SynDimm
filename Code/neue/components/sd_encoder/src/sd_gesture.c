// sd_gesture — pure jest sınıflandırıcı (bkz. sd_gesture.h).

#include <stddef.h>

#include "sd_gesture.h"

static void flush_pending_click(sd_gesture_t *g)
{
    if (g->pending_clicks > 0) {
        g->pending_clicks = 0;
        g->emit(SD_GESTURE_CLICK, 0, g->user);
    }
}

void sd_gesture_init(sd_gesture_t *g, sd_gesture_emit_t emit, void *user,
                     bool gestures_enabled)
{
    g->gestures_enabled      = gestures_enabled;
    g->emit                  = emit;
    g->user                  = user;
    g->pressed               = false;
    g->press_time            = 0;
    g->rotated_while_pressed = false;
    g->pending_clicks        = 0;
    g->last_release_time     = 0;
}

void sd_gesture_set_enabled(sd_gesture_t *g, bool enabled, int64_t now_ms)
{
    (void)now_ms;
    if (g->gestures_enabled && !enabled) {
        // Kapanırken bekleyen tık kaybolmasın — anında CLICK.
        flush_pending_click(g);
    }
    g->gestures_enabled = enabled;
}

void sd_gesture_on_rotate(sd_gesture_t *g, int dir, int64_t now_ms)
{
    (void)now_ms;
    if (dir != 1 && dir != -1) return;

    if (g->pressed) {
        // Basılı + çevirme = slot adımı. Bırakma sınıflandırmasını bastır
        // (≥5 sn kontrol bantları dahil — eski mode_change_flag semantiği).
        g->rotated_while_pressed = true;
        g->emit(SD_GESTURE_MODE_STEP, dir, g->user);
        return;
    }

    // Bekleyen tık varsa çevirmeden önce boşalt (eylem sırası korunur:
    // kullanıcı tık + çevirme yaptıysa toggle önce gelmeli).
    flush_pending_click(g);
    g->emit(SD_GESTURE_ROTATE, dir, g->user);
}

void sd_gesture_on_button(sd_gesture_t *g, bool pressed, int64_t now_ms)
{
    if (pressed) {
        if (g->pressed) return;              // yinelenen basış — yok say
        g->pressed               = true;
        g->press_time            = now_ms;
        g->rotated_while_pressed = false;
        return;
    }

    if (!g->pressed) return;                 // eşleşmeyen bırakma — yok say
    g->pressed = false;
    int64_t dur = now_ms - g->press_time;
    if (dur < 0) dur = 0;                    // zaman sıçraması → güvenli taraf

    if (g->rotated_while_pressed) {
        g->rotated_while_pressed = false;    // slot adımı zaten işlendi
        return;
    }

    if (dur >= SD_GESTURE_CONTROL_MIN_MS) {
        // Kontrol bandı — sınıflandırmayı sk_control yapar (5-10/≥10 sn).
        g->emit(SD_GESTURE_CONTROL_RELEASE, (int32_t)dur, g->user);
        return;
    }

    if (dur >= SD_GESTURE_LONG_MAX_MS) {
        return;                              // 1.5-5 sn ölü bölge
    }

    if (!g->gestures_enabled) {
        // Jestler kapalı: her <1.5 sn bırakma ANINDA tek tık.
        g->emit(SD_GESTURE_CLICK, 0, g->user);
        return;
    }

    if (dur >= SD_GESTURE_CLICK_MAX_MS) {
        g->emit(SD_GESTURE_LONG_PRESS, 0, g->user);
        return;
    }

    // Kısa tık — çift-tık penceresi mantığı.
    if (g->pending_clicks == 1 &&
        (now_ms - g->last_release_time) < SD_GESTURE_DOUBLE_WINDOW_MS) {
        g->pending_clicks = 0;
        g->emit(SD_GESTURE_DOUBLE_CLICK, 0, g->user);
        return;
    }
    // Pencere dışında bekleyen tık kaldıysa (poll henüz koşmadıysa) önce onu bas.
    flush_pending_click(g);
    g->pending_clicks    = 1;
    g->last_release_time = now_ms;
}

void sd_gesture_poll(sd_gesture_t *g, int64_t now_ms)
{
    if (g->pending_clicks == 1 &&
        (now_ms - g->last_release_time) >= SD_GESTURE_DOUBLE_WINDOW_MS) {
        g->pending_clicks = 0;
        g->emit(SD_GESTURE_CLICK, 0, g->user);
    }
}

int64_t sd_gesture_hold_ms(const sd_gesture_t *g, int64_t now_ms)
{
    if (!g->pressed || g->rotated_while_pressed) return -1;
    int64_t hold = now_ms - g->press_time;
    return hold < 0 ? 0 : hold;
}
