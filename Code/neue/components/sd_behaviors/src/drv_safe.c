// drv_safe — dizi kilidi sürücüsü: enkoder hareket dizisi eşleşince sk_api
// USER endpoint'i tetiklenir (eski safe_lock.c'nin davranış-sürücüsü hali;
// inline URL konfigürasyonu YOK — endpoint referansı, plan §4.3).
//
// Akış: girdi → sd_seq_input birikimi + 2 sn hareketsizlik zamanlayıcısı
// (sd_ctx_arm_timeout) → on_timeout: finalize → sd_safe_store_match →
//   eşleşme: sd_ctx_api_send + safe.triggered{ok:true} (bip: sd_feedback)
//   yanlış : sayaç++ → politika eşiğinde lockout + safe.lockout olayı
// Lockout SLOT genelidir (bkz. sd_safe_store.h notu); süre boyunca girdi
// yok sayılır. Sayaç RAM'de — reboot sıfırlar (plan: kabul edilmiş).
//
// Öneri: safe bağlamasında params.gestures_enabled=false kullanın — buton
// basışları anında segment kapatır (çift-tık bekleme gecikmesi olmaz).

#include <stdio.h>
#include <stdlib.h>

#include "esp_log.h"

#include "sd_util.h"
#include "sd_behavior.h"
#include "sd_behaviors.h"
#include "sd_sequence.h"
#include "sd_safe_store.h"

static const char *TAG = "drv_safe";

#define SAFE_IDLE_COMMIT_MS  2000   // eski safe_lock.c:39 ile aynı

// Taşan giriş (7+ segment / 51+ tik) BAŞARISIZ deneme sayılır ve lockout
// sayacını besler. 0 yapılırsa eski davranışa döner: taşma sessizce düşer —
// ama o zaman saldırgan hep-uzun denemelerle lockout'u hiç tetiklemeden
// sınırsız yoklama yapabilir (ürün kararı 2026-07-03: sayılır).
#define SAFE_OVERFLOW_COUNTS_AS_FAIL 1

typedef struct {
    sd_seq_input_t in;
    int            fail_count;
    int64_t        lockout_until_ms;   // 0 = kilit yok
    int64_t        last_input_ms;
    bool           lockout_beeped;
} safe_priv_t;

static esp_err_t safe_init(sd_behavior_ctx_t *ctx)
{
    safe_priv_t *p = calloc(1, sizeof(*p));
    if (!p) return ESP_ERR_NO_MEM;
    sd_seq_input_init(&p->in);
    sd_ctx_set_priv(ctx, p);
    ESP_LOGI(TAG, "slot %d hazir", sd_ctx_slot(ctx));
    return ESP_OK;
}

static void safe_deinit(sd_behavior_ctx_t *ctx)
{
    sd_ctx_arm_timeout(ctx, 0);
    free(sd_ctx_priv(ctx));
    sd_ctx_set_priv(ctx, NULL);
}

static esp_err_t safe_on_input(sd_behavior_ctx_t *ctx, const sd_input_event_t *evt)
{
    safe_priv_t *p = sd_ctx_priv(ctx);
    if (!p) return ESP_ERR_INVALID_STATE;
    p->last_input_ms = evt->ts_ms;

    if (p->lockout_until_ms && evt->ts_ms < p->lockout_until_ms) {
        if (!p->lockout_beeped) {
            p->lockout_beeped = true;
            sd_ctx_beep(ctx, SD_BEEP_ERR);   // ilk denemede tek uyarı
        }
        return ESP_OK;                        // kilitli — girdi yok sayılır
    }
    p->lockout_until_ms = 0;
    p->lockout_beeped   = false;

    switch (evt->type) {
        case SD_INPUT_ROTATE:
            sd_seq_input_rotate(&p->in, evt->delta > 0 ? 1 : -1);
            break;
        case SD_INPUT_CLICK:
        case SD_INPUT_DOUBLE_CLICK:   // jest katmanı açık kalmışsa da
        case SD_INPUT_LONG_PRESS:     // her basış = segment sınırı
            sd_seq_input_button(&p->in);
            break;
        default:
            return ESP_OK;
    }
    sd_ctx_arm_timeout(ctx, SAFE_IDLE_COMMIT_MS);
    return ESP_OK;
}

static esp_err_t safe_on_timeout(sd_behavior_ctx_t *ctx)
{
    safe_priv_t *p = sd_ctx_priv(ctx);
    if (!p) return ESP_ERR_INVALID_STATE;

    // finalize giriş durumunu sıfırlar — taşma durumunu ÖNCE yakala
    // (bekleyen 7. segment dahil; bkz. sd_seq_input_overflowed).
    bool overflowed = sd_seq_input_overflowed(&p->in);

    sd_seq_t attempt;
    bool have_attempt = sd_seq_input_finalize(&p->in, &attempt);
    if (!have_attempt && !(SAFE_OVERFLOW_COUNTS_AS_FAIL && overflowed)) {
        return ESP_OK;   // hiç giriş yoktu — deneme sayılmaz
    }

    char endpoint[SD_SAFE_ENDPOINT_MAX + 1];
    int n = have_attempt
        ? sd_safe_store_match(&attempt, endpoint, sizeof(endpoint))
        : 0;   // taşan deneme: eşleşme denenmez, doğrudan başarısız yolu
    char evt[64];

    if (n > 0) {
        p->fail_count = 0;
        char payload[96];
        snprintf(payload, sizeof(payload),
                 "{\"event\":\"safe.triggered\",\"n\":%d}", n);
        esp_err_t err = sd_ctx_api_send(ctx, endpoint, payload);   // async
        snprintf(evt, sizeof(evt), "{\"n\":%d,\"ok\":%s}",
                 n, err == ESP_OK ? "true" : "false");
        sd_ctx_publish(ctx, "safe.triggered", evt);
        ESP_LOGI(TAG, "eslesme e%d -> %s (%s)", n, endpoint,
                 esp_err_to_name(err));
        return ESP_OK;
    }

    // Yanlış dizi ya da taşan (6+ segment) deneme.
    p->fail_count++;
    snprintf(evt, sizeof(evt), "{\"n\":0,\"ok\":false}");
    sd_ctx_publish(ctx, "safe.triggered", evt);

    int after, seconds;
    if (sd_safe_store_lockout_policy(&after, &seconds) &&
        p->fail_count >= after) {
        p->lockout_until_ms = p->last_input_ms + (int64_t)seconds * 1000;
        p->fail_count       = 0;
        p->lockout_beeped   = false;
        snprintf(evt, sizeof(evt), "{\"n\":0,\"seconds\":%d}", seconds);
        sd_ctx_publish(ctx, "safe.lockout", evt);
        ESP_LOGW(TAG, "lockout %d sn", seconds);
    }
    return ESP_OK;
}

static esp_err_t safe_get_state(sd_behavior_ctx_t *ctx, char *json, size_t cap)
{
    safe_priv_t *p = sd_ctx_priv(ctx);
    snprintf(json, cap, "{\"entering\":%s,\"locked\":%s}",
             (p && p->in.active) ? "true" : "false",
             (p && p->lockout_until_ms) ? "true" : "false");
    return ESP_OK;
}

static esp_err_t safe_test(sd_behavior_ctx_t *ctx)
{
    // Test ateşi: ilk etkin kaydın endpoint'ine test yükü (async).
    char endpoint[SD_SAFE_ENDPOINT_MAX + 1];
    if (!sd_safe_store_first_endpoint(endpoint, sizeof(endpoint))) {
        return ESP_ERR_NOT_FOUND;
    }
    return sd_ctx_api_send(ctx, endpoint,
                           "{\"event\":\"safe.test\",\"n\":0}");
}

static const sd_behavior_t SAFE = {
    .id          = "safe",
    .api_version = SD_BEHAVIOR_API_V1,
    .init        = safe_init,
    .deinit      = safe_deinit,
    .on_input    = safe_on_input,
    .get_state   = safe_get_state,
    .test        = safe_test,
    .on_timeout  = safe_on_timeout,
};

const sd_behavior_t *sd_behavior_safe(void) { return &SAFE; }
