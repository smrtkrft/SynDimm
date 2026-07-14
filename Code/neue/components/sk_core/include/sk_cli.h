#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "sk_errors.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct sk_cli_ctx sk_cli_ctx_t;

// Handler contract. Writes response via sk_cli_write_ok / sk_cli_write_err.
// Returning a non-SK_OK value causes the dispatcher to emit an error
// envelope automatically if the handler did not already call write_*.
typedef sk_err_t (*sk_cli_handler_t)(sk_cli_ctx_t *ctx);

// Declarative command definition. All strings must have static lifetime
// (typically string literals). NULL fields are fine for optional slots.
typedef struct {
    const char       *name;         // canonical dot-notation, e.g. "timer.set"
    const char       *summary;      // one-line description (shown by `help`)
    const char       *usage;        // syntax line, e.g. "timer set <value> <unit>"
    const char       *help_block;   // multi-line inline-help body (Principle #11),
                                    // shown by `help <cmd>` and for status/get
    bool              critical;     // requires confirm token
    bool              hidden;       // omit from `help` overview / topic lists.
                                    // Still callable; still resolvable via
                                    // `help <name>`. Use for machine-only or
                                    // SKAPP-internal commands the human user
                                    // never types.
    bool              requires_auth; // dispatcher rejects with
                                    // ERR_NOT_AUTHENTICATED when invoked over
                                    // an unauthenticated transport (USB CLI
                                    // is always unauthenticated; BLE/TCP are
                                    // authenticated only after the
                                    // sk_secure_session HMAC envelope path).
                                    // Use for commands that read or write
                                    // material that must never leak via USB
                                    // (encrypted store, user scratch area,
                                    // long-lived secrets).
    sk_cli_handler_t  handler;
} sk_cli_command_t;

// Output writer: transport-specific sink. `len == 0` means "flush" (useful
// for line-buffered terminals). The dispatcher calls writer potentially many
// times per command (streamed output).
typedef void (*sk_cli_writer_t)(const char *chunk, size_t len, void *user);

typedef enum {
    SK_CLI_MODE_HUMAN,
    SK_CLI_MODE_MACHINE,
} sk_cli_mode_t;

esp_err_t sk_cli_init(void);

// Register a command. `cmd` and its string fields must remain valid for the
// lifetime of the program — typically file-scope static structs.
esp_err_t sk_cli_register(const sk_cli_command_t *cmd);

// Primary entrypoint: feed a single line (no trailing newline). Detects
// machine vs human mode on the fly (leading '{' = machine). The response —
// whether ok envelope, error envelope, or multi-line inline-help — is
// written via `writer`.
esp_err_t sk_cli_dispatch_line(const char *line, sk_cli_writer_t writer, void *user);

// Same as sk_cli_dispatch_line, but tells the dispatcher this line came from
// an authenticated transport (post HMAC-envelope verify in sk_secure_session).
// Commands marked `requires_auth = true` will only run when invoked through
// this entrypoint — the plain dispatch_line path always counts as
// unauthenticated regardless of which transport called it.
esp_err_t sk_cli_dispatch_line_authenticated(const char *line, sk_cli_writer_t writer, void *user);

// Explicit mode control for transports that want to pin mode.
void sk_cli_set_mode(sk_cli_mode_t mode);
sk_cli_mode_t sk_cli_get_mode(void);

// === Handler-facing helpers =================================================

bool           sk_cli_is_machine_mode(sk_cli_ctx_t *ctx);
int            sk_cli_argc(sk_cli_ctx_t *ctx);
const char    *sk_cli_arg(sk_cli_ctx_t *ctx, int idx);                   // human mode positional
const char    *sk_cli_arg_named(sk_cli_ctx_t *ctx, const char *key);     // --key value / {"key":...} string only
// Read a numeric argument that may land as a JSON number (machine mode
// — SKAPP sends `args:{"size":42}`) OR as a string (human mode --key 42
// or machine mode `args:{"size":"42"}`). Returns true and writes
// *out_value on success; returns false if the key is absent or the value
// is not a parseable integer. Without this helper, handlers that read
// numbers via sk_cli_arg_named + strtol see NULL for native JSON numbers
// and silently default to a sentinel, surfacing as ERR_INVALID_ARG to
// the user (notebook truncate, userdata read/write, delay-after, ...).
bool           sk_cli_arg_long(sk_cli_ctx_t *ctx, const char *key, long *out_value);
const char    *sk_cli_confirm_token(sk_cli_ctx_t *ctx);                  // NULL if absent
bool           sk_cli_is_authenticated(sk_cli_ctx_t *ctx);               // true iff dispatched via _authenticated entrypoint

void           sk_cli_write(sk_cli_ctx_t *ctx, const char *chunk, size_t len);
void           sk_cli_writef(sk_cli_ctx_t *ctx, const char *fmt, ...);

// Response envelope helpers. `data_json_or_null` must be a valid JSON
// object/array/primitive, or NULL for empty data.
void           sk_cli_ok(sk_cli_ctx_t *ctx, const char *data_json_or_null);
void           sk_cli_err(sk_cli_ctx_t *ctx, sk_err_t err, const char *params_json_or_null);

// Walk registered commands — used by `help` and sk_capabilities.
typedef void (*sk_cli_walk_cb_t)(const sk_cli_command_t *cmd, void *user);
void           sk_cli_walk(sk_cli_walk_cb_t cb, void *user);

// Get a command by canonical name. NULL if not registered.
const sk_cli_command_t *sk_cli_lookup(const char *name);

// Register a one-line summary for a command namespace (the part before the
// first '.' in dot-notation). Optional — namespaces without a registered
// summary still appear in `help`, just without a description. Each module
// should call this once for the namespace it owns:
//   sk_cli_register_topic("wifi", "Network connection", "SETUP");
// `category` is an UPPERCASE bucket name shown by the `help` overview to
// group topics. NULL = uncategorized (rendered under a default bucket).
// Devices pick their own category vocabulary; sk_cli only sorts topics
// under whatever category strings it sees. Strings must have static
// lifetime — sk_cli stores the pointers.
esp_err_t      sk_cli_register_topic(const char *topic,
                                     const char *summary,
                                     const char *category);

// Lookup a topic summary. NULL if none registered.
const char    *sk_cli_topic_summary(const char *topic);

// Lookup a topic category. NULL if none registered or topic was registered
// without one.
const char    *sk_cli_topic_category(const char *topic);

// === Confirm-token issuer hook ============================================
//
// Critical commands (`.critical = true`) require a confirm token. Two ways
// for the user to obtain one:
//
//  (1) Pre-fetch path: explicitly call the registered issuer's command
//      (`device confirm-token` in this codebase) to grab a fresh token,
//      then chain it onto the critical command yourself.
//
//  (2) Auto-issue path (preferred for humans): just call the critical
//      command without a token. The dispatcher detects the missing token,
//      asks the registered issuer to mint one, and returns an error
//      envelope that contains the exact ready-to-paste retry command.
//
// The dispatcher only runs path (2) if a confirm-token issuer has been
// registered via this hook. Without an issuer, the existing behavior
// (`ERR_CONFIRM_TOKEN_REQUIRED` from each handler) is preserved.

typedef esp_err_t (*sk_cli_confirm_issuer_t)(char *out_hex, size_t out_hex_size,
                                              uint32_t *out_ttl_sec);

// Register the function the dispatcher will call to mint a confirm token
// when a critical command is invoked without one. Optional. Pass NULL to
// disable the auto-issue path.
esp_err_t      sk_cli_set_confirm_issuer(sk_cli_confirm_issuer_t issuer);

#ifdef __cplusplus
}
#endif
