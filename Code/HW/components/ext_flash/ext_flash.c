/**
 * SynDimm - Harici Flash Modulu
 * Macronix MX25L SPI Flash (16 MB veya 32 MB) - 3 Partition Yapisi
 *
 * Partition Haritasi (8 MB aktif / kalan yedek):
 *   Slot A    : 0x000000 - 0x3FFFFF  (4 MB)  - GUI + Kullanici verileri (LittleFS)
 *   Slot B    : 0x400000 - 0x7FFFFF  (4 MB)  - GUI OTA slot (LittleFS)
 *   Yedek     : 0x800000 - cip sonu           - Loglar, yedekler (16MB: 8MB, 32MB: 24MB)
 *
 * Pin Baglantilari:
 *   CS   = D3 = GPIO21
 *   MISO = D0 = GPIO0
 *   MOSI = D4 = GPIO22
 *   SCLK = D6 = GPIO16
 */

#include "ext_flash.h"
#include "esp_log.h"
#include "esp_littlefs.h"
#include "driver/spi_common.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "esp_flash.h"
#include "esp_flash_spi_init.h"
#include "nvs_flash.h"
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <errno.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/task.h"

static const char *TAG = "EXT_FLASH";

// SPI Pin tanimlamalari
#define EXT_FLASH_SPI_HOST   SPI2_HOST
/* Varsayilan pinler (XIAO ESP32-C6: D3, D0, D4, D6). Gercek eslesme NVS'den
 * ("ext_flash" ad alani) yuklenir; cip yanit vermezse 24 rol kombinasyonu taranir. */
#define PIN_CS_DEFAULT       21   // D3 = GPIO21
#define PIN_MISO_DEFAULT     0    // D0 = GPIO0
#define PIN_MOSI_DEFAULT     22   // D4 = GPIO22
#define PIN_SCLK_DEFAULT     16   // D6 = GPIO16

static int PIN_CS   = PIN_CS_DEFAULT;
static int PIN_MISO = PIN_MISO_DEFAULT;
static int PIN_MOSI = PIN_MOSI_DEFAULT;
static int PIN_SCLK = PIN_SCLK_DEFAULT;

// ============================================================
// Partition Haritasi (8MB aktif + kalan yedek; cip boyutu calisma zamaninda okunur)
//
// Aktif alan (0x000000 - 0x7FFFFF = 8 MB):
//   Slot A    : 4 MB  - GUI Web + Kullanici verileri
//   Slot B    : 4 MB  - GUI OTA + Kullanici verileri yedek
//
// Yedek alan (0x800000 - cip sonu): /reserve/logs, /reserve/backup ...
// ============================================================
#define SLOT_A_OFFSET        0x000000
#define SLOT_A_SIZE          (4 * 1024 * 1024)    // 4 MB
#define SLOT_A_LABEL         "slot_a"
#define SLOT_A_MOUNT         "/slot_a"

#define SLOT_B_OFFSET        0x400000
#define SLOT_B_SIZE          (4 * 1024 * 1024)    // 4 MB
#define SLOT_B_LABEL         "slot_b"
#define SLOT_B_MOUNT         "/slot_b"

// Yedek alan: 0x800000'den cipin sonuna kadar. Boyut calisma zamaninda
// cip kapasitesinden hesaplanir (16 MB cip -> 8 MB, 32 MB cip -> 24 MB).
#define RESERVE_OFFSET       0x800000
#define RESERVE_MIN_SIZE     (1 * 1024 * 1024)    // Bundan kucukse mount edilmez
#define RESERVE_LABEL        "reserve"
#define RESERVE_MOUNT        "/reserve"

static uint32_t s_ext_flash_size = 0;   // esp_flash_get_size() sonucu (byte)
static uint32_t s_reserve_size = 0;     // Hesaplanan yedek alan boyutu (byte)

static esp_flash_t *s_ext_flash = NULL;
static bool s_reserve_mounted = false;
static bool s_slot_a_mounted = false;
static bool s_slot_b_mounted = false;
static bool s_user_data_mounted = false;
static char s_active_slot = 'a';  // NVS'den yuklenir

#define NVS_GUI_OTA_NS "sd_gui_ota"

// NVS'den aktif slot bilgisini yukle
static void load_active_slot(void)
{
    nvs_handle_t nvs;
    if (nvs_open(NVS_GUI_OTA_NS, NVS_READONLY, &nvs) == ESP_OK) {
        char slot[4] = {0};
        size_t len = sizeof(slot);
        if (nvs_get_str(nvs, "active_slot", slot, &len) == ESP_OK) {
            if (slot[0] == 'b') {
                s_active_slot = 'b';
            }
        }
        nvs_close(nvs);
    }
    ESP_LOGI(TAG, "Aktif GUI slot: %c", s_active_slot);
}

/*
 * LittleFS'i GUVENLI mount et.
 * Eski davranis format_if_mount_failed=true idi: SPI hattinda gecici bir okuma
 * hatasi (sinyal butunlugu, besleme dusmesi) Slot A'yi -> GUI + profiller +
 * modlar -> aninda biciyordu. Simdi: 3 deneme; hala olmuyorsa bolumun basi
 * tamamen 0xFF ise (hic yazilmamis cip) formatla, degilse DOKUNMA ve hata don.
 */
static esp_err_t mount_littlefs_safe(const esp_partition_t *part, const char *label,
                                     const char *base_path)
{
    esp_vfs_littlefs_conf_t conf = {
        .base_path = base_path,
        .partition_label = label,
        .format_if_mount_failed = false,
        .dont_mount = false,
    };
    esp_err_t ret = ESP_FAIL;
    for (int attempt = 1; attempt <= 3; attempt++) {
        ret = esp_vfs_littlefs_register(&conf);
        if (ret == ESP_OK) return ESP_OK;
        ESP_LOGW(TAG, "%s mount denemesi %d/3 basarisiz: %s", label, attempt, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    /* Bos cip tespiti: ilk 4 KB tamamen 0xFF mi? */
    bool blank = true;
    uint8_t *probe = malloc(4096);
    if (probe && part && esp_partition_read(part, 0, probe, 4096) == ESP_OK) {
        for (int i = 0; i < 4096; i++) {
            if (probe[i] != 0xFF) { blank = false; break; }
        }
    } else {
        blank = false;
    }
    free(probe);

    if (blank) {
        ESP_LOGW(TAG, "%s bos gorunuyor, formatlaniyor (ilk kurulum)", label);
        conf.format_if_mount_failed = true;
        ret = esp_vfs_littlefs_register(&conf);
    } else {
        ESP_LOGE(TAG, "%s mount edilemedi ve veri iceriyor: FORMAT YAPILMADI (veri korundu)", label);
    }
    return ret;
}

/* Verilen pinlerle bus'i ac, cipi resetle (0xAB,0x66,0x99), JEDEC ID oku, bus'i kapat.
 * esp_flash surucusu kullanilmaz: bus temiz kalir, tekrar tekrar cagrilabilir. */
static bool raw_jedec_id(int cs, int miso, int mosi, int sclk, uint8_t id[3])
{
    id[0] = id[1] = id[2] = 0;
    spi_bus_config_t bus = { .mosi_io_num = mosi, .miso_io_num = miso, .sclk_io_num = sclk,
                             .quadwp_io_num = -1, .quadhd_io_num = -1, .max_transfer_sz = 64 };
    esp_err_t e = spi_bus_initialize(EXT_FLASH_SPI_HOST, &bus, SPI_DMA_DISABLED);
    if (e != ESP_OK) { ESP_LOGW(TAG, "raw: bus init %s", esp_err_to_name(e)); return false; }
    spi_device_interface_config_t dcfg = { .clock_speed_hz = 1000000, .mode = 0, .spics_io_num = cs, .queue_size = 1 };
    spi_device_handle_t dev = NULL;
    bool ok = false;
    e = spi_bus_add_device(EXT_FLASH_SPI_HOST, &dcfg, &dev);
    if (e != ESP_OK) ESP_LOGW(TAG, "raw: add device %s", esp_err_to_name(e));
    if (e == ESP_OK) {
        const uint8_t seq[] = { 0xAB, 0x66, 0x99 };
        for (size_t i = 0; i < sizeof(seq); i++) {
            spi_transaction_t tr = { .length = 8, .tx_buffer = &seq[i] };
            spi_device_polling_transmit(dev, &tr);
            esp_rom_delay_us(100);
        }
        esp_rom_delay_us(2000);   /* tRST; vTaskDelay(2) 100 Hz tick'te 0 olurdu */
        uint8_t tx[4] = { 0x9F, 0, 0, 0 }, rx[4] = { 0 };
        spi_transaction_t tr = { .length = 32, .tx_buffer = tx, .rx_buffer = rx };
        e = spi_device_polling_transmit(dev, &tr);
        if (e == ESP_OK) {
            id[0] = rx[1]; id[1] = rx[2]; id[2] = rx[3];
            ok = true;
        } else {
            ESP_LOGW(TAG, "raw: transmit %s", esp_err_to_name(e));
        }
        spi_bus_remove_device(dev);
    }
    e = spi_bus_free(EXT_FLASH_SPI_HOST);
    if (e != ESP_OK) ESP_LOGW(TAG, "raw: bus free %s", esp_err_to_name(e));
    return ok;
}

static bool jedec_valid(const uint8_t id[3])
{
    bool all0 = (id[0] == 0 && id[1] == 0 && id[2] == 0);
    bool allf = (id[0] == 0xFF && id[1] == 0xFF && id[2] == 0xFF);
    return !all0 && !allf && id[0] != 0 && id[0] != 0xFF;
}

static void load_pins_from_nvs(void)
{
    nvs_handle_t nvs;
    if (nvs_open("ext_flash", NVS_READONLY, &nvs) != ESP_OK) return;
    uint8_t v;
    if (nvs_get_u8(nvs, "cs", &v) == ESP_OK)   PIN_CS = v;
    if (nvs_get_u8(nvs, "miso", &v) == ESP_OK) PIN_MISO = v;
    if (nvs_get_u8(nvs, "mosi", &v) == ESP_OK) PIN_MOSI = v;
    if (nvs_get_u8(nvs, "sclk", &v) == ESP_OK) PIN_SCLK = v;
    nvs_close(nvs);
}

static void save_pins_to_nvs(void)
{
    nvs_handle_t nvs;
    if (nvs_open("ext_flash", NVS_READWRITE, &nvs) != ESP_OK) return;
    nvs_set_u8(nvs, "cs", PIN_CS); nvs_set_u8(nvs, "miso", PIN_MISO);
    nvs_set_u8(nvs, "mosi", PIN_MOSI); nvs_set_u8(nvs, "sclk", PIN_SCLK);
    nvs_commit(nvs);
    nvs_close(nvs);
}

/* Dort pinin 24 rol kombinasyonunu tara; gecerli JEDEC ID veren ilkini sec. */
static bool probe_pin_roles(void)
{
    const int pins[4] = { PIN_CS, PIN_MISO, PIN_MOSI, PIN_SCLK };
    static const int perm[24][4] = {
        {0,1,2,3},{0,1,3,2},{0,2,1,3},{0,2,3,1},{0,3,1,2},{0,3,2,1},
        {1,0,2,3},{1,0,3,2},{1,2,0,3},{1,2,3,0},{1,3,0,2},{1,3,2,0},
        {2,0,1,3},{2,0,3,1},{2,1,0,3},{2,1,3,0},{2,3,0,1},{2,3,1,0},
        {3,0,1,2},{3,0,2,1},{3,1,0,2},{3,1,2,0},{3,2,0,1},{3,2,1,0} };
    ESP_LOGW(TAG, "Pin rolleri taraniyor (%d, %d, %d, %d)...", pins[0], pins[1], pins[2], pins[3]);
    for (int i = 0; i < 24; i++) {
        int cs = pins[perm[i][0]], miso = pins[perm[i][1]], mosi = pins[perm[i][2]], sclk = pins[perm[i][3]];
        uint8_t id[3];
        if (raw_jedec_id(cs, miso, mosi, sclk, id) && jedec_valid(id)) {
            ESP_LOGW(TAG, "BULUNDU: CS=%d MISO=%d MOSI=%d SCLK=%d -> JEDEC %02X %02X %02X",
                     cs, miso, mosi, sclk, id[0], id[1], id[2]);
            PIN_CS = cs; PIN_MISO = miso; PIN_MOSI = mosi; PIN_SCLK = sclk;
            save_pins_to_nvs();
            return true;
        }
    }
    ESP_LOGE(TAG, "Hicbir pin kombinasyonunda cip yanit vermedi: besleme (3.3V/GND) veya cip arizali");
    return false;
}

esp_err_t ext_flash_init(void)
{
    ESP_LOGI(TAG, "Harici flash baslatiliyor...");
    load_pins_from_nvs();

    /* On kontrol: ham JEDEC ID. Gecersizse pin rollerini tara. */
    {
        uint8_t id[3];
        raw_jedec_id(PIN_CS, PIN_MISO, PIN_MOSI, PIN_SCLK, id);
        ESP_LOGI(TAG, "Ham JEDEC ID (1 MHz): %02X %02X %02X", id[0], id[1], id[2]);
        if (!jedec_valid(id) && !probe_pin_roles()) {
            return ESP_ERR_INVALID_RESPONSE;
        }
    }
    ESP_LOGI(TAG, "Pinler: CS=GPIO%d, MISO=GPIO%d, MOSI=GPIO%d, SCLK=GPIO%d",
             PIN_CS, PIN_MISO, PIN_MOSI, PIN_SCLK);

    // SPI bus konfigurasyonu
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 64 * 1024,
    };

    esp_err_t ret = spi_bus_initialize(EXT_FLASH_SPI_HOST, &bus_cfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SPI bus baslatilamadi: %s", esp_err_to_name(ret));
        return ret;
    }

    gpio_set_direction(PIN_CS, GPIO_MODE_OUTPUT);
    gpio_set_level(PIN_CS, 1);

    // Harici flash chip konfigurasyonu
    // NOT: MX25L25645G, DIO modunda >6 byte yazma bozuluyor (sinyal butunlugu)
    // SLOWRD + 20MHz stabil calisiyor
    esp_flash_spi_device_config_t dev_cfg = {
        .host_id = EXT_FLASH_SPI_HOST,
        .cs_io_num = PIN_CS,
        .io_mode = SPI_FLASH_SLOWRD,
        .freq_mhz = 20,
        .cs_id = 0,
    };

    ret = spi_bus_add_flash_device(&s_ext_flash, &dev_cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Flash cihazi eklenemedi: %s", esp_err_to_name(ret));
        return ret;
    }

    /* Cip kimligi ilk denemede okunamayabilir (guc oturmamis, cip onceki
     * calismadan tuhaf bir durumda kalmis). 3 deneme, aralarda kisa bekleme. */
    for (int attempt = 1; attempt <= 3; attempt++) {
        ret = esp_flash_init(s_ext_flash);
        if (ret == ESP_OK) break;
        ESP_LOGW(TAG, "Flash init denemesi %d/3 basarisiz: %s", attempt, esp_err_to_name(ret));
        vTaskDelay(pdMS_TO_TICKS(150));
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Flash init hatasi: %s (cip yanit vermiyor: baglanti/besleme/cip kontrol edin)",
                 esp_err_to_name(ret));
        return ret;
    }

    // Flash boyutunu kontrol et
    uint32_t flash_size;
    ret = esp_flash_get_size(s_ext_flash, &flash_size);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Flash boyutu alinamadi: %s", esp_err_to_name(ret));
        return ret;
    }
    ESP_LOGI(TAG, "Harici flash boyutu: %lu MB", (unsigned long)(flash_size / (1024 * 1024)));
    s_ext_flash_size = flash_size;

    // Slot A + Slot B icin en az 8 MB gerekir
    if (flash_size < (uint32_t)(SLOT_B_OFFSET + SLOT_B_SIZE)) {
        ESP_LOGE(TAG, "Harici flash cok kucuk (%lu MB), en az 8 MB gerekli",
                 (unsigned long)(flash_size / (1024 * 1024)));
        return ESP_ERR_INVALID_SIZE;
    }
    // Yedek alan = cipin geri kalani (16 MB cipte 8 MB, 32 MB cipte 24 MB)
    s_reserve_size = (flash_size > RESERVE_OFFSET) ? (flash_size - RESERVE_OFFSET) : 0;

    // Yazma korumasi devre disi birak (MX25L25645G WP pini / status register)
    bool wp = false;
    esp_flash_get_chip_write_protect(s_ext_flash, &wp);
    if (wp) {
        ESP_LOGW(TAG, "Flash yazma korumasi aktif, devre disi birakiliyor...");
        ret = esp_flash_set_chip_write_protect(s_ext_flash, false);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Yazma korumasi kaldirilmadi: %s", esp_err_to_name(ret));
        } else {
            ESP_LOGI(TAG, "Yazma korumasi kaldirildi");
        }
    }

    // 2 Partition: Slot A (4MB GUI+Data) + Slot B (4MB OTA) + yedek (kalan)
    ESP_LOGI(TAG, "Partition: Slot A %dMB + Slot B %dMB + %luMB yedek",
             SLOT_A_SIZE / (1024 * 1024), SLOT_B_SIZE / (1024 * 1024),
             (unsigned long)(s_reserve_size / (1024 * 1024)));

    // Slot A partition (GUI + Kullanici verileri)
    const esp_partition_t *slot_a_part;
    ret = esp_partition_register_external(s_ext_flash, SLOT_A_OFFSET, SLOT_A_SIZE,
                                          SLOT_A_LABEL, ESP_PARTITION_TYPE_DATA,
                                          ESP_PARTITION_SUBTYPE_ANY, &slot_a_part);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Slot A partition kaydi hatasi: %s", esp_err_to_name(ret));
        return ret;
    }

    // Slot B partition (GUI OTA)
    const esp_partition_t *slot_b_part;
    ret = esp_partition_register_external(s_ext_flash, SLOT_B_OFFSET, SLOT_B_SIZE,
                                          SLOT_B_LABEL, ESP_PARTITION_TYPE_DATA,
                                          ESP_PARTITION_SUBTYPE_ANY, &slot_b_part);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Slot B partition kaydi hatasi: %s", esp_err_to_name(ret));
        return ret;
    }

    // ============================================================
    // LittleFS Mount - Slot A (GUI + Data)
    // ============================================================
    ret = mount_littlefs_safe(slot_a_part, SLOT_A_LABEL, SLOT_A_MOUNT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Slot A LittleFS mount hatasi: %s", esp_err_to_name(ret));
    } else {
        s_slot_a_mounted = true;
        s_user_data_mounted = true;  // User data artik Slot A icinde
        ESP_LOGI(TAG, "Slot A mount edildi: %s (LittleFS, 4MB, GUI+Data)", SLOT_A_MOUNT);
    }

    // ============================================================
    // LittleFS Mount - Slot B (GUI OTA)
    // ============================================================
    ret = mount_littlefs_safe(slot_b_part, SLOT_B_LABEL, SLOT_B_MOUNT);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Slot B LittleFS mount hatasi: %s", esp_err_to_name(ret));
    } else {
        s_slot_b_mounted = true;
        ESP_LOGI(TAG, "Slot B mount edildi: %s (LittleFS, 4MB, OTA)", SLOT_B_MOUNT);
    }

    if (!s_slot_a_mounted) {
        return ESP_FAIL;
    }

    // ============================================================
    // LittleFS Mount - Reserve (cipin kalani: 16MB cipte 8MB, 32MB cipte 24MB)
    // ============================================================
    const esp_partition_t *reserve_part;
    if (s_reserve_size < RESERVE_MIN_SIZE) {
        ESP_LOGW(TAG, "Yedek alan icin yer yok (%lu KB), atlaniyor",
                 (unsigned long)(s_reserve_size / 1024));
        ret = ESP_ERR_INVALID_SIZE;
    } else {
        ret = esp_partition_register_external(s_ext_flash, RESERVE_OFFSET, s_reserve_size,
                                              RESERVE_LABEL, ESP_PARTITION_TYPE_DATA,
                                              ESP_PARTITION_SUBTYPE_ANY, &reserve_part);
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Reserve partition kaydi hatasi: %s", esp_err_to_name(ret));
    } else {
        esp_vfs_littlefs_conf_t reserve_conf = {
            .base_path = RESERVE_MOUNT,
            .partition_label = RESERVE_LABEL,
            .format_if_mount_failed = true,
            .dont_mount = false,
        };
        ret = esp_vfs_littlefs_register(&reserve_conf);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Reserve LittleFS mount hatasi: %s", esp_err_to_name(ret));
        } else {
            s_reserve_mounted = true;
            ESP_LOGI(TAG, "Reserve mount edildi: %s (LittleFS, %luMB)", RESERVE_MOUNT,
                     (unsigned long)(s_reserve_size / (1024 * 1024)));
            // Dizin yapisi olustur
            mkdir(RESERVE_MOUNT "/logs", 0755);
            mkdir(RESERVE_MOUNT "/backup", 0755);
            mkdir(RESERVE_MOUNT "/archive", 0755);
            mkdir(RESERVE_MOUNT "/profiles", 0755);
        }
    }

    // NVS'den aktif slot bilgisini yukle
    load_active_slot();

    return ESP_OK;
}

// ============================================================
// Slot A (GUI) dosya islemleri - /slot_a mount noktasi
// ============================================================

int ext_flash_slot_a_read_file(const char *path, char *buf, size_t buf_size)
{
    if (!s_slot_a_mounted || !path || !buf) {
        return -1;
    }

    char full_path[128];
    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", SLOT_A_MOUNT, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", SLOT_A_MOUNT, path);
    }

    FILE *f = fopen(full_path, "rb");
    if (!f) {
        ESP_LOGW(TAG, "Slot A dosya acilamadi: %s", full_path);
        return -1;
    }

    int bytes_read = fread(buf, 1, buf_size - 1, f);
    buf[bytes_read] = '\0';
    fclose(f);

    ESP_LOGD(TAG, "Slot A dosya okundu: %s (%d byte)", full_path, bytes_read);
    return bytes_read;
}

long ext_flash_slot_a_get_file_size(const char *path)
{
    if (!s_slot_a_mounted || !path) return -1;

    char full_path[128];
    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", SLOT_A_MOUNT, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", SLOT_A_MOUNT, path);
    }

    struct stat st;
    if (stat(full_path, &st) != 0) return -1;
    return (long)st.st_size;
}

int ext_flash_slot_a_read_file_chunk(const char *path, char *buf, size_t buf_size, long offset)
{
    if (!s_slot_a_mounted || !path || !buf) return -1;

    char full_path[128];
    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", SLOT_A_MOUNT, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", SLOT_A_MOUNT, path);
    }

    FILE *f = fopen(full_path, "rb");
    if (!f) return -1;

    if (offset > 0) {
        fseek(f, offset, SEEK_SET);
    }

    int bytes_read = fread(buf, 1, buf_size, f);
    fclose(f);
    return bytes_read;
}

// ============================================================
// Slot A dosya handle islemleri (tek fopen/fclose ile tum chunklari oku)
// ============================================================

struct ext_flash_file {
    FILE *fp;
    long size;
};

ext_flash_file_t ext_flash_slot_a_fopen(const char *path, long *out_size)
{
    if (!s_slot_a_mounted || !path) return NULL;

    char full_path[128];
    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", SLOT_A_MOUNT, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", SLOT_A_MOUNT, path);
    }

    FILE *fp = fopen(full_path, "rb");
    if (!fp) return NULL;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    struct ext_flash_file *f = malloc(sizeof(struct ext_flash_file));
    if (!f) {
        fclose(fp);
        return NULL;
    }
    f->fp = fp;
    f->size = size;

    if (out_size) *out_size = size;
    return f;
}

int ext_flash_slot_a_fread(ext_flash_file_t f, char *buf, size_t buf_size)
{
    if (!f || !f->fp || !buf) return -1;
    return (int)fread(buf, 1, buf_size, f->fp);
}

void ext_flash_slot_a_fclose(ext_flash_file_t f)
{
    if (!f) return;
    if (f->fp) fclose(f->fp);
    free(f);
}

// ============================================================
// User Data dosya islemleri - /ext mount noktasi (geriye uyumlu)
// ============================================================

int ext_flash_read_file(const char *path, char *buf, size_t buf_size)
{
    if (!s_slot_a_mounted || !path || !buf) {
        return -1;
    }

    // path tam VFS yolu olmali (ör: /slot_a/profiles/dimmer/file.json)
    FILE *f = fopen(path, "r");
    if (!f) {
        ESP_LOGW(TAG, "Dosya acilamadi: %s", path);
        return -1;
    }

    int bytes_read = fread(buf, 1, buf_size - 1, f);
    buf[bytes_read] = '\0';
    fclose(f);

    ESP_LOGD(TAG, "Dosya okundu: %s (%d byte)", path, bytes_read);
    return bytes_read;
}

esp_err_t ext_flash_write_file(const char *path, const char *data, size_t len)
{
    if (!s_slot_a_mounted || !path || !data) {
        return ESP_ERR_INVALID_ARG;
    }

    // Dizin yoksa olustur (recursive mkdir)
    char dir[320];
    strncpy(dir, path, sizeof(dir) - 1);
    dir[sizeof(dir) - 1] = '\0';
    for (char *p = dir + 1; *p; p++) {
        if (*p == '/') {
            *p = '\0';
            mkdir(dir, 0755);
            *p = '/';
        }
    }

    FILE *f = fopen(path, "w");
    if (!f) {
        ESP_LOGE(TAG, "Dosya yazilamadi: %s", path);
        return ESP_FAIL;
    }

    size_t written = fwrite(data, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "Yazma hatasi: %zu/%zu byte", written, len);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Dosya yazildi: %s (%zu byte)", path, written);
    return ESP_OK;
}

esp_err_t ext_flash_delete_file(const char *path)
{
    if (!s_slot_a_mounted || !path) {
        return ESP_ERR_INVALID_ARG;
    }

    if (remove(path) != 0) {
        ESP_LOGE(TAG, "Dosya silinemedi: %s", path);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Dosya silindi: %s", path);
    return ESP_OK;
}

// ============================================================
// Aktif Slot Yonetimi
// ============================================================

char ext_flash_get_active_slot(void)
{
    return s_active_slot;
}

esp_err_t ext_flash_set_active_slot(char slot)
{
    if (slot != 'a' && slot != 'b') return ESP_ERR_INVALID_ARG;

    nvs_handle_t nvs;
    esp_err_t err = nvs_open(NVS_GUI_OTA_NS, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;

    char val[2] = { slot, '\0' };
    err = nvs_set_str(nvs, "active_slot", val);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);

    if (err == ESP_OK) {
        s_active_slot = slot;
        ESP_LOGI(TAG, "Aktif slot degistirildi: %c", slot);
    }
    return err;
}

const char *ext_flash_get_active_mount(void)
{
    return (s_active_slot == 'b') ? SLOT_B_MOUNT : SLOT_A_MOUNT;
}

const char *ext_flash_get_inactive_mount(void)
{
    return (s_active_slot == 'b') ? SLOT_A_MOUNT : SLOT_B_MOUNT;
}

// ============================================================
// Aktif slot dosya islemleri (hangi slot aktifse oradan oku)
// ============================================================

ext_flash_file_t ext_flash_active_fopen(const char *path, long *out_size)
{
    const char *mount = ext_flash_get_active_mount();
    bool mounted = (s_active_slot == 'b') ? s_slot_b_mounted : s_slot_a_mounted;
    if (!mounted || !path) return NULL;

    char full_path[128];
    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", mount, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", mount, path);
    }

    FILE *fp = fopen(full_path, "rb");
    if (!fp) {
        // Aktif slot'ta yoksa fallback: diger slot'u dene
        return ext_flash_slot_a_fopen(path, out_size);
    }

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    struct ext_flash_file *f = malloc(sizeof(struct ext_flash_file));
    if (!f) {
        fclose(fp);
        return NULL;
    }
    f->fp = fp;
    f->size = size;

    if (out_size) *out_size = size;
    return f;
}

int ext_flash_active_fread(ext_flash_file_t f, char *buf, size_t buf_size)
{
    return ext_flash_slot_a_fread(f, buf, buf_size); // ayni struct, ayni okuma
}

void ext_flash_active_fclose(ext_flash_file_t f)
{
    ext_flash_slot_a_fclose(f); // ayni struct, ayni kapatma
}

long ext_flash_active_get_file_size(const char *path)
{
    const char *mount = ext_flash_get_active_mount();
    bool mounted = (s_active_slot == 'b') ? s_slot_b_mounted : s_slot_a_mounted;
    if (!mounted || !path) return -1;

    char full_path[128];
    if (path[0] == '/') {
        snprintf(full_path, sizeof(full_path), "%s%s", mount, path);
    } else {
        snprintf(full_path, sizeof(full_path), "%s/%s", mount, path);
    }

    struct stat st;
    if (stat(full_path, &st) != 0) {
        // Fallback: Slot A
        return ext_flash_slot_a_get_file_size(path);
    }
    return (long)st.st_size;
}

// ============================================================
// Durum sorgulama
// ============================================================

bool ext_flash_is_mounted(void)
{
    return s_slot_a_mounted || s_user_data_mounted;
}

bool ext_flash_is_slot_a_mounted(void)
{
    return s_slot_a_mounted;
}

bool ext_flash_is_slot_b_mounted(void)
{
    return s_slot_b_mounted;
}

bool ext_flash_is_user_data_mounted(void)
{
    return s_user_data_mounted;
}

esp_err_t ext_flash_get_info(size_t *total, size_t *free_space)
{
    if (!s_slot_a_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t total_bytes = 0, used_bytes = 0;
    esp_err_t ret = esp_littlefs_info(SLOT_A_LABEL, &total_bytes, &used_bytes);
    if (ret != ESP_OK) {
        return ret;
    }

    if (total) *total = total_bytes;
    if (free_space) *free_space = total_bytes - used_bytes;

    return ESP_OK;
}

esp_err_t ext_flash_get_slot_a_info(size_t *total, size_t *free_space)
{
    if (!s_slot_a_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t total_bytes = 0, used_bytes = 0;
    esp_err_t ret = esp_littlefs_info(SLOT_A_LABEL, &total_bytes, &used_bytes);
    if (ret != ESP_OK) {
        return ret;
    }

    if (total) *total = total_bytes;
    if (free_space) *free_space = total_bytes - used_bytes;

    return ESP_OK;
}

esp_err_t ext_flash_get_slot_b_info(size_t *total, size_t *free_space)
{
    if (!s_slot_b_mounted) {
        return ESP_ERR_INVALID_STATE;
    }

    size_t total_bytes = 0, used_bytes = 0;
    esp_err_t ret = esp_littlefs_info(SLOT_B_LABEL, &total_bytes, &used_bytes);
    if (ret != ESP_OK) {
        return ret;
    }

    if (total) *total = total_bytes;
    if (free_space) *free_space = total_bytes - used_bytes;

    return ESP_OK;
}

// ============================================================
// GUI dosyalari (LS/GUI/ klasorunden embed edilir)
// CMake EMBED_TXTFILES ile derleme sirasinda binary'ye gomulur
// Slot A'ya yazilir
// ============================================================

/* index/style/app derleme aninda gzip'lenir (bkz. CMakeLists.txt + gzip_gui.py).
 * Slot A'ya .gz olarak yazilir; web_server once .gz'yi arar, Content-Encoding: gzip ile sunar. */
extern const char gui_index_html_gz_start[] asm("_binary_index_html_gz_start");
extern const char gui_index_html_gz_end[]   asm("_binary_index_html_gz_end");
extern const char gui_style_css_gz_start[]  asm("_binary_style_css_gz_start");
extern const char gui_style_css_gz_end[]    asm("_binary_style_css_gz_end");
extern const char gui_app_js_gz_start[]     asm("_binary_app_js_gz_start");
extern const char gui_app_js_gz_end[]       asm("_binary_app_js_gz_end");
extern const char gui_config_json_start[] asm("_binary_config_json_start");
extern const char gui_config_json_end[]   asm("_binary_config_json_end");
extern const char gui_logo_png_start[]      asm("_binary_logo_png_start");
extern const char gui_logo_png_end[]        asm("_binary_logo_png_end");
extern const char gui_darklogo_png_start[]  asm("_binary_darklogo_png_start");
extern const char gui_darklogo_png_end[]    asm("_binary_darklogo_png_end");
extern const char gui_lang_en_json_start[]  asm("_binary_en_json_start");
extern const char gui_lang_en_json_end[]    asm("_binary_en_json_end");
extern const char gui_lang_de_json_start[]  asm("_binary_de_json_start");
extern const char gui_lang_de_json_end[]    asm("_binary_de_json_end");
extern const char gui_lang_es_json_start[]  asm("_binary_es_json_start");
extern const char gui_lang_es_json_end[]    asm("_binary_es_json_end");
extern const char gui_lang_fr_json_start[]  asm("_binary_fr_json_start");
extern const char gui_lang_fr_json_end[]    asm("_binary_fr_json_end");
extern const char gui_lang_it_json_start[]  asm("_binary_it_json_start");
extern const char gui_lang_it_json_end[]    asm("_binary_it_json_end");
extern const char gui_lang_tr_json_start[]  asm("_binary_tr_json_start");
extern const char gui_lang_tr_json_end[]    asm("_binary_tr_json_end");

#define WEB_FILES_VERSION "0.0.21"

static bool file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

// Dizin ve icindeki tum dosyalari recursive sil
// skip_name != NULL ise, o isimli alt dizin atlanir (ör: "profiles")
static void delete_dir_contents_ex(const char *dir_path, const char *skip_name)
{
    DIR *dir = opendir(dir_path);
    if (!dir) return;

    struct dirent *entry;
    char path[320];

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        // Belirtilen dizini atla (kullanici verileri korunur)
        if (skip_name && strcmp(entry->d_name, skip_name) == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", dir_path, entry->d_name);

        struct stat st;
        if (stat(path, &st) == 0) {
            if (S_ISDIR(st.st_mode)) {
                delete_dir_contents_ex(path, NULL);
                rmdir(path);
                ESP_LOGI(TAG, "  Dizin silindi: %s", path);
            } else {
                remove(path);
                ESP_LOGI(TAG, "  Dosya silindi: %s", path);
            }
        }
    }

    closedir(dir);
}

static void delete_dir_contents(const char *dir_path)
{
    delete_dir_contents_ex(dir_path, NULL);
}

static void write_file_to_slot_a(const char *name, const char *content, size_t len)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", SLOT_A_MOUNT, name);

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "  [HATA] Dosya olusturulamadi: %s (errno=%d)", name, errno);
        return;
    }
    size_t written = fwrite(content, 1, len, f);
    fclose(f);

    if (written != len) {
        ESP_LOGE(TAG, "  [HATA] Yazma eksik: %s (%zu/%zu byte)", name, written, len);
        return;
    }

    // Dogrulama: dosyayi geri oku ve boyutunu kontrol et
    f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "  [HATA] Dogrulama: dosya acilamadi: %s", name);
        return;
    }
    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    fclose(f);

    if ((size_t)file_size != len) {
        ESP_LOGE(TAG, "  [HATA] Dogrulama: boyut uyumsuz: %s (yazilan=%zu, diskteki=%ld)", name, len, file_size);
    } else {
        ESP_LOGI(TAG, "  [OK] %s (%zu byte) -> Slot A", name, len);
    }
}

static void ensure_dir_slot_a(const char *dir)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/%s", SLOT_A_MOUNT, dir);
    mkdir(path, 0755);
}

// Versiyon kontrolu: Slot A'daki config.json icindeki version ile karsilastir
static bool needs_update(void)
{
    char path[64];
    snprintf(path, sizeof(path), "%s/config.json", SLOT_A_MOUNT);

    if (!file_exists(path)) {
        ESP_LOGI(TAG, "config.json bulunamadi, guncelleme gerekli");
        return true;
    }

    char buf[256];
    FILE *f = fopen(path, "r");
    if (!f) return true;
    size_t len = fread(buf, 1, sizeof(buf) - 1, f);
    buf[len] = '\0';
    fclose(f);

    /* Slot A'daki surum gomuluden ESKIYSE yaz. "Esit degilse yaz" kurali GUI OTA ile
     * indirilen daha yeni arayuzu bir sonraki acilista siliyordu. */
    char slot_ver[24] = {0};
    cJSON *root = cJSON_Parse(buf);
    if (root) {
        cJSON *v = cJSON_GetObjectItem(root, "version");
        if (v && cJSON_IsString(v)) strncpy(slot_ver, v->valuestring, sizeof(slot_ver) - 1);
        cJSON_Delete(root);
    }
    ESP_LOGI(TAG, "Slot A GUI surumu: %s, gomulu: %s", slot_ver[0] ? slot_ver : "?", WEB_FILES_VERSION);
    if (!slot_ver[0]) return true;

    int a[3] = {0}, b[3] = {0};
    sscanf(slot_ver, "%d.%d.%d", &a[0], &a[1], &a[2]);
    sscanf(WEB_FILES_VERSION, "%d.%d.%d", &b[0], &b[1], &b[2]);
    for (int i = 0; i < 3; i++) {
        if (b[i] > a[i]) { ESP_LOGI(TAG, "Gomulu GUI daha yeni, guncelleme gerekli"); return true; }
        if (b[i] < a[i]) { ESP_LOGI(TAG, "Slot A GUI daha yeni (OTA), korunuyor"); return false; }
    }
    ESP_LOGI(TAG, "GUI surumu esit, guncelleme gerekli degil");
    return false;
}

esp_err_t ext_flash_format_user_data(void)
{
    if (!s_slot_a_mounted) {
        ESP_LOGE(TAG, "Slot A mount degil, format yapilamaz");
        return ESP_ERR_INVALID_STATE;
    }

    ESP_LOGW(TAG, "Kullanici verileri siliniyor...");
    delete_dir_contents(SLOT_A_MOUNT "/profiles");
    // Mod yapilandirmasini da sil
    char path[64];
    snprintf(path, sizeof(path), "%s/modes.json", SLOT_A_MOUNT);
    remove(path);  // Dosya yoksa hata vermez
    ESP_LOGI(TAG, "Kullanici verileri silindi");
    return ESP_OK;
}

esp_err_t ext_flash_create_default_files(void)
{
    if (!s_slot_a_mounted) {
        ESP_LOGE(TAG, "Slot A mount degil, GUI dosyalari yazilamaz");
        return ESP_ERR_INVALID_STATE;
    }

    if (!needs_update()) {
        ESP_LOGI(TAG, "Web dosyalari guncel (v" WEB_FILES_VERSION ") [Slot A]");
        return ESP_OK;
    }

    ESP_LOGI(TAG, "Web dosyalari Slot A'ya yaziliyor (v" WEB_FILES_VERSION ")...");

    // Sadece bilinen GUI dosyalarini sil, kullanici verilerini koru (modes.json, profiles/)
    ESP_LOGI(TAG, "Slot A eski GUI dosyalari siliniyor (kullanici verileri korunuyor)...");
    {
        static const char *gui_files_to_delete[] = {
            "index.html", "style.css", "app.js", "config.json",
            "index.html.gz", "style.css.gz", "app.js.gz", NULL
        };
        for (int i = 0; gui_files_to_delete[i]; i++) {
            char path[320];
            snprintf(path, sizeof(path), "%s/%s", SLOT_A_MOUNT, gui_files_to_delete[i]);
            remove(path);
        }
        // pic/ ve lang/ dizinlerini recursive sil (GUI dosyalari)
        char pic_path[320], lang_path[320];
        snprintf(pic_path, sizeof(pic_path), "%s/pic", SLOT_A_MOUNT);
        snprintf(lang_path, sizeof(lang_path), "%s/lang", SLOT_A_MOUNT);
        delete_dir_contents_ex(pic_path, NULL);
        rmdir(pic_path);
        delete_dir_contents_ex(lang_path, NULL);
        rmdir(lang_path);
    }

    // Yeni dosyalari Slot A'ya yaz (metin dosyalari gzip'li; binary -> end-start)
    write_file_to_slot_a("index.html.gz", gui_index_html_gz_start, gui_index_html_gz_end - gui_index_html_gz_start);
    write_file_to_slot_a("style.css.gz",  gui_style_css_gz_start,  gui_style_css_gz_end - gui_style_css_gz_start);
    write_file_to_slot_a("app.js.gz",     gui_app_js_gz_start,     gui_app_js_gz_end - gui_app_js_gz_start);
    write_file_to_slot_a("config.json", gui_config_json_start, strlen(gui_config_json_start));

    // EMBED_FILES (binary) - null terminator yok, end-start dogru
    ensure_dir_slot_a("pic");
    write_file_to_slot_a("pic/logo.png", gui_logo_png_start, gui_logo_png_end - gui_logo_png_start);
    write_file_to_slot_a("pic/darklogo.png", gui_darklogo_png_start, gui_darklogo_png_end - gui_darklogo_png_start);

    // Dil dosyalari
    ensure_dir_slot_a("lang");
    write_file_to_slot_a("lang/en.json", gui_lang_en_json_start, strlen(gui_lang_en_json_start));
    write_file_to_slot_a("lang/de.json", gui_lang_de_json_start, strlen(gui_lang_de_json_start));
    write_file_to_slot_a("lang/es.json", gui_lang_es_json_start, strlen(gui_lang_es_json_start));
    write_file_to_slot_a("lang/fr.json", gui_lang_fr_json_start, strlen(gui_lang_fr_json_start));
    write_file_to_slot_a("lang/it.json", gui_lang_it_json_start, strlen(gui_lang_it_json_start));
    write_file_to_slot_a("lang/tr.json", gui_lang_tr_json_start, strlen(gui_lang_tr_json_start));

    ESP_LOGI(TAG, "Web dosyalari Slot A'ya yazildi");

    // Firmware guncelleme sonrasi: Slot A yeniden yazildiysa aktif slot'u A'ya zorla
    if (s_active_slot != 'a') {
        ESP_LOGW(TAG, "Slot A yeniden yazildi, aktif slot A'ya zorlaniyor");
        ext_flash_set_active_slot('a');
    }

    return ESP_OK;
}

// ============================================================
// Reserve partition durum
// ============================================================

bool ext_flash_is_reserve_mounted(void) { return s_reserve_mounted; }

esp_err_t ext_flash_get_reserve_info(size_t *total, size_t *free_space) {
    if (!s_reserve_mounted) return ESP_ERR_INVALID_STATE;
    size_t total_bytes = 0, used_bytes = 0;
    esp_err_t ret = esp_littlefs_info(RESERVE_LABEL, &total_bytes, &used_bytes);
    if (ret != ESP_OK) return ret;
    if (total) *total = total_bytes;
    if (free_space) *free_space = total_bytes - used_bytes;
    return ESP_OK;
}
