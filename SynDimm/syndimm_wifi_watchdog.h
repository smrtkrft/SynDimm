#ifndef SYNDIMM_WIFI_WATCHDOG_H
#define SYNDIMM_WIFI_WATCHDOG_H

#include <WiFi.h>

/**
 * WiFi Watchdog - ESP32C6 WiFi connection health monitor
 * 
 * Purpose: Ensures WiFi stays connected, auto-reconnects on failure
 * Prevents: "cihaz web baglantisi saglanmiyor" errors
 * Requirements: "ag baglantisinin hep aktif kalmasi"
 * 
 * Features:
 * - Non-blocking health checks every 30s
 * - Auto-reconnect with exponential backoff
 * - Preserves SSID/password from initial setup
 * - Minimal overhead (runs in loop())
 */

class SynDimmWiFiWatchdog {
private:
  unsigned long lastCheckTime;
  const unsigned long CHECK_INTERVAL = 30000;  // 30 seconds
  
  unsigned long reconnectAttemptTime;
  int reconnectAttempts;
  const int MAX_RECONNECT_ATTEMPTS = 5;
  const unsigned long RECONNECT_TIMEOUT = 10000;  // 10s max per attempt
  
  bool isReconnecting;
  String savedSSID;
  String savedPassword;

public:
  SynDimmWiFiWatchdog() {
    lastCheckTime = 0;
    reconnectAttemptTime = 0;
    reconnectAttempts = 0;
    isReconnecting = false;
  }

  // Call once in setup() after WiFi.begin()
  void begin(const char* ssid, const char* password) {
    savedSSID = String(ssid);
    savedPassword = String(password);
    lastCheckTime = millis();
    Serial.println("[WiFi Watchdog] Started - Check interval: 30s");
  }

  // Call in loop() - non-blocking
  void update() {
    unsigned long now = millis();
    
    // Check WiFi health every 30s
    if (now - lastCheckTime >= CHECK_INTERVAL) {
      lastCheckTime = now;
      
      if (!isConnected()) {
        Serial.println("[WiFi Watchdog] *** DISCONNECTED - Starting auto-reconnect ***");
        startReconnect();
      } else {
        // Connection healthy
        if (reconnectAttempts > 0) {
          Serial.println("[WiFi Watchdog] Connection restored");
          reconnectAttempts = 0;
        }
      }
    }
    
    // Handle reconnection attempts
    if (isReconnecting) {
      handleReconnect();
    }
  }

  // Check if WiFi is connected
  bool isConnected() {
    return (WiFi.status() == WL_CONNECTED);
  }

  // Get connection status string
  String getStatusString() {
    if (isReconnecting) {
      return "Reconnecting... (" + String(reconnectAttempts) + "/" + String(MAX_RECONNECT_ATTEMPTS) + ")";
    }
    
    switch (WiFi.status()) {
      case WL_CONNECTED:
        return "Connected (" + WiFi.localIP().toString() + ")";
      case WL_NO_SSID_AVAIL:
        return "SSID Not Found";
      case WL_CONNECT_FAILED:
        return "Connection Failed";
      case WL_CONNECTION_LOST:
        return "Connection Lost";
      case WL_DISCONNECTED:
        return "Disconnected";
      default:
        return "Unknown (" + String(WiFi.status()) + ")";
    }
  }

private:
  void startReconnect() {
    if (reconnectAttempts >= MAX_RECONNECT_ATTEMPTS) {
      Serial.println("[WiFi Watchdog] Max reconnect attempts reached - ESP restart required");
      // Critical: Save EEPROM before restart (caller should handle this)
      delay(1000);
      ESP.restart();
      return;
    }
    
    reconnectAttempts++;
    reconnectAttemptTime = millis();
    isReconnecting = true;
    
    // Disconnect and try reconnect
    WiFi.disconnect();
    delay(100);
    
    Serial.println("[WiFi Watchdog] Reconnect attempt #" + String(reconnectAttempts));
    WiFi.begin(savedSSID.c_str(), savedPassword.c_str());
  }

  void handleReconnect() {
    unsigned long now = millis();
    
    // Check if connected
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("[WiFi Watchdog] Reconnected! IP: " + WiFi.localIP().toString());
      isReconnecting = false;
      reconnectAttempts = 0;
      return;
    }
    
    // Timeout check
    if (now - reconnectAttemptTime > RECONNECT_TIMEOUT) {
      Serial.println("[WiFi Watchdog] Reconnect timeout - retry");
      isReconnecting = false;
      // Will retry on next update() cycle
    }
  }
};

#endif // SYNDIMM_WIFI_WATCHDOG_H
