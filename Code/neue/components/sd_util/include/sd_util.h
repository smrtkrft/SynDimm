#pragma once

// sd_util — sd_* bileşenlerinin paylaştığı küçük yardımcılar.
// (Kod incelemesi: node_int/str/bool ve arg_json_dup üç ayrı bileşende
// kopyalanmıştı — tek kaynak burası.) Pure kısım: sd_util_str.h.

#include "sd_util_str.h"

#include "sk_cli.h"

#ifdef __cplusplus
extern "C" {
#endif

// Makine modu: {"json":"<string>"}; insan modu: start_idx'ten itibaren
// pozisyonel argümanlar tek boşlukla birleştirilir (tezgah kullanımı —
// string içi çoklu boşluk tekile iner). Dönen buffer'ı çağıran free eder.
char *sd_cliu_json_arg_dup(sk_cli_ctx_t *ctx, int start_idx);

#ifdef __cplusplus
}
#endif
