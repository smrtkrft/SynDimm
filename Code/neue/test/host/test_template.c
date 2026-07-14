// sd_template birim testleri (host, assert tabanlı).

#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "sd_template.h"

static void test_value_and_toggle(void)
{
    char out[128];
    sd_tmpl_vars_t v = { .value = 72, .toggle = true };
    sd_template_expand("/light/0?brightness={value}&on={toggle}",
                       out, sizeof(out), &v);
    assert(strcmp(out, "/light/0?brightness=72&on=true") == 0);

    v.toggle = false;
    sd_template_expand("{toggle}", out, sizeof(out), &v);
    assert(strcmp(out, "false") == 0);

    // MQTT literalleri
    v.toggle = true; v.toggle_true = "ON"; v.toggle_false = "OFF";
    sd_template_expand("cmnd/dimmer/POWER {toggle}", out, sizeof(out), &v);
    assert(strcmp(out, "cmnd/dimmer/POWER ON") == 0);
}

static void test_auth_and_device(void)
{
    char out[128];
    sd_tmpl_vars_t v = { .value = 50, .auth_key = "KEY123", .device_id = "7" };
    sd_template_expand("/api/{auth_key}/lights/{device_id}/state",
                       out, sizeof(out), &v);
    assert(strcmp(out, "/api/KEY123/lights/7/state") == 0);
}

static void test_null_token_empty(void)
{
    char out[128];
    sd_tmpl_vars_t v = { .value = 1 };   // auth_key NULL
    sd_template_expand("/api/{auth_key}/x", out, sizeof(out), &v);
    assert(strcmp(out, "/api//x") == 0);   // bilinen+NULL → "" (eski davranış)
    sd_template_expand("key={auth_key}&id={device_id}&v={value}",
                       out, sizeof(out), &v);
    assert(strcmp(out, "key=&id=&v=1") == 0);
}

static void test_unknown_token_passthrough(void)
{
    char out[128];
    sd_tmpl_vars_t v = { .value = 1 };
    sd_template_expand("{bilinmeyen} ve {value}", out, sizeof(out), &v);
    assert(strcmp(out, "{bilinmeyen} ve 1") == 0);
}

static void test_mqtt_topic_tokens(void)
{
    char out[128];
    sd_tmpl_vars_t v = { .prefix = "syndimm/salon", .state_topic = "stat" };
    sd_template_expand("{prefix}/cmnd", out, sizeof(out), &v);
    assert(strcmp(out, "syndimm/salon/cmnd") == 0);
    sd_template_expand("{prefix}/{state}/RESULT", out, sizeof(out), &v);
    assert(strcmp(out, "syndimm/salon/stat/RESULT") == 0);
}

static void test_truncation(void)
{
    char out[8];
    sd_tmpl_vars_t v = { .value = 12345 };
    size_t n = sd_template_expand("X{value}Y{value}Z", out, sizeof(out), &v);
    assert(n == 7);                       // cap-1'e dayandı → kesildi
    assert(strlen(out) == 7);
    assert(strncmp(out, "X12345Y", 7) == 0);
}

static void test_brace_without_close(void)
{
    char out[32];
    sd_tmpl_vars_t v = { .value = 5 };
    sd_template_expand("acik{brace kalan {value}", out, sizeof(out), &v);
    // İlk '{' kapanmıyor gibi görünse de strchr sonraki '}'yi bulur —
    // "{brace kalan {value}" bilinen token değil → aynen geçer.
    assert(strcmp(out, "acik{brace kalan {value}") == 0);
}

static void test_map_value(void)
{
    // 0-100 → 0-254 (Hue)
    assert(sd_map_value(0,   0, 100, 0, 254) == 0);
    assert(sd_map_value(100, 0, 100, 0, 254) == 254);
    assert(sd_map_value(50,  0, 100, 0, 254) == 127);   // 127.0 → 127
    // Ters yön: 0-254 → 0-100
    assert(sd_map_value(254, 0, 254, 0, 100) == 100);
    assert(sd_map_value(127, 0, 254, 0, 100) == 50);
    // Kırpma
    assert(sd_map_value(150, 0, 100, 0, 254) == 254);
    assert(sd_map_value(-5,  0, 100, 0, 254) == 0);
    // Dejenere kaynak aralığı
    assert(sd_map_value(42, 7, 7, 0, 100) == 0);
    // Birebir
    assert(sd_map_value(33, 0, 100, 0, 100) == 33);
}

int main(void)
{
    test_value_and_toggle();
    test_auth_and_device();
    test_null_token_empty();
    test_unknown_token_passthrough();
    test_mqtt_topic_tokens();
    test_truncation();
    test_brace_without_close();
    test_map_value();
    printf("test_template: OK\n");
    return 0;
}
