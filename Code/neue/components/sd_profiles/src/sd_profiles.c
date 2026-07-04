// sd_profiles — bkz. sd_profiles.h. Depolama deseni: sd_prefs (NVS aç/kapa
// per işlem); doğrulama pragmatik — tip/uzunluk/enum denetimi burada,
// şablon içeriğinin anlamı yürütücülerde (sd_proto).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_log.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "sk_capabilities.h"
#include "sk_cli.h"
#include "sk_errors.h"
#include "sk_event_bus.h"

#include "sd_util.h"
#include "sd_profiles.h"

static const char *TAG = "sd_profiles";

#define NVS_NS "sd_profile"

static sd_profiles_ref_checker_t s_ref_checker;

void sd_profiles_set_ref_checker(sd_profiles_ref_checker_t fn)
{
    s_ref_checker = fn;
}

// --- NVS yardımcıları ---------------------------------------------------

// Ham JSON'u yükler; buf malloc'lanır, çağıran free eder. NULL = yok/hata.
static char *load_raw(const char *id)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) != ESP_OK) return NULL;
    size_t len = 0;
    if (nvs_get_str(h, id, NULL, &len) != ESP_OK || len == 0 ||
        len > SD_PROFILE_JSON_MAX + 1) {
        nvs_close(h);
        return NULL;
    }
    char *buf = malloc(len);
    if (buf && nvs_get_str(h, id, buf, &len) != ESP_OK) {
        free(buf);
        buf = NULL;
    }
    nvs_close(h);
    return buf;
}

cJSON *sd_profiles_load(const char *id)
{
    if (!id || !id[0]) return NULL;
    char *raw = load_raw(id);
    if (!raw) return NULL;
    cJSON *root = cJSON_Parse(raw);
    free(raw);
    return root;
}

bool sd_profiles_exists(const char *id)
{
    char *raw = load_raw(id);
    bool ok = (raw != NULL);
    free(raw);
    return ok;
}

// --- Doğrulama ------------------------------------------------------------
// profile_validate → sd_profiles_validate.c'ye AYRIŞTIRILDI (saf: cJSON +
// sd_util yalnız) — host testi test/host/test_profiles.c kuralları VE repo
// profiles/*.json kataloğunun tamamını oradan geçirir. Public giriş noktası
// sd_profiles_validate_json (sd_profiles.h).

esp_err_t sd_profiles_store_json(const cJSON *root)
{
    const char *id = sd_jsonu_str(root, "id");
    if (!id) return ESP_ERR_INVALID_ARG;
    char *compact = cJSON_PrintUnformatted(root);
    if (!compact) return ESP_ERR_NO_MEM;
    esp_err_t err = ESP_ERR_INVALID_SIZE;
    if (strlen(compact) <= SD_PROFILE_JSON_MAX) {
        nvs_handle_t h;
        err = nvs_open(NVS_NS, NVS_READWRITE, &h);
        if (err == ESP_OK) {
            err = nvs_set_str(h, id, compact);
            if (err == ESP_OK) err = nvs_commit(h);
            nvs_close(h);
        }
    }
    free(compact);
    return err;
}

// --- CLI -------------------------------------------------------------------

static sk_err_t cmd_profile_list(sk_cli_ctx_t *ctx)
{
    char *out = malloc(2048);
    if (!out) { sk_cli_err(ctx, SK_ERR_INTERNAL, NULL); return SK_OK; }
    size_t off = 0, cap = 2048;
    off += snprintf(out + off, cap - off, "[");

    nvs_iterator_t it = NULL;
    esp_err_t res = nvs_entry_find(NVS_DEFAULT_PART_NAME, NVS_NS, NVS_TYPE_STR, &it);
    bool first = true, truncated = false;
    while (res == ESP_OK) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);
        cJSON *p = sd_profiles_load(info.key);
        if (p) {
            const cJSON *name  = cJSON_GetObjectItemCaseSensitive(p, "name");
            const cJSON *proto = cJSON_GetObjectItemCaseSensitive(p, "protocol");
            const cJSON *behs  = cJSON_GetObjectItemCaseSensitive(p, "behaviors");
            char *behs_str = behs ? cJSON_PrintUnformatted(behs) : NULL;
            int n = snprintf(out + off, cap - off,
                             "%s{\"id\":\"%s\",\"name\":\"%s\",\"protocol\":\"%s\","
                             "\"behaviors\":%s}",
                             first ? "" : ",",
                             info.key,
                             cJSON_IsString(name)  ? name->valuestring  : "",
                             cJSON_IsString(proto) ? proto->valuestring : "",
                             behs_str ? behs_str : "[]");
            if (n < 0 || (size_t)n >= cap - off) { truncated = true; }
            else { off += (size_t)n; first = false; }
            free(behs_str);
            cJSON_Delete(p);
        }
        res = nvs_entry_next(&it);
    }
    nvs_release_iterator(it);
    snprintf(out + off, cap - off, "]");

    if (truncated) {
        sk_cli_err(ctx, SK_ERR_INTERNAL, "{\"reason\":\"list_truncated\"}");
    } else {
        sk_cli_ok(ctx, out);
    }
    free(out);
    return SK_OK;
}

static sk_err_t cmd_profile_get(sk_cli_ctx_t *ctx)
{
    const char *id = sk_cli_arg_named(ctx, "id");
    if (!id) id = sk_cli_arg(ctx, 0);
    if (!id) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"id\"}"); return SK_OK; }

    char *raw = load_raw(id);
    if (!raw) { sk_cli_err(ctx, SK_ERR_NOT_FOUND, NULL); return SK_OK; }
    sk_cli_ok(ctx, raw);
    free(raw);
    return SK_OK;
}

static sk_err_t cmd_profile_add(sk_cli_ctx_t *ctx)
{
    char *raw = sd_cliu_json_arg_dup(ctx, 0);
    if (!raw) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"json\"}"); return SK_OK; }

    cJSON *root = cJSON_Parse(raw);
    free(raw);
    if (!root) { sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"parse\"}"); return SK_OK; }

    const char *reason = NULL;
    if (!sd_profiles_validate_json(root, &reason)) {
        char buf[64];
        snprintf(buf, sizeof(buf), "{\"reason\":\"%s\"}", reason);
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, buf);
        cJSON_Delete(root);
        return SK_OK;
    }

    const char *id = cJSON_GetObjectItemCaseSensitive(root, "id")->valuestring;
    esp_err_t err = sd_profiles_store_json(root);

    if (err == ESP_ERR_INVALID_SIZE) {
        sk_cli_err(ctx, SK_ERR_INVALID_ARG, "{\"reason\":\"too_large\"}");
    } else if (err != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_NVS_WRITE, NULL);
    } else {
        char buf[48];
        snprintf(buf, sizeof(buf), "{\"id\":\"%s\"}", id);
        sk_event_bus_publishf("profile.added", "{\"id\":\"%s\"}", id);
        sk_cli_ok(ctx, buf);
    }
    cJSON_Delete(root);
    return SK_OK;
}

static sk_err_t cmd_profile_remove(sk_cli_ctx_t *ctx)
{
    const char *id = sk_cli_arg_named(ctx, "id");
    if (!id) id = sk_cli_arg(ctx, 0);
    if (!id) { sk_cli_err(ctx, SK_ERR_MISSING_ARG, "{\"field\":\"id\"}"); return SK_OK; }

    if (!sd_profiles_exists(id)) { sk_cli_err(ctx, SK_ERR_NOT_FOUND, NULL); return SK_OK; }

    int refs = s_ref_checker ? s_ref_checker(id) : 0;
    if (refs > 0) {
        char buf[56];
        snprintf(buf, sizeof(buf), "{\"reason\":\"in_use\",\"refs\":%d}", refs);
        sk_cli_err(ctx, SK_ERR_BUSY, buf);
        return SK_OK;
    }

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err == ESP_OK) {
        err = nvs_erase_key(h, id);
        if (err == ESP_OK) err = nvs_commit(h);
        nvs_close(h);
    }
    if (err != ESP_OK) {
        sk_cli_err(ctx, SK_ERR_NVS_WRITE, NULL);
    } else {
        sk_event_bus_publishf("profile.removed", "{\"id\":\"%s\"}", id);
        sk_cli_ok(ctx, NULL);
    }
    return SK_OK;
}

static const sk_cli_command_t s_cmd_list = {
    .name    = "profile.list",
    .summary = "List stored device profiles (compact)",
    .usage   = "profile list",
    .handler = cmd_profile_list,
};
static const sk_cli_command_t s_cmd_get = {
    .name    = "profile.get",
    .summary = "Show full profile JSON",
    .usage   = "profile get <id>",
    .handler = cmd_profile_get,
};
static const sk_cli_command_t s_cmd_add = {
    .name    = "profile.add",
    .summary = "Add or replace a device profile (JSON v2)",
    .usage   = "profile add {json}",
    .help_block =
        "Profil JSON v2: {\"v\":2,\"id\":\"...\",\"name\":\"...\","
        "\"protocol\":\"http|udp|mqtt\",\"port\":80,\"behaviors\":[\"dimmer\"],"
        "\"commands\":{...}}\n"
        "id en fazla 15 karakter (NVS anahtar siniri). Ayni id ustune yazar.",
    .handler = cmd_profile_add,
};
static const sk_cli_command_t s_cmd_remove = {
    .name    = "profile.remove",
    .summary = "Delete a profile (refused while referenced)",
    .usage   = "profile remove <id>",
    .handler = cmd_profile_remove,
};

// --- Factory reset + init ----------------------------------------------------

static void on_factory_reset(const sk_event_t *evt, void *user)
{
    (void)evt; (void)user;
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_erase_all(h);
        nvs_commit(h);
        nvs_close(h);
    }
    ESP_LOGW(TAG, "factory reset: profiller silindi");
}

esp_err_t sd_profiles_init(void)
{
    sk_cli_register(&s_cmd_list);
    sk_cli_register(&s_cmd_get);
    sk_cli_register(&s_cmd_add);
    sk_cli_register(&s_cmd_remove);

    int sub;
    sk_event_bus_subscribe("device.factory-reset.requested",
                           on_factory_reset, NULL, &sub);

    sk_capabilities_register_book("sd_profiles", "1.0.0");
    ESP_LOGI(TAG, "ready");
    return ESP_OK;
}
