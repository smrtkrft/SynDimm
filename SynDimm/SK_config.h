/**
 * SK_config.h
 * SmartKraft SynDimm - Configuration & Constants
 * Version: v1.1.1
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Bu sistem ENCODER merkezlidir!
 * - Web arayüzü sadece bilgilendirme ve ince ayar için
 * - Tüm asıl kontroller ENCODER ile yapılır
 * - Web'den kritik fonksiyon çalıştırılmaz
 * - Sadece lokal AP erişimi (internet YOK)
 * ========================================
 */

#ifndef SK_CONFIG_H
#define SK_CONFIG_H

// ========================================
// DEBUG AYARLARI
// ========================================
// Geliştirme için aç: #define DEBUG_MODE
#define DEBUG_MODE

#ifdef DEBUG_MODE
  #define DEBUG_PRINT(x)    Serial.print(x)
  #define DEBUG_PRINTLN(x)  Serial.println(x)
  #define DEBUG_PRINTF(...) Serial.printf(__VA_ARGS__)
#else
  #define DEBUG_PRINT(x)
  #define DEBUG_PRINTLN(x)
  #define DEBUG_PRINTF(...)
#endif

// Kritik hatalar her zaman gösterilir
#define ERROR_PRINT(x)    Serial.print(x)
#define ERROR_PRINTLN(x)  Serial.println(x)
#define ERROR_PRINTF(...) Serial.printf(__VA_ARGS__)

// Version Information
#define VERSION "v1.1.1"
#define DEVICE_NAME "SmartKraft SynDimm"

// ========================================
// BELLEK YÖNETİMİ SABITLERI
// ========================================
#define MAX_SAVED_DEVICES 10          // Maksimum kaydedilmiş cihaz sayısı
#define MAX_DISCOVERED_DEVICES 30     // Tarama sırasında maks cihaz
#define HTTP_CLIENT_REUSE false       // HTTPClient reuse devre dışı

// KY-040 Rotary Encoder Pins
#define ENCODER_CLK 19
#define ENCODER_DT  20
#define ENCODER_SW  18

// Web Server
#define WEB_SERVER_PORT 80

// Serial Communication
#define SERIAL_BAUD_RATE 115200

// Debounce Settings
#define BUTTON_DEBOUNCE_MS 200

// WiFi Settings
#define WIFI_CONNECT_TIMEOUT_MS 10000  // 10 saniye bağlantı denemesi
#define WIFI_SCAN_INTERVAL_MS 60000    // 60 saniye (AP modundayken tarama aralığı)

// Preferences Namespace
#define PREFS_NAMESPACE "sknetwork"
#define DEVICE_ID_NAMESPACE "skdevice"
#define DEVICE_ID_KEY "device_id"

// ========================================
// SYSTEM MODES
// ========================================
enum SystemMode {
    MODE_DIMMER = 0,
    MODE_SHUTTER = 1,
    MODE_SAFE = 2
};

// ========================================
// BENZERSIZ CIHAZ ID SISTEMI
// ========================================
// NVS'te kalıcı olarak saklanır, OTA sonrası bile korunur
#include <Preferences.h>

inline String getOrCreateDeviceId() {
    Preferences prefs;
    String deviceId = "";
    
    // 1. Önce NVS'te ara (en kalıcı)
    if (prefs.begin(DEVICE_ID_NAMESPACE, true)) {
        deviceId = prefs.getString(DEVICE_ID_KEY, "");
        prefs.end();
        if (deviceId.length() == 12) {
            DEBUG_PRINTLN("[DeviceID] NVS'ten yuklendi: " + deviceId);
            return deviceId;
        }
    }
    
    // 2. Yeni benzersiz ID oluştur
    uint64_t mac = ESP.getEfuseMac();
    uint32_t random1 = esp_random();
    uint32_t random2 = esp_random();
    uint32_t bootTime = micros();
    
    // XOR ile karıştır
    uint32_t part1 = (uint32_t)(mac & 0xFFFFFFFF) ^ random1 ^ bootTime;
    uint32_t part2 = (uint32_t)(mac >> 32) ^ random2 ^ (bootTime >> 8);
    
    // Hash karıştırma (Murmur3 benzeri)
    part1 = ((part1 >> 16) ^ part1) * 0x45d9f3b;
    part1 = ((part1 >> 16) ^ part1) * 0x45d9f3b;
    part1 = (part1 >> 16) ^ part1;
    
    part2 = ((part2 >> 16) ^ part2) * 0x45d9f3b;
    part2 = ((part2 >> 16) ^ part2) * 0x45d9f3b;
    part2 = (part2 >> 16) ^ part2;
    
    // 12 karakterlik hex ID
    char newId[13];
    snprintf(newId, sizeof(newId), "%04X%08X", (uint16_t)(part2 & 0xFFFF), part1);
    deviceId = String(newId);
    
    // 3. NVS'e kaydet
    if (prefs.begin(DEVICE_ID_NAMESPACE, false)) {
        prefs.putString(DEVICE_ID_KEY, deviceId);
        prefs.end();
        DEBUG_PRINTLN("[DeviceID] Yeni ID olusturuldu ve kaydedildi: " + deviceId);
    }
    
    return deviceId;
}

// Son 6 karakter (AP SSID için)
inline String getDeviceIdShort() {
    String fullId = getOrCreateDeviceId();
    return fullId.substring(fullId.length() - 6);
}

#endif // SK_CONFIG_H
