/**
 * SK_ota.h
 * SmartKraft SynDimm - GitHub OTA Update System
 * Version: v0.9.1
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
#include <Update.h>
#include <ArduinoJson.h>
#include <Preferences.h>

// GitHub Repository Information
#define GITHUB_REPO_OWNER "smrtkrft"
#define GITHUB_REPO_NAME "SynDimm"
#define GITHUB_API_URL "https://api.github.com/repos/" GITHUB_REPO_OWNER "/" GITHUB_REPO_NAME "/releases/latest"

// Current firmware version
#define CURRENT_VERSION "v0.9.1"

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
    
    Serial.println("[OTA] System initialized");
    Serial.printf("[OTA] Current version: %s\n", otaInfo.currentVersion.c_str());
    Serial.printf("[OTA] Auto-update: %s\n", otaInfo.autoUpdateEnabled ? "Enabled" : "Disabled");
}

// Compare version strings (v0.9.1 vs v1.0.0)
bool isNewerVersion(String current, String latest) {
    // Remove 'v' prefix if exists
    if (current.startsWith("v")) current = current.substring(1);
    if (latest.startsWith("v")) latest = latest.substring(1);
    
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
    
    Serial.println("[OTA] Checking for updates...");
    
    HTTPClient http;
    http.begin(GITHUB_API_URL);
    http.addHeader("User-Agent", "ESP32-SynDimm");
    http.setTimeout(10000); // 10 seconds timeout
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "GitHub API error: " + String(httpCode);
        Serial.printf("[OTA] Failed to fetch release info: %d\n", httpCode);
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
        Serial.println("[OTA] Failed to parse JSON response");
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
        Serial.println("[OTA] No .bin file found in release assets");
        return false;
    }
    
    // Check if update available
    otaInfo.updateAvailable = isNewerVersion(otaInfo.currentVersion, otaInfo.latestVersion);
    
    if (otaInfo.updateAvailable) {
        otaInfo.status = OTA_UPDATE_AVAILABLE;
        Serial.printf("[OTA] New version available: %s\n", otaInfo.latestVersion.c_str());
    } else {
        otaInfo.status = OTA_IDLE;
        Serial.println("[OTA] Already on latest version");
    }
    
    return otaInfo.updateAvailable;
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
    
    Serial.println("[OTA] Starting firmware download...");
    Serial.printf("[OTA] URL: %s\n", otaInfo.downloadURL.c_str());
    
    HTTPClient http;
    http.begin(otaInfo.downloadURL);
    http.addHeader("User-Agent", "ESP32-SynDimm");
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK && httpCode != HTTP_CODE_MOVED_PERMANENTLY) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Download failed: " + String(httpCode);
        Serial.printf("[OTA] Download failed: %d\n", httpCode);
        http.end();
        return false;
    }
    
    int contentLength = http.getSize();
    
    if (contentLength <= 0) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Invalid content length";
        Serial.println("[OTA] Invalid content length");
        http.end();
        return false;
    }
    
    Serial.printf("[OTA] Firmware size: %d bytes\n", contentLength);
    
    // Begin OTA update
    if (!Update.begin(contentLength)) {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Not enough space for OTA";
        Serial.println("[OTA] Not enough space for OTA");
        http.end();
        return false;
    }
    
    otaInfo.status = OTA_INSTALLING;
    
    // Download and write firmware
    WiFiClient *stream = http.getStreamPtr();
    size_t written = 0;
    uint8_t buff[512];
    
    while (http.connected() && (written < contentLength)) {
        size_t available = stream->available();
        
        if (available) {
            int c = stream->readBytes(buff, std::min(available, sizeof(buff)));
            
            if (Update.write(buff, c) != c) {
                otaInfo.status = OTA_ERROR;
                otaInfo.errorMessage = "Write failed";
                Serial.println("[OTA] Write failed");
                Update.abort();
                http.end();
                return false;
            }
            
            written += c;
            otaInfo.progress = (written * 100) / contentLength;
            
            // Print progress every 10%
            if (otaInfo.progress % 10 == 0) {
                Serial.printf("[OTA] Progress: %d%%\n", otaInfo.progress);
            }
        }
        
        delay(1);
    }
    
    http.end();
    
    // Finalize update
    if (Update.end()) {
        if (Update.isFinished()) {
            otaInfo.status = OTA_SUCCESS;
            otaInfo.progress = 100;
            Serial.println("[OTA] Update successful! Rebooting...");
            delay(1000);
            ESP.restart();
            return true;
        } else {
            otaInfo.status = OTA_ERROR;
            otaInfo.errorMessage = "Update not finished";
            Serial.println("[OTA] Update not finished");
            return false;
        }
    } else {
        otaInfo.status = OTA_ERROR;
        otaInfo.errorMessage = "Update error: " + String(Update.getError());
        Serial.printf("[OTA] Update error: %d\n", Update.getError());
        return false;
    }
}

// Get OTA settings as JSON
String getOTASettingsJSON() {
    JsonDocument doc;
    
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
    Serial.printf("[OTA] Auto-update set to: %s\n", autoUpdate ? "Enabled" : "Disabled");
}

// Auto-update check (called periodically)
void autoUpdateCheck() {
    if (!otaInfo.autoUpdateEnabled) {
        return;
    }
    
    Serial.println("[OTA] Auto-update check...");
    
    if (checkForUpdates()) {
        Serial.println("[OTA] New version found! Starting auto-update...");
        performOTAUpdate();
    }
}

#endif // SK_OTA_H
