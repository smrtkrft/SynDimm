/**
 * SynDimm - Buzzer (ESP-IDF)
 * GPIO17, 2kHz PWM, non-blocking dit patterns
 */

#include "buzzer.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "BUZZER";

/* ========== Hardware Config ========== */
#define BUZZER_GPIO         GPIO_NUM_17
#define BUZZER_FREQ_HZ      2000
#define BUZZER_LEDC_TIMER   LEDC_TIMER_0
#define BUZZER_LEDC_CHANNEL LEDC_CHANNEL_0
#define BUZZER_DUTY_ON      128     /* 50% of 8-bit (256) */

/* ========== Timing (ms) ========== */
#define DIT_DURATION_MS     80
#define DIT_PAUSE_MS        100

/* ========== State Machine ========== */
typedef enum {
    BZ_IDLE = 0,
    BZ_BEEPING,
    BZ_PAUSING,
} buzzer_state_t;

static buzzer_state_t bz_state = BZ_IDLE;
static int            remaining_dits = 0;
static int64_t        state_start_us = 0;

/* ========== Helpers ========== */
static inline int64_t now_ms(void)
{
    return esp_timer_get_time() / 1000;
}

static void beep_on(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, BUZZER_DUTY_ON);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

static void beep_off(void)
{
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL, 0);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BUZZER_LEDC_CHANNEL);
}

/* ========== Public API ========== */

esp_err_t buzzer_init(void)
{
    /* LEDC Timer */
    ledc_timer_config_t timer_conf = {
        .speed_mode      = LEDC_LOW_SPEED_MODE,
        .duty_resolution = LEDC_TIMER_8_BIT,
        .timer_num       = BUZZER_LEDC_TIMER,
        .freq_hz         = BUZZER_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,
    };
    esp_err_t ret = ledc_timer_config(&timer_conf);
    if (ret != ESP_OK) return ret;

    /* LEDC Channel */
    ledc_channel_config_t ch_conf = {
        .gpio_num   = BUZZER_GPIO,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel    = BUZZER_LEDC_CHANNEL,
        .timer_sel  = BUZZER_LEDC_TIMER,
        .duty       = 0,
        .hpoint     = 0,
    };
    ret = ledc_channel_config(&ch_conf);
    if (ret != ESP_OK) return ret;

    bz_state       = BZ_IDLE;
    remaining_dits = 0;

    ESP_LOGI(TAG, "Buzzer baslatildi (GPIO%d, %dHz PWM)", BUZZER_GPIO, BUZZER_FREQ_HZ);
    return ESP_OK;
}

void buzzer_update(void)
{
    if (bz_state == BZ_IDLE) return;

    int64_t now     = now_ms();
    int64_t elapsed = now - state_start_us;

    if (bz_state == BZ_BEEPING) {
        if (elapsed >= DIT_DURATION_MS) {
            beep_off();
            remaining_dits--;

            if (remaining_dits > 0) {
                bz_state       = BZ_PAUSING;
                state_start_us = now;
            } else {
                bz_state = BZ_IDLE;
            }
        }
    }
    else if (bz_state == BZ_PAUSING) {
        if (elapsed >= DIT_PAUSE_MS) {
            beep_on();
            bz_state       = BZ_BEEPING;
            state_start_us = now;
        }
    }
}

void buzzer_play_dits(int count)
{
    if (count < 1 || count > 3) return;
    if (bz_state != BZ_IDLE) return;    /* mesgulse yoksay */

    ESP_LOGI(TAG, "%d dit caliniyor", count);

    remaining_dits = count;
    bz_state       = BZ_BEEPING;
    state_start_us = now_ms();
    beep_on();
}

bool buzzer_is_playing(void)
{
    return bz_state != BZ_IDLE;
}

void buzzer_stop(void)
{
    beep_off();
    bz_state       = BZ_IDLE;
    remaining_dits = 0;
}
