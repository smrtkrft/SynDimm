#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

// Register the SKAPP GATT service during NimBLE bring-up. Called once.
esp_err_t skbt_gatt_init(void);

// Called by the adapter when the peer writes to cmd_rx. The payload is a
// single NDJSON line (possibly reassembled from multiple ATT chunks).
void      skbt_gatt_on_cmd_rx(uint16_t conn_handle, const char *line, size_t len);

// Emit an event notification to the currently-connected peer. Used by the
// event bus bridge.
void      skbt_gatt_notify_event(const char *payload, size_t len);

// Lifecycle hooks called from the GAP event handler in sk_transport_ble.c.
void      skbt_gatt_on_connect(uint16_t conn_handle);
void      skbt_gatt_on_disconnect(uint16_t conn_handle);

// Peer event_tx CCCD subscribe yaptığında çağrılır. Bond varken
// secure-session handshake'i (auth.challenge yayını) buradan başlatılır
// — on_connect anında yayın yapılırsa peer henüz subscribe değildir,
// notify drop olur.
//
// attr_handle: peer'in subscribe ettiği characteristic value handle —
//   bizim event_tx ile eşleşmiyorsa CCCD başka bir char için yazılmış
//   demek, session_begin'i tetiklememeliyiz.
// cur_notify  : notify-enable bit (CCCD 0x0001) şu an açık mı.
// cur_indicate: indication-enable bit (CCCD 0x0002) şu an açık mı.
//   event_tx NOTIFY-only — peer indication yazdıysa peer broken, link'i
//   düşürürüz.
void      skbt_gatt_on_subscribe(uint16_t conn_handle,
                                 uint16_t attr_handle,
                                 bool     cur_notify,
                                 bool     cur_indicate);

// True if a peer is currently connected. Used by sk_transport_ble.c's
// pairing-close and idle-timeout handlers to decide whether to stop the radio.
bool      skbt_gatt_is_connected(void);

// True only when the peer has finished the secure-session handshake (bond
// + mutual challenge-response). Used by the event-bus bridge to suppress
// spam during the pairing window: a peer doing ECDH must not be drowned
// in face/timer/power notifications, which both confuses the APP and
// blocks its `pairing.ecdh.exchange` write behind the notify queue.
bool      skbt_gatt_is_authenticated(void);

// Called by the GAP event handler when the peer negotiates a new ATT MTU.
// The writer uses (mtu - 3) as the per-notify payload cap so large CLI
// responses (device.info, api.endpoint.list, userdata.read) don't get
// silently truncated by NimBLE. Without this, post-MTU-bump traffic still
// fragments at the default 20-byte boundary and most real responses fail.
void      skbt_gatt_set_mtu(uint16_t mtu);
