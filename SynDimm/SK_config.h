/**
 * SK_config.h
 * SmartKraft SynDimm - Configuration & Constants
 * Version: v1.0.1
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
#define VERSION "v1.0.1"
#define DEVICE_NAME "SmartKraft SynDimm"

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

// ========================================
// SYSTEM MODES
// ========================================
enum SystemMode {
    MODE_DIMMER = 0,
    MODE_SHUTTER = 1,
    MODE_SAFE = 2
};

#endif // SK_CONFIG_H
