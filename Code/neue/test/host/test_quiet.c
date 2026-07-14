// sd_quiet birim testleri (host, assert tabanlı).

#include <assert.h>
#include <stdio.h>

#include "sd_quiet.h"

#define MIN(h, m) ((uint16_t)((h) * 60 + (m)))

static void test_parse_valid(void)
{
    uint16_t f, t;
    assert(sd_quiet_parse("23:00-07:00", &f, &t) && f == MIN(23, 0) && t == MIN(7, 0));
    assert(sd_quiet_parse("09:15-17:45", &f, &t) && f == MIN(9, 15) && t == MIN(17, 45));
    assert(sd_quiet_parse("00:00-23:59", &f, &t) && f == 0 && t == MIN(23, 59));
}

static void test_parse_invalid(void)
{
    uint16_t f, t;
    assert(!sd_quiet_parse("", &f, &t));
    assert(!sd_quiet_parse("off", &f, &t));
    assert(!sd_quiet_parse("25:00-07:00", &f, &t));      // saat > 23
    assert(!sd_quiet_parse("23:60-07:00", &f, &t));      // dakika > 59
    assert(!sd_quiet_parse("23:00-07:00x", &f, &t));     // kuyruk çöpü
    assert(!sd_quiet_parse("23:00_07:00", &f, &t));      // yanlış ayraç
    assert(!sd_quiet_parse("3:00-07:00", &f, &t));       // tek haneli saat
    assert(!sd_quiet_parse("23:00-", &f, &t));           // eksik bitiş
    assert(!sd_quiet_parse(NULL, &f, &t));
}

static void test_active_no_wrap(void)
{
    // 09:00-17:00 — gün içi aralık, [from, to)
    uint16_t f = MIN(9, 0), t = MIN(17, 0);
    assert(!sd_quiet_active(f, t, MIN(8, 59)));
    assert( sd_quiet_active(f, t, MIN(9, 0)));           // başlangıç dahil
    assert( sd_quiet_active(f, t, MIN(12, 0)));
    assert( sd_quiet_active(f, t, MIN(16, 59)));
    assert(!sd_quiet_active(f, t, MIN(17, 0)));          // bitiş hariç
    assert(!sd_quiet_active(f, t, MIN(23, 30)));
}

static void test_active_wrap(void)
{
    // 23:00-07:00 — gece sarmalı
    uint16_t f = MIN(23, 0), t = MIN(7, 0);
    assert( sd_quiet_active(f, t, MIN(23, 0)));
    assert( sd_quiet_active(f, t, MIN(23, 30)));
    assert( sd_quiet_active(f, t, MIN(0, 0)));           // gece yarısı
    assert( sd_quiet_active(f, t, MIN(6, 59)));
    assert(!sd_quiet_active(f, t, MIN(7, 0)));           // bitiş hariç
    assert(!sd_quiet_active(f, t, MIN(12, 0)));
    assert(!sd_quiet_active(f, t, MIN(22, 59)));
}

static void test_active_degenerate(void)
{
    // from == to → hiçbir zaman aktif değil
    assert(!sd_quiet_active(MIN(8, 0), MIN(8, 0), MIN(8, 0)));
    assert(!sd_quiet_active(MIN(8, 0), MIN(8, 0), MIN(12, 0)));
}

static void test_tz_parse(void)
{
    int16_t off;
    assert(sd_tz_parse("+00:00", &off) && off == 0);
    assert(sd_tz_parse("+02:00", &off) && off == 120);
    assert(sd_tz_parse("-05:30", &off) && off == -330);
    assert(sd_tz_parse("+14:00", &off) && off == 840);   // gerçek TZ üst sınırı
    assert(!sd_tz_parse("+15:00", &off));                // sınır dışı
    assert(!sd_tz_parse("02:00", &off));                 // işaretsiz
    assert(!sd_tz_parse("+2:00", &off));                 // tek hane
    assert(!sd_tz_parse("+02:00x", &off));               // kuyruk çöpü
    assert(!sd_tz_parse("", &off));
    assert(!sd_tz_parse(NULL, &off));
}

int main(void)
{
    test_parse_valid();
    test_parse_invalid();
    test_active_no_wrap();
    test_active_wrap();
    test_active_degenerate();
    test_tz_parse();
    printf("test_quiet: OK\n");
    return 0;
}
