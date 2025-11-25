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
#include "SK_config.h"
#include "SKwifi.h"
#include "SK_encoder.h"
#include "SK_buzzer.h"
#include "SK_mode_manager.h"
#include "SK_webserver.h"
#include "SK_dimmer.h"
#include "SK_shutter.h"
#include "SK_ota.h"

// Create instances
SKWiFi wifi;
SKEncoder encoder;
SKBuzzer buzzer;
SKModeManager modeManager(&buzzer);
SKWebServer webServer;

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  // Print startup info
  Serial.println("\n================================");
  Serial.println(String(DEVICE_NAME) + " " + String(VERSION));
  Serial.println("================================");
  
  // Get and print full chip ID
  uint64_t chipid = ESP.getEfuseMac();
  char chipIDStr[13];
  sprintf(chipIDStr, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
  String fullChipID = String(chipIDStr);
  Serial.println("Chip ID (full): " + fullChipID);
  Serial.println();
  
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
  
  // Initialize OTA System
  initOTA();
  
  // Start Web Server
  webServer.begin(fullChipID);
  webServer.setWiFiManager(&wifi);  // Link WiFi manager to web server
  webServer.setModeManager(&modeManager);  // Link mode manager to web server
  
  Serial.println("\n================================");
  Serial.println("System Ready!");
  Serial.println("Chip ID: " + fullChipID);
  Serial.println(wifi.getCurrentNetworkInfo());
  Serial.printf("Active Mode: %s\n", modeManager.getActiveModeName());
  Serial.println("================================\n");
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
  
  // Update shutter status periodically
  updateShutter();
  
  // Handle Dimmer operations
  dimmerLoop();
  
  // Encoder event handling
  if (encoder.available()) {
    char event = encoder.read();
    
    // Check if in mode selection mode
    if (modeManager.isInSelectionMode() || event == 'P') {
      // Mode selection has priority
      modeManager.handleEncoderEvent(event);
    } else {
      // Normal mode - route to active mode
      SystemMode activeMode = modeManager.getActiveMode();
      
      switch(activeMode) {
        case MODE_DIMMER:
          handleDimmerEncoderEvent(event);
          break;
        case MODE_SHUTTER:
          handleShutterEncoderEvent(event);
          break;
        case MODE_SAFE:
          // Safe mode handler (future implementation)
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
      Serial.println("[Dimmer] Left rotation");
      adjustBrightness(-1);
      break;
    case 'R':
      Serial.println("[Dimmer] Right rotation");
      adjustBrightness(1);
      break;
    case 'B':
      Serial.println("[Dimmer] Button pressed");
      toggleDimmer();
      break;
  }
}

void handleShutterEncoderEvent(char event) {
  switch(event) {
    case 'L':
      Serial.println("[Shutter] Left rotation - Move UP");
      handleShutterEncoderRotate(-1);  // -1 = left = up
      break;
    case 'R':
      Serial.println("[Shutter] Right rotation - Move DOWN");
      handleShutterEncoderRotate(1);   // +1 = right = down
      break;
    case 'B':
      Serial.println("[Shutter] Button pressed - Stop/Toggle");
      handleShutterEncoderButton();
      break;
  }
}
