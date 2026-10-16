#pragma once
/**
 * SynDimm - Sistem Olay Kaydi (sys_log)
 *
 * Amac: Cihazin NEDEN yeniden basladigini kesin olarak kaydetmek.
 *
 *  - RTC (LP) bellekte tutulan kucuk bir kayit brownout / panic / watchdog /
 *    yazilim resetinde SILINMEZ; sadece tam guc kaybinda silinir. Bu kayitta
 *    calisma suresi, son olay (AP istemci baglandi, GUI servis edildi, STA
 *    koptu...), WiFi baglami (mod, istemci, RSSI, TX gucu) ve heap durumu tutulur.
 *  - Her acilista onceki calisma analiz edilir ve 5 sn'den uzunsa harici flash'a
 *    (/reserve/logs/events.log, JSONL) tek satir kesin siniflandirmayla yazilir.
 *  - 10 dakikada bir kontrol noktasi yazilir; boylece tam guc kaybinda bile
 *    yaklasik calisma suresi bilinir.
 *  - Calisma sirasinda WiFi kopmasi gibi olaylar ayri satir olarak eklenir.
 */

#include "esp_err.h"
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

/* Son olay (RTC baglami). Reset nedeni yorumlanirken kullanilir. */
typedef enum {
    SL_EV_NONE = 0,
    SL_EV_BOOT,             /* Acilis tamamlandi */
    SL_EV_AP_CLIENT_JOIN,   /* AP'ye istemci (telefon) baglandi, arg=AID */
    SL_EV_AP_CLIENT_LEAVE,  /* AP'den istemci ayrildi */
    SL_EV_STA_CONNECTING,   /* Router'a baglanma denemesi basladi */
    SL_EV_STA_GOT_IP,       /* Router'a baglandi, IP alindi, arg=RSSI */
    SL_EV_STA_LOST,         /* Router baglantisi koptu, arg=reason */
    SL_EV_GUI_SERVE,        /* GUI dosyasi sunuluyor (yogun TX) */
    SL_EV_HTTP_CMD,         /* Hedef cihaza HTTP/UDP/MQTT komut gonderildi */
    SL_EV_OTA_FW,           /* Firmware OTA basladi */
    SL_EV_OTA_GUI,          /* GUI OTA basladi */
    SL_EV_TX_POWER,         /* TX gucu degistirildi, arg=dBm */
    SL_EV_USER_RESTART,     /* Kullanici yeniden baslatma istedi */
    SL_EV_MAX
} sys_log_event_t;

/* esp_restart() oncesi kaydedilen kasitli yeniden baslatma nedeni. */
typedef enum {
    SL_RST_NONE = 0,
    SL_RST_USER_WEB,        /* Web arayuzunden "Yeniden Baslat" */
    SL_RST_USER_ENCODER,    /* Encoder uzun basma */
    SL_RST_FACTORY,         /* Fabrika sifirlama */
    SL_RST_OTA_FW,          /* Firmware guncellemesi */
    SL_RST_OTA_GUI,         /* GUI guncellemesi / rollback */
    SL_RST_GENERIC,         /* esp_restart cagrildi, kaynak belirtilmedi */
} sys_log_restart_t;

/* WiFi modu (RTC baglami icin kucuk kod) */
typedef enum {
    SL_WIFI_OFF = 0,
    SL_WIFI_AP,
    SL_WIFI_STA,
    SL_WIFI_APSTA,
} sys_log_wifi_mode_t;

/**
 * Baslat. ext_flash_init() sonrasinda cagrilmali (reserve mount gerekir;
 * mount yoksa RTC analizi yine yapilir ama dosyaya yazilamaz).
 * Onceki calismayi analiz eder, acilis kaydini yazar, izleme gorevini baslatir.
 */
esp_err_t sys_log_init(void);

/** Son olayi isaretle (ucuz: sadece RTC bellek, flash yazmaz). */
void sys_log_mark_event(sys_log_event_t ev, int32_t arg);

/** WiFi baglamini guncelle (ucuz, RTC bellek). tx_dbm: anlik TX ust siniri. */
void sys_log_set_wifi_ctx(sys_log_wifi_mode_t mode, uint8_t ap_clients,
                          int8_t sta_rssi, int8_t tx_dbm);

/** esp_restart() cagirmadan ONCE kasitli neden isaretle. */
void sys_log_set_restart_reason(sys_log_restart_t reason);

/** SNTP ile saat senkronlandiginda cagir; acilis zamani RTC'ye yazilir. */
void sys_log_time_synced(void);

/**
 * Calisma zamani olay satiri yaz (WiFi koptu, vb.). Flash'a hemen yazar.
 * level: "INFO" | "WARN" | "ERR"; cat: "wifi" | "power" | "sys" | "ota" ...
 */
void sys_log_write(const char *level, const char *cat, const char *fmt, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * Son kayitlari JSON dizisi olarak dondur (en yeni once).
 * Donen tampon malloc ile ayrilir, cagiran free() etmeli. Hata: NULL.
 */
char *sys_log_read_tail_json(int max_entries);

/** Tum kayitlari sil. */
esp_err_t sys_log_clear(void);

/** Bu acilisin numarasi (1'den baslar). */
uint32_t sys_log_boot_count(void);

/** Loga yazilmayan (<5 sn) kisa acilis sayisi (son uzun acilistan beri). */
uint32_t sys_log_short_boots(void);

/** Bu acilisin baslangic epoch'u (SNTP senkronu yoksa 0). */
uint32_t sys_log_boot_epoch(void);
