// =====================================================================
// SynDimm — main.c
// =====================================================================
// Döner-enkoderli evrensel kumanda. Boot sırası (şablon: BF main.c):
//   1) NVS init
//   2) sk_core (identity, CLI, event bus, errors, capabilities)
//   3) USB CLI transport (Serial/JTAG)
//   4) esp_pm (USB enumerasyonundan SONRA — yarış koşulu notu aşağıda)
//   5) sk_auth + sk_passphrase
//   6) WiFi STA + mDNS + BLE GATT + TCP NDJSON
//   7) sk_ota + sk_api
//   8) sd_app_init() — cihaza özgü sd_* bileşenleri (recovery-boot sınırı)
//   9) Pairing penceresi: her açılışta 60 sn (kilitli karar — jest yok)
//
// BF'den farklar:
//   - sk_button_init YOK: tek fiziksel buton enkoder SW'si, sahibi
//     sd_encoder. Kontrol bantları (≥5 sn restart / ≥10 sn factory reset)
//     sd_encoder'ın YALNIZCA uzun bırakmalarda yayınladığı sentetik
//     "button.released" olayıyla sk_control'e akar (plan: Buton bantları).
//   - gpio_wakeup YOK: sabit güç, uyandırma kaynağına gerek yok.
//   - Her boot'ta sk_auth_open_pairing_mode(60): yeni telefon eşleştirme
//     akışı "5 sn bas → restart → ilk 60 sn açık". Pencere kapanınca
//     kurulu bağlantılar KOPMAZ (yalnızca yeni eşleşme kabulü kapanır).
// =====================================================================

#include <string.h>

#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_pm.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include "sk_core.h"   // umbrella — tüm public sk_core API'si (sk_ota dahil)
#include "sk_api.h"    // giden HTTP (webhook'lar + SKAPP system slotları)
#include "sk_log.h"    // yapılandırılmış olay logu (boot reason vb.)

#include "sd_pins.h"
#include "sd_prefs.h"
#include "sd_buzzer.h"
#include "sd_encoder.h"
#include "sd_feedback.h"
#include "sd_profiles.h"
#include "sd_proto.h"
#include "sd_behaviors.h"
#include "sd_mode_engine.h"

// === EDIT [Identity] =================================================
//
// 2 karakter büyük harf cihaz tipi kodu. "SD" = SynDimm.
#define SK_DEVICE_TYPE_PREFIX   "SD"

// Donanım revizyonu. 'B' = harici flash'ı çıkarılmış ikinci PCB.
#define SK_HW_REV               'B'

// CLI banner'ında görünen ürün adı:
//   SD-XXXXXXXX - SmartKraft SynDimm v2.0.0 (...)
#define SK_PRODUCT_NAME         "SynDimm"

// Firmware sürümü — semver. Eski web-GUI'li seri 0.0.x idi; neue = 2.x.
// NOT: version.txt (esp_app_desc) ve GitHub Releases manifest.json ile
// senkron tutulacak (plan T6.3).
#define SK_FW_VERSION           "2.0.0"

// Opsiyonel build etiketi (git sha / CI no). NULL = yok.
#define SK_BUILD_INFO           NULL
//
// === EDIT [Wireless] =================================================
//
// TCP NDJSON portu — mDNS'te `_skapp._tcp` olarak da duyurulur.
#define SK_TCP_PORT             8080
//
// === EDIT [Pairing] ==================================================
//
// Her açılışta eşleşme penceresi süresi (sn). Kilitli karar: jest yok,
// yeni telefon = 5 sn bas (restart) → ilk PAIRING_BOOT_WINDOW_SEC açık.
#define SD_PAIRING_BOOT_WINDOW_SEC  60
//
// === EDIT [Optional features] ========================================
#define SK_API_ENABLE           1   // Giden HTTP (safe endpoint'leri + SKAPP)
//
// === EDIT [sk_ota] ===================================================
//
// Manifest-driven OTA. Boş URL → sk_ota_init no-op (runtime'da disabled).
// T6.3'te gerçek URL: github.com/smrtkrft/SynDimm releases latest.
#define SK_OTA_ENABLE           1
#define SK_OTA_MANIFEST_URL     ""
// =====================================================================

// Derleme zamanı korumaları — EDIT bloğu yazım hataları şimdi yakalanır.
_Static_assert(sizeof(SK_DEVICE_TYPE_PREFIX) == 3,
               "SK_DEVICE_TYPE_PREFIX must be exactly 2 ASCII characters");
_Static_assert(SK_HW_REV >= 'A' && SK_HW_REV <= 'Z',
               "SK_HW_REV must be an uppercase letter 'A' through 'Z'");
_Static_assert(SK_TCP_PORT > 0 && SK_TCP_PORT < 65536,
               "SK_TCP_PORT must be a valid TCP port (1-65535)");
_Static_assert(SD_PIN_ENC_CLK != SD_PIN_BOOT_RESERVED &&
               SD_PIN_ENC_DT  != SD_PIN_BOOT_RESERVED &&
               SD_PIN_ENC_SW  != SD_PIN_BOOT_RESERVED &&
               SD_PIN_BUZZER  != SD_PIN_BOOT_RESERVED,
               "sd_pins collides with the ESP32-C6 boot pin (GPIO9)");

static const char *TAG = "main";

// === Recovery boot (plan T6.1) =======================================
// RTC_NOINIT: yeniden başlatmada korunur, güç kesintisinde çöp kalır —
// magic ile ayırt edilir. 3 ardışık kirli reset (PANIC/WDT/...) →
// sd_app_init(recovery=true): slot/davranış/proto yüklenmez, cihaz
// transportlar + CLI + OTA ile erişilebilir kalır ("tuğlalaşmaz" ilkesi).
// 120 sn stabil çalışma sayacı sıfırlar.
#define SD_CRASH_MAGIC        0x53444E45u   // "SDNE"
#define SD_CRASH_LIMIT        3
// Stabil-uptime eşiği: bu kadar kesintisiz çalışınca crash sayacı sıfırlanır.
// 120 sn → 10 dk yükseltildi (uzun-çalışma P1/B3): eşikten HIZLI (periyodu <10
// dk) crash-loop'lar da sayacı biriktirip recovery'ye girer; eski 120 sn'de
// periyodu 2-10 dk arası yavaş loop'lar sayacı her boot sıfırlayıp kaçıyordu.
#define SD_STABLE_UPTIME_MS   600000

static RTC_NOINIT_ATTR uint32_t s_crash_magic;
static RTC_NOINIT_ATTR uint32_t s_crash_count;
static bool s_recovery_boot;

static void stable_uptime_cb(void *arg)
{
    (void)arg;
    if (s_crash_count) {
        ESP_LOGI(TAG, "stabil uptime — crash sayaci sifirlandi (%u idi)",
                 (unsigned)s_crash_count);
    }
    s_crash_count = 0;
}

// === Sağlık izleme (uzun-çalışma P1/B4) ==============================
// Periyodik heap + task yığın high-water örneklemesi. Yavaş bir leak veya
// fragmantasyon "yalnız OOM crash'inde fark et" yerine önceden görünür olur.
// Eşik altına düşünce `heap.low` olayı yayınlanır (SKAPP loglar); soak için
// periyodik INFO satırı da basılır. Timer callback'i bloklamaz.
#define SD_HEALTH_PERIOD_MS      60000    // 60 sn'de bir örnek
#define SD_HEALTH_INFO_EVERY     10       // her 10 örnekte (~10 dk) INFO satırı
#define SD_HEALTH_HEAP_LOW       20000    // boş heap bunun altına düşerse uyar
#define SD_HEALTH_BLOCK_LOW      12000    // en büyük blok bunun altına düşerse uyar

static void health_cb(void *arg)
{
    (void)arg;
    static uint32_t tick;
    uint32_t freeb   = (uint32_t)esp_get_free_heap_size();
    uint32_t minfree = (uint32_t)esp_get_minimum_free_heap_size();
    uint32_t largest = (uint32_t)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

    // Task yığın high-water (kelime cinsinden min boş) — sd_input 3 KB marjı
    // (C5) ve sd_cmd 6 KB gözlemi. Handle'ları ada göre çöz (INCLUDE_xTaskGetHandle).
    TaskHandle_t tin  = xTaskGetHandle("sd_input");
    TaskHandle_t tcmd = xTaskGetHandle("sd_cmd");
    unsigned hw_in  = tin  ? (unsigned)uxTaskGetStackHighWaterMark(tin)  : 0;
    unsigned hw_cmd = tcmd ? (unsigned)uxTaskGetStackHighWaterMark(tcmd) : 0;

    if (freeb < SD_HEALTH_HEAP_LOW || largest < SD_HEALTH_BLOCK_LOW) {
        ESP_LOGW(TAG, "DUSUK HEAP: free=%u largest=%u min=%u",
                 (unsigned)freeb, (unsigned)largest, (unsigned)minfree);
        sk_event_bus_publishf("heap.low",
            "{\"free\":%u,\"largest\":%u,\"min_free\":%u}",
            (unsigned)freeb, (unsigned)largest, (unsigned)minfree);
    }
    if ((tick % SD_HEALTH_INFO_EVERY) == 0) {
        ESP_LOGI(TAG, "health: free=%u largest=%u min=%u stack(in=%u cmd=%u)",
                 (unsigned)freeb, (unsigned)largest, (unsigned)minfree,
                 hw_in, hw_cmd);
    }
    tick++;
}

// CLI banner durum satırı: SD-XXXX - SmartKraft SynDimm v2.0.0 (wifi: ...)
// Batarya yok (sabit güç); recovery durumunda başa bayrak düşer.
static size_t sd_status_line(char *out, size_t cap)
{
    sk_wifi_status_t w;
    sk_wifi_status(&w);
    return (size_t)snprintf(out, cap, "%swifi: %s",
                            s_recovery_boot ? "RECOVERY, " : "",
                            w.connected ? "connected" : "off");
}

// --- Gizli tanılama komutları (recovery testi + soak izleme) -----------
static sk_err_t cmd_debug_panic(sk_cli_ctx_t *ctx)
{
    sk_cli_ok(ctx, "{\"panic\":\"now\"}");
    vTaskDelay(pdMS_TO_TICKS(100));   // cevap gitsin
    abort();                          // kasıtlı panik — recovery sayacı test
}

static sk_err_t cmd_debug_heap(sk_cli_ctx_t *ctx)
{
    char buf[96];
    snprintf(buf, sizeof(buf),
             "{\"free\":%u,\"min_free\":%u,\"largest\":%u}",
             (unsigned)esp_get_free_heap_size(),
             (unsigned)esp_get_minimum_free_heap_size(),
             (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_8BIT));
    sk_cli_ok(ctx, buf);
    return SK_OK;
}

static const sk_cli_command_t s_cmd_debug_panic = {
    .name = "debug.panic", .summary = "Force a panic (recovery-boot test)",
    .usage = "debug panic", .hidden = true, .critical = true,
    .handler = cmd_debug_panic,
};
static const sk_cli_command_t s_cmd_debug_heap = {
    .name = "debug.heap", .summary = "Heap watermark",
    .usage = "debug heap", .hidden = true, .handler = cmd_debug_heap,
};

// Cihaza özgü sd_* bileşen init sınırı. T6.1 recovery boot bu fonksiyonu
// sarar: recovery=true iken CLI komutları kayıtlı kalır ama slot/davranış/
// proto yüklenmez (cihaz transportlar + OTA ile erişilebilir kalır).
// Faz 1-4 görevleri init çağrılarını buraya ekleyecek.
static void sd_app_init(bool recovery)
{
    // Prefs ve buzzer recovery modunda da yüklenir — zararsız ve cihazın
    // geri bildirim verebilmesi istenir. Recovery'nin atladıkları:
    // slot/davranış/proto (T6.1). Sıra önemli: buzzer, prefs'i okur.
    ESP_ERROR_CHECK(sd_prefs_init());
    ESP_ERROR_CHECK(sd_buzzer_init());
    // Enkoder recovery'de de canlı: kontrol bantları (restart/factory reset)
    // her koşulda çalışmalı — cihaz tuğlalaşmaz ilkesinin parçası.
    ESP_ERROR_CHECK(sd_encoder_init());
    ESP_ERROR_CHECK(sd_feedback_init());
    // Profil deposu recovery'de de açık: bozuk konfigürasyon CLI'dan
    // düzeltilebilsin (recovery'nin atladığı şey slot/davranış YÜKLEMEsi).
    ESP_ERROR_CHECK(sd_profiles_init());
    // Motor CLI'sı her boot'ta kayıtlı (mode.set recovery'de kalıcılaştırır).
    ESP_ERROR_CHECK(sd_mode_engine_init(recovery));

    if (recovery) return;   // ↓ recovery'de yüklenmeyenler (plan T6.1)

    ESP_ERROR_CHECK(sd_proto_init());
    ESP_ERROR_CHECK(sd_behaviors_init());
    ESP_ERROR_CHECK(sd_mode_engine_start());   // slotlar + sd_cmd task
}

void app_main(void)
{
    // Gürültülü stack loglarını sustur (gerekçeler: BF main.c:127-157).
    esp_log_level_set("NimBLE",      ESP_LOG_WARN);
    esp_log_level_set("wifi",        ESP_LOG_ERROR);
    esp_log_level_set("wifi_init",   ESP_LOG_WARN);
    esp_log_level_set("BLE_INIT",    ESP_LOG_WARN);
    esp_log_level_set("pp",          ESP_LOG_WARN);
    esp_log_level_set("net80211",    ESP_LOG_WARN);
    esp_log_level_set("phy",         ESP_LOG_WARN);
    esp_log_level_set("phy_init",    ESP_LOG_WARN);
    esp_log_level_set("pm",          ESP_LOG_WARN);
    esp_log_level_set("sleep_clock", ESP_LOG_WARN);
    esp_log_level_set("sleep_gpio",  ESP_LOG_WARN);

    // NVS bootstrap — hemen her sk_* kütüphanesi NVS kullanır.
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // sk_core: identity, CLI, event bus, errors, capabilities.
    ESP_ERROR_CHECK(sk_core_init(&(sk_core_cfg_t){
        .device_type_prefix = SK_DEVICE_TYPE_PREFIX,
        .hw_rev             = SK_HW_REV,
        .fw_version         = SK_FW_VERSION,
        .build_info         = SK_BUILD_INFO,
    }));

    sk_core_set_product(SK_PRODUCT_NAME, NULL);  // sürüm fw_version'a düşer
    sk_core_set_status_provider(sd_status_line);

    // Boot reason olayı — PANIC/WDT/BROWNOUT önceki koşunun beklenmedik
    // öldüğü anlamına gelir; logs.get'te görünmeli. (T6.1 recovery boot
    // sayacı da aynı reset-reason okumasına bağlanacak.)
    {
        esp_reset_reason_t rr = esp_reset_reason();
        const char *rr_name = "UNKNOWN";
        bool       unclean  = false;   // log için (beklenmedik ölüm)
        bool       power_ev = false;   // güç kaynağı olayı — firmware crash'i DEĞİL
        switch (rr) {
            case ESP_RST_POWERON:    rr_name = "POWERON";    break;
            case ESP_RST_EXT:        rr_name = "EXT";        break;
            case ESP_RST_SW:         rr_name = "SW";         break;
            case ESP_RST_DEEPSLEEP:  rr_name = "DEEPSLEEP";  break;
            case ESP_RST_PANIC:      rr_name = "PANIC";      unclean = true; break;
            case ESP_RST_INT_WDT:    rr_name = "INT_WDT";    unclean = true; break;
            case ESP_RST_TASK_WDT:   rr_name = "TASK_WDT";   unclean = true; break;
            case ESP_RST_WDT:        rr_name = "WDT";        unclean = true; break;
            case ESP_RST_BROWNOUT:   rr_name = "BROWNOUT";   unclean = true; power_ev = true; break;
            case ESP_RST_SDIO:       rr_name = "SDIO";       break;
#ifdef ESP_RST_USB
            case ESP_RST_USB:        rr_name = "USB";        break;
#endif
#ifdef ESP_RST_JTAG
            case ESP_RST_JTAG:       rr_name = "JTAG";       break;
#endif
#ifdef ESP_RST_EFUSE
            case ESP_RST_EFUSE:      rr_name = "EFUSE";      unclean = true; break;
#endif
#ifdef ESP_RST_PWR_GLITCH
            case ESP_RST_PWR_GLITCH: rr_name = "PWR_GLITCH"; unclean = true; power_ev = true; break;
#endif
#ifdef ESP_RST_CPU_LOCKUP
            case ESP_RST_CPU_LOCKUP: rr_name = "CPU_LOCKUP"; unclean = true; break;
#endif
            default: break;
        }
        if (unclean) {
            SK_LOG_W("boot", "unclean", "reset=%s fw=%s", rr_name, SK_FW_VERSION);
        } else {
            SK_LOG_I("boot", "up", "reset=%s fw=%s", rr_name, SK_FW_VERSION);
        }

        // Recovery boot sayacı (T6.1). Güç kesintisi RTC alanını bozar —
        // magic tutmuyorsa sıfırdan başla.
        if (s_crash_magic != SD_CRASH_MAGIC) {
            s_crash_magic = SD_CRASH_MAGIC;
            s_crash_count = 0;
        }
        if (unclean && !power_ev) {
            // Gerçek firmware faultu (PANIC/WDT/CPU_LOCKUP/...) → say.
            s_crash_count++;
            ESP_LOGW(TAG, "kirli reset #%u", (unsigned)s_crash_count);
        } else if (!unclean) {
            // Temiz reset → sıfırla.
            s_crash_count = 0;
        }
        // power_ev (BROWNOUT/PWR_GLITCH): güç sorunu, firmware crash'i değil —
        // sayaç değişmez (marjinal güçte yanlış recovery'yi önler).
        s_recovery_boot = (s_crash_count >= SD_CRASH_LIMIT);
        if (s_recovery_boot) {
            ESP_LOGE(TAG, "RECOVERY BOOT: %u ardisik crash — slotlar/proto "
                          "yuklenmeyecek, transportlar acik",
                     (unsigned)s_crash_count);
        }
    }

    // Topic kataloğu — `help` görünüm sırası. Üç kova:
    //   SYSTEM — tüm SmartKraft cihazlarında ortak taban
    //   SKAPP  — eşleşmiş telefon yönetimi
    //   DEVICE — SynDimm'in işi: mod slotları, profiller, safe, tercihler
    sk_cli_register_topic("wifi",    "Network connection",                              "SYSTEM");
    sk_cli_register_topic("ble",     "Bluetooth transport",                             "SYSTEM");
    sk_cli_register_topic("ota",     "Firmware updates (check / install / rollback)",   "SYSTEM");
    sk_cli_register_topic("device",  "Identity, restart, factory reset",                "SYSTEM");
    sk_cli_register_topic("logs",    "Log entries (ring buffer)",                       "SYSTEM");

    sk_cli_register_topic("pairing", "SKAPP pairing window (open / status / close)",    "SKAPP");
    sk_cli_register_topic("auth",    "SKAPP connection passphrase (set / change / mode)","SKAPP");
    sk_cli_register_topic("bond",    "Paired SKAPP installs (list / remove)",           "SKAPP");

    sk_cli_register_topic("mode",    "Mode slots (bind / select / test / value)",       "DEVICE");
    sk_cli_register_topic("profile", "Target device profiles (catalog)",                "DEVICE");
    sk_cli_register_topic("safe",    "Sequence lock → webhook trigger",                 "DEVICE");
    sk_cli_register_topic("prefs",   "Feature switches (gestures / buzzer / quiet)",    "DEVICE");
    sk_cli_register_topic("api",     "Outbound webhook presets",                        "DEVICE");

    ESP_ERROR_CHECK(sk_transport_usb_init(NULL));

    // PM aktivasyonu USB init SONRASI — aksi halde USB Serial/JTAG
    // enumerasyon yarışını kaybediyor (gerekçe: BF main.c:250-267).
    esp_pm_config_t pm_cfg = {
        .max_freq_mhz       = 160,
        .min_freq_mhz       = 40,
        .light_sleep_enable = true,
    };
    ESP_ERROR_CHECK(esp_pm_configure(&pm_cfg));

    // Auth abonelikleri cihaz bileşenlerinden önce kurulmalı ki boot
    // sırasındaki bir olay düşmesin.
    ESP_ERROR_CHECK(sk_auth_init());
    ESP_ERROR_CHECK(sk_passphrase_init());

    // Kablosuz yığın.
    ESP_ERROR_CHECK(sk_wifi_init());
    ESP_ERROR_CHECK(sk_mdns_init(SK_TCP_PORT, SK_FW_VERSION));
    ESP_ERROR_CHECK(sk_transport_ble_init(NULL));
    ESP_ERROR_CHECK(sk_transport_tcp_init(&(sk_transport_tcp_cfg_t){
        .port = SK_TCP_PORT,
    }));

#if SK_OTA_ENABLE
    ESP_ERROR_CHECK(sk_ota_init(&(sk_ota_cfg_t){
        .fw_version   = SK_FW_VERSION,
        .manifest_url = SK_OTA_MANIFEST_URL[0] ? SK_OTA_MANIFEST_URL : NULL,
    }));
#endif

#if SK_API_ENABLE
    ESP_ERROR_CHECK(sk_api_init());
#endif

    // Gizli tanılama komutları (recovery testi + soak izleme).
    sk_cli_register(&s_cmd_debug_panic);
    sk_cli_register(&s_cmd_debug_heap);

    // Cihaza özgü bileşenler (recovery-boot sınırı — T6.1).
    sd_app_init(s_recovery_boot);

    if (s_recovery_boot) {
        sk_event_bus_publishf("device.recovery", "{\"count\":%u}",
                              (unsigned)s_crash_count);
    }

    // Stabil çalışma → crash sayacı sıfır (tek atımlık, SD_STABLE_UPTIME_MS).
    {
        const esp_timer_create_args_t targs = {
            .callback = stable_uptime_cb,
            .name     = "sd_stable",
        };
        esp_timer_handle_t t;
        if (esp_timer_create(&targs, &t) == ESP_OK) {
            esp_timer_start_once(t, (uint64_t)SD_STABLE_UPTIME_MS * 1000);
        }
    }

    // Periyodik sağlık izleme (heap + task yığın high-water) — soak/erken uyarı.
    {
        const esp_timer_create_args_t hargs = {
            .callback = health_cb,
            .name     = "sd_health",
        };
        esp_timer_handle_t h;
        if (esp_timer_create(&hargs, &h) == ESP_OK) {
            esp_timer_start_periodic(h, (uint64_t)SD_HEALTH_PERIOD_MS * 1000);
        }
    }

    // Kilitli karar [Pairing]: her açılışta 60 sn eşleşme penceresi.
    // BLE transport init'ten SONRA çağrılır ki 'par' reklamı hemen başlasın.
    // Pencere dolunca kurulu bağlantılar kopmaz; bond'lular her an bağlanır.
    //
    // DONANIM BULGUSU (2026-07-03): bond YOKKEN sk_auth_init pencereyi ZATEN
    // otomatik açıyor; bu ikinci çağrı ESP_ERR_INVALID_STATE dönüyordu ve
    // ESP_ERROR_CHECK abort ediyordu → her fabrika-taze cihaz boot-loop.
    // Pencerenin zaten açık olması bizim için BAŞARI: hoş gör, sadece
    // beklenmeyen gerçek hatalarda abort et.
    esp_err_t pair_err = sk_auth_open_pairing_mode(SD_PAIRING_BOOT_WINDOW_SEC);
    if (pair_err != ESP_OK && pair_err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(pair_err);
    }

    ESP_LOGI(TAG, "SynDimm neue up — id=%s fw=%s%s",
             sk_identity_get(), SK_FW_VERSION,
             s_recovery_boot ? " [RECOVERY]" : "");
}
