// sd_binding birim testleri (host, assert tabanlı).

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "cJSON.h"
#include "sd_binding.h"

static bool validate_str(const char *json, char *err, size_t cap)
{
    cJSON *root = cJSON_Parse(json);
    bool ok = sd_binding_validate(root, err, cap);
    cJSON_Delete(root);
    return ok;
}

#define OK(json)          do { char e[48] = ""; assert(validate_str(json, e, sizeof(e))); } while (0)
#define BAD(json, reason) do { char e[48] = ""; assert(!validate_str(json, e, sizeof(e))); \
                               assert(strcmp(e, reason) == 0); } while (0)

static const char *FULL =
    "{\"v\":2,\"behavior\":\"dimmer\",\"enabled\":true,\"name\":\"Salon\","
    "\"profile\":\"shelly_dimmer2\","
    "\"targets\":[{\"host\":\"192.168.1.40\",\"port\":80,"
    "\"device_id\":\"\",\"auth_key\":\"\"}],"
    "\"params\":{\"step\":1,\"accel\":true,"
    "\"presets\":{\"double_click\":100,\"long_press\":10},"
    "\"gestures_enabled\":true}}";

static void test_valid(void)
{
    OK(FULL);
    OK("{\"v\":2,\"behavior\":\"dimmer\"}");                       // asgari
    OK("{\"v\":2,\"behavior\":\"dimmer\",\"enabled\":false,\"name\":\"Rezerve\"}");
    OK("{\"v\":2,\"behavior\":\"mqtt_remote\","
       "\"params\":{\"topic\":\"syndimm/knob\",\"payload_value\":\"{value}\"}}");
    OK("{\"v\":2,\"behavior\":\"dimmer\",\"targets\":[]}");        // boş dizi ok
    OK("{\"v\":2,\"behavior\":\"dimmer\",\"yeni_alan\":123}");     // bilinmeyen alan toleransı
}

static void test_version(void)
{
    BAD("{\"behavior\":\"dimmer\"}",          "bad_version");
    BAD("{\"v\":1,\"behavior\":\"dimmer\"}",  "bad_version");
    BAD("{\"v\":\"2\",\"behavior\":\"x\"}",   "bad_version");      // string v olmaz
    BAD("null",                                "not_object");
    BAD("[]",                                  "not_object");
}

static void test_behavior_field(void)
{
    BAD("{\"v\":2}",                        "missing_behavior");
    BAD("{\"v\":2,\"behavior\":\"\"}",      "behavior_length");
    BAD("{\"v\":2,\"behavior\":42}",        "behavior_not_string");
}

static void test_profile_limits(void)
{
    // 15 karakter sınırı (NVS key)
    OK ("{\"v\":2,\"behavior\":\"d\",\"profile\":\"123456789012345\"}");
    BAD("{\"v\":2,\"behavior\":\"d\",\"profile\":\"1234567890123456\"}",
        "profile_length");
    BAD("{\"v\":2,\"behavior\":\"d\",\"profile\":\"\"}", "profile_length");
}

static void test_targets(void)
{
    BAD("{\"v\":2,\"behavior\":\"d\",\"targets\":{}}",  "targets_not_array");
    BAD("{\"v\":2,\"behavior\":\"d\",\"targets\":[{}]}","missing_host");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"targets\":[{\"host\":\"a\",\"port\":0}]}",   "port_range");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"targets\":[{\"host\":\"a\",\"port\":65536}]}","port_range");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"targets\":[{\"host\":\"a\",\"port\":8.5}]}", "port_not_int");
    // 5 hedef → sınır aşımı (maks 4)
    BAD("{\"v\":2,\"behavior\":\"d\",\"targets\":["
        "{\"host\":\"a\"},{\"host\":\"b\"},{\"host\":\"c\"},"
        "{\"host\":\"d\"},{\"host\":\"e\"}]}",          "targets_count");
}

static void test_params(void)
{
    BAD("{\"v\":2,\"behavior\":\"d\",\"params\":[]}",   "params_not_object");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"params\":{\"step\":0}}",                     "step_range");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"params\":{\"step\":26}}",                    "step_range");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"params\":{\"accel\":1}}",                    "accel_not_bool");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"params\":{\"presets\":{\"double_click\":101}}}", "double_click_range");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"params\":{\"presets\":{\"long_press\":-1}}}",    "long_press_range");
    BAD("{\"v\":2,\"behavior\":\"d\","
        "\"params\":{\"presets\":[]}}",                 "presets_not_object");
}

int main(void)
{
    test_valid();
    test_version();
    test_behavior_field();
    test_profile_limits();
    test_targets();
    test_params();
    printf("test_binding: OK\n");
    return 0;
}
