// sd_sequence birim testleri (host, assert tabanlı).

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sd_sequence.h"

static void rot(sd_seq_input_t *in, int dir, int n)
{
    for (int i = 0; i < n; i++) sd_seq_input_rotate(in, dir);
}

static void test_parse_valid(void)
{
    sd_seq_t s;
    assert(sd_seq_parse("L3-R5-L2", &s) && s.len == 3);
    assert(s.seg[0] == -3 && s.seg[1] == 5 && s.seg[2] == -2);
    assert(sd_seq_parse("R10-L5-R3-B", &s) && s.len == 3);       // -B yutulur
    assert(s.seg[0] == 10 && s.seg[1] == -5 && s.seg[2] == 3);
    assert(sd_seq_parse("R50-L50-R50", &s) && s.len == 3);       // tik sınırı
    assert(sd_seq_parse("R1", &s) && s.len == 1);                // parse gevşek
}

static void test_parse_invalid(void)
{
    sd_seq_t s;
    assert(!sd_seq_parse("", &s));
    assert(!sd_seq_parse("X3-R5", &s));       // bilinmeyen yön
    assert(!sd_seq_parse("L0-R5", &s));       // 0 tik
    assert(!sd_seq_parse("L51", &s));         // tik > 50
    assert(!sd_seq_parse("l3-r5", &s));       // küçük harf yok
    assert(!sd_seq_parse("L3-", &s));         // sarkan ayraç
    assert(!sd_seq_parse("L3 R5", &s));       // boşluk
    assert(!sd_seq_parse("B", &s));           // yalnız B
    assert(!sd_seq_parse("L3-B-R5", &s));     // B yalnız kuyrukta
    // 17 segment → sınır aşımı
    assert(!sd_seq_parse("R1-L1-R1-L1-R1-L1-R1-L1-R1-L1-R1-L1-R1-L1-R1-L1-R1", &s));
}

static void test_format_roundtrip(void)
{
    sd_seq_t s, s2;
    char buf[96];
    assert(sd_seq_parse("R4-L2-R3-L1", &s));
    assert(sd_seq_format(&s, buf, sizeof(buf)) > 0);
    assert(strcmp(buf, "R4-L2-R3-L1") == 0);
    assert(sd_seq_parse(buf, &s2) && sd_seq_equal(&s, &s2));
}

static void test_input_direction_commit(void)
{
    // Kılavuz örneği: [4,-2,3,-1] = 4R,2L,3R,1L — yön değişimiyle.
    sd_seq_input_t in;
    sd_seq_t out, expect;
    sd_seq_input_init(&in);
    rot(&in, +1, 4);
    rot(&in, -1, 2);
    rot(&in, +1, 3);
    rot(&in, -1, 1);
    assert(sd_seq_input_finalize(&in, &out));
    assert(sd_seq_parse("R4-L2-R3-L1", &expect));
    assert(sd_seq_equal(&out, &expect));
}

static void test_input_button_commit(void)
{
    // Buton aynı yönde ardışık segmentlere izin verir: R3-B-R2 → [3,2].
    sd_seq_input_t in;
    sd_seq_t out;
    sd_seq_input_init(&in);
    rot(&in, +1, 3);
    sd_seq_input_button(&in);
    rot(&in, +1, 2);
    assert(sd_seq_input_finalize(&in, &out));
    assert(out.len == 2 && out.seg[0] == 3 && out.seg[1] == 2);
}

static void test_input_empty_and_reset(void)
{
    sd_seq_input_t in;
    sd_seq_t out;
    sd_seq_input_init(&in);
    assert(!sd_seq_input_finalize(&in, &out));    // giriş yok
    sd_seq_input_button(&in);                     // girişsiz buton etkisiz
    assert(!in.active);
    // finalize sonrası temiz başlangıç
    rot(&in, +1, 2);
    assert(sd_seq_input_finalize(&in, &out) && out.len == 1);
    assert(!in.active && in.cur.len == 0);
}

static void test_input_overflow(void)
{
    sd_seq_input_t in;
    sd_seq_t out;
    // 17 segment → taşma → finalize false (eşleşme denenmez).
    sd_seq_input_init(&in);
    for (int i = 0; i < 17; i++) rot(&in, (i % 2) ? -1 : +1, 1);
    assert(!sd_seq_input_finalize(&in, &out));
    // Tek segmentte 51 tik → taşma.
    sd_seq_input_init(&in);
    rot(&in, +1, 51);
    assert(!sd_seq_input_finalize(&in, &out));
}

static void test_equal(void)
{
    sd_seq_t a, b;
    assert(sd_seq_parse("L3-R5", &a));
    assert(sd_seq_parse("L3-R5", &b));
    assert(sd_seq_equal(&a, &b));
    assert(sd_seq_parse("L3-R6", &b));
    assert(!sd_seq_equal(&a, &b));
    assert(sd_seq_parse("L3-R5-L1", &b));
    assert(!sd_seq_equal(&a, &b));                // uzunluk farkı
}

int main(void)
{
    test_parse_valid();
    test_parse_invalid();
    test_format_roundtrip();
    test_input_direction_commit();
    test_input_button_commit();
    test_input_empty_and_reset();
    test_input_overflow();
    test_equal();
    printf("test_sequence: OK\n");
    return 0;
}
