#include "sd_util.h"
#include <string.h>
#include <stdio.h>
#include "cJSON.h"

int sd_json_get_str(const char *json, const char *key, char *out, size_t out_size)
{
    if (!json || !key || !out || out_size == 0) return -1;
    cJSON *root = cJSON_Parse(json);
    if (!root) return -1;
    int len = -1;
    cJSON *v = cJSON_GetObjectItem(root, key);
    if (v && cJSON_IsString(v) && v->valuestring) {
        len = (int)strlen(v->valuestring);
        if ((size_t)len >= out_size) len = (int)out_size - 1;
        memcpy(out, v->valuestring, (size_t)len);
        out[len] = '\0';
    }
    cJSON_Delete(root);
    return len;
}

/* out+oi konumuna src'yi kalan alana sigacak kadar kopyala; yazilan uzunlugu don */
static size_t put_str(char *out, size_t out_size, size_t oi, const char *src)
{
    if (!src) return 0;
    size_t avail = (oi < out_size) ? out_size - 1 - oi : 0;
    size_t n = strlen(src);
    if (n > avail) n = avail;
    memcpy(out + oi, src, n);
    return n;
}

void sd_template_expand(const char *tmpl, char *out, size_t out_size, const sd_tmpl_vars_t *v)
{
    if (!out || out_size == 0) return;
    out[0] = '\0';
    if (!tmpl) return;

    static const sd_tmpl_vars_t empty = {0};
    if (!v) v = &empty;

    char numbuf[16];
    snprintf(numbuf, sizeof(numbuf), "%d", v->value);

    const struct { const char *key; size_t klen; const char *val; } map[] = {
        { "{value}",     7,  numbuf },
        { "{toggle}",    8,  v->toggle_str },
        { "{auth_key}",  10, v->auth_key },
        { "{device_id}", 11, v->device_id },
        { "{prefix}",    8,  v->prefix },
        { "{state}",     7,  v->state },
    };

    size_t oi = 0, ti = 0;
    while (tmpl[ti] && oi < out_size - 1) {
        bool matched = false;
        if (tmpl[ti] == '{') {
            for (size_t k = 0; k < sizeof(map) / sizeof(map[0]); k++) {
                if (strncmp(tmpl + ti, map[k].key, map[k].klen) == 0) {
                    oi += put_str(out, out_size, oi, map[k].val);
                    ti += map[k].klen;
                    matched = true;
                    break;
                }
            }
        }
        if (!matched) out[oi++] = tmpl[ti++];
    }
    out[oi] = '\0';
}
