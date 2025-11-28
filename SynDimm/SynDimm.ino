/**
 * SynDimm.ino
 * SmartKraft SynDimm - Main Application
 * Version: v1.0.1
 * 
 * ESP32-C6 with KY-040 Rotary Encoder
 * Web Interface with AP Mode
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Web arayüzü SADECE ESP32-C6 içindir!
 * - Kullanıcı sadece cihazın kendisinden erişebilir (AP mode)
 * - Dışarıdan internet erişimi YOK
 * - Sadece bilgilendirme ve basit ayarlar için
 * - Tüm kritik kontroller ENCODER ile yapılır
 * - Web sadece izleme ve ince ayar içindir
 * 
 * ========================================
 * KOD STANDARTLARI
 * ========================================
 * ❌ ASLA EMOJİ KULLANMA!
 *    - HTML/CSS/JS içinde emoji karakterleri yasak
 *    - Sebep: Flash bellek boyutu, tarayıcı uyumluluğu
 *    - Alternatif: Unicode semboller (•, ⌕, ×) veya düz metin
 * ========================================
 */

// Include all modules
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <ESPmDNS.h>
#include <EEPROM.h>
#include "SK_config.h"
#include "SKwifi.h"
#include "SK_encoder.h"
#include "SK_buzzer.h"
#include "SK_mode_manager.h"
#include "SK_webserver.h"
#include "SK_dimmer.h"
#include "SK_shutter.h"
#include "SK_mode_safe.h"
#include "SK_mode_safe_api.h"
#include "SK_ota.h"

// Create instances
SKWiFi wifi;
SKEncoder encoder;
SKBuzzer buzzer;
SKModeManager modeManager(&buzzer);
SKWebServer webServer;
SafeLock safeLock;
SafeLockAPIHandler safeApiHandler;

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  // CPU frekansini maksimumda tut (160MHz for ESP32-C6)
  setCpuFrequencyMhz(160);
  DEBUG_PRINTF("[STARTUP] CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
  
  // Watchdog Timer - 30 saniye timeout (donma durumunda reset)
  // ESP-IDF 5.x'te WDT zaten başlatılmış olabilir, reconfigure kullan
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,
    .idle_core_mask = (1 << 0),
    .trigger_panic = true
  };
  esp_err_t wdt_err = esp_task_wdt_reconfigure(&wdt_config);
  if (wdt_err == ESP_ERR_INVALID_STATE) {
    // WDT henüz başlatılmamış, init et
    esp_task_wdt_init(&wdt_config);
  }
  esp_task_wdt_add(NULL);
  DEBUG_PRINTLN("[STARTUP] Watchdog Timer aktif (30s)");
  
  // Get chip ID
  uint64_t chipid = ESP.getEfuseMac();
  char chipIDStr[13];
  sprintf(chipIDStr, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  String fullChipID = String(chipIDStr);
  
  // Initialize Encoder
  encoder.begin();
  
  // Initialize Buzzer
  buzzer.begin();
  
  // Initialize Mode Manager
  modeManager.begin();
  
  // Initialize WiFi (with network scan and smart connection)
  wifi.begin();
  
  // Initialize Dimmer System
  initDimmer();
  
  // Initialize Shutter System
  initShutter();

  // Initialize Safe Lock System
  safeLock.begin();
  safeApiHandler.setSafeLock(&safeLock);
  safeLock.setPasswordMatchCallback([](uint8_t pwdIndex) {
    SafeLockAPIHandler::onPasswordMatch(pwdIndex, &safeLock, &safeApiHandler);
  });

  // Initialize Language System
  initLanguage();
  DEBUG_PRINTF("[STARTUP] Language: %s\n", getCurrentLangCode().c_str());

  // Initialize OTA System
  initOTA();
  
  // Start Web Server
  webServer.begin(fullChipID);
  webServer.setWiFiManager(&wifi);  // Link WiFi manager to web server
  webServer.setModeManager(&modeManager);  // Link mode manager to web server
  webServer.setSafeLock(&safeLock);  // Link Safe Lock to web server
  webServer.setSafeApiHandler(&safeApiHandler);  // Link Safe API Handler (ESP32 handles all HTTP)
  
  // Auto-reconnect ALL modes at startup (background connections)
  SystemMode activeMode = modeManager.getActiveMode();
  DEBUG_PRINTF("[STARTUP] Active Mode: %s\n", activeMode == MODE_DIMMER ? "DIMMER" : activeMode == MODE_SHUTTER ? "SHUTTER" : "SAFE");
  
  // Always try to connect to saved devices (background, read-only for inactive modes)
  DEBUG_PRINTLN("[STARTUP] Connecting to saved devices...");
  autoReconnect();        // Dimmer - always try
  autoReconnectShutter(); // Shutter - always try
  DEBUG_PRINTLN("[STARTUP] Device connections complete");
  
  // İlk OTA kontrolü (WiFi bağlıysa)
  if (wifi.isConnectedToWiFi()) {
    DEBUG_PRINTLN("[STARTUP] Checking for updates...");
    checkForUpdates();  // Sadece kontrol et, otomatik güncelleme yapma
  }
  
  // Random seed'i chip ID ile başlat (her cihaz farklı random değer üretir)
  randomSeed(ESP.getEfuseMac() ^ millis());
}

// OTA auto-check variables
static unsigned long lastOTACheck = 0;
static unsigned long otaCheckInterval = 0;

// 24 saat + random(1-1440 dakika) = 24-48 saat arası
unsigned long calculateNextOTAInterval() {
  const unsigned long BASE_INTERVAL = 24UL * 60UL * 60UL * 1000UL;  // 24 saat (ms)
  const unsigned long RANDOM_RANGE = 1440UL * 60UL * 1000UL;        // 1440 dakika (ms)
  unsigned long randomMinutes = random(1, 1441);                     // 1-1440 dakika
  unsigned long interval = BASE_INTERVAL + (randomMinutes * 60UL * 1000UL);
  DEBUG_PRINTF("[OTA] Next check in %lu hours %lu minutes\n", 
               interval / 3600000UL, (interval % 3600000UL) / 60000UL);
  return interval;
}

void loop() {
  // Handle WiFi status (reconnection & AP mode scanning)
  wifi.handleWiFi();
  
  // Handle web server requests
  webServer.handleClient();
  
  // Update buzzer (non-blocking)
  buzzer.update();
  
  // Update mode manager (handle timeout)
  modeManager.update();
  
  // OTA otomatik güncelleme kontrolü (24-48 saat arası random)
  if (wifi.isConnectedToWiFi()) {
    // İlk çalışmada interval hesapla
    if (otaCheckInterval == 0) {
      otaCheckInterval = calculateNextOTAInterval();
      lastOTACheck = millis();
    }
    
    // Süre dolduysa kontrol et ve yeni interval hesapla
    if (millis() - lastOTACheck > otaCheckInterval) {
      lastOTACheck = millis();
      autoUpdateCheck();
      otaCheckInterval = calculateNextOTAInterval();  // Sonraki kontrol için yeni random
    }
  }
  
  // ========================================
  // MOD İZOLASYONU - Sadece aktif mod güncellenir
  // ========================================
  SystemMode activeMode = modeManager.getActiveMode();
  
  switch(activeMode) {
    case MODE_DIMMER:
      dimmerLoop();
      break;
    case MODE_SHUTTER:
      updateShutter();
      break;
    case MODE_SAFE:
      // Safe mod loop gerektirmez, event-driven çalışır
      break;
  }
  
  // Encoder event handling
  if (encoder.available()) {
    char event = encoder.read();
    
    // Check if in mode selection mode
    if (modeManager.isInSelectionMode() || event == 'P') {
      // Mode selection has priority
      modeManager.handleEncoderEvent(event);
    } else {
      // Normal mode - route to active mode
      switch(activeMode) {
        case MODE_DIMMER:
          handleDimmerEncoderEvent(event);
          break;
        case MODE_SHUTTER:
          handleShutterEncoderEvent(event);
          break;
        case MODE_SAFE:
          handleSafeEncoderEvent(event);
          break;
      }
    }
  }
  
  // Watchdog'u besle - donma önleme
  esp_task_wdt_reset();
  
  delay(10);
}

// ========================================
// MODE-SPECIFIC ENCODER HANDLERS
// ========================================

void handleDimmerEncoderEvent(char event) {
  switch(event) {
    case 'L':
      adjustBrightness(-1);
      break;
    case 'R':
      adjustBrightness(1);
      break;
    case 'B':
      toggleDimmer();
      break;
  }
}

void handleShutterEncoderEvent(char event) {
  switch(event) {
    case 'L':
      handleShutterEncoderRotate(-1);  // -1 = left = up
      break;
    case 'R':
      handleShutterEncoderRotate(1);   // +1 = right = down
      break;
    case 'B':
      handleShutterEncoderButton();
      break;
  }
}

void handleSafeEncoderEvent(char event) {
  switch(event) {
    case 'L':
      safeLock.onEncoderMove('L', 1);
      break;
    case 'R':
      safeLock.onEncoderMove('R', 1);
      break;
    case 'B':
      safeLock.onEncoderButton();
      break;
  }
}
