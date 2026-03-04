/**
 * SynDimm - EC11 Rotary Encoder (ESP-IDF)
 * Quadrature decoder + button handler
 * CLK=GPIO19, DT=GPIO20, SW=GPIO18
 */

#include "encoder.h"
#include <string.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "ENCODER";

/* ========== Pin Definitions ========== */
#define ENCODER_CLK     GPIO_NUM_19
#define ENCODER_DT      GPIO_NUM_20
#define ENCODER_SW      GPIO_NUM_18

/* ========== Timing Constants (ms) ========== */
#define BUTTON_DEBOUNCE_MS      50
#define MODE_CHANGE_DELAY_MS    1000
#define REBOOT_PRESS_MS         5000
#define FACTORY_RESET_MS        10000

/* ========== Encoder Constants ========== */
#define STEPS_PER_DETENT        4       // EC11: 4 step = 1 detent
#define EVENT_BUFFER_SIZE       10

/* ========== Quadrature Lookup Table ========== */
// Index = (lastState << 2) | newState
// +1 = sag, -1 = sol, 0 = gecersiz/gurultu
static const int8_t ENCODER_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
};

/* ========== ISR Shared State (volatile) ========== */
static volatile uint8_t  enc_state;
static volatile int8_t   enc_position;
static volatile char     event_buffer[EVENT_BUFFER_SIZE];
static volatile uint8_t  buf_write;
static volatile uint8_t  buf_read;
static volatile char     last_dir;

/* ========== Button State (polled) ========== */
static bool     last_btn_state;
static int64_t  last_btn_time;
static int64_t  btn_press_time;
static bool     btn_was_pressed;
static bool     btn_is_held;
static bool     mode_change_flag;
static bool     reboot_warned;
static bool     factory_warned;

/* ========== Helpers ========== */
static inline int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void IRAM_ATTR add_event(char evt)
{
    uint8_t next = (buf_write + 1) % EVENT_BUFFER_SIZE;
    if (next != buf_read) {
        event_buffer[buf_write] = evt;
        buf_write = next;
    }
}

/* ========== GPIO ISR Handler ========== */
static void IRAM_ATTR encoder_isr(void *arg)
{
    uint8_t clk = gpio_get_level(ENCODER_CLK);
    uint8_t dt  = gpio_get_level(ENCODER_DT);
    uint8_t new_state = (clk << 1) | dt;

    if (new_state == enc_state) return;

    int8_t dir = ENCODER_TABLE[(enc_state << 2) | new_state];
    enc_state = new_state;
    enc_position += dir;

    if (enc_position >= STEPS_PER_DETENT) {
        add_event('R');
        last_dir = 'R';
        enc_position = 0;
    } else if (enc_position <= -STEPS_PER_DETENT) {
        add_event('L');
        last_dir = 'L';
        enc_position = 0;
    }
}

/* ========== Factory Reset ========== */
static void perform_factory_reset(void)
{
    ESP_LOGW(TAG, "==========================================");
    ESP_LOGW(TAG, "     FACTORY RESET BASLATILDI");
    ESP_LOGW(TAG, "==========================================");

    esp_err_t ret = nvs_flash_erase();
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "NVS tamamen silindi");
    } else {
        ESP_LOGE(TAG, "NVS silme hatasi: %s", esp_err_to_name(ret));
    }

    ESP_LOGW(TAG, "Cihaz yeniden baslatiliyor...");
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

/* ========== Public API ========== */

esp_err_t encoder_init(void)
{
    /* CLK ve DT pinleri - input, pull-up, ANYEDGE interrupt */
    gpio_config_t rot_conf = {
        .pin_bit_mask = (1ULL << ENCODER_CLK) | (1ULL << ENCODER_DT),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    esp_err_t ret = gpio_config(&rot_conf);
    if (ret != ESP_OK) return ret;

    /* SW pin - input, pull-up, no interrupt (polled) */
    gpio_config_t sw_conf = {
        .pin_bit_mask = (1ULL << ENCODER_SW),
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ret = gpio_config(&sw_conf);
    if (ret != ESP_OK) return ret;

    /* Pull-up stabilizasyonu */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Baslangic state'i oku */
    enc_state    = (gpio_get_level(ENCODER_CLK) << 1) | gpio_get_level(ENCODER_DT);
    enc_position = 0;
    buf_write    = 0;
    buf_read     = 0;
    last_dir     = 0;

    last_btn_state    = gpio_get_level(ENCODER_SW);
    last_btn_time     = 0;
    btn_press_time    = 0;
    btn_was_pressed   = false;
    btn_is_held       = false;
    mode_change_flag  = false;
    reboot_warned     = false;
    factory_warned    = false;

    /* GPIO ISR servisi */
    ret = gpio_install_isr_service(0);
    if (ret == ESP_ERR_INVALID_STATE) {
        ret = ESP_OK;   /* zaten kurulu */
    }
    if (ret != ESP_OK) return ret;

    ret = gpio_isr_handler_add(ENCODER_CLK, encoder_isr, NULL);
    if (ret != ESP_OK) return ret;
    ret = gpio_isr_handler_add(ENCODER_DT, encoder_isr, NULL);
    if (ret != ESP_OK) return ret;

    ESP_LOGI(TAG, "Encoder baslatildi (CLK=%d, DT=%d, SW=%d)",
             ENCODER_CLK, ENCODER_DT, ENCODER_SW);
    return ESP_OK;
}

bool encoder_available(void)
{
    /* Rotation event buffer'da varsa hemen don */
    if (buf_read != buf_write) {
        return true;
    }

    /* ---- Button polling ---- */
    int cur_btn = gpio_get_level(ENCODER_SW);
    int64_t now = now_ms();

    btn_is_held = (cur_btn == 0 && btn_was_pressed);

    /* Basili tutarken sure takibi */
    if (cur_btn == 0 && btn_was_pressed) {
        int64_t hold = now - btn_press_time;

        if (mode_change_flag) {
            /* Mod degisikligi yapildiysa reboot/reset iptal */
            reboot_warned  = false;
            factory_warned = false;
        }
        else if (hold >= FACTORY_RESET_MS && !factory_warned) {
            factory_warned = true;
            ESP_LOGW(TAG, "!!! FACTORY RESET HAZIR - Butonu birakin !!!");
        }
        else if (hold >= REBOOT_PRESS_MS && !reboot_warned) {
            reboot_warned = true;
            ESP_LOGW(TAG, "5sn - REBOOT icin birakin, 10sn = FACTORY RESET");
        }
    }

    /* Buton state degisimi (debounce) */
    if (cur_btn != last_btn_state) {
        if ((now - last_btn_time) > BUTTON_DEBOUNCE_MS) {

            if (cur_btn == 0) {
                /* Buton basildi */
                btn_press_time    = now;
                btn_was_pressed   = true;
                btn_is_held       = true;
                mode_change_flag  = false;
                reboot_warned     = false;
                factory_warned    = false;
                last_btn_time     = now;

            } else if (cur_btn == 1 && btn_was_pressed) {
                /* Buton birakildi */
                int64_t dur = now - btn_press_time;

                if (mode_change_flag) {
                    add_event('M');     /* Mod onaylandi */
                }
                else if (dur >= FACTORY_RESET_MS) {
                    perform_factory_reset();
                    /* Buraya donmez */
                }
                else if (dur >= REBOOT_PRESS_MS) {
                    ESP_LOGW(TAG, "REBOOT baslatiliyor...");
                    vTaskDelay(pdMS_TO_TICKS(100));
                    esp_restart();
                }
                else {
                    add_event('B');     /* Kisa basi */
                }

                btn_was_pressed   = false;
                btn_is_held       = false;
                mode_change_flag  = false;
                reboot_warned     = false;
                factory_warned    = false;
                last_btn_time     = now;
            }
        }
    }

    last_btn_state = cur_btn;
    return (buf_read != buf_write);
}

char encoder_read(void)
{
    if (buf_read != buf_write) {
        char evt = event_buffer[buf_read];
        buf_read = (buf_read + 1) % EVENT_BUFFER_SIZE;
        return evt;
    }
    return 0;
}

void encoder_clear(void)
{
    buf_read = buf_write;
}

bool encoder_is_button_held(void)
{
    return btn_is_held;
}

bool encoder_is_held_long_enough(void)
{
    if (!btn_is_held) return false;
    return (now_ms() - btn_press_time) >= MODE_CHANGE_DELAY_MS;
}

void encoder_set_mode_change_triggered(void)
{
    mode_change_flag = true;
}

char encoder_get_last_direction(void)
{
    return last_dir;
}

int64_t encoder_get_hold_duration_ms(void)
{
    if (!btn_is_held) return 0;
    return now_ms() - btn_press_time;
}
