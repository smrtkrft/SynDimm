#pragma once

#include "esp_err.h"
#include <stdbool.h>
#include <stddef.h>

/**
 * SynDimm - Device Driver
 * Profil tabanli cihaz kontrolu: Dimmer, Shutter, Safe
 * Protokoller: HTTP, UDP, MQTT
 *
 * Profiller harici flash'ta JSON olarak saklanir (/ext/profiles/)
 * Ayarlar NVS'te saklanir (namespace: sd_device)
 */

// Mod tipleri
typedef enum {
    DD_MODE_DIMMER  = 0,
    DD_MODE_SHUTTER = 1,
    DD_MODE_SAFE    = 2,
} dd_mode_t;

// Protokol tipleri
typedef enum {
    DD_PROTO_NONE = 0,
    DD_PROTO_HTTP,
    DD_PROTO_UDP,
    DD_PROTO_MQTT,
} dd_proto_t;

// Profil ozet bilgisi (listeleme icin)
typedef struct {
    char id[32];
    char name[48];
    char manufacturer[32];
    char protocol[8];
    char mode[8];
} dd_profile_summary_t;

// Cihaz durumu (GUI icin)
typedef struct {
    dd_mode_t mode;
    int       value;          // 0-100 arasi
    bool      state;          // on/off
    bool      connected;      // cihaz erisilebilir mi
    char      device_name[48];
    char      profile_id[32];
    char      mode_str[12];   // "dimmer","shutter","safe"
} dd_status_t;

/**
 * Device driver baslatma
 * NVS'ten mod/profil/host bilgisi yukler, profili acar
 */
esp_err_t device_driver_init(void);

/**
 * Mod degistirme: "dimmer", "shutter", "safe"
 * NVS'e kaydeder, profil sifirlar
 */
esp_err_t device_driver_set_mode(const char *mode);

/**
 * Aktif mod string olarak
 */
const char *device_driver_get_mode(void);

/**
 * Profil yukle (id ile). Host/device_id/auth_key ayri set edilir.
 * JSON dosyayi /ext/profiles/{mode}/{id}.json'dan okur.
 */
esp_err_t device_driver_load_profile(const char *profile_id);

/**
 * Aktif profil ID
 */
const char *device_driver_get_active_profile(void);

/**
 * Belirli moddaki profilleri JSON dizisi olarak listele
 * json_out: [{"id":"...","name":"...","manufacturer":"...","protocol":"..."},...]
 */
esp_err_t device_driver_list_profiles(const char *mode, char *json_out, size_t max_len);

/**
 * Profil dosyasi sil
 */
esp_err_t device_driver_delete_profile(const char *profile_id);

/**
 * Profil sec + hedef cihaz bilgisi ayarla
 * host, device_id, auth_key NVS'e kaydedilir
 */
esp_err_t device_driver_select_profile(const char *profile_id,
                                        const char *host,
                                        const char *device_id,
                                        const char *auth_key);

/**
 * Encoder event isleme (input_task'tan cagirilir)
 * 'L' = sola, 'R' = saga, 'B' = buton, 'M' = mod degisikligi
 */
esp_err_t device_driver_handle_event(char event);

/**
 * Mod degistirme (sinir kontrollu). true=degisti, false=sinirda (degismedi)
 * direction: 'N' = sonraki, 'P' = onceki
 */
bool device_driver_handle_mode_change(char direction);

/**
 * Mevcut deger (0-100)
 */
int device_driver_get_value(void);

/**
 * Cihaz adi (profil name alani)
 */
const char *device_driver_get_device_name(void);

/**
 * Durum metni: "on", "off", "Sifre Bekleniyor" vs.
 */
const char *device_driver_get_status_text(void);

/**
 * Cihaz baglantisi aktif mi
 */
bool device_driver_is_connected(void);

/**
 * Tüm durum bilgisi (GUI icin)
 */
void device_driver_get_status(dd_status_t *out);

/**
 * MQTT broker bilgisi ayarla (mqtt profilleri icin)
 */
esp_err_t device_driver_set_mqtt_config(const char *broker, int port);

/**
 * Safe profil ekle (JSON string olarak)
 */
esp_err_t device_driver_add_safe_profile(const char *json_str);

/**
 * Safe profilleri listele
 */
esp_err_t device_driver_list_safe_profiles(char *json_out, size_t max_len);

// ---- Dahili protokol API'leri (proto_*.c tarafindan implement edilir) ----

// HTTP protokol
esp_err_t proto_http_send_command(const char *host, int port,
                                  const char *method, const char *path,
                                  const char *body, const char *auth_key,
                                  const char *device_id, int value, bool toggle_val);

esp_err_t proto_http_get_status(const char *host, int port,
                                 const char *method, const char *path,
                                 const char *auth_key, const char *device_id,
                                 const char *value_key, const char *state_key,
                                 int *out_value, bool *out_state,
                                 int map_min, int map_max);

// UDP protokol
esp_err_t proto_udp_send_command(const char *host, int port,
                                  const char *packet_template,
                                  int value, bool toggle_val);

esp_err_t proto_udp_get_status(const char *host, int port,
                                const char *packet_template,
                                const char *value_key, const char *state_key,
                                int *out_value, bool *out_state);

// MQTT protokol
esp_err_t proto_mqtt_init(const char *broker, int port);
void      proto_mqtt_stop(void);
bool      proto_mqtt_is_connected(void);

esp_err_t proto_mqtt_send_command(const char *topic, const char *payload,
                                   const char *prefix, const char *state_prefix,
                                   int value, bool toggle_val);

esp_err_t proto_mqtt_subscribe_status(const char *topic, const char *prefix,
                                       const char *state_prefix);

esp_err_t proto_mqtt_get_status(const char *value_key, const char *state_key,
                                 int *out_value, bool *out_state);

/**
 * State degisiklik bildirimi callback'i ayarla
 * Web server bu callback'i SSE gondermek icin kaydeder
 */
void device_driver_set_notify_cb(void (*cb)(void));

// Safe lock
esp_err_t safe_lock_init(void);
esp_err_t safe_lock_handle_event(char event);
void      safe_lock_reset(void);
bool      safe_lock_is_active(void);
void      safe_lock_set_timeout_cb(void (*cb)(void));
void      safe_lock_process_timeout(void);
