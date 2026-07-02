#pragma once

// sk_baseline — APP-facing baseline commands required by every SmartKraft
// device per shared/cli_contract.md §3:
//   device.info, device.commands, device.status, device.manifest,
//   logs.get, time.set
//
// Auto-initialised by sk_core_init(). Devices do not call this directly.

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

// Bumped together with shared/cli_contract.md.
#define SK_PROTOCOL_VERSION  "0.2.0"

esp_err_t sk_baseline_init(const char *fw_version, const char *build_info);

// Battery telemetry provider for device.info. A device component (e.g.
// bf_battery) registers this so device.info reports real battery state
// instead of a placeholder. Return true if a battery is present and fill
// *mv (millivolts), *pct (0-100), *charge ("charging"|"full"|"discharging");
// out-params are non-NULL when called. Pass NULL to clear.
typedef bool (*sk_battery_provider_fn)(int *mv, int *pct, const char **charge);
void sk_baseline_set_battery_provider(sk_battery_provider_fn fn);

#ifdef __cplusplus
}
#endif
