// sd_sequence — bkz. sd_sequence.h.

#include <stdio.h>
#include <string.h>

#include "sd_sequence.h"

void sd_seq_input_init(sd_seq_input_t *in)
{
    memset(in, 0, sizeof(*in));
}

static void commit_segment(sd_seq_input_t *in)
{
    if (in->cur_dir == 0 || in->cur_ticks == 0) return;
    if (in->cur.len >= SD_SEQ_MAX_SET_SEGMENTS || in->cur_ticks > SD_SEQ_MAX_TICKS) {
        in->overflow = true;
        return;
    }
    in->cur.seg[in->cur.len++] = (int8_t)(in->cur_dir * in->cur_ticks);
    in->cur_ticks = 0;
    in->cur_dir   = 0;
}

void sd_seq_input_rotate(sd_seq_input_t *in, int dir)
{
    if (dir != 1 && dir != -1) return;
    in->active = true;
    if (in->cur_dir != 0 && dir != in->cur_dir) {
        commit_segment(in);          // yön değişimi = segment sınırı
    }
    in->cur_dir = dir;
    if (++in->cur_ticks > SD_SEQ_MAX_TICKS) in->overflow = true;
}

void sd_seq_input_button(sd_seq_input_t *in)
{
    if (!in->active) return;
    commit_segment(in);
}

bool sd_seq_input_overflowed(const sd_seq_input_t *in)
{
    if (!in->active) return false;
    if (in->overflow) return true;
    // Bekleyen segment finalize'daki commit'te sınırı aşacaksa deneme
    // şimdiden taşmış demektir (7. segment yön-değişimiyle değil idle
    // timeout'la kapanırken bayrak ancak finalize içinde set edilirdi).
    return in->cur_dir != 0 && in->cur_ticks > 0 &&
           in->cur.len >= SD_SEQ_MAX_SET_SEGMENTS;
}

bool sd_seq_input_finalize(sd_seq_input_t *in, sd_seq_t *out)
{
    if (!in->active) return false;
    commit_segment(in);
    bool ok = !in->overflow && in->cur.len > 0;
    if (ok && out) *out = in->cur;
    sd_seq_input_init(in);           // her denemede temiz başlangıç
    return ok;
}

bool sd_seq_equal(const sd_seq_t *a, const sd_seq_t *b)
{
    if (a->len != b->len) return false;
    return memcmp(a->seg, b->seg, (size_t)a->len) == 0;
}

bool sd_seq_parse(const char *s, sd_seq_t *out)
{
    if (!s || !out) return false;
    sd_seq_t seq = { .len = 0 };

    const char *p = s;
    while (*p) {
        int dir;
        if      (*p == 'R') dir = 1;
        else if (*p == 'L') dir = -1;
        else if (*p == 'B' && p[1] == '\0' && seq.len > 0) {
            p++;                     // kuyruk "-B" — bilgi taşımaz
            break;
        }
        else return false;
        p++;

        int ticks = 0;
        while (*p >= '0' && *p <= '9') {
            ticks = ticks * 10 + (*p - '0');
            if (ticks > SD_SEQ_MAX_TICKS) return false;
            p++;
        }
        if (ticks < 1) return false;
        if (seq.len >= SD_SEQ_MAX_SEGMENTS) return false;
        seq.seg[seq.len++] = (int8_t)(dir * ticks);

        if (*p == '-') {
            p++;
            if (*p == '\0') return false;   // sarkan ayraç: "L3-"
            continue;
        }
        if (*p == '\0') break;
        return false;
    }
    if (*p != '\0' || seq.len == 0) return false;
    *out = seq;
    return true;
}

size_t sd_seq_format(const sd_seq_t *seq, char *out, size_t cap)
{
    if (!seq || !out || cap == 0) return 0;
    size_t off = 0;
    out[0] = '\0';
    for (int i = 0; i < seq->len; i++) {
        int v = seq->seg[i];
        int n = snprintf(out + off, cap - off, "%s%c%d",
                         i ? "-" : "", v > 0 ? 'R' : 'L', v > 0 ? v : -v);
        if (n < 0 || (size_t)n >= cap - off) {
            out[0] = '\0';
            return 0;
        }
        off += (size_t)n;
    }
    return off;
}
