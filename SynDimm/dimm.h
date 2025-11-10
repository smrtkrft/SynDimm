/*
 * SynDimm - Dimmer Control Library (REBUILT FROM SCRATCH)
 * Clean and simple Shelly Dimmer control with proper debounce
 * 
 * KEY PRINCIPLES:
 * 1. Encoder stops -> Wait 300ms -> Send ONCE to Shelly
 * 2. Web NEVER reads from Shelly (no race conditions)
 * 3. Web only displays encoder value (fast, no blocking)
 * 4. Simple timer logic (no complex race condition handling)
 * 
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
  String deviceIP;                    // Connected Shelly IP
  bool deviceConnected;               // Connection status
  
  // Encoder debounce (SIMPLE!)
  unsigned long lastEncoderChange;    // Last encoder movement timestamp
  int lastSentValue;                  // Last value successfully sent to Shelly
  bool pendingSend;                   // Flag: value waiting to be sent
  
  // Timing constants
  static const unsigned long DEBOUNCE_DELAY = 300;  // Wait 300ms after encoder stops
  static const unsigned long HTTP_TIMEOUT = 500;    // HTTP request timeout
  
  // Ratio (encoder sensitivity)
  int dimmRatio;  // 1-5 range
  
  // Preferences
  Preferences prefs;
  
public:
  DimmerControl(Encoder* enc) : encoder(enc), 
                               deviceIP(""), deviceConnected(false),
                               lastEncoderChange(0), lastSentValue(-1),
                               pendingSend(false), dimmRatio(1) {}
  
  void begin() {
    // Load saved settings
    prefs.begin("syndimm", true);  // Read-only
    dimmRatio = prefs.getInt("dimmRatio", 1);  // Default 1
    String savedIP = prefs.getString("deviceIP", "");
    prefs.end();
    
    // Set initial encoder value
    encoder->set_dimm_sayac(100);
    
    Serial.println("[Dimm] Initialized - Ratio: " + String(dimmRatio));
    
    // Load saved device if exists
    if (savedIP != "") {
      Serial.println("[Dimm] Saved device found: " + savedIP);
      deviceIP = savedIP;
      deviceConnected = false;  // Will reconnect on first update()
    }
  }
  
  // ========== RATIO MANAGEMENT ==========
  
  void setDimmRatio(int ratio) {
    if (ratio >= 1 && ratio <= 5) {
      dimmRatio = ratio;
      
      // Save to EEPROM
      prefs.begin("syndimm", false);
      prefs.putInt("dimmRatio", ratio);
      prefs.end();
      
      Serial.println("[Dimm] Ratio set: " + String(ratio));
    }
  }
  
  int getDimmRatio() {
    return dimmRatio;
  }
  
  // ========== CONNECTION MANAGEMENT ==========
  
  bool connectShelly(String ip) {
    Serial.println("[Dimm] Connecting to: " + ip);
    
    // Test connection
    HTTPClient http;
    String url = "http://" + ip + "/shelly";
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == 200) {
      deviceIP = ip;
      deviceConnected = true;
      
      // Save to EEPROM
      prefs.begin("syndimm", false);
      prefs.putString("deviceIP", ip);
      prefs.end();
      
      Serial.println("[Dimm] *** CONNECTED AND SAVED *** IP: " + ip);
      
      // Send current encoder value immediately
      sendToShelly();
      
      return true;
    } else {
      Serial.println("[Dimm] Connection failed: " + String(httpCode));
      return false;
    }
  }
  
  void disconnectShelly() {
    deviceConnected = false;
    deviceIP = "";
    
    // Clear from EEPROM
    prefs.begin("syndimm", false);
    prefs.remove("deviceIP");
    prefs.end();
    
    Serial.println("[Dimm] Disconnected");
  }
  
  // ========== ENCODER EVENT HANDLING ==========
  
  void processEncoderEvent(char event) {
    // Encoder changed (R or L)
    if (event == 'R' || event == 'L') {
      lastEncoderChange = millis();
      pendingSend = true;
      
      // Note: Actual send happens in update() after 300ms debounce
    }
  }
  
  // ========== MAIN UPDATE LOOP ==========
  
  void update() {
    // If not connected but IP exists, try to reconnect
    if (!deviceConnected && deviceIP != "") {
      static unsigned long lastReconnect = 0;
      unsigned long now = millis();
      
      if (now - lastReconnect > 5000) {  // Try every 5 seconds
        Serial.println("[Dimm] Reconnecting...");
        connectShelly(deviceIP);
        lastReconnect = now;
      }
      return;
    }
    
    // If not connected, nothing to do
    if (!deviceConnected) return;
    
    // Check if encoder has stopped and value needs to be sent
    unsigned long now = millis();
    
    if (pendingSend && (now - lastEncoderChange >= DEBOUNCE_DELAY)) {
      sendToShelly();
      pendingSend = false;
    }
  }
  
  // ========== SHELLY COMMUNICATION ==========
  
  void sendToShelly() {
    if (!deviceConnected) return;
    
    int currentValue = encoder->get_dimm_sayac();
    
    // Skip if value hasn't changed
    if (currentValue == lastSentValue) return;
    
    HTTPClient http;
    String url;
    
    if (currentValue == 0) {
      // Turn off
      url = "http://" + deviceIP + "/light/0?turn=off&transition=0";
      Serial.println("[Dimm] Sending: OFF");
    } else {
      // Set brightness and ensure device is ON
      url = "http://" + deviceIP + "/light/0?turn=on&brightness=" + String(currentValue) + "&transition=0";
      Serial.println("[Dimm] Sending: Brightness " + String(currentValue));
    }
    
    http.begin(url);
    http.setTimeout(HTTP_TIMEOUT);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      lastSentValue = currentValue;
      Serial.println("[Dimm] ✓ Sent successfully");
    } else {
      Serial.println("[Dimm] ✗ Send failed: " + String(httpCode));
      
      // Disconnect after 3 consecutive failures
      static int failCount = 0;
      failCount++;
      if (failCount >= 3) {
        Serial.println("[Dimm] Too many failures - disconnecting");
        disconnectShelly();
        failCount = 0;
      }
    }
    
    http.end();
  }
  
  // ========== GETTERS ==========
  
  bool isShellyConnected() {
    return deviceConnected;
  }
  
  String getShellyIP() {
    return deviceIP;
  }
  
  int getShellyBrightness() {
    // Return encoder value (web displays this, no Shelly read!)
    return encoder->get_dimm_sayac();
  }
  
  bool getShellyIson() {
    // Assume ON if brightness > 0
    return (encoder->get_dimm_sayac() > 0);
  }
};

#endif // DIMM_H
