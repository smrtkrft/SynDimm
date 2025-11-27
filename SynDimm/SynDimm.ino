/**
 * SynDimm.ino
 * SmartKraft SynDimm - Main Application
 * Version: v0.9.1
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
  EEPROM.begin(4096);  // Initialize EEPROM for Safe Lock
  safeLock.begin();
  safeApiHandler.setSafeLock(&safeLock);
  safeLock.setPasswordMatchCallback([](uint8_t pwdIndex) {
    SafeLockAPIHandler::onPasswordMatch(pwdIndex, &safeLock, &safeApiHandler);
  });

  // Initialize OTA System
  initOTA();
  
  // Start Web Server
  webServer.begin(fullChipID);
  webServer.setWiFiManager(&wifi);  // Link WiFi manager to web server
  webServer.setModeManager(&modeManager);  // Link mode manager to web server
  webServer.setSafeLock(&safeLock);  // Link Safe Lock to web server
  webServer.setSafeApiHandler(&safeApiHandler);  // Link Safe API Handler (ESP32 handles all HTTP)
  
  // Auto-reconnect based on active mode
  SystemMode activeMode = modeManager.getActiveMode();
  DEBUG_PRINTF("[STARTUP] Mode: %s\n", activeMode == MODE_DIMMER ? "DIMMER" : activeMode == MODE_SHUTTER ? "SHUTTER" : "SAFE");
  
  if (activeMode == MODE_DIMMER) {
    autoReconnect();  // Reconnect to last dimmer
  }
  // Shutter reconnection will be manual via web interface
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
