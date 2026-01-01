/**
 * SK_ota.h
 * SmartKraft SynDimm - GitHub OTA Update System
 * Version: v1.1.0
 * 
 * ========================================
 * GitHub OTA Update Features:
 * ========================================
 * - Check for new releases from GitHub
 * - Compare current version with latest
 * - Download and install firmware OTA
 * - Auto-update or manual mode
 * - Progress tracking
 * ========================================
 */

#ifndef SK_OTA_H
#define SK_OTA_H

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include <Update.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "SK_config.h"

// GitHub Repository Information
#define GITHUB_REPO_OWNER "smrtkrft"
#define GITHUB_REPO_NAME "SynDimm"
#define GITHUB_API_URL "https://api.github.com/repos/" GITHUB_REPO_OWNER "/" GITHUB_REPO_NAME "/releases/latest"

// Current firmware version (use VERSION from SK_config.h)
#define CURRENT_VERSION VERSION

// OTA Status
enum OTAStatus {
    OTA_IDLE,
    OTA_CHECKING,
    OTA_UPDATE_AVAILABLE,
    OTA_DOWNLOADING,
    OTA_INSTALLING,
    OTA_SUCCESS,
    OTA_ERROR
};

struct OTAInfo {
    String currentVersion;
    String latestVersion;
    String releaseURL;
    String downloadURL;
    String releaseNotes;
    String publishedAt;
    bool updateAvailable;
    bool autoUpdateEnabled;
    OTAStatus status;
    int progress;
    String errorMessage;
};

// Anonymous namespace to prevent multiple definition
namespace {
    Preferences otaPrefs;
    OTAInfo otaInfo;
}

// Initialize OTA system
void initOTA() {
    otaPrefs.begin("ota-settings", false);
    
    // Load settings
    otaInfo.currentVersion = CURRENT_VERSION;
    otaInfo.autoUpdateEnabled = otaPrefs.getBool("auto_update", false);
    otaInfo.status = OTA_IDLE;
    otaInfo.progress = 0;
    otaInfo.updateAvailable = false;
    
    DEBUG_PRINTLN("[OTA] System initialized");
    DEBUG_PRINTF("[OTA] Current version: %s\n", otaInfo.currentVersion.c_str());
    DEBUG_PRINTF("[OTA] Auto-update: %s\n", otaInfo.autoUpdateEnabled ? "Enabled" : "Disabled");
}

// Compare version strings (v0.9.1 vs V1.0.1, OTA-v0.9.2 vs v1.0.1)
bool isNewerVersion(String current, String latest) {
    // Remove 'OTA-' prefix if exists (case insensitive)
    if (current.startsWith("OTA-") || current.startsWith("ota-")) current = current.substring(4);
    if (latest.startsWith("OTA-") || latest.startsWith("ota-")) latest = latest.substring(4);
    // Remove 'v' or 'V' prefix if exists
    if (current.startsWith("v") || current.startsWith("V")) current = current.substring(1);
    if (latest.startsWith("v") || latest.startsWith("V")) latest = latest.substring(1);
    
    // Split by dots
    int currentParts[3] = {0, 0, 0};
    int latestParts[3] = {0, 0, 0};
    
    // Parse current version
    int partIndex = 0;
    int lastIndex = 0;
    for (int i = 0; i <= current.length(); i++) {
        if (i == current.length() || current[i] == '.') {
            if (partIndex < 3) {
                currentParts[partIndex++] = current.substring(lastIndex, i).toInt();
                lastIndex = i + 1;
            }
        }
    }
    
    // Parse latest version
    partIndex = 0;
    lastIndex = 0;
    for (int i = 0; i <= latest.length(); i++) {
        if (i == latest.length() || latest[i] == '.') {
            if (partIndex < 3) {
                latestParts[partIndex++] = latest.substring(lastIndex, i).toInt();
                lastIndex = i + 1;
            }
        }
    }
    
    // Compare
    for (int i = 0; i < 3; i++) {
        if (latestParts[i] > currentParts[i]) return true;
        if (latestParts[i] < currentParts[i]) return false;
    }
    
    return false; // Same version
}

// Check for updates from GitHub
bool checkForUpdates() {
    otaInfo.status = OTA_CHECKING;
    otaInfo.errorMessage = "";
    
    DEBUG_PRINTLN("[OTA] Checking for updates...");
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    http.begin(GITHUB_API_URL);
    http.addHeader("User-Agent", "ESP32-SynDimm/" CURRENT_VERSION);
    http.addHeader("Accept", "application/vnd.github.v3+json");
    http.setTimeout(15000); // 15 seconds timeout
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        otaInfo.status = OTA_ERROR;
        if (httpCode == 403) {
            otaInfo.errorMessage = "GitHub rate limit exceeded";
            DEBUG_PRINTLN("[OTA] GitHub API rate limit exceeded. Try again later.");
        } else {
            otaInfo.errorMessage = "GitHub API error: " + String(httpCode);
            DEBUG_PRINTF("[OTA] Failed to fetch release info: %d\n", httpCode);
        }
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, payload);
    
    if (error) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "JSON parse error";
        DEBUG_PRINTLN("[OTA] Failed to parse JSON response");
        return false;
    }
    
    // Extract release information
    otaInfo.latestVersion = doc["tag_name"].as<String>();
    otaInfo.releaseNotes = doc["body"].as<String>();
    otaInfo.publishedAt = doc["published_at"].as<String>();
    otaInfo.releaseURL = doc["html_url"].as<String>();
    
    // Find .bin file in assets
    JsonArray assets = doc["assets"];
    bool foundBin = false;
    
    for (JsonObject asset : assets) {
        String name = asset["name"].as<String>();
        if (name.endsWith(".bin")) {
            otaInfo.downloadURL = asset["browser_download_url"].as<String>();
            foundBin = true;
            break;
        }
    }
    
    if (!foundBin) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "No firmware binary found in release";
        DEBUG_PRINTLN("[OTA] No .bin file found in release assets");
        return false;
    }
    
    // Check if update available
    otaInfo.updateAvailable = isNewerVersion(otaInfo.currentVersion, otaInfo.latestVersion);
    
    if (otaInfo.updateAvailable) {
        otaInfo.status = OTA_UPDATE_AVAILABLE;
        DEBUG_PRINTF("[OTA] New version available: %s\n", otaInfo.latestVersion.c_str());
    } else {
        otaInfo.status = OTA_IDLE;
        DEBUG_PRINTLN("[OTA] Already on latest version");
    }
    
    return true; // Kontrol başarılı (güncelleme olsun veya olmasın)
}

// Perform OTA update
bool performOTAUpdate() {
    if (otaInfo.downloadURL.length() == 0) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "No download URL";
        return false;
    }
    
    otaInfo.status = OTA_DOWNLOADING;
    otaInfo.progress = 0;
    
    DEBUG_PRINTLN("[OTA] Starting firmware download...");
    DEBUG_PRINTF("[OTA] URL: %s\n", otaInfo.downloadURL.c_str());
    
    // WiFiClientSecure for HTTPS with redirect support
    WiFiClientSecure *client = new WiFiClientSecure;
    if (!client) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Failed to create client";
        return false;
    }
    
    // Skip certificate verification for GitHub
    client->setInsecure();
    client->setTimeout(30000);
    
    HTTPClient http;
    http.setFollowRedirects(HTTPC_FORCE_FOLLOW_REDIRECTS);  // Force follow all redirects
    http.setRedirectLimit(10);  // Allow up to 10 redirects
    http.setTimeout(60000); // 60 second timeout for large files
    http.begin(*client, otaInfo.downloadURL);
    http.addHeader("User-Agent", "ESP32-SynDimm/" CURRENT_VERSION);
    http.addHeader("Accept", "*/*");
    http.addHeader("Cache-Control", "no-cache");
    
    DEBUG_PRINTLN("[OTA] Connecting to download server...");
    int httpCode = http.GET();
    DEBUG_PRINTF("[OTA] HTTP Response: %d\n", httpCode);
    
    // Handle redirect manually if needed
    if (httpCode == HTTP_CODE_MOVED_PERMANENTLY || httpCode == HTTP_CODE_FOUND || 
        httpCode == HTTP_CODE_SEE_OTHER || httpCode == HTTP_CODE_TEMPORARY_REDIRECT) {
        String redirectURL = http.getLocation();
        DEBUG_PRINTF("[OTA] Redirect to: %s\n", redirectURL.c_str());
        http.end();
        
        // Follow redirect
        http.begin(*client, redirectURL);
        http.addHeader("User-Agent", "ESP32-SynDimm/" CURRENT_VERSION);
        http.addHeader("Accept", "*/*");
        httpCode = http.GET();
        DEBUG_PRINTF("[OTA] After redirect HTTP Response: %d\n", httpCode);
    }
    
    if (httpCode != HTTP_CODE_OK) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "HTTP " + String(httpCode);
        DEBUG_PRINTF("[OTA] Download failed: %d\n", httpCode);
        DEBUG_PRINTF("[OTA] Failed URL: %s\n", otaInfo.downloadURL.c_str());
        http.end();
        delete client;
        return false;
    }
    
    int contentLength = http.getSize();
    
    if (contentLength <= 0) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Invalid content length";
        DEBUG_PRINTLN("[OTA] Invalid content length");
        http.end();
        delete client;
        return false;
    }
    
    DEBUG_PRINTF("[OTA] Firmware size: %d bytes\n", contentLength);
    
    // Begin OTA update
    if (!Update.begin(contentLength)) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Not enough space for OTA";
        DEBUG_PRINTLN("[OTA] Not enough space for OTA");
        http.end();
        delete client;
        return false;
    }
    
    otaInfo.status = OTA_INSTALLING;
    
    // Download and write firmware
    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[1024];
    int lastProgress = -1;
    
    DEBUG_PRINTLN("[OTA] Downloading and installing...");
    
    while (http.connected() && (written < contentLength)) {
        size_t available = stream->available();
        
        if (available) {
            int c = stream->readBytes(buff, std::min(available, sizeof(buff)));
            
            if (Update.write(buff, c) != c) {
                otaInfo.status = OTA_ERROR;
                otaInfo.errorMessage = "Write failed";
                DEBUG_PRINTLN("[OTA] Write failed");
                Update.abort();
                http.end();
                delete client;
                return false;
            }
            
            written += c;
            otaInfo.progress = (written * 100) / contentLength;
            
            // Print progress every 10%
            int currentProgress = (otaInfo.progress / 10) * 10;
            if (currentProgress != lastProgress && currentProgress > 0) {
                DEBUG_PRINTF("[OTA] Progress: %d%%\n", currentProgress);
                lastProgress = currentProgress;
            }
        }
        
        delay(1);
        esp_task_wdt_reset(); // Feed watchdog during long download
    }
    
    http.end();
    delete client;
    
    // Finalize update
    if (Update.end()) {
        if (Update.isFinished()) {
            otaInfo.status = OTA_SUCCESS;
            otaInfo.progress = 100;
            DEBUG_PRINTLN("[OTA] Update successful! Rebooting...");
            delay(1000);
            ESP.restart();
            return true;
        } else {
            otaInfo.status = OTA_ERROR;
            otaInfo.errorMessage = "Update not finished";
            DEBUG_PRINTLN("[OTA] Update not finished");
            return false;
        }
    } else {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Update error: " + String(Update.getError());
        DEBUG_PRINTF("[OTA] Update error: %d\n", Update.getError());
        return false;
    }
}

// Get OTA settings as JSON
String getOTASettingsJSON() {
    JsonDocument doc;
    
    doc["success"] = true;
    doc["currentVersion"] = otaInfo.currentVersion;
    doc["latestVersion"] = otaInfo.latestVersion;
    doc["updateAvailable"] = otaInfo.updateAvailable;
    doc["autoUpdateEnabled"] = otaInfo.autoUpdateEnabled;
    doc["status"] = (int)otaInfo.status;
    doc["progress"] = otaInfo.progress;
    doc["releaseURL"] = otaInfo.releaseURL;
    doc["releaseNotes"] = otaInfo.releaseNotes;
    doc["publishedAt"] = otaInfo.publishedAt;
    doc["errorMessage"] = otaInfo.errorMessage;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// Save OTA settings
void saveOTASettings(bool autoUpdate) {
    otaInfo.autoUpdateEnabled = autoUpdate;
    otaPrefs.putBool("auto_update", autoUpdate);
    DEBUG_PRINTF("[OTA] Auto-update set to: %s\n", autoUpdate ? "Enabled" : "Disabled");
}

// ========================================
// OTA KONTROL SİSTEMİ
// ========================================
// Sadece ilk açılışta ve buton tıklandığında kontrol edilir
// Otomatik güncelleme YOK - kullanıcı manuel olarak yükselter

// Startup'ta güncelleme kontrolü (sadece bilgilendirme, otomatik yükleme yok)
void startupUpdateCheck() {
    DEBUG_PRINTLN("[OTA] Baslangic guncelleme kontrolu...");
    
    // Güncelleme kontrolü yap
    if (checkForUpdates() && otaInfo.updateAvailable) {
        DEBUG_PRINTLN("[OTA] Yeni versiyon mevcut! Manuel guncelleme bekleniyor.");
        DEBUG_PRINTF("[OTA] Mevcut: %s -> Yeni: %s\n", 
                     otaInfo.currentVersion.c_str(), 
                     otaInfo.latestVersion.c_str());
    } else {
        DEBUG_PRINTLN("[OTA] Sistem guncel.");
    }
}

#endif // SK_OTA_H
