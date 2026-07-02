// sd_util — CLI'ya bağımlı yardımcılar (pure kısım: sd_util_str.c).

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sd_util.h"

char *sd_cliu_json_arg_dup(sk_cli_ctx_t *ctx, int start_idx)
{
    const char *named = sk_cli_arg_named(ctx, "json");
    if (named) return strdup(named);

    int argc = sk_cli_argc(ctx);
    if (argc <= start_idx) return NULL;
    size_t total = 0;
    for (int i = start_idx; i < argc; i++) {
        total += strlen(sk_cli_arg(ctx, i)) + 1;
    }
    char *buf = malloc(total + 1);
    if (!buf) return NULL;
    size_t off = 0;
    for (int i = start_idx; i < argc; i++) {
        off += (size_t)snprintf(buf + off, total + 1 - off, "%s%s",
                                i > start_idx ? " " : "", sk_cli_arg(ctx, i));
    }
    return buf;
}
