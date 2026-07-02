// sd_binding — bkz. sd_binding.h.

#include <stdio.h>
#include <string.h>

#include "sd_binding.h"

static bool fail(char *err, size_t cap, const char *reason)
{
    if (err && cap) snprintf(err, cap, "%s", reason);
    return false;
}

// Alan varsa string tipinde ve uzunluk sınırında olmalı. min_len=0 → boş ok.
static bool check_str(const cJSON *obj, const char *key, size_t min_len,
                      size_t max_len, bool required, char *err, size_t cap)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!it) {
        if (required) {
            char buf[48];
            snprintf(buf, sizeof(buf), "missing_%s", key);
            return fail(err, cap, buf);
        }
        return true;
    }
    if (!cJSON_IsString(it) || !it->valuestring) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%s_not_string", key);
        return fail(err, cap, buf);
    }
    size_t len = strlen(it->valuestring);
    if (len < min_len || len > max_len) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%s_length", key);
        return fail(err, cap, buf);
    }
    return true;
}

static bool check_bool(const cJSON *obj, const char *key, char *err, size_t cap)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (it && !cJSON_IsBool(it)) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%s_not_bool", key);
        return fail(err, cap, buf);
    }
    return true;
}

static bool check_int_range(const cJSON *obj, const char *key, int lo, int hi,
                            char *err, size_t cap)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(obj, key);
    if (!it) return true;
    if (!cJSON_IsNumber(it) || it->valuedouble != (double)(int)it->valuedouble) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%s_not_int", key);
        return fail(err, cap, buf);
    }
    int v = (int)it->valuedouble;
    if (v < lo || v > hi) {
        char buf[48];
        snprintf(buf, sizeof(buf), "%s_range", key);
        return fail(err, cap, buf);
    }
    return true;
}

static bool validate_target(const cJSON *t, char *err, size_t cap)
{
    if (!cJSON_IsObject(t)) return fail(err, cap, "target_not_object");
    if (!check_str(t, "host", 1, 63, true, err, cap)) return false;
    if (!check_int_range(t, "port", 1, 65535, err, cap)) return false;
    if (!check_str(t, "device_id", 0, 31, false, err, cap)) return false;
    if (!check_str(t, "auth_key", 0, 127, false, err, cap)) return false;
    return true;
}

static bool validate_params(const cJSON *p, char *err, size_t cap)
{
    if (!cJSON_IsObject(p)) return fail(err, cap, "params_not_object");
    if (!check_int_range(p, "step", 1, 25, err, cap)) return false;
    if (!check_bool(p, "accel", err, cap)) return false;
    if (!check_bool(p, "gestures_enabled", err, cap)) return false;
    // mqtt_remote parametreleri (T3.3) — tip denetimi şimdiden.
    if (!check_str(p, "topic", 0, 127, false, err, cap)) return false;
    if (!check_str(p, "payload_value", 0, 255, false, err, cap)) return false;
    if (!check_str(p, "payload_gesture", 0, 255, false, err, cap)) return false;

    const cJSON *presets = cJSON_GetObjectItemCaseSensitive(p, "presets");
    if (presets) {
        if (!cJSON_IsObject(presets)) return fail(err, cap, "presets_not_object");
        if (!check_int_range(presets, "double_click", 0, 100, err, cap)) return false;
        if (!check_int_range(presets, "long_press", 0, 100, err, cap)) return false;
    }
    return true;
}

bool sd_binding_validate(const cJSON *root, char *err, size_t err_cap)
{
    if (!cJSON_IsObject(root)) return fail(err, err_cap, "not_object");

    const cJSON *v = cJSON_GetObjectItemCaseSensitive(root, "v");
    if (!cJSON_IsNumber(v) || (int)v->valuedouble != 2) {
        return fail(err, err_cap, "bad_version");
    }

    if (!check_str(root, "behavior", 1, 31, true, err, err_cap)) return false;
    if (!check_bool(root, "enabled", err, err_cap)) return false;
    if (!check_str(root, "name", 0, 47, false, err, err_cap)) return false;
    if (!check_str(root, "profile", 1, 15, false, err, err_cap)) return false;

    const cJSON *targets = cJSON_GetObjectItemCaseSensitive(root, "targets");
    if (targets) {
        if (!cJSON_IsArray(targets)) return fail(err, err_cap, "targets_not_array");
        int n = cJSON_GetArraySize(targets);
        if (n > SD_BINDING_TARGETS_MAX) return fail(err, err_cap, "targets_count");
        for (int i = 0; i < n; i++) {
            if (!validate_target(cJSON_GetArrayItem(targets, i), err, err_cap)) {
                return false;
            }
        }
    }

    const cJSON *params = cJSON_GetObjectItemCaseSensitive(root, "params");
    if (params && !validate_params(params, err, err_cap)) return false;

    return true;
}
