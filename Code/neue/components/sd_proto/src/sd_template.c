// sd_template — bkz. sd_template.h.

#include <stdio.h>
#include <string.h>

#include "sd_template.h"

// out+off'a src'yi sığdığı kadar kopyalar, yeni off döner (NUL garantili).
static size_t append(char *out, size_t cap, size_t off, const char *src, size_t n)
{
    if (off >= cap - 1) return off;
    size_t room = cap - 1 - off;
    if (n > room) n = room;
    memcpy(out + off, src, n);
    out[off + n] = '\0';
    return off + n;
}

typedef struct { const char *token; const char *value; } tmpl_pair_t;

size_t sd_template_expand(const char *tmpl, char *out, size_t cap,
                          const sd_tmpl_vars_t *vars)
{
    if (!out || cap == 0) return 0;
    out[0] = '\0';
    if (!tmpl || !vars) return 0;

    char value_buf[16];
    snprintf(value_buf, sizeof(value_buf), "%d", vars->value);
    const char *toggle_str = vars->toggle
        ? (vars->toggle_true  ? vars->toggle_true  : "true")
        : (vars->toggle_false ? vars->toggle_false : "false");

    const tmpl_pair_t pairs[] = {
        { "{value}",     value_buf         },
        { "{toggle}",    toggle_str        },
        { "{auth_key}",  vars->auth_key    },   // NULL → token aynen kalır
        { "{device_id}", vars->device_id   },
        { "{prefix}",    vars->prefix      },
        { "{state}",     vars->state_topic },
    };
    const size_t npairs = sizeof(pairs) / sizeof(pairs[0]);

    size_t off = 0;
    const char *p = tmpl;
    while (*p) {
        if (*p == '{') {
            const char *close = strchr(p, '}');
            if (close) {
                size_t tok_len = (size_t)(close - p) + 1;
                const char *repl = NULL;
                bool known = false;
                for (size_t i = 0; i < npairs; i++) {
                    if (strncmp(p, pairs[i].token, tok_len) == 0 &&
                        pairs[i].token[tok_len] == '\0') {
                        known = true;
                        repl  = pairs[i].value;
                        break;
                    }
                }
                if (known) {
                    // NULL değerli bilinen token "" olur (eski davranış —
                    // kod incelemesi: auth'suz hedefte {auth_key} harfiyen
                    // kalınca komutlar bozuluyordu, proto_http.c:103-116).
                    if (repl) off = append(out, cap, off, repl, strlen(repl));
                    p += tok_len;
                    continue;
                }
                // Bilinmeyen token aynen geçer (teşhis edilebilir kalsın).
                off = append(out, cap, off, p, tok_len);
                p += tok_len;
                continue;
            }
        }
        off = append(out, cap, off, p, 1);
        p++;
    }
    return off;
}

int sd_map_value(int v, int from_min, int from_max, int to_min, int to_max)
{
    if (from_min == from_max) return to_min;

    // v'yi kaynak aralığa kırp (ters aralıkları da destekle).
    int lo = from_min < from_max ? from_min : from_max;
    int hi = from_min < from_max ? from_max : from_min;
    if (v < lo) v = lo;
    if (v > hi) v = hi;

    // Yuvarlamalı doğrusal eşleme — 64-bit ara değer, taşma güvenli.
    long long num  = (long long)(v - from_min) * (to_max - to_min);
    long long den  = from_max - from_min;
    long long half = den > 0 ? den / 2 : -den / 2;
    long long r    = (num >= 0) ? (num + half) / den : (num - half) / den;
    return to_min + (int)r;
}
