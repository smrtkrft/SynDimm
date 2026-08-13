#include "sk_auth.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

#include "esp_timer.h"
#include "mbedtls/md.h"

// Symbol defined in sk_auth.c:
extern const uint8_t *sk_auth__token_ptr(void);
extern bool           sk_auth__has_token(void);

// Replay guard: last 64 nonces seen.
//
// Reset on every fresh secure-session handshake (sk_secure_session_reset →
// sk_auth_replay_reset). The C-R handshake exchanges fresh challenges, so
// any captured pre-reset envelope can no longer be replayed: its `sig`
// was bound to the old token-derived MAC chain, and the new session would
// reject the bytes as soon as the verify runs against the new context
// anyway. Without the reset, every reconnect would burn nonces from the
// previous session and the SKAPP-side counter (which starts back at 1
// per fresh CliSigner) would collide forever — exactly the symptom that
// turned `userdata.size` / `api.endpoint.list` into silent timeouts after
// a Riverpod autoDispose / invalidate cycle.
// Single source of truth with sk_auth_replay_t so the global and the
// per-session rings can never drift apart.
#define NONCE_WINDOW SK_AUTH_NONCE_WINDOW
static uint32_t s_nonces[NONCE_WINDOW];
static int      s_nonce_head = 0;

void sk_auth_replay_reset(void)
{
    for (int i = 0; i < NONCE_WINDOW; i++) s_nonces[i] = 0;
    s_nonce_head = 0;
}

#define TS_WINDOW_SEC 60

// Wall clock is "set" once SKAPP pushes `time.set` (settimeofday in
// sk_baseline.c). Before that the RTC reads ~1970, so any value past this
// threshold (~2023-11) means a real time baseline is available and ts_unix
// can be validated. Mirrors sk_log.c's post-2023 "real time" heuristic.
#define SK_AUTH_CLOCK_SET_EPOCH 1700000000LL

static bool nonce_seen_in(const uint32_t *ring, uint32_t n)
{
    for (int i = 0; i < NONCE_WINDOW; i++) {
        if (ring[i] == n && n != 0) return true;
    }
    return false;
}

static bool nonce_seen(uint32_t n)
{
    return nonce_seen_in(s_nonces, n);
}

static void nonce_mark(uint32_t n)
{
    s_nonces[s_nonce_head] = n;
    s_nonce_head = (s_nonce_head + 1) % NONCE_WINDOW;
}

void sk_auth_replay_ctx_reset(sk_auth_replay_t *replay)
{
    if (!replay) return;
    memset(replay->nonces, 0, sizeof(replay->nonces));
    replay->head = 0;
}

static esp_err_t hmac_with_key(const uint8_t key[SK_AUTH_TOKEN_LEN],
                               const char *body, size_t len,
                               uint8_t out[SK_AUTH_HMAC_LEN])
{
    const mbedtls_md_info_t *md = mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    if (!md) return ESP_FAIL;
    uint8_t full[32];
    int rc = mbedtls_md_hmac(md, key, SK_AUTH_TOKEN_LEN,
                             (const uint8_t *)body, len, full);
    if (rc != 0) return ESP_FAIL;
    memcpy(out, full, SK_AUTH_HMAC_LEN);
    return ESP_OK;
}

static esp_err_t compute_hmac(const char *body, size_t len, uint8_t out[SK_AUTH_HMAC_LEN])
{
    if (!sk_auth__has_token()) return ESP_ERR_INVALID_STATE;
    return hmac_with_key(sk_auth__token_ptr(), body, len, out);
}

// Shared timestamp gate — see the TS_WINDOW_SEC comment in
// sk_auth_verify_message for why it only engages once SKAPP has pushed
// `time.set`.
static bool ts_out_of_window(int64_t ts_unix)
{
    time_t wall = time(NULL);
    if ((int64_t)wall <= SK_AUTH_CLOCK_SET_EPOCH) return false;
    int64_t skew = (int64_t)wall - ts_unix;
    if (skew < 0) skew = -skew;
    return skew > TS_WINDOW_SEC;
}

esp_err_t sk_auth_sign_message(const char *body, size_t len, uint8_t sig[SK_AUTH_HMAC_LEN])
{
    if (!body || !sig) return ESP_ERR_INVALID_ARG;
    return compute_hmac(body, len, sig);
}

esp_err_t sk_auth_verify_message(const char *body, size_t len,
                                 uint32_t nonce, int64_t ts_unix,
                                 const uint8_t sig[SK_AUTH_HMAC_LEN])
{
    if (!body || !sig) return ESP_ERR_INVALID_ARG;
    if (!sk_auth__has_token()) return ESP_ERR_INVALID_STATE;

    // Timestamp window (güvenlik.md Madde 17). Cihazda SNTP yok, ama SKAPP
    // `time.set` ile settimeofday çağırıyor; o andan itibaren time(NULL)
    // gerçek UNIX zamanı döner. Saat ayarlıysa (post-2023) ts_unix'i
    // ±TS_WINDOW_SEC penceresinde doğrula — böylece cihaz reboot'undan sonra
    // nonce ringi sıfırlansa bile eski bir capture'ın ts'i bayat kalır ve
    // replay reddedilir. Saat ayarlı değilse (boot sonrası, time.set öncesi)
    // eski davranışa düş: replay guard'ı yalnızca nonce benzersizliği taşır.
    // SKAPP tarafı ts'i gerçek wall-clock saniye olarak gönderiyor
    // (cli_signer.dart: DateTime.now().millisecondsSinceEpoch ~/ 1000).
    if (ts_out_of_window(ts_unix)) return ESP_FAIL;  // stale / future ts

    if (nonce_seen(nonce)) return ESP_FAIL;

    uint8_t expect[SK_AUTH_HMAC_LEN];
    esp_err_t e = compute_hmac(body, len, expect);
    if (e != ESP_OK) return e;

    // Constant-time compare.
    uint8_t diff = 0;
    for (int i = 0; i < SK_AUTH_HMAC_LEN; i++) diff |= expect[i] ^ sig[i];
    if (diff != 0) return ESP_FAIL;

    nonce_mark(nonce);
    return ESP_OK;
}

esp_err_t sk_auth_verify_message_with(const uint8_t     bond_key[SK_AUTH_TOKEN_LEN],
                                      sk_auth_replay_t *replay,
                                      const char       *body,
                                      size_t            len,
                                      uint32_t          nonce,
                                      int64_t           ts_unix,
                                      const uint8_t     sig[SK_AUTH_HMAC_LEN])
{
    if (!bond_key || !replay || !body || !sig) return ESP_ERR_INVALID_ARG;

    if (ts_out_of_window(ts_unix)) return ESP_FAIL;
    if (nonce_seen_in(replay->nonces, nonce)) return ESP_FAIL;

    uint8_t expect[SK_AUTH_HMAC_LEN];
    esp_err_t e = hmac_with_key(bond_key, body, len, expect);
    if (e != ESP_OK) return e;

    uint8_t diff = 0;
    for (int i = 0; i < SK_AUTH_HMAC_LEN; i++) diff |= expect[i] ^ sig[i];
    if (diff != 0) return ESP_FAIL;

    replay->nonces[replay->head] = nonce;
    replay->head = (uint8_t)((replay->head + 1) % NONCE_WINDOW);
    return ESP_OK;
}
