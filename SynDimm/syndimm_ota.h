/*
 * SynDimm OTA Update System
 * GitHub Releases tabanlı otomatik güncelleme
 * 
 * Kullanım:
 * - Her 5 dakikada bir otomatik kontrol
 * - Yeni version varsa otomatik indirir ve yükler
 * - Semantic versioning (v1.0.0, v1.0.1, vb.)
 */

#ifndef SYNDIMM_OTA_H
#define SYNDIMM_OTA_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <HTTPUpdate.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Preferences.h>
#include "version.h"  // Version bilgisi buradan alınır

class SynDimmOTA {
private:
  // GitHub repo bilgileri
  const String GITHUB_OWNER = "smrtkrft";
  const String GITHUB_REPO = "SynDimm";
  const String GITHUB_API_URL = "https://api.github.com/repos/smrtkrft/SynDimm/releases/latest";  // HTTPS geri
  
  // OTA ayarları
  unsigned long lastCheckTime;
  const unsigned long checkInterval = 1 * 60 * 1000;  // 1 dakika (60000 ms) - TEST İÇİN
  bool updateInProgress;
  bool updateAvailable;
  String latestVersion;
  String downloadUrl;
  bool autoUpdateEnabled;  // Otomatik güncelleme aktif mi?
  
  // Progress callback için
  typedef void (*ProgressCallback)(int progress);
  ProgressCallback progressCallback;
  
  // Version karşılaştırma (semantic versioning)
  // v1.0.0 vs v1.0.1 gibi
  bool isNewerVersion(String current, String latest) {
    // "v" karakterini kaldır
    current.replace("v", "");
    latest.replace("v", "");
    
    int currentMajor = 0, currentMinor = 0, currentPatch = 0;
    int latestMajor = 0, latestMinor = 0, latestPatch = 0;
    
    // Current version parse
    sscanf(current.c_str(), "%d.%d.%d", &currentMajor, &currentMinor, &currentPatch);
    // Latest version parse
    sscanf(latest.c_str(), "%d.%d.%d", &latestMajor, &latestMinor, &latestPatch);
    
    // Karşılaştır
    if (latestMajor > currentMajor) return true;
    if (latestMajor == currentMajor && latestMinor > currentMinor) return true;
    if (latestMajor == currentMajor && latestMinor == currentMinor && latestPatch > currentPatch) return true;
    
    return false;
  }
  
  // GitHub API'den son release bilgilerini al
  bool fetchLatestRelease() {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[OTA] WiFi not connected");
      return false;
    }
    
    Serial.println("[OTA] Checking for updates...");
    Serial.print("[OTA] Current version: ");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("[OTA] WiFi RSSI: ");
    Serial.println(WiFi.RSSI());
    Serial.print("[OTA] Local IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("[OTA] Gateway: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print("[OTA] DNS: ");
    Serial.println(WiFi.dnsIP());
    
    // DNS test - GitHub IP'sini resolve et
    IPAddress githubIP;
    Serial.print("[OTA] Resolving api.github.com... ");
    if (WiFi.hostByName("api.github.com", githubIP)) {
      Serial.print("OK: ");
      Serial.println(githubIP);
    } else {
      Serial.println("FAILED! DNS problem detected.");
      return false;
    }
    
    Serial.print("[OTA] Connecting to GitHub API: ");
    Serial.println(GITHUB_API_URL);
    
    WiFiClientSecure client;
    client.setInsecure();  // Sertifika doğrulamasını atla
    
    HTTPClient http;
    http.begin(client, GITHUB_API_URL);
    http.addHeader("Accept", "application/vnd.github.v3+json");
    http.addHeader("User-Agent", "SynDimm-ESP32");
    http.setTimeout(20000);  // 20 saniye timeout
    http.setConnectTimeout(10000);  // 10 saniye connect timeout
    
    Serial.println("[OTA] Sending GET request...");
    int httpCode = http.GET();
    Serial.print("[OTA] HTTP response code: ");
    Serial.println(httpCode);
    
    if (httpCode == -1) {
      Serial.println("[OTA] Connection failed. SSL/TLS handshake problem.");
      Serial.println("  This is common on ESP32C6 with HTTPS.");
    }
    
    if (httpCode == HTTP_CODE_OK) {
      String payload = http.getString();
      
      // JSON parse
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (error) {
        Serial.print("[OTA] JSON parse error: ");
        Serial.println(error.c_str());
        http.end();
        return false;
      }
      
      // Version ve download URL al
      latestVersion = doc["tag_name"].as<String>();
      
      // Assets içinde .bin dosyası ara
      JsonArray assets = doc["assets"].as<JsonArray>();
      for (JsonObject asset : assets) {
        String name = asset["name"].as<String>();
        if (name.endsWith(".bin")) {
          downloadUrl = asset["browser_download_url"].as<String>();
          // HTTPS kullan (GitHub HTTP'yi desteklemiyor)
          Serial.print("[OTA] Binary URL: ");
          Serial.println(downloadUrl);
          break;
        }
      }
      
      Serial.print("[OTA] Latest version: ");
      Serial.println(latestVersion);
      
      if (downloadUrl.length() == 0) {
        Serial.println("[OTA] No .bin file found in release");
        http.end();
        return false;
      }
      
      // Version karşılaştır
      if (isNewerVersion(FIRMWARE_VERSION, latestVersion)) {
        Serial.println("[OTA] New version available!");
        Serial.print("[OTA] Download URL: ");
        Serial.println(downloadUrl);
        updateAvailable = true;
        http.end();
        return true;
      } else {
        Serial.println("[OTA] Already up to date");
        updateAvailable = false;
        http.end();
        return false;
      }
      
    } else {
      Serial.print("[OTA] HTTP error: ");
      Serial.println(httpCode);
      http.end();
      return false;
    }
  }
  
  // OTA update progress callback (HTTPUpdate kütüphanesi için)
  static void onUpdateProgress(int current, int total) {
    int progress = (current * 100) / total;
    Serial.print("[OTA] Progress: ");
    Serial.print(progress);
    Serial.println("%");
  }
  
public:
  SynDimmOTA() : lastCheckTime(0), updateInProgress(false), updateAvailable(false), 
                 latestVersion(""), downloadUrl(""), progressCallback(nullptr), autoUpdateEnabled(true) {}
  
  void begin() {
    // EEPROM'dan otomatik güncelleme ayarını yükle
    Preferences prefs;
    prefs.begin("ota", true);
    autoUpdateEnabled = prefs.getBool("autoUpdate", true);  // Varsayılan: true
    prefs.end();
    
    Serial.println("[OTA] OTA Update System initialized");
    Serial.print("[OTA] Current firmware: v");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("[OTA] Check interval: ");
    Serial.print(checkInterval / 60000);
    Serial.println(" minutes");
    Serial.print("[OTA] Auto Update: ");
    Serial.println(autoUpdateEnabled ? "ENABLED" : "DISABLED");
    
    // HTTPUpdate callbacks
    httpUpdate.onProgress(onUpdateProgress);
    
    // İlk kontrol 30 saniye sonra (WiFi bağlanması için)
    lastCheckTime = millis() - checkInterval + 30000;
  }
  
  // Loop içinde çağrılacak - otomatik check
  void update() {
    if (updateInProgress) return;
    if (!autoUpdateEnabled) return;  // Otomatik güncelleme kapalıysa atla
    
    unsigned long now = millis();
    if (now - lastCheckTime >= checkInterval) {
      lastCheckTime = now;
      checkAndUpdate();
    }
  }
  
  // Manuel update kontrolü (web'den veya otomatik)
  bool checkAndUpdate() {
    if (updateInProgress) {
      Serial.println("[OTA] Update already in progress");
      return false;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[OTA] Cannot check update - WiFi not connected");
      return false;
    }
    
    // GitHub'dan son version'ı kontrol et
    if (fetchLatestRelease()) {
      // Yeni version var - OTA başlat
      return performUpdate();
    }
    
    return false;
  }
  
  // Sadece version kontrolü yap (güncelleme yapma)
  bool checkOnly() {
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[OTA] Cannot check - WiFi not connected");
      return false;
    }
    
    return fetchLatestRelease();
  }
  
  // OTA güncellemeyi gerçekleştir
  bool performUpdate() {
    if (downloadUrl.length() == 0) {
      Serial.println("[OTA] No download URL available");
      return false;
    }
    
    updateInProgress = true;
    
    Serial.println("[OTA] ===================================");
    Serial.println("[OTA] STARTING OTA UPDATE");
    Serial.print("[OTA] Current: v");
    Serial.println(FIRMWARE_VERSION);
    Serial.print("[OTA] New: ");
    Serial.println(latestVersion);
    Serial.print("[OTA] Download URL: ");
    Serial.println(downloadUrl);
    Serial.println("[OTA] ===================================");
    
    // Her zaman HTTPS kullan (GitHub HTTP'yi desteklemiyor)
    WiFiClientSecure client;
    client.setInsecure();  // Sertifika doğrulamasını atla
    client.setTimeout(30);  // 30 saniye timeout
    
    Serial.println("[OTA] Using HTTPS client with setInsecure()");
    Serial.println("[OTA] Starting binary download...");
    
    // HTTPUpdate başlat
    httpUpdate.setLedPin(LED_BUILTIN, LOW);
    httpUpdate.rebootOnUpdate(true);  // Güncelleme sonrası otomatik reboot
    httpUpdate.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);  // Redirect'leri takip et
    
    t_httpUpdate_return ret = httpUpdate.update(client, downloadUrl);
    
    switch (ret) {
      case HTTP_UPDATE_FAILED:
        Serial.print("[OTA] Update FAILED: ");
        Serial.println(httpUpdate.getLastErrorString());
        updateInProgress = false;
        return false;
        
      case HTTP_UPDATE_NO_UPDATES:
        Serial.println("[OTA] No update needed");
        updateInProgress = false;
        return false;
        
      case HTTP_UPDATE_OK:
        Serial.println("[OTA] Update SUCCESS! Rebooting...");
        // Reboot otomatik olacak
        return true;
    }
    
    updateInProgress = false;
    return false;
  }
  
  // Getter functions
  String getCurrentVersion() { return FIRMWARE_VERSION; }
  String getLatestVersion() { return latestVersion; }
  bool isUpdateAvailable() { return updateAvailable; }
  bool isUpdateInProgress() { return updateInProgress; }
  String getDownloadUrl() { return downloadUrl; }
  
  // Auto-update ayarları
  bool getAutoUpdate() const { return autoUpdateEnabled; }
  
  void setAutoUpdate(bool enabled) {
    autoUpdateEnabled = enabled;
    
    // EEPROM'a kaydet
    Preferences prefs;
    prefs.begin("ota", false);
    prefs.putBool("autoUpdate", enabled);
    prefs.end();
    
    Serial.print("[OTA] Auto Update: ");
    Serial.println(enabled ? "ENABLED" : "DISABLED");
  }
  
  // Progress callback set et
  void setProgressCallback(ProgressCallback cb) {
    progressCallback = cb;
  }
};

#endif
