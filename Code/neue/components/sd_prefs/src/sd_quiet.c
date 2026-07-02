// sd_quiet — pure çözümleyici (IDF'siz; bkz. sd_quiet.h).

#include <stddef.h>

#include "sd_quiet.h"

// "HH:MM" → dakika. Katı: tam 5 karakter, sayılar aralıkta. Dönüş: -1 hata.
static int parse_hhmm(const char *s)
{
    if (!s) return -1;
    if (s[0] < '0' || s[0] > '9' || s[1] < '0' || s[1] > '9') return -1;
    if (s[2] != ':') return -1;
    if (s[3] < '0' || s[3] > '9' || s[4] < '0' || s[4] > '9') return -1;
    int h = (s[0] - '0') * 10 + (s[1] - '0');
    int m = (s[3] - '0') * 10 + (s[4] - '0');
    if (h > 23 || m > 59) return -1;
    return h * 60 + m;
}

bool sd_quiet_parse(const char *s, uint16_t *from_min, uint16_t *to_min)
{
    if (!s || !from_min || !to_min) return false;
    int from = parse_hhmm(s);
    if (from < 0 || s[5] != '-') return false;
    int to = parse_hhmm(s + 6);
    if (to < 0 || s[11] != '\0') return false;
    *from_min = (uint16_t)from;
    *to_min   = (uint16_t)to;
    return true;
}

bool sd_quiet_active(uint16_t from_min, uint16_t to_min, uint16_t now_min)
{
    if (from_min == to_min) return false;          // dejenere → kapalı
    if (from_min < to_min) {                       // gün içi: [from, to)
        return now_min >= from_min && now_min < to_min;
    }
    // Gece sarmalı: [from, 24:00) ∪ [00:00, to)
    return now_min >= from_min || now_min < to_min;
}

bool sd_tz_parse(const char *s, int16_t *offset_min)
{
    if (!s || !offset_min) return false;
    int sign;
    if (s[0] == '+') sign = 1;
    else if (s[0] == '-') sign = -1;
    else return false;
    int v = parse_hhmm(s + 1);
    if (v < 0 || s[6] != '\0') return false;
    if (v > 14 * 60) return false;                 // |ofset| ≤ 14:00 (gerçek TZ sınırı)
    *offset_min = (int16_t)(sign * v);
    return true;
}
