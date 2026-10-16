#pragma once
/**
 * SynDimm - Ortak yardimcilar
 *  - sd_json_get_str : JSON metninden string alan oku (cJSON; tek dogru ayristirici)
 *  - sd_template_expand : {value} {toggle} {auth_key} {device_id} {prefix} {state}
 *    yer tutucularini guvenli genislet (taşma yok, bilinmeyen yer tutucu aynen kalir)
 */
#include <stddef.h>
#include <stdbool.h>

/**
 * JSON icindeki "key" string alanini out'a kopyalar.
 * Donus: kopyalanan uzunluk, alan yoksa/JSON bozuksa -1.
 */
int sd_json_get_str(const char *json, const char *key, char *out, size_t out_size);

typedef struct {
    int value;                /* {value} */
    const char *toggle_str;   /* {toggle} -> ornegin "true"/"false" veya "ON"/"OFF" */
    const char *auth_key;     /* {auth_key} */
    const char *device_id;    /* {device_id} */
    const char *prefix;       /* {prefix} */
    const char *state;        /* {state} */
} sd_tmpl_vars_t;

/**
 * Sablonu genislet. NULL degerler bos string olarak islenir.
 * out her zaman NUL ile biter; cikti out_size-1'i asamaz.
 */
void sd_template_expand(const char *tmpl, char *out, size_t out_size, const sd_tmpl_vars_t *v);
