#pragma once

// sd_template — profil komut şablonu genişletici. PURE modül (libc only,
// host testi: test/host/test_template.c). Eski üç expander'ın birleşimi:
// proto_http.c:80-121 + proto_udp.c:25-51 + proto_mqtt.c:29-84.
//
// Desteklenen yer tutucular:
//   {value}      → vars->value (%d)
//   {toggle}     → vars->toggle_true / toggle_false
//                  (HTTP/UDP profillerinde "true"/"false", MQTT'de "ON"/"OFF")
//   {auth_key}   → vars->auth_key
//   {device_id}  → vars->device_id
//   {prefix}     → vars->prefix       (MQTT topic kökü)
//   {state}      → vars->state_topic  (MQTT durum topic'i)
//
// NULL string değeri olan BİLİNEN yer tutucu "" ile değiştirilir (eski
// firmware davranışı — auth'suz hedefler bozulmasın). BİLİNMEYEN {token}
// olduğu gibi geçer (yazım hatası teşhis edilebilir kalsın).

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int         value;
    bool        toggle;
    const char *auth_key;
    const char *device_id;
    const char *prefix;
    const char *state_topic;
    const char *toggle_true;    // NULL → "true"
    const char *toggle_false;   // NULL → "false"
} sd_tmpl_vars_t;

// tmpl'i out'a genişletir. Dönüş: yazılan bayt sayısı (NUL hariç).
// out her zaman NUL'lanır; sığmayan içerik sessizce kesilir (dönüş değeri
// cap-1'e dayanmışsa kesilme olmuş demektir).
size_t sd_template_expand(const char *tmpl, char *out, size_t cap,
                          const sd_tmpl_vars_t *vars);

// Doğrusal aralık eşleme, yuvarlamalı (0-100 ↔ 0-254 gibi).
// from aralığı dejenere ise (from_min == from_max) to_min döner.
// v, from aralığına kırpılır.
int sd_map_value(int v, int from_min, int from_max, int to_min, int to_max);

#ifdef __cplusplus
}
#endif
