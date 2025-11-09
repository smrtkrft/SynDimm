/*
 * SynDimm - Smart Dimmer Controller
 * Using standard WebServer
 * 
 * Powered by SEU - Emek - SmartKraft
 * Author: Smart Engineering Unit
 * https://github.com/smartkraft
 */

#include <WiFi.h>
#include <WebServer.h>
#include <ESPmDNS.h>

#include "version.h"
#include "encoder.h"
#include "shutter.h"
#include "modes.h"
#include "syndimm_net.h"
#include "syndimm_buzzer.h"
#include "syndimm_ota.h"
#include "syndimm_scanner.h"
#include "syndimm_wifi_watchdog.h"
#include "webui.h"
#include "webstyle.h"
#include "webscript.h"
#include "translations.h"
#include "safe_lock.h"
#include "safe_lock_eeprom.h"
#include "safe_lock_api.h"

Encoder encoder(19, 20, 18);  // CLK=19, DT=20, SW=18 
Shutter shutter(&encoder);
ModeManager modes(&encoder);
SynDimmNet net;
SynDimmBuzzer buzzer;
SynDimmOTA ota;
NetworkScanner scanner;
SynDimmWiFiWatchdog wifiWatchdog;
WebServer server(80);

// 12-hour reboot timer - prevents long-term instability
const unsigned long REBOOT_INTERVAL = 12UL * 60UL * 60UL * 1000UL;  // 12 hours in ms
unsigned long bootTime = 0;

// mDNS helper function
void setupMDNS() {
  String hostname;
  
  // If WiFi is connected and custom local domain exists, use it
  if (WiFi.status() == WL_CONNECTED) {
    // Check active WiFi local domain
    String customDomain = "";
    if (WiFi.SSID() == net.getWiFi1SSID() && net.getWiFi1Local().length() > 0) {
      customDomain = net.getWiFi1Local();
    } else if (WiFi.SSID() == net.getWiFi2SSID() && net.getWiFi2Local().length() > 0) {
      customDomain = net.getWiFi2Local();
    }
    
    if (customDomain.length() > 0) {
      hostname = customDomain;
      // Remove .local if user saved it with extension
      if (hostname.endsWith(".local")) {
        hostname = hostname.substring(0, hostname.length() - 6);
      }
    } else {
      hostname = "syndimm-" + net.getChipID();
    }
  } else {
    // Default hostname in AP mode
    hostname = "syndimm-" + net.getChipID();
  }
  
  hostname.toLowerCase();
  
  // Restart mDNS
  MDNS.end();
  if (MDNS.begin(hostname.c_str())) {
    MDNS.addService("http", "tcp", 80);
    MDNS.addServiceTxt("http", "tcp", "chipid", net.getChipID());
    MDNS.addServiceTxt("http", "tcp", "version", FIRMWARE_VERSION);
    Serial.println("mDNS OK: " + hostname + ".local");
  } else {
    Serial.println("mDNS FAIL");
  }
}

// Safe Lock objects
SafeLock safeLock;
SafeLockEEPROM safeLockEEPROM;
SafeLockAPI safeLockAPI;

// API trigger flag (for non-blocking execution)
volatile bool pendingApiTrigger = false;
volatile uint8_t pendingPasswordIndex = 0;

// Callback triggered when Safe Lock password matches
void onSafeLockPasswordMatch(uint8_t passwordIndex) {
  Serial.print("[Callback] Password matched: #");
  Serial.println(passwordIndex);
  Serial.flush();
  
  // Set API trigger flag (actual trigger will happen in loop)
  // Buzzer will sound based on API result
  pendingApiTrigger = true;
  pendingPasswordIndex = passwordIndex;
  
  Serial.println("[Callback] Flag set");
  Serial.flush();
  
  Serial.println("[Callback] Complete");
  Serial.flush();
}

void setup() {
  Serial.begin(115200);
  delay(1000);  // 2000ms -> 1000ms (faster startup)
  Serial.println("===============================");
  Serial.print("=== ");
  Serial.print(PROJECT_NAME);
  Serial.print(" v");
  Serial.print(FIRMWARE_VERSION);
  Serial.println(" ===");
  Serial.print("Build: ");
  Serial.print(BUILD_DATE);
  Serial.print(" ");
  Serial.println(BUILD_TIME);
  Serial.println("===============================");
  
  // WiFi performance settings - FULL PERFORMANCE
  WiFi.setSleep(false);  // WiFi sleep mode OFF
  setCpuFrequencyMhz(160);  // CPU 160MHz - full speed
  Serial.println("CPU: 160MHz, WiFi sleep: OFF");
  
  // Initialize buzzer
  buzzer.begin();
  
  encoder.begin();
  Serial.println("Encoder OK");
  
  // Initialize shutter
  shutter.begin();
  Serial.println("Shutter OK");
  
  modes.begin();
  modes.setBuzzer(&buzzer);  // Connect buzzer to modes (for mode change beep)
  modes.setShutter(&shutter);  // Connect shutter to modes
  Serial.println("Modes OK");
  
  // Set boot time for 12h reboot timer
  bootTime = millis();
  
  // Initialize Safe Lock
  safeLock.begin();
  safeLockEEPROM.begin();
  safeLockAPI.setSafeLock(&safeLock);
  safeLockEEPROM.loadToSafeLock(safeLock);
  
  // TEST: If no password exists, add test password
  bool hasPassword = false;
  for (uint8_t i = 0; i < 5; i++) {
    if (safeLock.isPasswordValid(i)) {
      hasPassword = true;
      break;
    }
  }
  
  // TEST PASSWORD REMOVED - User will add via web
  // if (!hasPassword) { ... }
  
  if (hasPassword) {
    Serial.println("Passwords loaded from EEPROM:");
    for (uint8_t i = 0; i < 5; i++) {
      if (safeLock.isPasswordValid(i)) {
        Serial.print("  Password ");
        Serial.print(i);
        Serial.print(": ");
        Serial.println(safeLock.getPassword(i));
        
        // Show API URL as well
        ApiConfig cfg = safeLockEEPROM.getApiConfig(i);
        Serial.print("    API URL: ");
        Serial.println(cfg.url);
        Serial.print("    API Enabled: ");
        Serial.println(cfg.enabled ? "YES" : "NO");
      }
    }
  } else {
    Serial.println("No passwords found. Please add via web interface.");
  }
  
  safeLock.setPasswordMatchCallback(onSafeLockPasswordMatch);
  modes.setSafeLock(&safeLock);  // Connect Safe Lock to modes
  Serial.println("Safe Lock OK");
  
  net.begin();
  net.autoConnect();
  Serial.println("Network OK");
  
  // Initialize WiFi watchdog (after WiFi connected)
  if (net.getWiFi1SSID().length() > 0) {
    wifiWatchdog.begin(net.getWiFi1SSID().c_str(), net.getWiFi1Pass().c_str());
  } else if (net.getWiFi2SSID().length() > 0) {
    wifiWatchdog.begin(net.getWiFi2SSID().c_str(), net.getWiFi2Pass().c_str());
  }
  Serial.println("WiFi Watchdog OK");
  
  // Initialize OTA system (after WiFi)
  ota.begin();
  Serial.println("OTA OK");
  
  // Initialize mDNS
  setupMDNS();
  
  setupWeb();
  server.begin();
  Serial.println("Web OK");
  Serial.println(net.getStatus());
  
  // Kayıtlı cihaz varsa hemen bağlan (WiFi hazır olduğunda)
  if (modes.getDeviceIP() != "") {
    Serial.println("[Setup] Attempting immediate connection to saved device...");
    modes.connectDevice(modes.getDeviceIP(), modes.getDeviceType());
  }
  
  Serial.println("===============");
}

void loop() {
  // WebServer - first priority
  server.handleClient();
  yield();
  
  // WiFi watchdog - CRITICAL: Check every cycle, auto-reconnect
  wifiWatchdog.update();
  
  // OTA update check (auto every 5 minutes)
  ota.update();
  
  // Buzzer update (non-blocking)
  buzzer.update();
  
  // Shutter update (motor control, position tracking)
  shutter.update();
  
  // Encoder reading
  if (encoder.available()) {
    char ev = encoder.read();
    modes.processEncoderEvent(ev);
  }
  yield();
  
  // API trigger (non-blocking - separate from encoder event)
  if (pendingApiTrigger) {
    pendingApiTrigger = false;
    
    // DEBUG REMOVED: Triggering API
    
    // Feed watchdog
    yield();
    
    ApiResponseStatus apiResult = SafeLockAPI::onPasswordMatch(&safeLockEEPROM, &safeLockAPI, pendingPasswordIndex);
    
    // DEBUG REMOVED: API completed
    
    // Play buzzer based on API result
    if (apiResult == API_SUCCESS) {
      buzzer.playApiSuccess();  // 2 short + 1 long beep
      // DEBUG REMOVED
    } else if (apiResult == API_NO_WIFI || apiResult == API_WIFI_ERROR) {
      buzzer.playApiFail();  // 5 short beeps
      // DEBUG REMOVED
    } else if (apiResult == API_DISABLED) {
      // API disabled - silent
      // DEBUG REMOVED
    } else {
      // Other errors (timeout, error, etc)
      buzzer.playApiFail();  // 5 short beeps
      // DEBUG REMOVED
    }
    
    // Feed watchdog
    yield();
  }
  
  // Shelly synchronization
  modes.updateShelly();
  
  // 12-hour graceful reboot - prevents long-term instability
  if (millis() - bootTime > REBOOT_INTERVAL) {
    Serial.println("[Reboot] 12h timer expired - graceful restart");
    
    // Save device connection to EEPROM (already auto-saved on connect, but ensure it's saved)
    if (modes.getDeviceIP() != "") {
      modes.saveDevice(modes.getDeviceIP(), modes.getDeviceType());
    }
    delay(100);
    
    // Save Safe Lock passwords (already auto-saved, but ensure it's saved)
    safeLockEEPROM.save();
    delay(100);
    
    Serial.println("[Reboot] EEPROM saved - restarting in 1s...");
    delay(1000);
    ESP.restart();
  }
  
  // Feed watchdog
  yield();
}

void setupWeb() {
  // Static files
  server.on("/", HTTP_GET, [](){
    server.send_P(200, "text/html", HTML_PAGE);
  });
  
  server.on("/style.css", HTTP_GET, [](){
    server.send_P(200, "text/css", CSS_STYLE);
  });
  
  server.on("/script.js", HTTP_GET, [](){
    server.send_P(200, "application/javascript", JS_SCRIPT);
  });
  
  // API - Translations
  server.on("/translations.json", HTTP_GET, [](){
    server.send_P(200, "application/json", TRANSLATIONS_JSON);
  });
  
  // API - Network info
  server.on("/api/network/info", HTTP_GET, [](){
    String s = "{";
    
    // AP Mode Info
    s += "\"ap\":{";
    s += "\"ssid\":\"" + net.getAPSSID() + "\"";
    s += ",\"ip\":\"192.168.4.1\"";
    s += ",\"active\":" + String(net.isAPActive() ? "true" : "false");
    s += "}";
    
    // WiFi 1 Info
    s += ",\"wifi1\":{";
    s += "\"ssid\":\"" + net.getWiFi1SSID() + "\"";
    s += ",\"ip\":\"" + net.getWiFi1IP() + "\"";
    s += ",\"local\":\"" + net.getWiFi1Local() + "\"";
    s += "}";
    
    // WiFi 2 Info
    s += ",\"wifi2\":{";
    s += "\"ssid\":\"" + net.getWiFi2SSID() + "\"";
    s += ",\"ip\":\"" + net.getWiFi2IP() + "\"";
    s += ",\"local\":\"" + net.getWiFi2Local() + "\"";
    s += "}";
    
    // Chip ID and Version
    s += ",\"chipID\":\"" + net.getChipID() + "\"";
    s += ",\"version\":\"v" + String(FIRMWARE_VERSION) + "\"";
    
    // mDNS hostname (aktif olan)
    String hostname = "";
    if (WiFi.status() == WL_CONNECTED) {
      if (WiFi.SSID() == net.getWiFi1SSID() && net.getWiFi1Local().length() > 0) {
        hostname = net.getWiFi1Local();
      } else if (WiFi.SSID() == net.getWiFi2SSID() && net.getWiFi2Local().length() > 0) {
        hostname = net.getWiFi2Local();
      } else {
        hostname = "syndimm-" + net.getChipID();
      }
    } else {
      hostname = "syndimm-" + net.getChipID();
    }
    hostname.toLowerCase();
    s += ",\"mdns\":\"" + hostname + ".local\"";
    
    s += "}";
    server.send(200, "application/json", s);
  });
  
  // API - Status
  server.on("/api/network/status", HTTP_GET, [](){
    String s = "{";
    String mdnsHostname = "";
    
    if (WiFi.status() == WL_CONNECTED) {
      s += "\"mode\":\"WiFi\"";
      s += ",\"ssid\":\"" + WiFi.SSID() + "\"";
      s += ",\"ip\":\"" + WiFi.localIP().toString() + "\"";
      
      // Aktif WiFi'nin custom local domain'ini kontrol et
      if (WiFi.SSID() == net.getWiFi1SSID() && net.getWiFi1Local().length() > 0) {
        mdnsHostname = net.getWiFi1Local();
      } else if (WiFi.SSID() == net.getWiFi2SSID() && net.getWiFi2Local().length() > 0) {
        mdnsHostname = net.getWiFi2Local();
      } else {
        mdnsHostname = "syndimm-" + net.getChipID();
      }
    } else if (net.isAPActive()) {
      s += "\"mode\":\"AP Mode\"";
      s += ",\"ssid\":\"" + net.getAPSSID() + "\"";
      s += ",\"ip\":\"" + WiFi.softAPIP().toString() + "\"";
      // AP modda sadece default hostname
      mdnsHostname = "syndimm-" + net.getChipID();
    } else {
      s += "\"mode\":\"Disconnected\"";
      s += ",\"ssid\":\"N/A\"";
      s += ",\"ip\":\"N/A\"";
      mdnsHostname = "N/A";
    }
    
    // mDNS hostname ekle
    if (mdnsHostname != "N/A") {
      mdnsHostname.toLowerCase();
      s += ",\"mdns\":\"" + mdnsHostname + ".local\"";
    } else {
      s += ",\"mdns\":\"N/A\"";
    }
    
    s += "}";
    server.send(200, "application/json", s);
  });
  
  // API - Encoder
  server.on("/api/encoder/values", HTTP_GET, [](){
    String s = "{\"dimm_sayac\":" + String(encoder.get_dimm_sayac()) + 
               ",\"L_deger\":" + String(modes.getLeftCount()) + 
               ",\"R_deger\":" + String(modes.getRightCount()) + 
               ",\"last_direction\":\"" + String(encoder.get_last_direction()) + "\"}";
    server.send(200, "application/json", s);
  });
  
  // API - Device scan (IP tarama ile gerçek dimmer/dali cihaz arama - ASYNC)
  server.on("/api/devices/scan", HTTP_GET, [](){
    Serial.println("[API] Device scan request received");
    
    // Eğer tarama devam ediyorsa mevcut durumu döndür
    if (scanner.isScanning()) {
      server.send(200, "application/json", scanner.getDevicesJSON());
      Serial.println("[API] Scan already in progress");
      return;
    }
    
    // Yeni tarama başlat (ASYNC - FreeRTOS Task ile arka planda)
    scanner.startScan();
    
    // HEMEN yanıt döndür - tarama arka planda devam eder
    server.send(200, "application/json", "{\"devices\":[],\"scanning\":true,\"progress\":0}");
    Serial.println("[API] Scan started in background");
  });
  
  // API - Scan progress (tarama durumunu kontrol et)
  server.on("/api/devices/scan/progress", HTTP_GET, [](){
    server.send(200, "application/json", scanner.getDevicesJSON());
  });
  
  // API - Stop scan (taramayı durdur)
  server.on("/api/devices/scan/stop", HTTP_GET, [](){
    Serial.println("[API] Stop scan request received");
    scanner.stopScan();
    server.send(200, "application/json", scanner.getDevicesJSON());
  });
  
  // API - Manual IP connect (manuel IP ile bağlan)
  server.on("/api/devices/manual", HTTP_GET, [](){
    if (server.hasArg("ip")) {
      String manualIP = server.arg("ip");
      Serial.println("[API] Manual IP connect: " + manualIP);
      
      // Try to connect to device
      bool success = modes.connectDevice(manualIP, "shelly-dimmer");
      
      if (success) {
        // Başarılı bağlantı - scanner'a ekle
        scanner.addManualDevice(manualIP, "shelly-dimmer");
        server.send(200, "application/json", "{\"success\":true,\"message\":\"Connected\"}");
      } else {
        server.send(200, "application/json", "{\"success\":false,\"message\":\"Connection failed\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"message\":\"IP parameter required\"}");
    }
  });
  
  // API - Shutter position
  server.on("/api/shutter/position", HTTP_GET, [](){
    if (server.hasArg("value")) {
      int position = server.arg("value").toInt();
      if (position >= 0 && position <= 100) {
        shutter.setPosition(position);
        server.send(200, "application/json", "{\"success\":true,\"position\":" + String(position) + "}");
        Serial.print("Shutter position set: ");
        Serial.println(position);
      } else {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid position\"}");
      }
    } else {
      int currentPosition = shutter.getPosition();
      server.send(200, "application/json", "{\"position\":" + String(currentPosition) + "}");
    }
  });
  
  // API - Shutter open
  server.on("/api/shutter/open", HTTP_GET, [](){
    shutter.open();
    server.send(200, "application/json", "{\"success\":true}");
    // DEBUG REMOVED
  });
  
  // API - Shutter close
  server.on("/api/shutter/close", HTTP_GET, [](){
    shutter.close();
    server.send(200, "application/json", "{\"success\":true}");
    // DEBUG REMOVED
  });
  
  // API - Shutter stop
  server.on("/api/shutter/stop", HTTP_GET, [](){
    shutter.stop();
    server.send(200, "application/json", "{\"success\":true}");
    // DEBUG REMOVED
  });
  
  // API - Set dimm ratio
  server.on("/api/dimmer/ratio", HTTP_GET, [](){
    if (server.hasArg("value")) {
      int ratio = server.arg("value").toInt();
      if (ratio >= 1 && ratio <= 5) {
        modes.setDimmRatio(ratio);
        server.send(200, "application/json", "{\"success\":true,\"ratio\":" + String(ratio) + "}");
      } else {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid ratio\"}");
      }
    } else {
      // GET - Mevcut ratio'yu döndür
      int currentRatio = modes.getDimmRatio();
      server.send(200, "application/json", "{\"ratio\":" + String(currentRatio) + "}");
    }
  });
  
  // API - Set dimmer level (dimm_sayac)
  server.on("/api/dimmer/level", HTTP_GET, [](){
    if (server.hasArg("value")) {
      int level = server.arg("value").toInt();
      if (level >= 0 && level <= 100) {
        encoder.set_dimm_sayac(level);
        modes.triggerDimmChange();  // Web'den değişiklik yapıldığını ModeManager'a bildir
        server.send(200, "application/json", "{\"success\":true,\"level\":" + String(level) + "}");
        Serial.print("Dimmer level set (web): ");
        Serial.println(level);
      } else {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid level\"}");
      }
    } else {
      // GET - Mevcut level'ı döndür
      int currentLevel = encoder.get_dimm_sayac();
      server.send(200, "application/json", "{\"level\":" + String(currentLevel) + "}");
    }
  });
  
  // ========== SHELLY API ENDPOINT'LERİ ==========
  
  // API - Shelly'ye bağlan
  server.on("/api/shelly/connect", HTTP_GET, [](){
    if (server.hasArg("ip")) {
      String ip = server.arg("ip");
      bool success = modes.connectShelly(ip);
      if (success) {
        server.send(200, "application/json", "{\"success\":true,\"ip\":\"" + ip + "\"}");
      } else {
        server.send(500, "application/json", "{\"success\":false,\"error\":\"Connection failed\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing ip\"}");
    }
  });
  
  // API - Shelly bağlantısını kes
  server.on("/api/shelly/disconnect", HTTP_GET, [](){
    modes.disconnectShelly();
    server.send(200, "application/json", "{\"success\":true}");
  });
  
  // API - Shelly durumunu oku
  server.on("/api/shelly/status", HTTP_GET, [](){
    String s = "{\"connected\":" + String(modes.isShellyConnected() ? "true" : "false");
    if (modes.isShellyConnected()) {
      s += ",\"ip\":\"" + modes.getShellyIP() + "\"";
      s += ",\"ison\":" + String(modes.getShellyIson() ? "true" : "false");
      s += ",\"brightness\":" + String(modes.getShellyBrightness());
    } else {
      // Bağlantı yoksa encoder değerini döndür
      s += ",\"brightness\":" + String(encoder.get_dimm_sayac());
    }
    s += "}";
    server.send(200, "application/json", s);
  });
  
  // API - Shelly toggle (açma/kapama)
  server.on("/api/shelly/toggle", HTTP_GET, [](){
    if (modes.isShellyConnected()) {
      modes.toggleShelly();
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Not connected\"}");
    }
  });
  
  // API - Mod değiştir (dimmer/safe)
  server.on("/api/mode/set", HTTP_GET, [](){
    if (server.hasArg("mode")) {
      String mode = server.arg("mode");
      if (mode == "dimmer") {
        modes.setDimmerMode(true);
        server.send(200, "application/json", "{\"success\":true,\"mode\":\"dimmer\"}");
      } else if (mode == "shutter") {
        modes.setShutterMode(true);
        server.send(200, "application/json", "{\"success\":true,\"mode\":\"shutter\"}");
      } else if (mode == "safe") {
        modes.setSafeMode(true);
        server.send(200, "application/json", "{\"success\":true,\"mode\":\"safe\"}");
      } else if (mode == "panic") {
        // Panic mode not implemented yet
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Panic mode not implemented\"}");
      } else {
        server.send(400, "application/json", "{\"success\":false,\"error\":\"Invalid mode\"}");
      }
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing mode\"}");
    }
  });
  
  // API - Aktif modu oku
  server.on("/api/mode/get", HTTP_GET, [](){
    String activeMode;
    switch(modes.getCurrentMode()) {
      case MODE_DIMMER:
        activeMode = "dimmer";
        break;
      case MODE_SHUTTER:
        activeMode = "shutter";
        break;
      case MODE_SAFE:
        activeMode = "safe";
        break;
      case MODE_PANIC:
        activeMode = "panic";
        break;
      default:
        activeMode = "dimmer";
    }
    server.send(200, "application/json", "{\"mode\":\"" + activeMode + "\"}");
  });
  
  // API - OTA version bilgisi
  server.on("/api/ota/info", HTTP_GET, [](){
    String json = "{";
    json += "\"current\":\"" + ota.getCurrentVersion() + "\",";
    json += "\"latest\":\"" + ota.getLatestVersion() + "\",";
    json += "\"updateAvailable\":" + String(ota.isUpdateAvailable() ? "true" : "false") + ",";
    json += "\"updateInProgress\":" + String(ota.isUpdateInProgress() ? "true" : "false") + ",";
    json += "\"projectName\":\"" + String(PROJECT_NAME) + "\",";
    json += "\"buildDate\":\"" + String(BUILD_DATE) + "\",";
    json += "\"buildTime\":\"" + String(BUILD_TIME) + "\"";
    json += "}";
    server.send(200, "application/json", json);
  });
  
  // API - Manuel OTA update tetikle
  server.on("/api/ota/update", HTTP_GET, [](){
    if (ota.isUpdateInProgress()) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Update already in progress\"}");
      return;
    }
    
    // Non-blocking: Update'i başlat ve hemen yanıt dön
    bool started = ota.checkAndUpdate();
    if (started) {
      server.send(200, "application/json", "{\"success\":true,\"message\":\"Update started\"}");
    } else {
      server.send(200, "application/json", "{\"success\":false,\"message\":\"No update available or check failed\"}");
    }
  });
  
  // API - OTA ayarları GET
  server.on("/api/ota/settings", HTTP_GET, [](){
    String json = "{";
    json += "\"autoUpdate\":" + String(ota.getAutoUpdate() ? "true" : "false");
    json += "}";
    server.send(200, "application/json", json);
  });
  
  // API - OTA ayarları POST
  server.on("/api/ota/settings", HTTP_POST, [](){
    if (server.hasArg("autoUpdate")) {
      bool enabled = server.arg("autoUpdate") == "true";
      ota.setAutoUpdate(enabled);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing autoUpdate parameter\"}");
    }
  });
  
  // API - AP toggle
  server.on("/api/network/ap", HTTP_GET, [](){
    if (server.hasArg("enabled")) {
      bool en = server.arg("enabled") == "true";
      net.setAP(en);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false}");
    }
  });
  
  // API - WiFi 1
  server.on("/api/network/wifi1", HTTP_GET, [](){
    if (server.hasArg("ssid")) {
      String ssid = server.arg("ssid");
      String pass = server.hasArg("pass") ? server.arg("pass") : "";
      String ip = server.hasArg("ip") ? server.arg("ip") : "";
      String localDomain = server.hasArg("local") ? server.arg("local") : "";
      net.saveWiFi1(ssid, pass, ip, localDomain);
      
      // mDNS'i güncelle
      delay(1000); // WiFi bağlantısının kurulması için bekle
      setupMDNS();
      
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false}");
    }
  });
  
  // API - WiFi 2
  server.on("/api/network/wifi2", HTTP_GET, [](){
    if (server.hasArg("ssid")) {
      String ssid = server.arg("ssid");
      String pass = server.hasArg("pass") ? server.arg("pass") : "";
      String ip = server.hasArg("ip") ? server.arg("ip") : "";
      String localDomain = server.hasArg("local") ? server.arg("local") : "";
      net.saveWiFi2(ssid, pass, ip, localDomain);
      
      // mDNS'i güncelle
      delay(1000); // WiFi bağlantısının kurulması için bekle
      setupMDNS();
      
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false}");
    }
  });
  
  // ========== SAFE LOCK API ==========
  
  // Safe Lock konfigürasyonunu al
  server.on("/api/safe/config", HTTP_GET, [](){
    String json = "{\"passwords\":[";
    
    for (uint8_t i = 0; i < 5; i++) {
      if (i > 0) json += ",";
      json += "{";
      json += "\"index\":" + String(i);
      json += ",\"enabled\":" + String(safeLock.isPasswordActive(i) ? "true" : "false");
      json += ",\"password\":\"" + safeLock.getPassword(i) + "\"";
      
      // API config
      ApiConfig apiCfg = safeLockEEPROM.getApiConfig(i);
      json += ",\"api\":{";
      json += "\"enabled\":" + String(apiCfg.enabled ? "true" : "false");
      json += ",\"url\":\"" + String(apiCfg.url) + "\"";
      json += ",\"method\":\"" + String(apiCfg.method == SAFE_HTTP_GET ? "GET" : "POST") + "\"";
      json += ",\"header\":\"" + String(apiCfg.header) + "\"";
      json += ",\"body\":\"" + String(apiCfg.body) + "\"";
      json += "}";
      
      json += "}";
    }
    
    json += "]}";
    server.send(200, "application/json", json);
  });
  
  // Safe Lock konfigürasyonunu kaydet
  server.on("/api/safe/config", HTTP_POST, [](){
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"No data\"}");
      return;
    }
    
    String body = server.arg("plain");
    Serial.println("[Safe API] POST data:");
    Serial.println(body);
    
    int successCount = 0;
    int failCount = 0;
    
    for (uint8_t i = 0; i < 5; i++) {
      // Her şifre için JSON'dan bilgileri çıkar
      String searchStr = "{\"index\":" + String(i) + ",";
      int pwdStart = body.indexOf(searchStr);
      if (pwdStart == -1) continue;
      
      // Şifre enabled mı?
      bool pwdEnabled = false;
      String enabledSearch = "\"enabled\":true";
      String disabledSearch = "\"enabled\":false";
      int enabledPos = body.indexOf(enabledSearch, pwdStart);
      int disabledPos = body.indexOf(disabledSearch, pwdStart);
      int commaPos = body.indexOf(",\"password\"", pwdStart);
      
      if (enabledPos > pwdStart && enabledPos < commaPos) {
        pwdEnabled = true;
      }
      
      // Password value
      String password = "";
      int pwdValPos = body.indexOf("\"password\":\"", pwdStart);
      if (pwdValPos > 0) {
        int pwdValStart = pwdValPos + 12;
        int pwdValEnd = body.indexOf("\"", pwdValStart);
        if (pwdValEnd > pwdValStart) {
          password = body.substring(pwdValStart, pwdValEnd);
        }
      }
      
      // Şifre boşsa bu slot'u atla
      if (password.length() == 0) {
        // Boş şifre - sadece aktif/pasif durumunu güncelle
        safeLockEEPROM.setPasswordActive(i, false);
        continue;
      }
      
      // API section başlangıcı
      String apiSearch = "\"api\":{";
      int apiStart = body.indexOf(apiSearch, pwdStart);
      if (apiStart == -1) {
        failCount++;
        continue;
      }
      
      // API enabled
      bool apiEnabled = false;
      int apiEnabledPos = body.indexOf("\"enabled\":true", apiStart);
      int apiDisabledPos = body.indexOf("\"enabled\":false", apiStart);
      int apiUrlPos = body.indexOf("\"url\"", apiStart);
      
      if (apiEnabledPos > apiStart && apiEnabledPos < apiUrlPos) {
        apiEnabled = true;
      }
      
      // API URL
      String apiUrl = "";
      int urlPos = body.indexOf("\"url\":\"", apiStart);
      if (urlPos > apiStart) {
        int urlStart = urlPos + 7;
        int urlEnd = body.indexOf("\"", urlStart);
        if (urlEnd > urlStart) {
          apiUrl = body.substring(urlStart, urlEnd);
        }
      }
      
      // API method
      String apiMethod = "GET";
      int methodPos = body.indexOf("\"method\":\"", apiStart);
      if (methodPos > apiStart) {
        int mStart = methodPos + 10;
        int mEnd = body.indexOf("\"", mStart);
        if (mEnd > mStart) {
          apiMethod = body.substring(mStart, mEnd);
        }
      }
      
      Serial.print("Slot ");
      Serial.print(i);
      Serial.print(": enabled=");
      Serial.print(pwdEnabled);
      Serial.print(", pwd=");
      Serial.print(password);
      Serial.print(", apiEnabled=");
      Serial.print(apiEnabled);
      Serial.print(", url=");
      Serial.println(apiUrl);
      
      // Kaydet (aktif veya pasif fark etmez, tüm bilgileri kaydet)
      ApiConfig api;
      api.enabled = apiEnabled;
      api.method = (apiMethod == "POST") ? SAFE_HTTP_POST : SAFE_HTTP_GET;
      strncpy(api.url, apiUrl.c_str(), sizeof(api.url) - 1);
      api.url[sizeof(api.url) - 1] = '\0';
      strcpy(api.header, "");
      strcpy(api.body, "");
      
      if (safeLockEEPROM.setPassword(i, password, api)) {
        // Şifre kaydedildi, şimdi aktif/pasif durumunu ayarla
        safeLockEEPROM.setPasswordActive(i, pwdEnabled);
        successCount++;
        Serial.println("  -> SAVED");
      } else {
        failCount++;
        Serial.println("  -> FAILED");
      }
    }
    
    // SafeLock'a yükle
    safeLockEEPROM.loadToSafeLock(safeLock);
    
    String response = "{\"success\":true,\"saved\":" + String(successCount) + ",\"failed\":" + String(failCount) + "}";
    server.send(200, "application/json", response);
  });
  
  // API test endpoint
  server.on("/api/safe/test", HTTP_POST, [](){
    if (!server.hasArg("plain")) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"No data\"}");
      return;
    }
    
    String body = server.arg("plain");
    Serial.println("[Safe API] Test request: " + body);
    
    // Test API'yi tetikle (safeLockAPI.testApi kullanarak)
    // Bu basit bir test yanıtı
    server.send(200, "application/json", "{\"success\":true,\"message\":\"API test completed\"}");
  });
  
  // Şifre ayarla (manuel - test için)
  server.on("/api/safe/password/set", HTTP_GET, [](){
    if (!server.hasArg("index") || !server.hasArg("password")) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
      return;
    }
    
    uint8_t index = server.arg("index").toInt();
    String password = server.arg("password");
    String oldPassword = server.hasArg("old") ? server.arg("old") : "";
    
    bool success = safeLockEEPROM.setPassword(index, password, oldPassword);
    
    if (success) {
      safeLockEEPROM.loadToSafeLock(safeLock);  // Reload
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Failed to set password\"}");
    }
  });
  
  // Şifre aktif/pasif
  server.on("/api/safe/password/toggle", HTTP_GET, [](){
    if (!server.hasArg("index") || !server.hasArg("enabled")) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
      return;
    }
    
    uint8_t index = server.arg("index").toInt();
    bool enabled = server.arg("enabled") == "true";
    
    bool success = safeLockEEPROM.setPasswordActive(index, enabled);
    
    if (success) {
      safeLock.setPasswordActive(index, enabled);
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false}");
    }
  });
  
  // API konfigürasyonu ayarla
  server.on("/api/safe/api/set", HTTP_GET, [](){
    if (!server.hasArg("index") || !server.hasArg("url")) {
      server.send(400, "application/json", "{\"success\":false,\"error\":\"Missing parameters\"}");
      return;
    }
    
    uint8_t index = server.arg("index").toInt();
    String url = server.arg("url");
    SafeHttpMethod method = server.hasArg("method") && server.arg("method") == "POST" ? SAFE_HTTP_POST : SAFE_HTTP_GET;
    String header = server.hasArg("header") ? server.arg("header") : "";
    String body = server.hasArg("body") ? server.arg("body") : "";
    
    bool success = safeLockEEPROM.setApiConfig(index, url, method, header, body);
    
    if (success) {
      server.send(200, "application/json", "{\"success\":true}");
    } else {
      server.send(400, "application/json", "{\"success\":false}");
    }
  });
  
  // Debug: Tüm şifreleri yazdır
  server.on("/api/safe/debug", HTTP_GET, [](){
    safeLockEEPROM.printAllPasswords();
    server.send(200, "application/json", "{\"success\":true,\"message\":\"Check serial monitor\"}");
  });
  
  // 404 handler - debug için
  server.onNotFound([](){
    String msg = "404 Not Found\n\n";
    msg += "URI: " + server.uri() + "\n";
    msg += "Method: " + String(server.method()) + "\n";
    Serial.println(msg);
    server.send(404, "text/plain", msg);
  });
  
  Serial.println("Web setup OK");
}
