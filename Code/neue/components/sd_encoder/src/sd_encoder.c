// sd_encoder — bkz. sd_encoder.h. Katman yapısı:
//   ISR (IRAM)      : quadrature çöz, detent başına 'R'/'L' → SPSC ring
//   sd_input task   : ring'i boşalt + butonu örnekle (50 ms debounce)
//                     → sd_gesture sınıflandırıcısı → olaylar
// Jest durumuna YALNIZ sd_input task dokunur; prefs değişimi bayrakla
// task'a taşınır (yarış yok).

#include <stdint.h>
#include <string.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sk_capabilities.h"
#include "sk_event_bus.h"

#include "sd_pins.h"
#include "sd_prefs.h"
#include "sd_buzzer.h"
#include "sd_gesture.h"
#include "sd_mode_engine.h"
#include "sd_encoder.h"

static const char *TAG = "sd_encoder";

#define STEPS_PER_DETENT    4      // EC11: 4 adım = 1 detent
#define EVENT_BUFFER_SIZE   10
#define BUTTON_DEBOUNCE_MS  50
#define POLL_PERIOD_MS      10

// Quadrature geçiş tablosu — indeks: (öncekiDurum << 2) | yeniDurum.
// +1 = sağ, -1 = sol, 0 = geçersiz/gürültü. (encoder.c:37-42 portu)
static const int8_t ENCODER_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

// ISR ↔ task paylaşımı — tek üretici (ISR) / tek tüketici (task) ring,
// kilitsiz; volatile yeterli (encoder.c:44-49 deseni).
static volatile uint8_t enc_state;
static volatile int8_t  enc_position;
static volatile char    event_buffer[EVENT_BUFFER_SIZE];
static volatile uint8_t buf_write;
static volatile uint8_t buf_read;

// Buton durumu (yalnız sd_input task erişir). Basış durumunun TEK doğruluk
// kaynağı sd_gesture'dır (kod incelemesi: kopya s_pressed/s_press_time
// ıraksayabiliyordu) — burada yalnız debounce için ham seviye tutulur.
static int     s_last_btn_level;
static int64_t s_last_btn_time;

// Jest sınıflandırıcı + basılı-tutma kademe bipleri (yalnız sd_input task).
static sd_gesture_t s_gesture;
static bool         s_warn_restart;    // 5 sn eşiği bildirildi
static bool         s_warn_factory;    // 10 sn eşiği bildirildi
static bool         s_gestures_eff;    // etkin bayrak (global pref AND binding)

static inline int64_t now_ms(void) { return esp_timer_get_time() / 1000; }

static void IRAM_ATTR add_event(char evt)
{
    uint8_t next = (buf_write + 1) % EVENT_BUFFER_SIZE;
    if (next != buf_read) {          // doluysa düşür (ring taşırma yok)
        event_buffer[buf_write] = evt;
        buf_write = next;
    }
}

static void IRAM_ATTR encoder_isr(void *arg)
{
    (void)arg;
    uint8_t clk = gpio_get_level(SD_PIN_ENC_CLK);
    uint8_t dt  = gpio_get_level(SD_PIN_ENC_DT);
    uint8_t new_state = (clk << 1) | dt;

    if (new_state == enc_state) return;

    int8_t dir = ENCODER_TABLE[(enc_state << 2) | new_state];
    enc_state = new_state;
    enc_position += dir;

    if (enc_position >= STEPS_PER_DETENT) {
        add_event('R');
        enc_position = 0;
    } else if (enc_position <= -STEPS_PER_DETENT) {
        add_event('L');
        enc_position = 0;
    }
}

// --- Jest yayını --------------------------------------------------------------
// ROTATE event-bus'a BASILMAZ: detent başına olay seli BLE/TCP istemcilerini
// boğar. T2.5'te doğrudan sd_mode_engine_input'a bağlanacak. Diğer jestler
// input.gesture olarak yayınlanır (SKAPP tanılama + mqtt_remote).
// CONTROL_RELEASE → "button.released" — sk_control 5-10 sn'yi restart,
// ≥10 sn'yi factory reset olarak sınıflandırır (sk_control.c:44-50).

static void engine_forward(sd_input_type_t type, int32_t delta)
{
    sd_input_event_t evt = {
        .type  = type,
        .delta = delta,
        .ts_ms = now_ms(),
    };
    sd_mode_engine_input(&evt);
}

static void gesture_emit(sd_gesture_type_t type, int32_t value, void *user)
{
    (void)user;
    switch (type) {
        case SD_GESTURE_ROTATE:
            // Event-bus'a BASILMAZ (detent seli) — doğrudan motora.
            engine_forward(SD_INPUT_ROTATE, value);
            break;
        case SD_GESTURE_CLICK:
            sk_event_bus_publish("input.gesture", "{\"type\":\"click\"}");
            engine_forward(SD_INPUT_CLICK, 0);
            break;
        case SD_GESTURE_DOUBLE_CLICK:
            sk_event_bus_publish("input.gesture", "{\"type\":\"double_click\"}");
            engine_forward(SD_INPUT_DOUBLE_CLICK, 0);
            break;
        case SD_GESTURE_LONG_PRESS:
            sk_event_bus_publish("input.gesture", "{\"type\":\"long_press\"}");
            engine_forward(SD_INPUT_LONG_PRESS, 0);
            break;
        case SD_GESTURE_MODE_STEP:
            sk_event_bus_publishf("input.gesture",
                                  "{\"type\":\"mode_step\",\"delta\":%d}", (int)value);
            sd_mode_engine_slot_step((int)value);
            break;
        case SD_GESTURE_CONTROL_RELEASE:
            sk_event_bus_publishf("button.released",
                                  "{\"duration_ms\":%d}", (int)value);
            break;
    }
}

// TEK etkin jest bayrağı: global prefs AND aktif bağlamanın parametresi
// (kod incelemesi: üç katmanlı kapılama tutarsızdı — binding kapalıyken
// bile çift-tık gecikmesi ödeniyordu). Her döngüde ucuz kontrol; yalnız
// değişince sınıflandırıcıya uygulanır.
static void refresh_effective_gestures(int64_t now)
{
    bool eff = sd_prefs_get_bool("gestures", true) &&
               sd_mode_engine_active_gestures();
    if (eff != s_gestures_eff) {
        s_gestures_eff = eff;
        sd_gesture_set_enabled(&s_gesture, eff, now);
        ESP_LOGI(TAG, "jestler %s", eff ? "acik" : "kapali");
    }
}

// --- Buton örnekleme (encoder.c:214-258 debounce yapısının portu, kontrol
//     yan-etkileri çıkarılmış hali) -------------------------------------------

static void poll_button(int64_t now)
{
    int cur = gpio_get_level(SD_PIN_ENC_SW);   // aktif-low

    if (cur != s_last_btn_level) {
        if ((now - s_last_btn_time) > BUTTON_DEBOUNCE_MS) {
            // Eşleşmeyen basış/bırakmayı sd_gesture kendisi yok sayar —
            // burada ikinci durum makinesi tutulmaz.
            sd_gesture_on_button(&s_gesture, cur == 0, now);
            s_last_btn_time = now;
        }
        s_last_btn_level = cur;
    }
}

// Basılı tutma kademe bipleri: 5 sn'de "bırakırsan restart" (LONG),
// 10 sn'de "bırakırsan factory reset" (ERR). Dönme olursa sd_gesture
// hold'u -1 döndürür → uyarılar sıfırlanır (eski encoder.c:199-211'in
// event tabanlı karşılığı; log yerine ses).
static void poll_hold_escalation(int64_t now)
{
    int64_t hold = sd_gesture_hold_ms(&s_gesture, now);
    if (hold < 0) {
        s_warn_restart = s_warn_factory = false;
        return;
    }
    if (hold >= 10000 && !s_warn_factory) {
        s_warn_factory = true;
        sd_buzzer_play(SD_BEEP_ERR);
    } else if (hold >= 5000 && !s_warn_restart) {
        s_warn_restart = true;
        sd_buzzer_play(SD_BEEP_LONG);
    }
}

static void input_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "sd_input task basladi");
    while (1) {
        int64_t now = now_ms();

        refresh_effective_gestures(now);

        while (buf_read != buf_write) {
            char evt = event_buffer[buf_read];
            buf_read = (buf_read + 1) % EVENT_BUFFER_SIZE;
            sd_gesture_on_rotate(&s_gesture, (evt == 'R') ? +1 : -1, now);
        }

        poll_button(now);
        sd_gesture_poll(&s_gesture, now);
        poll_hold_escalation(now);
        vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));
    }
}

// --- Init ---------------------------------------------------------------------

esp_err_t sd_encoder_init(void)
{
    gpio_config_t rot_conf = {
        .pin_bit_mask = (1ULL << SD_PIN_ENC_CLK) | (1ULL << SD_PIN_ENC_DT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    esp_err_t ret = gpio_config(&rot_conf);
    if (ret != ESP_OK) return ret;

    gpio_config_t sw_conf = {
        .pin_bit_mask = (1ULL << SD_PIN_ENC_SW),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,      // buton poll'lanır
    };
    ret = gpio_config(&sw_conf);
    if (ret != ESP_OK) return ret;

    vTaskDelay(pdMS_TO_TICKS(10));              // pull-up stabilizasyonu

    enc_state    = (gpio_get_level(SD_PIN_ENC_CLK) << 1) |
                    gpio_get_level(SD_PIN_ENC_DT);
    enc_position = 0;
    buf_write = buf_read = 0;

    s_last_btn_level = gpio_get_level(SD_PIN_ENC_SW);
    s_last_btn_time  = 0;

    ret = gpio_install_isr_service(0);
    if (ret == ESP_ERR_INVALID_STATE) ret = ESP_OK;   // zaten kurulu
    if (ret != ESP_OK) return ret;

    ret = gpio_isr_handler_add(SD_PIN_ENC_CLK, encoder_isr, NULL);
    if (ret != ESP_OK) return ret;
    ret = gpio_isr_handler_add(SD_PIN_ENC_DT, encoder_isr, NULL);
    if (ret != ESP_OK) return ret;

    s_gestures_eff = sd_prefs_get_bool("gestures", true);   // motor henüz yok
    sd_gesture_init(&s_gesture, gesture_emit, NULL, s_gestures_eff);
    s_warn_restart = s_warn_factory = false;

    BaseType_t r = xTaskCreate(input_task, "sd_input", 3072, NULL, 5, NULL);
    if (r != pdPASS) return ESP_FAIL;

    sk_capabilities_register_book("sd_encoder", "1.0.0");

    ESP_LOGI(TAG, "ready (CLK=%d DT=%d SW=%d, %d adim/detent)",
             SD_PIN_ENC_CLK, SD_PIN_ENC_DT, SD_PIN_ENC_SW, STEPS_PER_DETENT);
    return ESP_OK;
}
