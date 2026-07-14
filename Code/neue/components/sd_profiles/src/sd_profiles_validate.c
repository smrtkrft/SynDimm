// sd_profiles_validate — profil v2 JSON doğrulaması. sd_profiles.c'den
// AYRIŞTIRILDI ki saf kalsın (cJSON + sd_util yalnız): host testi
// test/host/test_profiles.c hem kuralları hem repo `profiles/*.json`
// kataloğunun tamamını buradan geçirir (katalog kapısı). NVS/esp bağımlılığı
// buraya SIZDIRILMAZ.

#include <string.h>

#include "sd_util_str.h"   // pure — host testinde de derlenir
#include "sd_profiles.h"

static bool str_field_ok(const cJSON *o, const char *key, size_t min_len,
                         size_t max_len, bool required)
{
    const cJSON *it = cJSON_GetObjectItemCaseSensitive(o, key);
    if (!it) return !required;
    if (!cJSON_IsString(it) || !it->valuestring) return false;
    size_t n = strlen(it->valuestring);
    return n >= min_len && n <= max_len;
}

// Hata nedeni reason'a yazılır (statik string).
bool sd_profiles_validate_json(const cJSON *root, const char **reason)
{
    *reason = "not_object";
    if (!cJSON_IsObject(root)) return false;

    const cJSON *v = cJSON_GetObjectItemCaseSensitive(root, "v");
    *reason = "bad_version";
    if (!cJSON_IsNumber(v) || (int)v->valuedouble != 2) return false;

    *reason = "bad_id";
    if (!str_field_ok(root, "id", 1, SD_PROFILE_ID_MAX, true)) return false;
    // id NVS anahtarı olur + JSON'a kaçışsız gömülür → kimlik alfabesi şart.
    const cJSON *idf = cJSON_GetObjectItemCaseSensitive(root, "id");
    if (!sd_stru_ident_ok(idf->valuestring)) return false;

    *reason = "bad_name";
    if (!str_field_ok(root, "name", 0, 47, false)) return false;
    const cJSON *namef = cJSON_GetObjectItemCaseSensitive(root, "name");
    if (cJSON_IsString(namef) && namef->valuestring[0] &&
        !sd_stru_json_safe(namef->valuestring)) return false;

    *reason = "bad_protocol";
    const cJSON *proto = cJSON_GetObjectItemCaseSensitive(root, "protocol");
    if (!cJSON_IsString(proto) || !proto->valuestring) return false;
    if (strcmp(proto->valuestring, "http") != 0 &&
        strcmp(proto->valuestring, "udp")  != 0 &&
        strcmp(proto->valuestring, "mqtt") != 0) return false;

    const cJSON *port = cJSON_GetObjectItemCaseSensitive(root, "port");
    *reason = "bad_port";
    if (port && (!cJSON_IsNumber(port) ||
                 port->valueint < 1 || port->valueint > 65535)) return false;

    const cJSON *behaviors = cJSON_GetObjectItemCaseSensitive(root, "behaviors");
    if (behaviors) {
        *reason = "bad_behaviors";
        if (!cJSON_IsArray(behaviors) || cJSON_GetArraySize(behaviors) > 4) return false;
        const cJSON *b;
        cJSON_ArrayForEach(b, behaviors) {
            if (!cJSON_IsString(b) || !b->valuestring ||
                strlen(b->valuestring) > 31 ||
                !sd_stru_ident_ok(b->valuestring)) return false;   // list JSON'u bozulmasın
        }
    }

    const cJSON *cmds = cJSON_GetObjectItemCaseSensitive(root, "commands");
    if (cmds) {
        *reason = "bad_commands";
        if (!cJSON_IsObject(cmds)) return false;
        static const char *known[] = { "set_value", "toggle", "stop", "status" };
        for (size_t i = 0; i < sizeof(known) / sizeof(known[0]); i++) {
            const cJSON *c = cJSON_GetObjectItemCaseSensitive(cmds, known[i]);
            if (c && !cJSON_IsObject(c)) return false;
        }
    }

    *reason = NULL;
    return true;
}
