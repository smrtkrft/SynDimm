// sd_util_str — pure yardımcılar (bkz. sd_util_str.h).

#include <ctype.h>

#include "sd_util_str.h"

int sd_jsonu_int(const cJSON *obj, const char *key, int def)
{
    const cJSON *it = obj ? cJSON_GetObjectItemCaseSensitive(obj, key) : NULL;
    return (it && cJSON_IsNumber(it)) ? (int)it->valuedouble : def;
}

bool sd_jsonu_bool(const cJSON *obj, const char *key, bool def)
{
    const cJSON *it = obj ? cJSON_GetObjectItemCaseSensitive(obj, key) : NULL;
    return cJSON_IsBool(it) ? cJSON_IsTrue(it) : def;
}

const char *sd_jsonu_str(const cJSON *obj, const char *key)
{
    const cJSON *it = obj ? cJSON_GetObjectItemCaseSensitive(obj, key) : NULL;
    return (it && cJSON_IsString(it)) ? it->valuestring : NULL;
}

bool sd_stru_json_safe(const char *s)
{
    if (!s) return false;
    for (const unsigned char *p = (const unsigned char *)s; *p; p++) {
        if (*p == '"' || *p == '\\' || *p < 0x20) return false;
    }
    return true;
}

bool sd_stru_ident_ok(const char *s)
{
    if (!s || !s[0]) return false;
    for (const char *p = s; *p; p++) {
        if (!isalnum((unsigned char)*p) && *p != '_' && *p != '-') return false;
    }
    return true;
}
