// sd_encoder — bkz. sd_encoder.h. Katman yapısı:
//   ISR (IRAM)      : quadrature çöz, detent başına 'R'/'L' → SPSC ring
//   sd_input task   : ring'i boşalt + butonu örnekle (50 ms debounce)
//                     → handle_rotate()/handle_button() kancaları
// T1.3: kancalar ham log basar. T1.4 bunları sd_gesture'a bağlayacak.

#include <stdint.h>

#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sk_capabilities.h"

#include "sd_pins.h"
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

// Buton durumu (yalnız sd_input task erişir).
static int     s_last_btn_level;
static int64_t s_last_btn_time;
static int64_t s_press_time;
static bool    s_pressed;

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

// --- Kancalar (T1.4'te sd_gesture'a bağlanacak) ------------------------------

static void handle_rotate(char dir, bool button_held)
{
    ESP_LOGI(TAG, "[raw] rotate %c%s", dir, button_held ? " (buton basili)" : "");
}

static void handle_button(bool pressed, int64_t duration_ms)
{
    if (pressed) {
        ESP_LOGI(TAG, "[raw] buton BASILDI");
    } else {
        ESP_LOGI(TAG, "[raw] buton BIRAKILDI (%lld ms)", (long long)duration_ms);
    }
}

// --- Buton örnekleme (encoder.c:214-258 debounce yapısının portu, kontrol
//     yan-etkileri çıkarılmış hali) -------------------------------------------

static void poll_button(int64_t now)
{
    int cur = gpio_get_level(SD_PIN_ENC_SW);   // aktif-low

    if (cur != s_last_btn_level) {
        if ((now - s_last_btn_time) > BUTTON_DEBOUNCE_MS) {
            if (cur == 0) {
                s_press_time = now;
                s_pressed    = true;
                handle_button(true, 0);
            } else if (s_pressed) {
                s_pressed = false;
                handle_button(false, now - s_press_time);
            }
            s_last_btn_time = now;
        }
        s_last_btn_level = cur;
    }
}

static void input_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "sd_input task basladi");
    while (1) {
        int64_t now = now_ms();

        while (buf_read != buf_write) {
            char evt = event_buffer[buf_read];
            buf_read = (buf_read + 1) % EVENT_BUFFER_SIZE;
            handle_rotate(evt, s_pressed);
        }

        poll_button(now);
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
    s_press_time     = 0;
    s_pressed        = false;

    ret = gpio_install_isr_service(0);
    if (ret == ESP_ERR_INVALID_STATE) ret = ESP_OK;   // zaten kurulu
    if (ret != ESP_OK) return ret;

    ret = gpio_isr_handler_add(SD_PIN_ENC_CLK, encoder_isr, NULL);
    if (ret != ESP_OK) return ret;
    ret = gpio_isr_handler_add(SD_PIN_ENC_DT, encoder_isr, NULL);
    if (ret != ESP_OK) return ret;

    BaseType_t r = xTaskCreate(input_task, "sd_input", 3072, NULL, 5, NULL);
    if (r != pdPASS) return ESP_FAIL;

    sk_capabilities_register_book("sd_encoder", "1.0.0");

    ESP_LOGI(TAG, "ready (CLK=%d DT=%d SW=%d, %d adim/detent)",
             SD_PIN_ENC_CLK, SD_PIN_ENC_DT, SD_PIN_ENC_SW, STEPS_PER_DETENT);
    return ESP_OK;
}
