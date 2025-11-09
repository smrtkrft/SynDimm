/*
 * SynDimm - Modes Library
 * Dimmer, Shutter, Safe and Panic mode management
 * Powered by SEU - Emek - SmartKraft
 */

#ifndef MODES_H
#define MODES_H

#include "encoder.h"
#include <HTTPClient.h>
#include <Preferences.h>
#include "safe_lock.h"  // Include SafeLock class
#include "syndimm_buzzer.h"  // For buzzer
#include "shutter.h"  // Include Shutter class

// Mode enumeration (order matters: Left to Right)
enum OperationMode {
  MODE_DIMMER = 1,   // 1 beep
  MODE_SHUTTER = 2,  // 2 beeps
  MODE_SAFE = 3,     // 3 beeps
  MODE_PANIC = 4     // 4 beeps (future implementation)
};

class ModeManager {
private:
  Encoder* encoder;
  SafeLock* safeLock;  // Safe Lock reference
  Shutter* shutter;    // Shutter reference
  SynDimmBuzzer* buzzer;  // Buzzer reference (for mode change beep)
  
  // Current active mode
  OperationMode currentMode;
  OperationMode pendingMode;  // Temporary mode during selection
  
  // Legacy mode flags (for backward compatibility during transition)
  bool dimmerMode;
  bool safeMode;
  
  int dimmRatio;  // 1-5 range, how many units change per encoder tick
  
  // Mathematical counters (moved from encoder)
  long L_deger;          // Left direction consecutive counter
  long R_deger;          // Right direction consecutive counter
  char lastDirection;    // Last direction ('L' or 'R')
  
  // Mode change logic (moved from encoder.h)
  bool modeSelectActive;        // Is mode select active
  char modeChangeDirection;     // Last direction during mode change
  bool encoderRotatedDuringPress;  // Did encoder rotate after P event
  
  // ========== DIMMER MODE - DEVICE INTEGRATION ==========
  String deviceIP;                    // Connected device IP address
  String deviceType;                  // Device type (shelly-dimmer, shelly-dali, etc)
  bool deviceConnected;               // Connection status
  bool deviceIson;                    // Device on/off status
  int deviceBrightness;               // Device dimm value (0-100)
  unsigned long lastDeviceSync;       // Last synchronization time
  unsigned long lastDimmChange;       // Last dimm_sayac change time
  int lastSentBrightness;             // Last value sent to device
  const unsigned long deviceSyncIntervalActive = 500;    // Active mode: 500ms (fast response)
  const unsigned long deviceSyncIntervalIdle = 5000;     // Idle mode: 5s (save bandwidth)
  const unsigned long dimmerSendDelay = 150;             // Send 150ms after encoder stops
  
  // Helper: Play buzzer based on mode number
  void playModeBuzzer(OperationMode mode) {
    if (!buzzer) return;
    
    switch(mode) {
      case MODE_DIMMER:
        buzzer->playShort();  // 1 beep
        break;
      case MODE_SHUTTER:
        buzzer->playDouble();  // 2 beeps
        break;
      case MODE_SAFE:
        buzzer->playTriple();  // 3 beeps
        break;
      case MODE_PANIC:
        // Future: 4 beeps
        break;
    }
  }
  
public:
  ModeManager(Encoder* enc) : encoder(enc), safeLock(nullptr), shutter(nullptr), buzzer(nullptr), 
                            currentMode(MODE_DIMMER), pendingMode(MODE_DIMMER),
                            dimmerMode(true), safeMode(false), dimmRatio(3), 
                            L_deger(0), R_deger(0), lastDirection(0), 
                            modeSelectActive(false), modeChangeDirection(0), encoderRotatedDuringPress(false),
                            deviceIP(""), deviceType(""), deviceConnected(false), deviceIson(false), 
                            deviceBrightness(0), lastDeviceSync(0), lastDimmChange(0), 
                            lastSentBrightness(-1) {}
  
  void setBuzzer(SynDimmBuzzer* bz) {
    buzzer = bz;
  }
  
  void setShutter(Shutter* sh) {
    shutter = sh;
  }
  
  void begin() {
    // Initial settings
    L_deger = 0;
    R_deger = 0;
    lastDirection = 0;
    
    // Load saved settings
    Preferences prefs;
    prefs.begin("syndimm", true);
    dimmRatio = prefs.getInt("dimmRatio", 1);  // Default 1
    
    // Load last active mode (continue after power loss)
    int savedMode = prefs.getInt("currentMode", MODE_DIMMER);
    currentMode = (OperationMode)savedMode;
    pendingMode = currentMode;
    
    // Update legacy flags for compatibility
    dimmerMode = (currentMode == MODE_DIMMER);
    safeMode = (currentMode == MODE_SAFE);
    
    // DEBUG REMOVED: Last mode loaded
    
    prefs.end();
    
    // Set dimm_sayac initial value to 100
    encoder->set_dimm_sayac(100);
    
    // DEBUG REMOVED: Dimmer start, ratio, counters
    
    // Load saved device
    loadDevice();
  }
  
  // Get mode name as string
  String getModeName(OperationMode mode) {
    switch(mode) {
      case MODE_DIMMER: return "Dimmer";
      case MODE_SHUTTER: return "Shutter";
      case MODE_SAFE: return "Safe";
      case MODE_PANIC: return "Panic";
      default: return "Unknown";
    }
  }
  
  // Mode selection (legacy compatibility)
  // CRITICAL: Mode changes NEVER disconnect the device
  // Device connection persists across all modes and reboots
  void setDimmerMode(bool enable) {
    if (enable) {
      currentMode = MODE_DIMMER;
      dimmerMode = true;
      safeMode = false;
      // DEBUG REMOVED
      
      // Save to EEPROM
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("currentMode", currentMode);
      prefs.end();
    }
  }
  
  void setShutterMode(bool enable) {
    if (enable) {
      currentMode = MODE_SHUTTER;
      dimmerMode = false;
      safeMode = false;
      // DEBUG REMOVED
      
      // Save to EEPROM
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("currentMode", currentMode);
      prefs.end();
    }
  }
  
  void setSafeMode(bool enable) {
    if (enable) {
      currentMode = MODE_SAFE;
      safeMode = true;
      dimmerMode = false;
      // DEBUG REMOVED
      
      // Save to EEPROM
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("currentMode", currentMode);
      prefs.end();
    }
  }
  
  // Mod değiştirme (toggle)
  void toggleMode() {
    if (dimmerMode) {
      setSafeMode(true);
      // DEBUG REMOVED
    } else {
      setDimmerMode(true);
      // DEBUG REMOVED
    }
  }
  
  bool isDimmerMode() { return dimmerMode; }
  bool isSafeMode() { return safeMode; }
  
  // Safe Lock referansını ayarla
  void setSafeLock(SafeLock* sl) {
    safeLock = sl;
  }
  
  // Dimm oranı ayarları
  void setDimmRatio(int ratio) {
    if (ratio >= 1 && ratio <= 5) {
      dimmRatio = ratio;
      
      // Preferences'a kaydet
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("dimmRatio", ratio);
      prefs.end();
      
      // DEBUG REMOVED
    }
  }
  
  int getDimmRatio() { return dimmRatio; }
  
  // Process encoder events and perform mathematical operations - SEU
  void processEncoderEvent(char event) {
    if (event == 0) return;  // Empty event
    
    // DEBUG REMOVED: Event received
    
    // === NEW MODE CHANGE LOGIC: DIMMER(1) -> SHUTTER(2) -> SAFE(3) -> PANIC(4) ===
    
    // 'P' event = Long press (3+ seconds)
    if (event == 'P') {
      // If mode select already active, this is CONFIRM
      if (modeSelectActive && encoderRotatedDuringPress) {
        Serial.println("[Mode] *** MODE CONFIRMED *** - Switching to: " + getModeName(pendingMode));
        
        // Apply the pending mode
        currentMode = pendingMode;
        
        // Update legacy flags
        dimmerMode = (currentMode == MODE_DIMMER);
        safeMode = (currentMode == MODE_SAFE);
        
        // Save to EEPROM
        Preferences prefs;
        prefs.begin("syndimm", false);
        prefs.putInt("currentMode", currentMode);
        prefs.end();
        
        // Play final buzzer
        playModeBuzzer(currentMode);
        
        // DEBUG REMOVED: Active mode
        
        // Exit mode select
        modeSelectActive = false;
        encoderRotatedDuringPress = false;
        return;
      }
      
      // First 'P' press - START mode select
      modeSelectActive = true;
      pendingMode = currentMode;  // Start from current mode
      encoderRotatedDuringPress = false;
      
      Serial.println("[Mode] *** MODE SELECT ACTIVE ***");
      Serial.print("[Mode] Current: ");
      Serial.print(getModeName(currentMode));
      Serial.println(" - Rotate to select, hold 3s to confirm");
      return;
    }
    
    // In mode select: handle LEFT/RIGHT rotation
    if (modeSelectActive) {
      if (event == 'L') {
        // LEFT: previous mode (with boundary check)
        OperationMode newMode = pendingMode;
        
        if (pendingMode == MODE_SHUTTER) {
          newMode = MODE_DIMMER;  // 2 -> 1
        } else if (pendingMode == MODE_SAFE) {
          newMode = MODE_SHUTTER;  // 3 -> 2
        } else if (pendingMode == MODE_PANIC) {
          newMode = MODE_SAFE;  // 4 -> 3
        }
        // If MODE_DIMMER, stay at MODE_DIMMER (boundary)
        
        if (newMode != pendingMode) {
          pendingMode = newMode;
          encoderRotatedDuringPress = true;
          playModeBuzzer(pendingMode);  // Play beeps for target mode
          // DEBUG REMOVED: LEFT
        } else {
          // DEBUG REMOVED: Boundary
        }
        return;
        
      } else if (event == 'R') {
        // RIGHT: next mode (with boundary check)
        OperationMode newMode = pendingMode;
        
        if (pendingMode == MODE_DIMMER) {
          newMode = MODE_SHUTTER;  // 1 -> 2
        } else if (pendingMode == MODE_SHUTTER) {
          newMode = MODE_SAFE;  // 2 -> 3
        } else if (pendingMode == MODE_SAFE) {
          newMode = MODE_PANIC;  // 3 -> 4 (not implemented yet)
        }
        // If MODE_PANIC or MODE_SAFE (until panic ready), stay at MODE_SAFE
        
        // Block transition to PANIC (not implemented)
        if (newMode == MODE_PANIC) {
          // DEBUG REMOVED: PANIC not implemented
          return;
        }
        
        if (newMode != pendingMode) {
          pendingMode = newMode;
          encoderRotatedDuringPress = true;
          playModeBuzzer(pendingMode);  // Play beeps for target mode
          // DEBUG REMOVED: RIGHT
        } else {
          // DEBUG REMOVED: Boundary
        }
        return;
        
      } else if (event == 'B') {
        // Short press - CANCEL mode select
        // DEBUG REMOVED: Cancelled
        pendingMode = currentMode;  // Reset to current
        modeSelectActive = false;
        encoderRotatedDuringPress = false;
      }
      return;  // In mode select, don't process normal operations
    }
    
    // === NORMAL MODE OPERATIONS (based on active mode) ===
    
    // Update direction counters (moved from encoder)
    if (event == 'L') {
      if (lastDirection != 'L') {
        R_deger = 0;  // Direction changed, reset opposite counter
      }
      L_deger++;
      lastDirection = 'L';
      
    } else if (event == 'R') {
      if (lastDirection != 'R') {
        L_deger = 0;  // Direction changed, reset opposite counter
      }
      R_deger++;
      lastDirection = 'R';
    }
    
    // Process based on current mode
    switch(currentMode) {
      case MODE_DIMMER:
        processDimmerMode(event);
        break;
      case MODE_SHUTTER:
        if (shutter) shutter->processEncoderEvent(event);
        break;
      case MODE_SAFE:
        processSafeMode(event);
        break;
      case MODE_PANIC:
        // Future implementation
        break;
    }
  }
  
  // Direction counters
  long getLeftCount() { return L_deger; }
  long getRightCount() { return R_deger; }
  
  // Current mode getter
  OperationMode getCurrentMode() { return currentMode; }
  String getCurrentModeName() { return getModeName(currentMode); }
  
private:
  // Dimmer mode processing - Emek
  void processDimmerMode(char event) {
    int currentValue = encoder->get_dimm_sayac();
    
    if (event == 'L') {
      // Left: decrease
      int newValue = currentValue - dimmRatio;
      if (newValue < 0) newValue = 0;
      encoder->set_dimm_sayac(newValue);
      lastDimmChange = millis();  // Record change time
      // DEBUG REMOVED: Dimmer value
      
    } else if (event == 'R') {
      // Sağ: arttır
      int newValue = currentValue + dimmRatio;
      if (newValue > 100) newValue = 100;
      encoder->set_dimm_sayac(newValue);
      lastDimmChange = millis();  // Değişiklik zamanını kaydet
      // DEBUG REMOVED: Dimmer value
      
    } else if (event == 'B') {
      // Buton: Cihaz toggle (açık/kapalı)
      if (deviceConnected) {
        toggleDevice();
      } else {
        // DEBUG REMOVED: Device not connected
      }
    }
  }
  
  void processSafeMode(char event) {
    // Safe mod: Encoder hareketlerini Safe Lock'a yönlendir
    if (safeLock == nullptr) {
      Serial.println("[ERROR] Safe Lock not initialized!");
      return;
    }
    
    if (event == 'L' || event == 'R') {
      // Encoder hareketi - Safe Lock'a bildir
      bool clockwise = (event == 'R');
      safeLock->onEncoderMove(clockwise);
      
      // Debug: L ve R sayaçlarını yazdır
      Serial.print(event);
      Serial.print(event == 'L' ? L_deger : R_deger);
      Serial.print(" ");
      safeLock->printBufferStatus();
      
    } else if (event == 'B') {
      // Buton basıldı - Safe Lock'a bildir
      safeLock->onButtonPress();
      // DEBUG REMOVED: Button pressed
    }
  }

public:
  // ========== CİHAZ YÖNETİMİ FONKSİYONLARI ==========
  
  // Cihaza bağlan (Shelly Dimmer, Shelly DALI, vb)
  bool connectDevice(String ip, String type = "shelly-dimmer") {
    deviceIP = ip;
    deviceType = type;
    deviceConnected = false;
    
    // Test bağlantısı yap
    HTTPClient http;
    String url = "http://" + ip + "/light/0";
    http.begin(url);
    http.setTimeout(500);  // CRITICAL: 500ms - fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      deviceConnected = true;
      Serial.println("[Device] Connected: " + ip + " (" + type + ")");
      // DEBUG REMOVED: Response payload
      
      // İlk durumu oku
      parseDeviceStatus(payload);
      
      // Cihaz bilgilerini kaydet
      saveDevice(ip, type);
      
      http.end();
      return true;
    } else {
      Serial.println("[ERROR] Connection failed: " + String(httpCode));
      http.end();
      return false;
    }
  }
  
  // Geriye uyumluluk için (eski API çağrıları için)
  bool connectShelly(String ip) {
    return connectDevice(ip, "shelly-dimmer");
  }
  
  // CRITICAL: Disconnect device connection and remove from EEPROM
  // WARNING: This should ONLY be called by user action via web UI
  // NEVER call this on mode change or reboot!
  // Device connection must persist across all modes and reboots
  void disconnectDevice() {
    deviceConnected = false;
    deviceIP = "";
    deviceType = "";
    deviceIson = false;
    deviceBrightness = 0;
    
    // Remove saved device from EEPROM
    Preferences prefs;
    prefs.begin("syndimm", false);
    prefs.remove("deviceIP");
    prefs.remove("deviceType");
    prefs.end();
    
    Serial.println("[Modes] *** DEVICE DISCONNECTED BY USER *** - Connection removed from EEPROM");
  }
  
  // Geriye uyumluluk için
  void disconnectShelly() {
    disconnectDevice();
  }
  
  // Cihaz durumunu oku (polling)
  void syncFromDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    HTTPClient http;
    String url = "http://" + deviceIP + "/light/0";
    http.begin(url);
    http.setTimeout(500);  // CRITICAL: 500ms - fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      parseDeviceStatus(payload);
    } else {
      Serial.print("[ERROR] Device read failed: ");
      Serial.print(httpCode);
      Serial.print(" (");
      Serial.print(deviceIP);
      Serial.println(")");
      // 3 başarısız denemeden sonra bağlantıyı kes
      static int failCount = 0;
      failCount++;
      if (failCount > 3) {
        Serial.println("[ERROR] 3 failed attempts - disconnecting");
        disconnectDevice();
        failCount = 0;
      }
    }
    
    http.end();
  }
  
  // Geriye uyumluluk için
  void syncFromShelly() {
    syncFromDevice();
  }
  
  // dimm_sayac'ı cihaza gönder
  void syncToDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    int currentBrightness = encoder->get_dimm_sayac();
    
    // Değişiklik yoksa gönderme
    if (currentBrightness == lastSentBrightness) return;
    
    HTTPClient http;
    String url;
    
    // Brightness = 0 ise cihazı kapat, değilse brightness gönder
    if (currentBrightness == 0) {
      // Shelly brightness=0 kabul etmiyor, turn=off kullan
      url = "http://" + deviceIP + "/light/0?turn=off&transition=0";
      if (deviceIson) {  // Sadece açıksa kapat
        deviceIson = false;
        // DEBUG REMOVED: Device turning off
      } else {
        http.end();
        return;  // Zaten kapalı, istek gönderme
      }
    } else {
      // Cihaz kapalıysa önce aç, sonra brightness ayarla
      if (!deviceIson) {
        url = "http://" + deviceIP + "/light/0?turn=on&brightness=" + String(currentBrightness) + "&transition=0";
        deviceIson = true;
        // DEBUG REMOVED: Device turning on
      } else {
        // Zaten açık, sadece brightness ayarla
        url = "http://" + deviceIP + "/light/0?brightness=" + String(currentBrightness) + "&transition=0";
      }
    }
    
    http.begin(url);
    http.setTimeout(500);  // CRITICAL: 500ms - fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      lastSentBrightness = currentBrightness;
      // DEBUG REMOVED: Brightness sent
    } else {
      Serial.println("[ERROR] Brightness send failed: " + String(httpCode));
    }
    
    http.end();
  }
  
  // Geriye uyumluluk için
  void syncToShelly() {
    syncToDevice();
  }
  
  // Cihaz açma/kapama toggle
  void toggleDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    String command = deviceIson ? "off" : "on";
    
    HTTPClient http;
    // transition=0 ile anında değişim
    String url = "http://" + deviceIP + "/light/0?turn=" + command + "&transition=0";
    
    // Açarken %80 brightness ayarla
    if (!deviceIson) {
      url += "&brightness=80";
    }
    
    http.begin(url);
    http.setTimeout(500);  // CRITICAL: 500ms - fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      deviceIson = !deviceIson;
      Serial.println("[Device] Toggle: " + command + " (" + deviceType + ")");
      
      // Açıldıysa mevcut dimm_sayac'ı gönder (delay yerine flag kullan)
      if (deviceIson) {
        lastDimmChange = millis() - dimmerSendDelay + 100; // 100ms sonra gönderilecek
      }
    } else {
      Serial.println("[ERROR] Toggle failed: " + String(httpCode));
    }
    
    http.end();
  }
  
  // Geriye uyumluluk için
  void toggleShelly() {
    toggleDevice();
  }
  
  // Cihaz status JSON parse (basit)
  void parseDeviceStatus(String json) {
    // {"ison":true,"brightness":50,...}
    int isonIndex = json.indexOf("\"ison\":");
    int brightnessIndex = json.indexOf("\"brightness\":");
    
    if (isonIndex > 0) {
      String isonStr = json.substring(isonIndex + 7, isonIndex + 12);
      deviceIson = (isonStr.indexOf("true") >= 0);
    }
    
    if (brightnessIndex > 0) {
      int startIdx = brightnessIndex + 13;
      int endIdx = json.indexOf(",", startIdx);
      if (endIdx < 0) endIdx = json.indexOf("}", startIdx);
      String brightStr = json.substring(startIdx, endIdx);
      int newBrightness = brightStr.toInt();
      
      // DeviceBrightness'ı HER ZAMAN güncelle (web arayüzü için)
      deviceBrightness = newBrightness;
      
      // Encoder'ı SADECE cihazdan farklı bir değer geldiğinde ve encoder durgunken güncelle
      if (newBrightness != encoder->get_dimm_sayac() && deviceIson && lastDimmChange == 0) {
        encoder->set_dimm_sayac(newBrightness);
        // DEBUG REMOVED: Encoder synced
      }
    }
  }
  
  // Geriye uyumluluk için
  void parseShellyStatus(String json) {
    parseDeviceStatus(json);
  }
  
  // CRITICAL: Device synchronization - called every loop()
  // This ensures persistent connection across all modes and reboots
  // Auto-reconnect mechanism: retries every 5 seconds if connection lost
  void updateDevice() {
    // If device not connected but IP exists, auto-reconnect
    if (!deviceConnected && deviceIP != "") {
      static unsigned long lastConnectAttempt = 0;
      unsigned long now = millis();
      
      // İlk deneme için özel durum (lastConnectAttempt == 0)
      // veya 5 saniye geçmişse tekrar dene
      if (lastConnectAttempt == 0 || (now - lastConnectAttempt > 5000)) {
        Serial.println("[Modes] Auto-reconnecting: " + deviceIP);
        connectDevice(deviceIP, deviceType);
        lastConnectAttempt = now;
      }
      return;
    }
    
    unsigned long now = millis();
    
    // Only sync in DIMMER mode (encoder controls brightness)
    if (currentMode != MODE_DIMMER) {
      // Keep connection alive with periodic polling even in other modes (5s idle)
      if (now - lastDeviceSync > deviceSyncIntervalIdle * 2) {
        syncFromDevice();
        lastDeviceSync = now;
      }
      return;
    }
    
    // DIMMER MODE: Active synchronization
    // If encoder stopped rotating, send value to device after 150ms
    if (lastDimmChange > 0 && now - lastDimmChange > dimmerSendDelay) {
      syncToDevice();
      lastDimmChange = 0;  // Reset
    }
    
    // Poll device status - Event-driven interval
    // Active: 500ms (encoder moving), Idle: 5s (encoder idle)
    unsigned long dynamicInterval = (lastDimmChange > 0) ? deviceSyncIntervalActive : deviceSyncIntervalIdle;
    
    if (lastDimmChange == 0 && now - lastDeviceSync > dynamicInterval) {
      syncFromDevice();
      lastDeviceSync = now;
    }
  }
  
  // For backward compatibility - SmartKraft
  void updateShelly() {
    updateDevice();
  }
  
  // Getters
  bool isDeviceConnected() { return deviceConnected; }
  String getDeviceIP() { return deviceIP; }
  String getDeviceType() { return deviceType; }
  bool getDeviceIson() { return deviceIson; }
  int getDeviceBrightness() { return deviceBrightness; }
  
  // Called when changes are made from web interface
  void triggerDimmChange() {
    lastDimmChange = millis();
  }
  
  // Geriye uyumluluk için (eski API)
  bool isShellyConnected() { return deviceConnected; }
  String getShellyIP() { return deviceIP; }
  bool getShellyIson() { return deviceIson; }
  int getShellyBrightness() { return deviceBrightness; }
  
  // CRITICAL: Save device to EEPROM for persistent connection
  // This ensures device reconnects after reboot in any mode
  // Called automatically when device connects successfully
  void saveDevice(String ip, String type) {
    Preferences prefs;
    prefs.begin("syndimm", false);
    prefs.putString("deviceIP", ip);
    prefs.putString("deviceType", type);
    prefs.end();
    Serial.println("[Modes] *** DEVICE SAVED TO EEPROM *** IP: " + ip + " Type: " + type);
    Serial.println("[Modes] Device will auto-reconnect after reboot in any mode");
  }

private:
  // ========== PREFERENCES FUNCTIONS ==========
  
  // Kaydedilmiş cihazı yükle (otomatik bağlantı loop'ta yapılacak)
  // CRITICAL: Load saved device from EEPROM on boot
  // This ensures device reconnects automatically after reboot
  // Called once in begin() - updateDevice() handles reconnection
  void loadDevice() {
    Preferences prefs;
    prefs.begin("syndimm", true);  // Read-only
    String savedIP = prefs.getString("deviceIP", "");
    String savedType = prefs.getString("deviceType", "shelly-dimmer");
    prefs.end();
    
    if (savedIP != "") {
      Serial.println("[Modes] *** SAVED DEVICE FOUND *** IP: " + savedIP + " Type: " + savedType);
      Serial.println("[Modes] Auto-reconnect will start in 5 seconds via updateDevice()");
      deviceIP = savedIP;
      deviceType = savedType;
      deviceConnected = false;  // Will be set to true by updateDevice() auto-reconnect
    } else {
      Serial.println("[Modes] No saved device - Connect via web UI");
    }
  }
};

#endif // MODES_H
