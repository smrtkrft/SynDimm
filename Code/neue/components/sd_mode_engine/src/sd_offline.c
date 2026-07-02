// sd_offline — bkz. sd_offline.h.

#include <string.h>

#include "sd_offline.h"

void sd_offline_init(sd_offline_t *o, int nslots)
{
    memset(o, 0, sizeof(*o));
    o->n = (nslots > SD_OFFLINE_MAX_SLOTS) ? SD_OFFLINE_MAX_SLOTS : nslots;
}

sd_offline_evt_t sd_offline_feed(sd_offline_t *o, int idx, bool ok, int64_t now_ms)
{
    if (idx < 0 || idx >= o->n) return SD_OFFLINE_NONE;
    sd_offline_slot_t *s = &o->s[idx];

    if (ok) {
        s->fail = 0;
        if (s->offline) {
            s->offline = false;
            return SD_OFFLINE_CAME;
        }
        return SD_OFFLINE_NONE;
    }

    if (s->offline) {
        s->last_probe_ms = now_ms;   // başarısız probe — sayacı yeniden kur
        return SD_OFFLINE_NONE;
    }
    if (++s->fail >= SD_OFFLINE_FAIL_LIMIT) {
        s->offline       = true;
        s->last_probe_ms = now_ms;
        return SD_OFFLINE_WENT;
    }
    return SD_OFFLINE_NONE;
}

bool sd_offline_should_send(sd_offline_t *o, int idx, int64_t now_ms)
{
    if (idx < 0 || idx >= o->n) return false;
    sd_offline_slot_t *s = &o->s[idx];
    if (!s->offline) return true;
    if (now_ms - s->last_probe_ms >= SD_OFFLINE_PROBE_MS) {
        s->last_probe_ms = now_ms;
        return true;
    }
    return false;
}

uint32_t sd_offline_wifi_down(sd_offline_t *o)
{
    uint32_t newly = 0;
    for (int i = 0; i < o->n; i++) {
        if (!o->s[i].offline) {
            newly |= (1u << i);
            o->s[i].offline = true;
        }
        o->s[i].fail = SD_OFFLINE_FAIL_LIMIT;
        // last_probe_ms olduğu gibi kalır — WiFi yokken probe da anlamsız;
        // wifi_up kapıyı açar.
    }
    return newly;
}

void sd_offline_wifi_up(sd_offline_t *o)
{
    for (int i = 0; i < o->n; i++) {
        o->s[i].fail          = 0;
        o->s[i].last_probe_ms = 0;   // probe kapısı hemen açık (uzlaştırma)
    }
}

bool sd_offline_is_offline(const sd_offline_t *o, int idx)
{
    return (idx >= 0 && idx < o->n) ? o->s[idx].offline : false;
}
