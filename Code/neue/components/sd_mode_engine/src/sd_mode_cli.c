// sd_mode_cli — mode.* komutları. Zarflar sk_cli_ok/err; sayısal argümanlar
// sk_cli_arg_long (SKAPP string sayı gönderir); mode.clear confirm-token
// deseni sk_control.c:120-152'den.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "nvs.h"

#include "sk_auth.h"
#include "sk_cli.h"
#include "sk_errors.h"

#include "sd_binding.h"
#include "sd_profiles.h"
#include "sd_mode_engine_internal.h"

// --- Argüman yardımcıları -----------------------------------------------

static bool arg_slot(sk_cli_ctx_t *ctx, int *out)
{
    long v = 0;
    if (!sk_cli_arg_long(ctx, "slot", &v)) {
        const char *p = sk_cli_arg(ctx, 0);
        if (!p) return false;
        v = strtol(p, NULL, 10);
    }
    if (v < 1 || v > SD_MODE_SLOTS) return false;
    *out = (int)v;
    return true;
}

// JSON: makine modunda {"json":"..."}; insan modunda pozisyonel argümanlar
// start_idx'ten itibaren tek boşlukla birleştirilir (sd_profiles ile aynı
// tezgah-kullanımı yaklaşımı).
static char *arg_json_dup(sk_cli_ctx_t *ctx, int start_idx)
{
    const char *named = sk_cli_arg_named(ctx, "json");
    if (named) return strdup(named);

    int argc = sk_cli_argc(ctx);
    if (argc <= start_idx) return NULL;
    size_t total = 0;
    for (int i = start_idx; i < argc; i++) total += strlen(sk_cli_arg(ctx, i)) + 1;
    char *buf = malloc(total + 1);
    if (!buf) return NULL;
    size_t off = 0;
    for (int i = start_idx; i < argc; i++) {
        off += (size_t)snprintf(buf + off, total + 1 - off, "%s%s",
                                i > start_idx ? " " : "", sk_cli_arg(ctx, i));
    }
    return buf;
}

// --- Komutlar ----------------------------------------------------------------

static sk_err_t cmd_mode_list(sk_cli_ctx_t *ctx)
{
    char *out = malloc(1024);
    if (!out) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }
    size_t off = 0, cap = 1024;

    eng_lock();
    off += snprintf(out + off, cap - off,
                    "{\"recovery\":%s,\"active\":%d,\"slots\":[",
                    s_recovery ? "true" : "false", eng_active_slot());
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        const slot_ctx_t *s = &s_slots[i];
        const cJSON *name = s->binding
            ? cJSON_GetObjectItemCaseSensitive(s->binding, "name") : NULL;
        const cJSON *beh  = s->binding
            ? cJSON_GetObjectItemCaseSensitive(s->binding, "behavior") : NULL;
        off += snprintf(out + off, cap - off,
                        "%s{\"slot\":%d,\"assigned\":%s,\"behavior\":\"%s\","
                        "\"name\":\"%s\",\"enabled\":%s,\"error\":%s,"
                        "\"value\":%d,\"state\":%s}",
                        i ? "," : "", i + 1,
                        s->binding ? "true" : "false",
                        cJSON_IsString(beh)  ? beh->valuestring  : "",
                        cJSON_IsString(name) ? name->valuestring : "",
                        s->enabled ? "true" : "false",
                        s->error   ? "true" : "false",
                        s->value, s->state ? "true" : "false");
    }
    eng_unlock();
    snprintf(out + off, cap - off, "]}");
    sk_cli_ok(ctx, out);
    free(out);
    return SK_OK;
}

static sk_err_t cmd_mode_get(sk_cli_ctx_t *ctx)
{
    int slot;
    if (!arg_slot(ctx, &slot)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"slot\"}");
        return SK_OK;
    }
    eng_lock();
    char *raw = s_slots[slot - 1].binding
        ? cJSON_PrintUnformatted(s_slots[slot - 1].binding) : NULL;
    eng_unlock();
    if (!raw) {
        // RAM'de yok (recovery/boş) — NVS'ten dene.
        nvs_handle_t h;
        char key[8];
        snprintf(key, sizeof(key), "slot%d", slot);
        size_t len = 0;
        if (nvs_open(SD_ENGINE_NVS_NS, NVS_READONLY, &h) == ESP_OK) {
            if (nvs_get_str(h, key, NULL, &len) == ESP_OK && len > 0) {
                raw = malloc(len);
                if (raw && nvs_get_str(h, key, raw, &len) != ESP_OK) {
                    free(raw);
                    raw = NULL;
                }
            }
            nvs_close(h);
        }
    }
    if (!raw) { sk_cli_err(ctx, SK_ERR_NOT_FOUND, NULL); return SK_OK; }
    sk_cli_ok(ctx, raw);
    free(raw);
    return SK_OK;
}

static sk_err_t cmd_mode_set(sk_cli_ctx_t *ctx)
{
    int slot;
    if (!arg_slot(ctx, &slot)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"slot\"}");
        return SK_OK;
    }
    char *raw = arg_json_dup(ctx, 1);
    if (!raw) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"json\"}"); return SK_OK; }
    if (strlen(raw) > SD_BINDING_JSON_MAX) {
        free(raw);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"too_large\"}");
        return SK_OK;
    }

    cJSON *root = cJSON_Parse(raw);
    free(raw);
    char verr[48];
    if (!root || !sd_binding_validate(root, verr, sizeof(verr))) {
        char buf[72];
        snprintf(buf, sizeof(buf), "{\"reason\":\"%s\"}", root ? verr : "parse");
        cJSON_Delete(root);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, buf);
        return SK_OK;
    }

    // Davranış kayıtlı mı? (recovery'de registry boş — o zaman atla,
    // normal boot'ta yükleme zaten yakalar ama erken hata daha iyi UX.)
    const cJSON *beh = cJSON_GetObjectItemCaseSensitive(root, "behavior");
    if (!s_recovery && sd_behavior_count() > 0 &&
        !sd_behavior_find(beh->valuestring)) {
        cJSON_Delete(root);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"behavior_unknown\"}");
        return SK_OK;
    }
    const cJSON *prof = cJSON_GetObjectItemCaseSensitive(root, "profile");
    if (cJSON_IsString(prof) && !sd_profiles_exists(prof->valuestring)) {
        cJSON_Delete(root);
        sk_cli_err(ctx, SK_ERR_NOT_FOUND, "{\"reason\":\"profile_missing\"}");
        return SK_OK;
    }

    char *compact = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!compact) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }

    esp_err_t err = eng_store_and_reload(slot, compact);
    free(compact);
    if (err != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_NVS_WRITE, "{\"reason\":\"store_or_load\"}");
    } else {
        char buf[32];
        snprintf(buf, sizeof(buf), "{\"slot\":%d}", slot);
        sk_cli_ok(ctx, buf);
    }
    return SK_OK;
}

static sk_err_t cmd_mode_select(sk_cli_ctx_t *ctx)
{
    int slot;
    if (!arg_slot(ctx, &slot)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"slot\"}");
        return SK_OK;
    }
    eng_set_active(slot, true);
    char buf[32];
    snprintf(buf, sizeof(buf), "{\"active\":%d}", slot);
    sk_cli_ok(ctx, buf);
    return SK_OK;
}

static sk_err_t cmd_mode_clear(sk_cli_ctx_t *ctx)
{
    // Yıkıcı: confirm-token (desen: sk_control.c:120-152).
    const char *tok = sk_cli_confirm_token(ctx);
    if (!tok) { sk_cli_err(ctx, SK_ERR_CONFIRM_TOKEN_REQUIRED, NULL); return SK_OK; }
    if (sk_auth_confirm_consume(tok) != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_CONFIRM_TOKEN_INVALID, NULL);
        return SK_OK;
    }
    int slot;
    if (!arg_slot(ctx, &slot)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"slot\"}");
        return SK_OK;
    }
    if (eng_clear_slot(slot) != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_NVS_WRITE, NULL);
    } else {
        sk_cli_ok(ctx, NULL);
    }
    return SK_OK;
}

static sk_err_t cmd_mode_value(sk_cli_ctx_t *ctx)
{
    int slot;
    if (!arg_slot(ctx, &slot)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"slot\"}");
        return SK_OK;
    }
    const char *val = sk_cli_arg_named(ctx, "value");
    if (!val) val = sk_cli_arg(ctx, 1);
    if (!val) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"value\"}"); return SK_OK; }

    esp_err_t err;
    if (strcmp(val, "toggle") == 0) {
        err = eng_toggle_direct(slot);
    } else {
        char *end = NULL;
        long v = strtol(val, &end, 10);
        if (end == val || *end != '\0') {
            sk_cli_err(ctx, SK_ERR_INVALID_VALUE, "{\"expected\":\"0-100|toggle\"}");
            return SK_OK;
        }
        err = eng_set_value_direct(slot, (int)v);
    }
    if (err == ESP_ERR_INVALID_STATE) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"slot_not_ready\"}");
    } else if (err != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_INTERNAL, "{\"reason\":\"enqueue\"}");
    } else {
        eng_lock();
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"slot\":%d,\"value\":%d,\"state\":%s}",
                 slot, s_slots[slot - 1].value,
                 s_slots[slot - 1].state ? "true" : "false");
        eng_unlock();
        sk_cli_ok(ctx, buf);
    }
    return SK_OK;
}

static sk_err_t cmd_mode_test(sk_cli_ctx_t *ctx)
{
    int slot;
    if (!arg_slot(ctx, &slot)) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"field\":\"slot\"}");
        return SK_OK;
    }
    int status = 0;
    esp_err_t err = eng_run_test(slot, &status, 5000);
    if (err == ESP_ERR_NOT_SUPPORTED) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"slot_not_ready\"}");
    } else if (err == ESP_ERR_TIMEOUT) {
        sk_cli_err(ctx, SK_ERR_API_TIMEOUT, NULL);
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"ok\":%s,\"err\":\"%s\",\"status\":%d}",
                 err == ESP_OK ? "true" : "false", esp_err_to_name(err), status);
        sk_cli_ok(ctx, buf);
    }
    return SK_OK;
}

// --- Kayıt ---------------------------------------------------------------------

static const sk_cli_command_t CMDS[] = {
    { .name = "mode.list",   .summary = "Show all 3 mode slots",
      .usage = "mode list",   .handler = cmd_mode_list },
    { .name = "mode.get",    .summary = "Show a slot's binding JSON",
      .usage = "mode get <slot>", .handler = cmd_mode_get },
    { .name = "mode.set",    .summary = "Bind a slot (validate + persist + hot-reload)",
      .usage = "mode set <slot> {json}",
      .help_block =
          "Baglama JSON v2: {\"v\":2,\"behavior\":\"dimmer\",\"name\":\"Salon\","
          "\"profile\":\"shelly_dimmer2\",\"targets\":[{\"host\":\"...\"}],"
          "\"params\":{...}}",
      .handler = cmd_mode_set },
    { .name = "mode.select", .summary = "Switch the active slot",
      .usage = "mode select <slot>", .handler = cmd_mode_select },
    { .name = "mode.clear",  .summary = "Unbind a slot",
      .usage = "mode clear <slot>", .critical = true, .handler = cmd_mode_clear },
    { .name = "mode.value",  .summary = "Set value or toggle (SKAPP slider path)",
      .usage = "mode value <slot> <0-100|toggle>", .handler = cmd_mode_value },
    { .name = "mode.test",   .summary = "Test-fire the slot's action once",
      .usage = "mode test <slot>", .handler = cmd_mode_test },
};

void eng_cli_register(void)
{
    for (size_t i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); i++) {
        sk_cli_register(&CMDS[i]);
    }
}
