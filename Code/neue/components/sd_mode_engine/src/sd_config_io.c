// sd_config_io — cihaz konfigürasyonunun tek JSON olarak dışa/içe aktarımı
// (plan T6.2). Kullanım: factory reset sonrası geri yükleme + cihaz klonlama.
//
//   config.export → {"v":1,"active":N,"prefs":{...},"slots":{"1":{...}},
//                    "profiles":{"id":{...}},"safe":"omitted"}
//     sd_safe HARİÇ tutulur (diziler sır — plan §D2).
//   config.import {json} → confirm-token'lı. İKİ GEÇİŞ: önce TÜM bölümler
//     doğrulanır (çöp import hiçbir şey yazmaz), sonra yazılır; ardından
//     cihaz yeniden başlar. "safe" bölümü yalnız kimlikli kanalda uygulanır.
//
// Boyut notu: export ~2-12 KB olabilir — USB/TCP sorunsuz; BLE'de MTU
// parçalama sınırı bilinir (SKAPP_CONTRACT.md), tercihen TCP'den kullanın.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "sk_auth.h"
#include "sk_cli.h"
#include "sk_errors.h"

#include "sd_prefs.h"
#include "sd_profiles.h"
#include "sd_binding.h"
#include "sd_safe_store.h"
#include "sd_util.h"
#include "sd_mode_engine_internal.h"

static const char *TAG = "sd_config";

// --- Export ----------------------------------------------------------------

static cJSON *export_prefs(void)
{
    cJSON *p = cJSON_CreateObject();
    if (!p) return NULL;
    char buf[24];
    cJSON_AddStringToObject(p, "gestures",
                            sd_prefs_get_bool("gestures", true) ? "on" : "off");
    cJSON_AddStringToObject(p, "buzzer",
                            sd_prefs_get_bool("buzzer", true) ? "on" : "off");
    if (sd_prefs_get_str("quiet", buf, sizeof(buf)) == ESP_OK && buf[0]) {
        cJSON_AddStringToObject(p, "quiet", buf);
    } else {
        cJSON_AddStringToObject(p, "quiet", "off");
    }
    if (sd_prefs_get_str("tz", buf, sizeof(buf)) == ESP_OK) {
        cJSON_AddStringToObject(p, "tz", buf);
    }
    return p;
}

static cJSON *export_slots(void)
{
    cJSON *slots = cJSON_CreateObject();
    if (!slots) return NULL;
    eng_lock();
    for (int i = 0; i < SD_MODE_SLOTS; i++) {
        if (!s_slots[i].binding) continue;
        char key[4];
        snprintf(key, sizeof(key), "%d", i + 1);
        cJSON_AddItemToObject(slots, key,
                              cJSON_Duplicate(s_slots[i].binding, true));
    }
    eng_unlock();
    return slots;
}

static cJSON *export_profiles(void)
{
    cJSON *profiles = cJSON_CreateObject();
    if (!profiles) return NULL;
    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(NVS_DEFAULT_PART_NAME, "sd_profile",
                                   NVS_TYPE_STR, &it);
    while (res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        cJSON *p = sd_profiles_load(info.key);
        if (p) cJSON_AddItemToObject(profiles, info.key, p);
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    return profiles;
}

static sk_err_t cmd_config_export(sk_cli_ctx_t *ctx)
{
    cJSON *root = cJSON_CreateObject();
    if (!root) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }
    cJSON_AddNumberToObject(root, "v", 1);
    cJSON_AddNumberToObject(root, "active", eng_active_slot());
    cJSON_AddItemToObject(root, "prefs", export_prefs());
    cJSON_AddItemToObject(root, "slots", export_slots());
    cJSON_AddItemToObject(root, "profiles", export_profiles());
    cJSON_AddStringToObject(root, "safe", "omitted");

    char *out = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!out) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }
    sk_cli_ok(ctx, out);
    free(out);
    return SK_OK;
}

// --- Import ----------------------------------------------------------------

// 1. geçiş: her bölümü doğrula. Hata → reason doldurulur.
static bool import_validate(const cJSON *root, char *reason, size_t cap)
{
#define FAIL(r) do { snprintf(reason, cap, "%s", r); return false; } while (0)
    if (!cJSON_IsObject(root)) FAIL("not_object");
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(root, "v");
    if (!cJSON_IsNumber(v) || (int)v->valuedouble != 1) FAIL("bad_version");

    const cJSON *prefs = cJSON_GetObjectItemCaseSensitive(root, "prefs");
    if (prefs) {
        if (!cJSON_IsObject(prefs)) FAIL("prefs_not_object");
        const cJSON *it;
        cJSON_ArrayForEach(it, prefs) {
            if (!cJSON_IsString(it)) FAIL("prefs_value");
            if (sd_prefs_set(it->string, it->valuestring, true) != ESP_OK) {
                FAIL("prefs_invalid");
            }
        }
    }
    const cJSON *slots = cJSON_GetObjectItemCaseSensitive(root, "slots");
    if (slots) {
        if (!cJSON_IsObject(slots)) FAIL("slots_not_object");
        const cJSON *it;
        cJSON_ArrayForEach(it, slots) {
            int n = atoi(it->string ? it->string : "0");
            if (n < 1 || n > SD_MODE_SLOTS) FAIL("slot_no");
            char verr[48];
            if (!sd_binding_validate(it, verr, sizeof(verr))) FAIL("binding_invalid");
        }
    }
    const cJSON *profiles = cJSON_GetObjectItemCaseSensitive(root, "profiles");
    if (profiles) {
        if (!cJSON_IsObject(profiles)) FAIL("profiles_not_object");
        const cJSON *it;
        cJSON_ArrayForEach(it, profiles) {
            const char *why;
            if (!sd_profiles_validate_json(it, &why)) FAIL("profile_invalid");
        }
    }
    const cJSON *safe = cJSON_GetObjectItemCaseSensitive(root, "safe");
    if (safe && cJSON_IsObject(safe)) {
        const cJSON *it;
        cJSON_ArrayForEach(it, safe) {
            int n = atoi(it->string && it->string[0] == 'e'
                         ? it->string + 1 : "0");
            if (n < 1 || n > SD_SAFE_ENTRIES) FAIL("safe_no");
            const char *why;
            if (!sd_safe_store_validate_json(it, &why)) FAIL("safe_invalid");
        }
    }
    const cJSON *active = cJSON_GetObjectItemCaseSensitive(root, "active");
    if (active && (!cJSON_IsNumber(active) || active->valueint < 1 ||
                   active->valueint > SD_MODE_SLOTS)) FAIL("active_no");
    return true;
#undef FAIL
}

static void restart_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(500));   // cevap zarfı gitsin
    esp_restart();
}

static sk_err_t cmd_config_import(sk_cli_ctx_t *ctx)
{
    // Yıkıcı: confirm-token (sk_control.c:120-152 deseni).
    const char *tok = sk_cli_confirm_token(ctx);
    if (!tok) { sk_cli_err(ctx, SK_ERR_CONFIRM_TOKEN_REQUIRED, NULL); return SK_OK; }
    if (sk_auth_confirm_consume(tok) != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_CONFIRM_TOKEN_INVALID, NULL);
        return SK_OK;
    }

    char *raw = sd_cliu_json_arg_dup(ctx, 0);
    if (!raw) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"json\"}"); return SK_OK; }
    cJSON *root = cJSON_Parse(raw);
    free(raw);
    if (!root) { sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"parse\"}"); return SK_OK; }

    char reason[48];
    if (!import_validate(root, reason, sizeof(reason))) {
        char buf[72];
        snprintf(buf, sizeof(buf), "{\"reason\":\"%s\"}", reason);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, buf);
        cJSON_Delete(root);
        return SK_OK;   // hiçbir şey yazılmadı
    }

    // 2. geçiş: yaz. Sıra: profiller → slotlar → prefs → safe → active.
    const cJSON *profiles = cJSON_GetObjectItemCaseSensitive(root, "profiles");
    const cJSON *it;
    if (profiles) {
        cJSON_ArrayForEach(it, profiles) sd_profiles_store_json(it);
    }
    const cJSON *slots = cJSON_GetObjectItemCaseSensitive(root, "slots");
    if (slots) {
        cJSON_ArrayForEach(it, slots) {
            char *compact = cJSON_PrintUnformatted(it);
            if (compact) {
                eng_store_and_reload(atoi(it->string), compact);
                free(compact);
            }
        }
    }
    const cJSON *prefs = cJSON_GetObjectItemCaseSensitive(root, "prefs");
    if (prefs) {
        cJSON_ArrayForEach(it, prefs) {
            sd_prefs_set(it->string, it->valuestring, false);
        }
    }
    const cJSON *safe = cJSON_GetObjectItemCaseSensitive(root, "safe");
    if (safe && cJSON_IsObject(safe)) {
        if (sk_cli_is_authenticated(ctx)) {
            cJSON_ArrayForEach(it, safe) {
                sd_safe_store_set_json(atoi(it->string + 1), it);
            }
        } else {
            ESP_LOGW(TAG, "safe bolumu atlandi (kimliksiz kanal)");
        }
    }
    const cJSON *active = cJSON_GetObjectItemCaseSensitive(root, "active");
    if (cJSON_IsNumber(active)) eng_set_active(active->valueint, true);
    cJSON_Delete(root);

    sk_cli_ok(ctx, "{\"restarting\":true}");
    xTaskCreate(restart_task, "sd_restart", 2048, NULL, 3, NULL);
    return SK_OK;
}

static const sk_cli_command_t CMDS[] = {
    { .name = "config.export",
      .summary = "Dump bindings+profiles+prefs as one JSON (safe omitted)",
      .usage = "config export", .handler = cmd_config_export },
    { .name = "config.import",
      .summary = "Restore a config.export dump (validates all, then reboots)",
      .usage = "config import {json}", .critical = true,
      .handler = cmd_config_import },
};

void eng_config_register(void)
{
    for (size_t i = 0; i < sizeof(CMDS) / sizeof(CMDS[0]); i++) {
        sk_cli_register(&CMDS[i]);
    }
}
