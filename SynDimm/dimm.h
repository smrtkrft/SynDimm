/*
 * SynDimm - Dimmer Control Library
 * Shelly Dimmer, Shelly DALI and other device control
 * Powered by SEU - Emek - SmartKraft
 */

#ifndef DIMM_H
#define DIMM_H

#include "encoder.h"
#include <HTTPClient.h>
#include <Preferences.h>

class DimmerControl {
private:
  Encoder* encoder;
  
  // Device connection
  String deviceIP;                    // Connected device IP address
  String deviceType;                  // Device type (shelly-dimmer, shelly-dali, etc)
  bool deviceConnected;               // Connection status
  bool deviceIson;                    // Device on/off status
  int deviceBrightness;               // Device dimm value (0-100)
  unsigned long lastDeviceSync;       // Last synchronization time
  unsigned long lastDimmChange;       // Last dimm_sayac change time
  int lastSentBrightness;             // Last value sent to device
  int failCount;                      // Connection fail counter (was static - BUG FIX)
  bool httpRequestInProgress;         // HTTP request lock to prevent race condition
  
  // Extra turn=on protection (Shelly turns off during fast brightness changes)
  unsigned long lastBrightnessChangeTime;  // Last brightness change timestamp
  bool needsExtraTurnOn;                   // Flag: needs extra turn=on command
  
  // Sync intervals
  const unsigned long deviceSyncIntervalActive = 500;     // Active mode: 500ms (encoder değişimi hemen gönder)
  const unsigned long deviceSyncIntervalIdle = 2000;      // Idle mode: 2s (Shelly işleme süresi + güvenlik marjini)
  const unsigned long dimmerSendDelay = 150;              // Send 150ms after encoder stops
  const unsigned long extraTurnOnDelay = 200;             // Wait 200ms after brightness, then send turn=on
  
  // Dimm ratio (sensitivity)
  int dimmRatio;  // 1-5 range, how many units change per encoder tick
  
public:
  DimmerControl(Encoder* enc) : encoder(enc), 
                               deviceIP(""), deviceType(""), deviceConnected(false), 
                               deviceIson(false), deviceBrightness(0), 
                               lastDeviceSync(0), lastDimmChange(0), 
                               lastSentBrightness(-1), dimmRatio(1), 
                               failCount(0), httpRequestInProgress(false),
                               lastBrightnessChangeTime(0), needsExtraTurnOn(false) {}
  
  void begin() {
    // Load saved settings
    Preferences prefs;
    prefs.begin("syndimm", true);
    dimmRatio = prefs.getInt("dimmRatio", 1);  // Default 1
    prefs.end();
    
    // Set dimm_sayac initial value to 100
    encoder->set_dimm_sayac(100);
    
    Serial.println("[Dimm] Initialized - Ratio: " + String(dimmRatio));
    
    // Load saved device
    loadDevice();
  }
  
  // ========== DIMM RATIO MANAGEMENT ==========
  
  void setDimmRatio(int ratio) {
    if (ratio >= 1 && ratio <= 5) {
      dimmRatio = ratio;
      
      // Save to Preferences
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("dimmRatio", ratio);
      prefs.end();
      
      Serial.println("[Dimm] Ratio set: " + String(ratio));
    }
  }
  
  int getDimmRatio() { return dimmRatio; }
  
  // ========== ENCODER EVENT PROCESSING ==========
  
  // Process encoder events for dimmer control
  void processEncoderEvent(char event) {
    if (event == 0) return;  // Empty event
    
    int currentValue = encoder->get_dimm_sayac();
    
    if (event == 'L') {
      // Left: decrease
      int newValue = currentValue - dimmRatio;
      if (newValue < 0) newValue = 0;
      encoder->set_dimm_sayac(newValue);
      lastDimmChange = millis();  // Record change time
      Serial.println("[Dimm] Decreased: " + String(newValue));
      
    } else if (event == 'R') {
      // Right: increase
      int newValue = currentValue + dimmRatio;
      if (newValue > 100) newValue = 100;
      encoder->set_dimm_sayac(newValue);
      lastDimmChange = millis();  // Record change time
      Serial.println("[Dimm] Increased: " + String(newValue));
      
    } else if (event == 'B') {
      // Button: Device toggle (on/off)
      if (deviceConnected) {
        toggleDevice();
      } else {
        Serial.println("[Dimm] Device not connected");
      }
    }
  }
  
  // ========== DEVICE CONNECTION MANAGEMENT ==========
  
  // Connect to device (Shelly Dimmer, Shelly DALI, etc)
  bool connectDevice(String ip, String type = "shelly-dimmer") {
    deviceIP = ip;
    deviceType = type;
    deviceConnected = false;
    
    // Test connection
    HTTPClient http;
    String url = "http://" + ip + "/light/0";
    http.begin(url);
    http.setTimeout(300);  // CRITICAL: 300ms - very fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      deviceConnected = true;
      Serial.println("[Dimm] Connected: " + ip + " (" + type + ")");
      
      // Read initial status
      parseDeviceStatus(payload);
      
      // Save device info
      saveDevice(ip, type);
      
      http.end();
      return true;
    } else {
      Serial.println("[ERROR] Connection failed: " + String(httpCode));
      http.end();
      return false;
    }
  }
  
  // Backward compatibility (old API calls)
  bool connectShelly(String ip) {
    return connectDevice(ip, "shelly-dimmer");
  }
  
  // CRITICAL: Disconnect device connection and remove from EEPROM
  // WARNING: This should ONLY be called by user action via web UI
  // NEVER call this on mode change or reboot!
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
    
    Serial.println("[Dimm] *** DEVICE DISCONNECTED BY USER *** - Connection removed from EEPROM");
  }
  
  // Backward compatibility
  void disconnectShelly() {
    disconnectDevice();
  }
  
  // ========== DEVICE SYNCHRONIZATION ==========
  
  // Read device status (polling)
  void syncFromDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    // Prevent multiple simultaneous HTTP requests
    if (httpRequestInProgress) return;
    httpRequestInProgress = true;
    
    HTTPClient http;
    String url = "http://" + deviceIP + "/light/0";
    http.begin(url);
    http.setTimeout(300);  // CRITICAL: 300ms - very fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      parseDeviceStatus(payload);
      failCount = 0;  // Reset fail counter on success
    } else {
      Serial.print("[ERROR] Device read failed: ");
      Serial.print(httpCode);
      Serial.print(" (");
      Serial.print(deviceIP);
      Serial.println(")");
      
      // Disconnect after 3 failed attempts
      failCount++;
      if (failCount > 3) {
        Serial.println("[ERROR] 3 failed attempts - disconnecting");
        disconnectDevice();
        failCount = 0;
      }
    }
    
    http.end();
    httpRequestInProgress = false;
  }
  
  // Backward compatibility
  void syncFromShelly() {
    syncFromDevice();
  }
  
  // Send dimm_sayac to device
  void syncToDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    // CRITICAL: Prevent multiple simultaneous HTTP requests during fast encoder rotation
    if (httpRequestInProgress) {
      Serial.println("[WARN] HTTP request in progress - skipping sync");
      return;
    }
    
    int currentBrightness = encoder->get_dimm_sayac();
    
    // Don't send if no change
    if (currentBrightness == lastSentBrightness) return;
    
    httpRequestInProgress = true;
    
    HTTPClient http;
    String url;
    bool willTurnOff = false;
    
    // If brightness = 0, turn off device; otherwise send brightness
    if (currentBrightness == 0) {
      // Shelly doesn't accept brightness=0, use turn=off
      url = "http://" + deviceIP + "/light/0?turn=off&transition=0";
      if (!deviceIson) {
        // Already off, don't send request
        httpRequestInProgress = false;
        return;
      }
      willTurnOff = true;
      Serial.println("[Dimm] Turning off device");
    } else {
      // Brightness 1-100: Device must be ON
      // CRITICAL: Always send turn=on to ensure device stays on during fast rotation
      url = "http://" + deviceIP + "/light/0?turn=on&brightness=" + String(currentBrightness) + "&transition=0";
      
      if (!deviceIson) {
        Serial.println("[Dimm] Turning on device");
      }
    }
    
    http.begin(url);
    http.setTimeout(300);  // CRITICAL: 300ms - very fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      lastSentBrightness = currentBrightness;
      
      // CRITICAL: Update deviceIson ONLY after successful HTTP request
      if (willTurnOff) {
        deviceIson = false;
        needsExtraTurnOn = false;  // No extra turn=on for off command
      } else {
        deviceIson = true;  // Brightness 1-100 = device should be ON
        
        // SHELLY BUG FIX: Shelly turns off during fast brightness changes
        // Schedule extra turn=on 200ms after brightness command
        lastBrightnessChangeTime = millis();
        needsExtraTurnOn = true;
      }
      
      Serial.println("[Dimm] Brightness sent: " + String(currentBrightness) + " (Device: " + (deviceIson ? "ON" : "OFF") + ")");
    } else {
      Serial.println("[ERROR] Brightness send failed: " + String(httpCode));
    }
    
    http.end();
    httpRequestInProgress = false;
  }
  
  // SHELLY BUG FIX: Send extra turn=on after brightness command
  // Shelly devices sometimes turn off during fast brightness changes
  void sendExtraTurnOn() {
    if (!deviceConnected || deviceIP == "") return;
    if (httpRequestInProgress) return;  // Don't interfere with ongoing requests
    
    int currentBrightness = encoder->get_dimm_sayac();
    
    // Only send if brightness > 0 (device should be on)
    if (currentBrightness == 0) return;
    
    httpRequestInProgress = true;
    
    HTTPClient http;
    String url = "http://" + deviceIP + "/light/0?turn=on&transition=0";
    
    http.begin(url);
    http.setTimeout(300);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      deviceIson = true;
      Serial.println("[Dimm] Extra turn=on sent (Shelly protection)");
    } else {
      Serial.println("[WARN] Extra turn=on failed: " + String(httpCode));
    }
    
    http.end();
    httpRequestInProgress = false;
  }
  
  // Backward compatibility
  void syncToShelly() {
    syncToDevice();
  }
  
  // Toggle device on/off
  void toggleDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    // CRITICAL: Prevent race condition during fast button presses
    if (httpRequestInProgress) {
      Serial.println("[WARN] HTTP request in progress - skipping toggle");
      return;
    }
    httpRequestInProgress = true;
    
    String command = deviceIson ? "off" : "on";
    
    HTTPClient http;
    // Instant transition with transition=0
    String url = "http://" + deviceIP + "/light/0?turn=" + command + "&transition=0";
    
    // When turning on, set 80% brightness
    if (!deviceIson) {
      url += "&brightness=80";
    }
    
    http.begin(url);
    http.setTimeout(300);  // CRITICAL: 300ms - very fast fail, non-blocking
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      deviceIson = !deviceIson;
      Serial.println("[Dimm] Toggle: " + command + " (" + deviceType + ")");
      
      // If turned on, send current dimm_sayac after delay
      if (deviceIson) {
        lastDimmChange = millis() - dimmerSendDelay + 100; // Will be sent after 100ms
      }
    } else {
      Serial.println("[ERROR] Toggle failed: " + String(httpCode));
    }
    
    http.end();
    httpRequestInProgress = false;
  }
  
  // Backward compatibility
  void toggleShelly() {
    toggleDevice();
  }
  
  // Parse device status JSON (simple)
  void parseDeviceStatus(String json) {
    // {"ison":true,"brightness":50,...}
    int isonIndex = json.indexOf("\"ison\":");
    int brightnessIndex = json.indexOf("\"brightness\":");
    
    // CRITICAL: Don't update deviceIson during active encoder rotation
    // This prevents race condition where Shelly returns old state during fast changes
    if (isonIndex > 0 && lastDimmChange == 0) {
      String isonStr = json.substring(isonIndex + 7, isonIndex + 12);
      deviceIson = (isonStr.indexOf("true") >= 0);
    }
    
    if (brightnessIndex > 0) {
      int startIdx = brightnessIndex + 13;
      int endIdx = json.indexOf(",", startIdx);
      if (endIdx < 0) endIdx = json.indexOf("}", startIdx);
      String brightStr = json.substring(startIdx, endIdx);
      int newBrightness = brightStr.toInt();
      
      // Always update deviceBrightness (for web UI)
      deviceBrightness = newBrightness;
      
      // Only update encoder when device value differs AND encoder is idle
      if (newBrightness != encoder->get_dimm_sayac() && deviceIson && lastDimmChange == 0) {
        encoder->set_dimm_sayac(newBrightness);
        Serial.println("[Dimm] Encoder synced from device: " + String(newBrightness));
      }
    }
  }
  
  // Backward compatibility
  void parseShellyStatus(String json) {
    parseDeviceStatus(json);
  }
  
  // ========== MAIN UPDATE LOOP ==========
  
  // CRITICAL: Device synchronization - called every loop()
  // This ensures persistent connection across all modes and reboots
  // Auto-reconnect mechanism: retries every 5 seconds if connection lost
  void update() {
    // If device not connected but IP exists, auto-reconnect
    if (!deviceConnected && deviceIP != "") {
      static unsigned long lastConnectAttempt = 0;
      unsigned long now = millis();
      
      // Special case for first attempt (lastConnectAttempt == 0)
      // or retry after 5 seconds
      if (lastConnectAttempt == 0 || (now - lastConnectAttempt > 5000)) {
        Serial.println("[Dimm] Auto-reconnecting: " + deviceIP);
        connectDevice(deviceIP, deviceType);
        lastConnectAttempt = now;
      }
      return;
    }
    
    unsigned long now = millis();
    
    // If encoder stopped rotating, send value to device after 150ms
    if (lastDimmChange > 0 && now - lastDimmChange > dimmerSendDelay) {
      syncToDevice();
      lastDimmChange = 0;  // Reset - mark encoder as idle
      lastDeviceSync = now;  // CRITICAL: Reset sync timer to prevent immediate read
    }
    
    // SHELLY BUG FIX: Send extra turn=on 200ms after brightness change
    // This prevents Shelly from turning off during fast encoder rotation
    if (needsExtraTurnOn && (now - lastBrightnessChangeTime > extraTurnOnDelay)) {
      sendExtraTurnOn();
      needsExtraTurnOn = false;
    }
    
    // Poll device status - Event-driven interval
    // Active: 500ms (encoder moving), Idle: 3s (encoder idle)
    // CRITICAL: Wait at least 500ms after encoder stops before reading device
    unsigned long dynamicInterval = deviceSyncIntervalIdle;  // Use idle interval (3s)
    
    if (lastDimmChange == 0 && now - lastDeviceSync > dynamicInterval) {
      syncFromDevice();
      lastDeviceSync = now;
    }
  }
  
  // For backward compatibility
  void updateShelly() {
    update();
  }
  
  // ========== GETTERS ==========
  
  bool isDeviceConnected() { return deviceConnected; }
  String getDeviceIP() { return deviceIP; }
  String getDeviceType() { return deviceType; }
  bool getDeviceIson() { return deviceIson; }
  int getDeviceBrightness() { return deviceBrightness; }
  
  // Called when changes are made from web interface
  void triggerDimmChange() {
    lastDimmChange = millis();
  }
  
  // Backward compatibility (old API)
  bool isShellyConnected() { return deviceConnected; }
  String getShellyIP() { return deviceIP; }
  bool getShellyIson() { return deviceIson; }
  int getShellyBrightness() { return deviceBrightness; }
  
private:
  // ========== PREFERENCES FUNCTIONS ==========
  
  // CRITICAL: Save device to EEPROM for persistent connection
  // This ensures device reconnects after reboot in any mode
  // Called automatically when device connects successfully
  void saveDevice(String ip, String type) {
    Preferences prefs;
    prefs.begin("syndimm", false);
    prefs.putString("deviceIP", ip);
    prefs.putString("deviceType", type);
    prefs.end();
    Serial.println("[Dimm] *** DEVICE SAVED TO EEPROM *** IP: " + ip + " Type: " + type);
    Serial.println("[Dimm] Device will auto-reconnect after reboot");
  }
  
  // CRITICAL: Load saved device from EEPROM on boot
  // This ensures device reconnects automatically after reboot
  // Called once in begin() - update() handles reconnection
  void loadDevice() {
    Preferences prefs;
    prefs.begin("syndimm", true);  // Read-only
    String savedIP = prefs.getString("deviceIP", "");
    String savedType = prefs.getString("deviceType", "shelly-dimmer");
    prefs.end();
    
    if (savedIP != "") {
      Serial.println("[Dimm] *** SAVED DEVICE FOUND *** IP: " + savedIP + " Type: " + savedType);
      Serial.println("[Dimm] Auto-reconnect will start in 5 seconds via update()");
      deviceIP = savedIP;
      deviceType = savedType;
      deviceConnected = false;  // Will be set to true by update() auto-reconnect
    } else {
      Serial.println("[Dimm] No saved device - Connect via web UI");
    }
  }
};

#endif // DIMM_H
