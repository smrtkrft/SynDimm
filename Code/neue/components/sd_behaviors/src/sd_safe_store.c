// sd_safe_store — bkz. sd_safe_store.h. Desen: sd_prefs (NVS + RAM önbellek
// + mutex) + sd_profiles (JSON doğrulama + CLI).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "sk_api.h"
#include "sk_capabilities.h"
#include "sk_cli.h"
#include "sk_errors.h"
#include "sk_event_bus.h"

#include "sd_util.h"
#include "sd_sequence.h"
#include "sd_safe_store.h"

static const char *TAG = "sd_safe";

#define NVS_NS        "sd_safe"
#define ENTRY_JSON_MAX 384

typedef struct {
    bool     in_use;
    bool     enabled;
    sd_seq_t seq;
    char     endpoint[SD_SAFE_ENDPOINT_MAX + 1];
    bool     lockout_enabled;
    int      lockout_after;
    int      lockout_seconds;
} safe_entry_t;

static safe_entry_t      s_entries[SD_SAFE_ENTRIES];
static SemaphoreHandle_t s_lock;

static void lock(void)   { xSemaphoreTake(s_lock, portMAX_DELAY); }
static void unlock(void) { xSemaphoreGive(s_lock); }

// --- JSON ↔ kayıt ---------------------------------------------------------

static bool entry_from_json(const cJSON *root, safe_entry_t *e,
                            const char **reason)
{
    *reason = "not_object";
    if (!cJSON_IsObject(root)) return false;
    *reason = "bad_version";
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(root, "v");
    if (!cJSON_IsNumber(v) || (int)v->valuedouble != 2) return false;

    *reason = "bad_sequence";
    const char *seq_str = sd_jsonu_str(root, "sequence");
    if (!seq_str || !sd_seq_parse(seq_str, &e->seq)) return false;
    if (e->seq.len < SD_SEQ_MIN_SEGMENTS) {
        *reason = "sequence_too_short";   // kılavuz: en az 3 segment
        return false;
    }

    *reason = "bad_endpoint";
    const char *ep = sd_jsonu_str(root, "endpoint");
    if (!ep || !ep[0] || strlen(ep) > SD_SAFE_ENDPOINT_MAX ||
        !sd_stru_json_safe(ep)) return false;
    strlcpy(e->endpoint, ep, sizeof(e->endpoint));

    e->enabled = sd_jsonu_bool(root, "enabled", true);

    const cJSON *lk = cJSON_GetObjectItemCaseSensitive(root, "lockout");
    e->lockout_enabled = sd_jsonu_bool(lk, "enabled", false);
    e->lockout_after   = sd_jsonu_int(lk, "after", 5);
    e->lockout_seconds = sd_jsonu_int(lk, "seconds", 30);
    *reason = "bad_lockout";
    if (e->lockout_after < 1 || e->lockout_after > 20 ||
        e->lockout_seconds < 5 || e->lockout_seconds > 3600) return false;

    e->in_use = true;
    *reason = NULL;
    return true;
}

static void load_from_nvs(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return;
    for (int i = 0; i < SD_SAFE_ENTRIES; i++) {
        char key[8];
        snprintf(key, sizeof(key), "e%d", i + 1);
        char buf[ENTRY_JSON_MAX];
        size_t len = sizeof(buf);
        if (nvs_get_str(h, key, buf, &len) != ESP_OK) continue;
        cJSON *root = cJSON_Parse(buf);
        const char *reason;
        if (root && entry_from_json(root, &s_entries[i], &reason)) {
            ESP_LOGI(TAG, "kayit e%d yuklendi (endpoint=%s)",
                     i + 1, s_entries[i].endpoint);
        } else {
            ESP_LOGW(TAG, "kayit e%d bozuk, atlandi", i + 1);
        }
        cJSON_Delete(root);
    }
    nvs_close(h);
}

// --- Public API ---------------------------------------------------------------

int sd_safe_store_match(const sd_seq_t *seq,
                        char *endpoint_out, size_t endpoint_cap)
{
    int hit = 0;
    lock();
    for (int i = 0; i < SD_SAFE_ENTRIES; i++) {
        if (s_entries[i].in_use && s_entries[i].enabled &&
            sd_seq_equal(seq, &s_entries[i].seq)) {
            hit = i + 1;
            if (endpoint_out) {
                strlcpy(endpoint_out, s_entries[i].endpoint, endpoint_cap);
            }
            break;
        }
    }
    unlock();
    return hit;
}

bool sd_safe_store_lockout_policy(int *out_after, int *out_seconds)
{
    bool found = false;
    lock();
    for (int i = 0; i < SD_SAFE_ENTRIES; i++) {
        if (s_entries[i].in_use && s_entries[i].enabled &&
            s_entries[i].lockout_enabled) {
            if (out_after)   *out_after   = s_entries[i].lockout_after;
            if (out_seconds) *out_seconds = s_entries[i].lockout_seconds;
            found = true;
            break;
        }
    }
    unlock();
    return found;
}

bool sd_safe_store_first_endpoint(char *endpoint_out, size_t endpoint_cap)
{
    bool found = false;
    lock();
    for (int i = 0; i < SD_SAFE_ENTRIES; i++) {
        if (s_entries[i].in_use && s_entries[i].enabled) {
            strlcpy(endpoint_out, s_entries[i].endpoint, endpoint_cap);
            found = true;
            break;
        }
    }
    unlock();
    return found;
}

// --- CLI (requires_auth — diziler sır) -----------------------------------------

static bool arg_entry_no(sk_cli_ctx_t *ctx, int *out)
{
    long v = 0;
    if (!sk_cli_arg_long(ctx, "n", &v)) {
        const char *p = sk_cli_arg(ctx, 0);
        if (!p) return false;
        v = strtol(p, NULL, 10);
    }
    if (v < 1 || v > SD_SAFE_ENTRIES) return false;
    *out = (int)v;
    return true;
}

static sk_err_t cmd_safe_list(sk_cli_ctx_t *ctx)
{
    char out[320];
    size_t off = 0;
    off += snprintf(out + off, sizeof(out) - off, "[");
    lock();
    bool first = true;
    for (int i = 0; i < SD_SAFE_ENTRIES; i++) {
        if (!s_entries[i].in_use) continue;
        off += snprintf(out + off, sizeof(out) - off,
                        "%s{\"n\":%d,\"enabled\":%s,\"endpoint\":\"%s\","
                        "\"segments\":%d,\"lockout\":%s}",
                        first ? "" : ",", i + 1,
                        s_entries[i].enabled ? "true" : "false",
                        s_entries[i].endpoint,
                        s_entries[i].seq.len,
                        s_entries[i].lockout_enabled ? "true" : "false");
        first = false;
    }
    unlock();
    snprintf(out + off, sizeof(out) - off, "]");
    sk_cli_ok(ctx, out);   // sequence GÖSTERİLMEZ (yalnız segment sayısı)
    return SK_OK;
}

static sk_err_t cmd_safe_set(sk_cli_ctx_t *ctx)
{
    int n;
    if (!arg_entry_no(ctx, &n)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"n\",\"range\":\"1-5\"}");
        return SK_OK;
    }
    char *raw = sd_cliu_json_arg_dup(ctx, 1);
    if (!raw) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"json\"}"); return SK_OK; }
    if (strlen(raw) > ENTRY_JSON_MAX - 1) {
        free(raw);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"too_large\"}");
        return SK_OK;
    }

    cJSON *root = cJSON_Parse(raw);
    safe_entry_t e = {0};
    const char *reason = "parse";
    bool ok = root && entry_from_json(root, &e, &reason);
    if (ok) {
        // endpoint gerçekten tanımlı bir sk_api kaydı mı?
        sk_api_endpoint_t eps[SK_API_MAX_ENDPOINTS];
        int cnt = sk_api_endpoint_list(eps, SK_API_MAX_ENDPOINTS);
        bool found = false;
        for (int i = 0; i < cnt; i++) {
            if (eps[i].in_use && strcmp(eps[i].name, e.endpoint) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            ok = false;
            reason = "endpoint_unknown";
        }
    }
    if (!ok) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"reason\":\"%s\"}", reason);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, buf);
        free(raw);
        cJSON_Delete(root);
        return SK_OK;
    }

    // Normalize edilmiş halde sakla (compact).
    char *compact = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    free(raw);
    if (!compact) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }

    nvs_handle_t h;
    char key[8];
    snprintf(key, sizeof(key), "e%d", n);
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_set_str(h, key, compact);
        if (err == ESP_OK) err = nvs_commit(h);
        nvs_close(h);
    }
    free(compact);
    if (err != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_NVS_WRITE, NULL);
        return SK_OK;
    }

    lock();
    s_entries[n - 1] = e;
    unlock();
    char buf[24];
    snprintf(buf, sizeof(buf), "{\"n\":%d}", n);
    sk_cli_ok(ctx, buf);
    return SK_OK;
}

static sk_err_t cmd_safe_clear(sk_cli_ctx_t *ctx)
{
    int n;
    if (!arg_entry_no(ctx, &n)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"n\",\"range\":\"1-5\"}");
        return SK_OK;
    }
    nvs_handle_t h;
    char key[8];
    snprintf(key, sizeof(key), "e%d", n);
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_key(h, key);
        nvs_commit(h);
        nvs_close(h);
    }
    lock();
    memset(&s_entries[n - 1], 0, sizeof(s_entries[0]));
    unlock();
    sk_cli_ok(ctx, NULL);
    return SK_OK;
}

static const sk_cli_command_t CMDS[] = {
    { .name = "safe.list",  .summary = "List safe entries (sequences hidden)",
      .usage = "safe list",  .requires_auth = true, .handler = cmd_safe_list },
    { .name = "safe.set",   .summary = "Set a safe entry (sequence → webhook)",
      .usage = "safe set <1-5> {json}",
      .help_block =
          "{\"v\":2,\"enabled\":true,\"sequence\":\"L3-R5-L2\","
          "\"endpoint\":\"<api.endpoint.add ile tanimli ad>\","
          "\"lockout\":{\"enabled\":true,\"after\":5,\"seconds\":30}}",
      .requires_auth = true, .handler = cmd_safe_set },
    { .name = "safe.clear", .summary = "Delete a safe entry",
      .usage = "safe clear <1-5>", .requires_auth = true,
      .handler = cmd_safe_clear },
};

// --- Factory reset + init --------------------------------------------------------

static void on_factory_reset(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    memset(s_entries, 0, sizeof(s_entries));
    ESP_LOGW(TAG, "factory reset: safe kayitlari silindi");
}

esp_err_t sd_safe_store_init(void)
{
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return ESP_ERR_NO_MEM;

    load_from_nvs();

    for (size_t i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); i++) {
        sk_cli_register(&CMDS[i]);
    }
    int sub;
    sk_event_bus_subscribe("device.factory-reset.requested",
                           on_factory_reset, NULL, &sub);
    sk_capabilities_register_book("sd_safe", "1.0.0");
    ESP_LOGI(TAG, "ready");
    return ESP_OK;
}
