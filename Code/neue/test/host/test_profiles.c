// sd_profiles_validate birim testleri (host, assert tabanlı) + KATALOG
// KAPISI: repo profiles/*.json dosyalarının TAMAMI validator'dan geçer —
// kataloğa bozuk profil girmesi build'de değil burada yakalanır.

#include <assert.h>
#include <glob.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cJSON.h"
#include "sd_profiles.h"

static bool validate_str(const char *json, const char **reason)
{
    cJSON *root = cJSON_Parse(json);
    // Parse hatası da "geçersiz" sayılır (firmware profile.add parse'ı
    // ayrıca reddeder; burada yalnız validator kuralları test edilir).
    if (!root) { *reason = "parse"; return false; }
    bool ok = sd_profiles_validate_json(root, reason);
    cJSON_Delete(root);
    return ok;
}

#define VALID_MIN "\"protocol\":\"http\",\"v\":2"

static void test_rules(void)
{
    const char *r;
    // Geçerli asgari profil.
    assert(validate_str("{\"id\":\"x\"," VALID_MIN "}", &r));
    // v yok / yanlış.
    assert(!validate_str("{\"id\":\"x\",\"protocol\":\"http\"}", &r) &&
           strcmp(r, "bad_version") == 0);
    assert(!validate_str("{\"id\":\"x\",\"protocol\":\"http\",\"v\":1}", &r));
    // id yok / 16 karakter / bozuk alfabe.
    assert(!validate_str("{" VALID_MIN "}", &r) && strcmp(r, "bad_id") == 0);
    assert(!validate_str("{\"id\":\"abcdefghijklmnop\"," VALID_MIN "}", &r));  // 16
    assert(validate_str("{\"id\":\"abcdefghijklmno\"," VALID_MIN "}", &r));    // 15
    assert(!validate_str("{\"id\":\"a b\"," VALID_MIN "}", &r));
    assert(!validate_str("{\"id\":\"a\\\"b\"," VALID_MIN "}", &r));
    // protokol.
    assert(!validate_str("{\"id\":\"x\",\"v\":2,\"protocol\":\"coap\"}", &r) &&
           strcmp(r, "bad_protocol") == 0);
    // port sınırları.
    assert(!validate_str("{\"id\":\"x\"," VALID_MIN ",\"port\":0}", &r) &&
           strcmp(r, "bad_port") == 0);
    assert(!validate_str("{\"id\":\"x\"," VALID_MIN ",\"port\":65536}", &r));
    assert(validate_str("{\"id\":\"x\"," VALID_MIN ",\"port\":8080}", &r));
    // behaviors: >4 eleman / string olmayan eleman.
    assert(!validate_str("{\"id\":\"x\"," VALID_MIN
                         ",\"behaviors\":[\"a\",\"b\",\"c\",\"d\",\"e\"]}", &r) &&
           strcmp(r, "bad_behaviors") == 0);
    assert(!validate_str("{\"id\":\"x\"," VALID_MIN ",\"behaviors\":[1]}", &r));
    assert(validate_str("{\"id\":\"x\"," VALID_MIN
                        ",\"behaviors\":[\"dimmer\",\"shutter\"]}", &r));
    // commands: bilinen anahtar nesne olmalı.
    assert(!validate_str("{\"id\":\"x\"," VALID_MIN
                         ",\"commands\":{\"set_value\":1}}", &r) &&
           strcmp(r, "bad_commands") == 0);
    assert(validate_str("{\"id\":\"x\"," VALID_MIN
                        ",\"commands\":{\"set_value\":{\"path\":\"/p\"}}}", &r));
    // name: 47 üstü / JSON-güvensiz.
    assert(!validate_str("{\"id\":\"x\"," VALID_MIN ",\"name\":"
        "\"aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa\"}", &r));  // 48
}

// Katalog kapısı: ../../profiles/*.json tamamı geçerli + kompakt ≤2048B.
static void test_catalog(void)
{
    glob_t g;
    assert(glob("../../profiles/*.json", 0, NULL, &g) == 0);
    assert(g.gl_pathc >= 12);   // 2026-07-03 itibarıyla 12 profil
    for (size_t i = 0; i < g.gl_pathc; i++) {
        FILE *f = fopen(g.gl_pathv[i], "rb");
        assert(f);
        char buf[8192];
        size_t n = fread(buf, 1, sizeof(buf) - 1, f);
        fclose(f);
        buf[n] = '\0';
        cJSON *root = cJSON_Parse(buf);
        if (!root) {
            fprintf(stderr, "KATALOG PARSE HATASI: %s\n", g.gl_pathv[i]);
            assert(0);
        }
        const char *reason = NULL;
        if (!sd_profiles_validate_json(root, &reason)) {
            fprintf(stderr, "KATALOG GECERSIZ: %s (%s)\n",
                    g.gl_pathv[i], reason ? reason : "?");
            assert(0);
        }
        char *compact = cJSON_PrintUnformatted(root);
        assert(compact && strlen(compact) <= SD_PROFILE_JSON_MAX);
        free(compact);
        cJSON_Delete(root);
    }
    globfree(&g);
}

int main(void)
{
    test_rules();
    test_catalog();
    printf("test_profiles: OK\n");
    return 0;
}
