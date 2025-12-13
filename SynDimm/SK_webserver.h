/**
 * SK_webserver.h - Web Server Management v1.1.1
 * KRITIK: Web arayüzü SADECE ESP32-C6 local erişim içindir!
 */

#ifndef SK_WEBSERVER_H
#define SK_WEBSERVER_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "SK_config.h"
#include "SK_html.h"
#include "SK_js.h"
#include "SK_scan.h"
#include "SK_dimmer.h"
#include "SK_shutter.h"
#include "SK_mode_safe.h"
#include "SK_mode_safe_api.h"
#include "SK_ota.h"
#include "SK_lang.h"

class SKWebServer {
private:
    WebServer* server;
    String chipID;
    class SKWiFi* wifiManager;  // Forward declaration pointer
    class SKModeManager* modeManager;  // Mode manager pointer
    void (*httpRequestCallback)();  // HTTP request callback
    
    // Handler functions
    void handleRoot() {
        if (httpRequestCallback) httpRequestCallback();
        String html = generateHTML(chipID, VERSION);
        server->send(200, "text/html", html);
    }
    
    void handleCSS() {
        server->sendHeader("Cache-Control", "max-age=86400");
        server->send_P(200, "text/css", SK_CSS);
    }
    
    void handleScript() {
        server->sendHeader("Cache-Control", "max-age=86400");
        server->setContentLength(strlen_P(SK_JS));
        server->send(200, "application/javascript", "");
        
        // Send in chunks to avoid timeout
        const char* ptr = SK_JS;
        size_t len = strlen_P(SK_JS);
        size_t chunkSize = 1024;
        
        for (size_t i = 0; i < len; i += chunkSize) {
            size_t remaining = len - i;
            size_t size = (remaining < chunkSize) ? remaining : chunkSize;
            
            char buffer[chunkSize + 1];
            memcpy_P(buffer, ptr + i, size);
            buffer[size] = '\0';
            
            server->sendContent(buffer);
            yield(); // Allow other tasks to run
        }
    }
    
    void handleFavicon() {
        server->send(204); // No Content
    }
    
    void handleNotFound() {
        server->send(404, "text/plain", "404: Not Found");
    }
    
    void handleSaveNetwork() {
        if (httpRequestCallback) httpRequestCallback();
        DEBUG_PRINTLN("\n=== /saveNetwork endpoint called ===");
        
        if (server->method() != HTTP_POST) {
            DEBUG_PRINTLN("ERROR: Not a POST request");
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        // GÜVENLİK: Request body loglanmıyor (WiFi şifresi içerebilir)
        DEBUG_PRINTLN("Request received, processing...");
        
        if (body.length() == 0) {
            DEBUG_PRINTLN("ERROR: Empty body");
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Boş veri gönderildi\"}");
            return;
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            DEBUG_PRINT("ERROR: JSON parse error: ");
            DEBUG_PRINTLN(error.c_str());
            String errorMsg = "{\"success\":false,\"message\":\"JSON hatası: ";
            errorMsg += error.c_str();
            errorMsg += "\"}";
            server->send(400, "application/json", errorMsg);
            return;
        }
        
        DEBUG_PRINTLN("JSON parsed successfully");
        
        // Extract Primary Network settings
        String p_ssid = doc["primary"]["ssid"] | "";
        String p_pass = doc["primary"]["password"] | "";
        String p_ip = doc["primary"]["staticIP"] | "";
        String p_mdns = doc["primary"]["mdns"] | "dimm";
        
        // Extract Backup Network settings
        String b_ssid = doc["backup"]["ssid"] | "";
        String b_pass = doc["backup"]["password"] | "";
        String b_ip = doc["backup"]["staticIP"] | "";
        String b_mdns = doc["backup"]["mdns"] | "dimm";
        
        DEBUG_PRINTLN("Extracted data:");
        DEBUG_PRINTLN("  Primary SSID: " + p_ssid);
        DEBUG_PRINTLN("  Backup SSID: " + b_ssid);
        
        // Backend validation
        if (p_ssid.length() == 0 && b_ssid.length() == 0) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"En az bir WiFi ağı yapılandırılmalıdır\"}");
            return;
        }
        
        // Validate Primary Network
        if (p_ssid.length() > 0) {
            if (p_ssid.length() > 32) {
                server->send(400, "application/json", "{\"success\":false,\"message\":\"Primary SSID maksimum 32 karakter olmalıdır\"}");
                return;
            }
            if (p_pass.length() > 0 && (p_pass.length() < 8 || p_pass.length() > 63)) {
                server->send(400, "application/json", "{\"success\":false,\"message\":\"Primary WiFi şifresi 8-63 karakter arasında olmalıdır (boş bırakılabilir)\"}");
                return;
            }
        }
        
        // Validate Backup Network
        if (b_ssid.length() > 0) {
            if (b_ssid.length() > 32) {
                server->send(400, "application/json", "{\"success\":false,\"message\":\"Backup SSID maksimum 32 karakter olmalıdır\"}");
                return;
            }
            if (b_pass.length() > 0 && (b_pass.length() < 8 || b_pass.length() > 63)) {
                server->send(400, "application/json", "{\"success\":false,\"message\":\"Backup WiFi şifresi 8-63 karakter arasında olmalıdır (boş bırakılabilir)\"}");
                return;
            }
        }
        
        // Save to preferences (WiFi manager will handle this)
        if (wifiManager) {
            DEBUG_PRINTLN("\n=== Saving Network Settings ===");
            DEBUG_PRINTLN("Primary SSID: " + p_ssid);
            DEBUG_PRINTLN("Backup SSID: " + b_ssid);
            
            bool saved = wifiManager->saveNetworkSettings(p_ssid, p_pass, p_ip, p_mdns,
                                                         b_ssid, b_pass, b_ip, b_mdns);
            
            if (saved) {
                DEBUG_PRINTLN("Settings saved successfully!");
                server->send(200, "application/json", "{\"success\":true,\"message\":\"Ayarlar kaydedildi! Cihaz yeniden başlatılıyor...\"}");
                
                // Schedule restart after response is sent
                delay(500);
                DEBUG_PRINTLN("Restarting ESP32...\n");
                delay(500);
                ESP.restart();
            } else {
                DEBUG_PRINTLN("Failed to save settings!");
                server->send(500, "application/json", "{\"success\":false,\"message\":\"Ayarlar kaydedilemedi - bellek hatası\"}");
            }
        } else {
            DEBUG_PRINTLN("ERROR: WiFi manager not found!");
            server->send(500, "application/json", "{\"success\":false,\"message\":\"WiFi yöneticisi bulunamadı\"}");
        }
    }
    
    void handleGetSettings() {
        if (httpRequestCallback) httpRequestCallback();
        if (wifiManager) {
            String json = wifiManager->getNetworkSettingsJSON();
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"WiFi manager not found\"}");
        }
    }
    
    void handleGetStatus() {
        if (httpRequestCallback) httpRequestCallback();
        if (wifiManager) {
            String json = wifiManager->getConnectionStatusJSON();
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"WiFi manager not found\"}");
        }
    }
    
    // OTA Endpoints
    void handleGetOTASettings() {
        String json = getOTASettingsJSON();
        server->send(200, "application/json", json);
    }
    
    void handleCheckOTAUpdate() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        bool success = checkForUpdates();
        
        if (success) {
            String json = getOTASettingsJSON();
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"success\":false,\"message\":\"Update check failed\"}");
        }
    }
    
    void handlePerformOTAUpdate() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Update started\"}");
        
        // Start update in background
        delay(100);
        performOTAUpdate();
    }
    
    void handleSaveOTASettings() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        bool autoUpdate = doc["autoUpdate"].as<bool>();
        saveOTASettings(autoUpdate);
        
        server->send(200, "application/json", "{\"success\":true}");
    }
    
    // Language Endpoints
    void handleGetLang() {
        String lang = "en";  // Default
        if (server->hasArg("lang")) {
            lang = server->arg("lang");
        }
        
        const char* langJSON = getLanguageJSON(lang);
        server->send(200, "application/json", langJSON);
    }
    
    void handleSetLang() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        String lang = doc["lang"].as<String>();
        bool success = setLanguageFromCode(lang);
        
        if (success) {
            server->send(200, "application/json", "{\"success\":true}");
        } else {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid language code\"}");
        }
    }
    
    void handleGetCurrentLang() {
        String json = "{\"lang\":\"" + getCurrentLangCode() + "\"}";
        server->send(200, "application/json", json);
    }
    
    // Dimmer Endpoints
    void handleConnectDimmer() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        String ip = doc["ip"].as<String>();
        
        if (ip.length() == 0) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"IP address required\"}");
            return;
        }
        
        // Call dimmer connection function (from SK_dimmer.h)
        bool success = connectToDimmer(ip);
        
        if (success) {
            server->send(200, "application/json", "{\"success\":true,\"message\":\"Connected to dimmer\"}");
        } else {
            server->send(200, "application/json", "{\"success\":false,\"message\":\"Connection failed\"}");
        }
    }
    
    void handleDisconnectDimmer() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        disconnectDimmer();
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Disconnected\"}");
    }
    
    void handleGetDimmerStatus() {
        String json = getDimmerStatusJSON();
        server->send(200, "application/json", json);
    }
    
    void handleSetDimmerRatio() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        int ratio = doc["ratio"].as<int>();
        
        if (ratio < 1 || ratio > 5) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Ratio must be between 1-5\"}");
            return;
        }
        
        setDimmerRatio(ratio);
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Ratio saved\"}");
    }
    
    void handleScanNetwork() {
        DEBUG_PRINTLN("[WEB] /scanNetwork called");
        
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        // Configure unified scanner for all device types
        ScanConfig config;
        config.filters = FILTER_DIMMERS | FILTER_SHUTTERS; // Scan both dimmer and shutter devices
        config.parallelCount = 10;
        config.tcpTimeout = 500;
        config.httpTimeout = 3000;
        config.autoSave = true;
        
        // Start unified scan (non-blocking FreeRTOS task)
        startNetworkScan(config, nullptr, nullptr, nullptr);
        
        String response = "{\"success\":true,\"message\":\"Network scan started\"}";
        server->send(200, "application/json", response);
    }
    
    void handleStopScan() {
        DEBUG_PRINTLN("[WEB] /stopScan called");
        
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        // Stop ongoing unified scan
        stopNetworkScan();
        
        String response = "{\"success\":true,\"message\":\"Scan stopped\"}";
        server->send(200, "application/json", response);
    }
    
    void handleGetScanResults() {
        DEBUG_PRINTLN("[WEB] /getScanResults called");
        
        // Get results from unified scanner
        JsonDocument doc;
        JsonArray devices = doc["devices"].to<JsonArray>();
        
        auto discoveredDevices = getDiscoveredDevices();
        for (const auto& device : discoveredDevices) {
            JsonObject obj = devices.add<JsonObject>();
            obj["ip"] = device.ip;
            obj["modelName"] = device.modelName;
            obj["displayName"] = device.displayName;
            obj["category"] = (int)device.category;
            obj["generation"] = device.generation;
            obj["supportsDimming"] = device.supportsDimming;
            obj["supportsShutter"] = device.supportsShutter;
        }
        
        doc["scanning"] = isNetworkScanning();
        doc["count"] = discoveredDevices.size();
        
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    }
    
    void handleGetScanProgress() {
        DEBUG_PRINTLN("[WEB] /getScanProgress called");
        String json = getNetworkScanProgressJSON();
        server->send(200, "application/json", json);
    }
    
    void handleGetSavedDevices() {
        String json = getSavedDevicesJSON();
        server->send(200, "application/json", json);
    }
    
    void handleRemoveSavedDevice() {
        if (!server->hasArg("plain")) {
            server->send(400, "application/json", "{\"success\": false, \"message\": \"No data\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\": false, \"message\": \"Invalid JSON\"}");
            return;
        }
        
        String ip = doc["ip"].as<String>();
        
        if (ip.length() == 0) {
            server->send(400, "application/json", "{\"success\": false, \"message\": \"No IP provided\"}");
            return;
        }
        
        // Remove device from saved list
        bool removed = removeDimmerDevice(ip);
        
        if (removed) {
            server->send(200, "application/json", "{\"success\": true}");
        } else {
            server->send(200, "application/json", "{\"success\": false, \"message\": \"Device not found\"}");
        }
    }
    
    // Shutter Endpoints
    void handleConnectShutter() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        String ip = doc["ip"].as<String>();
        
        if (ip.length() == 0) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"IP address required\"}");
            return;
        }
        
        // Call shutter connection function (from SK_shutter.h)
        bool success = connectToShutter(ip);
        
        if (success) {
            server->send(200, "application/json", "{\"success\":true,\"message\":\"Shutter bağlantısı başarılı\"}");
        } else {
            // Daha detaylı hata mesajı için detectDevice kullan
            DiscoveredDevice device = detectDevice(ip);
            String errorMsg = "Bağlantı başarısız: ";
            
            if (!device.isValid) {
                errorMsg += "Cihaz bulunamadı veya yanıt vermiyor";
            } else if (device.mode == "relay") {
                errorMsg += "Cihaz RELAY modunda (Roller moda geçirin)";
            } else if (!device.supportsShutter) {
                errorMsg += "Cihaz roller/shutter modunu desteklemiyor";
            } else {
                errorMsg += "Bilinmeyen hata (Mode: " + device.mode + ")";
            }
            
            String response = "{\"success\":false,\"message\":\"" + errorMsg + "\"}";
            server->send(200, "application/json", response);
        }
    }
    
    void handleDisconnectShutter() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        disconnectShutter();
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Disconnected\"}");
    }
    
    void handleScanShutterNetwork() {
        DEBUG_PRINTLN("[WEB] /scanShutterNetwork called");
        
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        // Configure scanner for shutter devices only
        ScanConfig config;
        config.filters = FILTER_SHUTTERS;
        config.parallelCount = 10;
        config.tcpTimeout = 500;
        config.httpTimeout = 3000;
        config.autoSave = true;
        
        // Start unified scan targeting shutters
        startNetworkScan(config, nullptr, nullptr, nullptr);
        
        String response = "{\"success\":true,\"message\":\"Shutter network scan started\"}";
        server->send(200, "application/json", response);
    }
    
    void handleGetShutterStatus() {
        String json = getShutterStatusJSON();
        server->send(200, "application/json", json);
    }
    
    void handleAdjustShutterStep() {
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        int step = doc["step"].as<int>();
        
        if (step < 1 || step > 5) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Step must be 1-5\"}");
            return;
        }
        
        setEncoderStep(step);
        server->send(200, "application/json", "{\"success\":true}");
    }
    
    // Mode Manager Endpoints
    void handleGetCurrentMode() {
        if (!modeManager) {
            server->send(500, "application/json", "{\"error\":\"Mode manager not initialized\"}");
            return;
        }
        
        String json = modeManager->getStatusJSON();
        server->send(200, "application/json", json);
    }
    
    void handleSetMode() {
        if (!modeManager) {
            server->send(500, "application/json", "{\"success\":false,\"message\":\"Mode manager not initialized\"}");
            return;
        }
        
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        // Try to get mode from form data first
        String modeStr = "";
        if (server->hasArg("mode")) {
            modeStr = server->arg("mode");
        } else {
            // Try JSON body
            String body = server->arg("plain");
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, body);
            
            if (!error && doc["mode"].is<JsonVariant>()) {
                if (doc["mode"].is<int>()) {
                    int modeInt = doc["mode"].as<int>();
                    if (modeInt == 0) modeStr = "DIMMER";
                    else if (modeInt == 1) modeStr = "SHUTTER";
                    else if (modeInt == 2) modeStr = "SAFE";
                } else {
                    modeStr = doc["mode"].as<String>();
                }
            }
        }
        
        if (modeStr.isEmpty()) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Mode parameter required\"}");
            return;
        }
        
        // Convert string to SystemMode enum
        SystemMode mode;
        if (modeStr == "DIMMER" || modeStr == "0") {
            mode = MODE_DIMMER;
        } else if (modeStr == "SHUTTER" || modeStr == "1") {
            mode = MODE_SHUTTER;
        } else if (modeStr == "SAFE" || modeStr == "2") {
            mode = MODE_SAFE;
        } else {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid mode\"}");
            return;
        }
        
        bool success = modeManager->setMode(mode);
        
        if (success) {
            server->send(200, "application/json", "{\"success\":true}");
        } else {
            server->send(500, "application/json", "{\"success\":false,\"message\":\"Failed to set mode\"}");
        }
    }
    
    // Safe Mode Endpoints
    SafeLock* safeLockPtr = nullptr;
    SafeLockAPIHandler* safeApiHandlerPtr = nullptr;
    
    void handleGetSafeStatus() {
        if (!safeLockPtr) {
            server->send(500, "application/json", "{\"error\":\"Safe Lock not initialized\"}");
            return;
        }
        
        JsonDocument doc;
        JsonArray passwords = doc["passwords"].to<JsonArray>();
        
        for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
            JsonObject pwd = passwords.add<JsonObject>();
            pwd["index"] = i;
            pwd["password"] = safeLockPtr->getPassword(i);
            pwd["active"] = safeLockPtr->isPasswordActive(i);
            
            SafeApiConfig apiConfig = safeLockPtr->getApiConfig(i);
            pwd["apiEnabled"] = apiConfig.enabled;
            pwd["apiUrl"] = String(apiConfig.url);
            pwd["apiMethod"] = apiConfig.method == SAFE_HTTP_POST ? "POST" : "GET";
            pwd["apiHeader"] = String(apiConfig.header);
        }
        
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    }
    
    void handleSetSafePassword() {
        if (!safeLockPtr) {
            server->send(500, "application/json", "{\"success\":false,\"message\":\"Safe Lock not initialized\"}");
            return;
        }
        
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        // GÜVENLİK: Hassas veri loglanmıyor
        DEBUG_PRINTLN("[WebServer] /saveSafePassword request received");
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        uint8_t index = doc["index"] | 0;
        String password = doc["password"] | "";
        bool pwdEnabled = doc["pwdEnabled"] | false;
        
        // API bilgileri nested object içinde
        JsonObject api = doc["api"];
        bool apiEnabled = api["enabled"] | false;
        String apiUrl = api["url"] | "";
        String apiMethod = api["method"] | "GET";
        String apiHeader = api["header"] | "";
        String apiBody = api["body"] | "";
        
        // GÜVENLİK: Şifre ve URL loglanmıyor
        DEBUG_PRINTF("[WebServer] Password slot #%d updated (pwdEnabled=%d, apiEnabled=%d)\n", 
                     index, pwdEnabled, apiEnabled);
        
        if (index >= SAFE_MAX_PASSWORDS) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid password index\"}");
            return;
        }
        
        SafeApiConfig apiConfig;
        apiUrl.toCharArray(apiConfig.url, SAFE_API_URL_MAX);
        apiConfig.method = (apiMethod == "POST") ? SAFE_HTTP_POST : SAFE_HTTP_GET;
        apiHeader.toCharArray(apiConfig.header, SAFE_API_HEADER_MAX);
        apiBody.toCharArray(apiConfig.body, SAFE_API_BODY_MAX);
        apiConfig.enabled = apiEnabled;
        
        bool success = safeLockPtr->setPassword(index, password, apiConfig, pwdEnabled);
        
        if (success) {
            server->send(200, "application/json", "{\"success\":true,\"message\":\"Password saved\"}");
        } else {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid password format\"}");
        }
    }
    
    void handleTestSafeApi() {
        // API testi ESP32-C6 tarafindan yapilir, web sadece ayar icin
        if (!safeApiHandlerPtr) {
            server->send(500, "application/json", "{\"success\":false,\"message\":\"Safe API Handler not initialized\"}");
            return;
        }
        
        if (server->method() != HTTP_POST) {
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Invalid JSON\"}");
            return;
        }
        
        String url = doc["url"] | "";
        String methodStr = doc["method"] | "GET";
        String header = doc["header"] | "";
        String bodyData = doc["body"] | "";
        
        if (url.length() == 0) {
            server->send(400, "application/json", "{\"success\":false,\"message\":\"URL required\"}");
            return;
        }
        
        // ESP32-C6 üzerinden API testi yap (SafeLockAPIHandler kullan)
        SafeHttpMethod method = (methodStr == "POST") ? SAFE_HTTP_POST : SAFE_HTTP_GET;
        bool success = safeApiHandlerPtr->testApi(url, method, header, bodyData);
        
        if (success) {
            server->send(200, "application/json", "{\"success\":true,\"message\":\"API test basarili (ESP32-C6)\"}");
        } else {
            server->send(200, "application/json", "{\"success\":false,\"message\":\"API test basarisiz\"}");
        }
    }
    
    // System Actions
    void handleRestart() {
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Restarting...\"}");
        delay(500);
        ESP.restart();
    }
    
    void handleFactoryReset() {
        DEBUG_PRINTLN("[FACTORY RESET] Starting factory reset...");
        
        // 1. WiFi ayarlarını sil
        Preferences wifiPrefs;
        wifiPrefs.begin(PREFS_NAMESPACE, false);
        wifiPrefs.clear();
        wifiPrefs.end();
        DEBUG_PRINTLN("[FACTORY RESET] WiFi settings cleared");
        
        // 2. Dimmer ayarlarını sil
        Preferences dimmerPrefs;
        dimmerPrefs.begin("dimmer-settings", false);
        dimmerPrefs.clear();
        dimmerPrefs.end();
        DEBUG_PRINTLN("[FACTORY RESET] Dimmer settings cleared");
        
        // 3. Shutter ayarlarını sil
        Preferences shutterPrefs;
        shutterPrefs.begin("shutter", false);
        shutterPrefs.clear();
        shutterPrefs.end();
        DEBUG_PRINTLN("[FACTORY RESET] Shutter settings cleared");
        
        // 4. Mode manager ayarlarını sil
        Preferences modePrefs;
        modePrefs.begin("mode", false);
        modePrefs.clear();
        modePrefs.end();
        DEBUG_PRINTLN("[FACTORY RESET] Mode settings cleared");
        
        // 5. Safe Lock ayarlarını sil
        Preferences safePrefs;
        safePrefs.begin("safelock", false);
        safePrefs.clear();
        safePrefs.end();
        DEBUG_PRINTLN("[FACTORY RESET] Safe Lock settings cleared");
        
        server->send(200, "application/json", "{\"success\":true,\"message\":\"Factory reset complete. Restarting...\"}");
        
        delay(1000);
        ESP.restart();
    }
    
    // Static wrapper functions for server callbacks
    static SKWebServer* instance;
    
    static void handleRootStatic() {
        if (instance) {
            instance->handleRoot();
        }
    }
    
    static void handleCSSStatic() {
        if (instance) {
            instance->handleCSS();
        }
    }
    
    static void handleScriptStatic() {
        if (instance) {
            instance->handleScript();
        }
    }
    
    static void handleFaviconStatic() {
        if (instance) {
            instance->handleFavicon();
        }
    }
    
    static void handleNotFoundStatic() {
        if (instance) {
            instance->handleNotFound();
        }
    }
    
    static void handleSaveNetworkStatic() {
        if (instance) {
            instance->handleSaveNetwork();
        }
    }
    
    static void handleGetSettingsStatic() {
        if (instance) {
            instance->handleGetSettings();
        }
    }
    
    static void handleGetStatusStatic() {
        if (instance) {
            instance->handleGetStatus();
        }
    }
    
    // OTA static wrappers
    static void handleGetOTASettingsStatic() {
        if (instance) {
            instance->handleGetOTASettings();
        }
    }
    
    static void handleCheckOTAUpdateStatic() {
        if (instance) {
            instance->handleCheckOTAUpdate();
        }
    }
    
    static void handlePerformOTAUpdateStatic() {
        if (instance) {
            instance->handlePerformOTAUpdate();
        }
    }
    
    static void handleSaveOTASettingsStatic() {
        if (instance) {
            instance->handleSaveOTASettings();
        }
    }
    
    // Language static wrappers
    static void handleGetLangStatic() {
        if (instance) {
            instance->handleGetLang();
        }
    }
    
    static void handleSetLangStatic() {
        if (instance) {
            instance->handleSetLang();
        }
    }
    
    static void handleGetCurrentLangStatic() {
        if (instance) {
            instance->handleGetCurrentLang();
        }
    }
    
    // Dimmer static wrappers
    static void handleConnectDimmerStatic() {
        if (instance) {
            instance->handleConnectDimmer();
        }
    }
    
    static void handleDisconnectDimmerStatic() {
        if (instance) {
            instance->handleDisconnectDimmer();
        }
    }
    
    static void handleGetDimmerStatusStatic() {
        if (instance) {
            instance->handleGetDimmerStatus();
        }
    }
    
    static void handleSetDimmerRatioStatic() {
        if (instance) {
            instance->handleSetDimmerRatio();
        }
    }
    
    static void handleScanNetworkStatic() {
        if (instance) {
            instance->handleScanNetwork();
        }
    }
    
    static void handleStopScanStatic() {
        if (instance) {
            instance->handleStopScan();
        }
    }
    
    static void handleGetScanResultsStatic() {
        if (instance) {
            instance->handleGetScanResults();
        }
    }
    
    static void handleGetScanProgressStatic() {
        if (instance) {
            instance->handleGetScanProgress();
        }
    }
    
    static void handleGetSavedDevicesStatic() {
        if (instance) {
            instance->handleGetSavedDevices();
        }
    }
    
    static void handleRemoveSavedDeviceStatic() {
        if (instance) {
            instance->handleRemoveSavedDevice();
        }
    }
    
    // Shutter static wrappers
    static void handleConnectShutterStatic() {
        if (instance) {
            instance->handleConnectShutter();
        }
    }
    
    static void handleDisconnectShutterStatic() {
        if (instance) {
            instance->handleDisconnectShutter();
        }
    }
    
    static void handleScanShutterNetworkStatic() {
        if (instance) {
            instance->handleScanShutterNetwork();
        }
    }
    
    static void handleGetShutterStatusStatic() {
        if (instance) {
            instance->handleGetShutterStatus();
        }
    }
    
    static void handleAdjustShutterStepStatic() {
        if (instance) {
            instance->handleAdjustShutterStep();
        }
    }
    
    // Mode manager static wrappers
    static void handleGetCurrentModeStatic() {
        if (instance) {
            instance->handleGetCurrentMode();
        }
    }
    
    static void handleSetModeStatic() {
        if (instance) {
            instance->handleSetMode();
        }
    }
    
    static void handleGetSafeStatusStatic() {
        if (instance) {
            instance->handleGetSafeStatus();
        }
    }
    
    static void handleSetSafePasswordStatic() {
        if (instance) {
            instance->handleSetSafePassword();
        }
    }
    
    static void handleTestSafeApiStatic() {
        if (instance) {
            instance->handleTestSafeApi();
        }
    }
    
    static void handleRestartStatic() {
        if (instance) {
            instance->handleRestart();
        }
    }
    
    static void handleFactoryResetStatic() {
        if (instance) {
            instance->handleFactoryReset();
        }
    }
    
public:
    SKWebServer() {
        server = new WebServer(WEB_SERVER_PORT);
        wifiManager = nullptr;
        modeManager = nullptr;
        httpRequestCallback = nullptr;
        instance = this;
    }
    
    ~SKWebServer() {
        if (server) {
            server->stop();
            delete server;
            server = nullptr;
        }
    }
    
    // Set WiFi manager reference
    void setWiFiManager(SKWiFi* manager) {
        wifiManager = manager;
    }
    
    // Set mode manager reference
    void setModeManager(SKModeManager* manager) {
        modeManager = manager;
    }
    
    // Set Safe Lock reference
    void setSafeLock(SafeLock* sl) {
        safeLockPtr = sl;
    }
    
    // Set Safe API Handler reference
    void setSafeApiHandler(SafeLockAPIHandler* handler) {
        safeApiHandlerPtr = handler;
    }
    
    // Set HTTP request callback (for counting requests)
    void setHttpRequestCallback(void (*callback)()) {
        httpRequestCallback = callback;
    }
    
    // Initialize web server
    void begin(String chip_id) {
        chipID = chip_id;
        
        // Setup routes
        server->on("/", handleRootStatic);
        server->on("/style.css", HTTP_GET, handleCSSStatic);
        server->on("/script.js", HTTP_GET, handleScriptStatic);
        server->on("/favicon.ico", HTTP_GET, handleFaviconStatic);
        server->on("/saveNetwork", HTTP_POST, handleSaveNetworkStatic);
        server->on("/getSettings", HTTP_GET, handleGetSettingsStatic);
        server->on("/getStatus", HTTP_GET, handleGetStatusStatic);
        
        // OTA routes
        server->on("/getOTASettings", HTTP_GET, handleGetOTASettingsStatic);
        server->on("/checkOTAUpdate", HTTP_POST, handleCheckOTAUpdateStatic);
        server->on("/performOTAUpdate", HTTP_POST, handlePerformOTAUpdateStatic);
        server->on("/saveOTASettings", HTTP_POST, handleSaveOTASettingsStatic);
        
        // Language routes
        server->on("/getLang", HTTP_GET, handleGetLangStatic);
        server->on("/setLang", HTTP_POST, handleSetLangStatic);
        server->on("/getCurrentLang", HTTP_GET, handleGetCurrentLangStatic);
        
        // Dimmer routes
        server->on("/connectDimmer", HTTP_POST, handleConnectDimmerStatic);
        server->on("/disconnectDimmer", HTTP_POST, handleDisconnectDimmerStatic);
        server->on("/getDimmerStatus", HTTP_GET, handleGetDimmerStatusStatic);
        server->on("/setDimmerRatio", HTTP_POST, handleSetDimmerRatioStatic);
        server->on("/scanNetwork", HTTP_POST, handleScanNetworkStatic);
        server->on("/stopScan", HTTP_POST, handleStopScanStatic);
        server->on("/getScanResults", HTTP_GET, handleGetScanResultsStatic);
        server->on("/getScanProgress", HTTP_GET, handleGetScanProgressStatic);
        server->on("/getSavedDevices", HTTP_GET, handleGetSavedDevicesStatic);
        server->on("/removeSavedDevice", HTTP_POST, handleRemoveSavedDeviceStatic);
        
        // Shutter routes
        server->on("/connectShutter", HTTP_POST, handleConnectShutterStatic);
        server->on("/disconnectShutter", HTTP_POST, handleDisconnectShutterStatic);
        server->on("/scanShutterNetwork", HTTP_POST, handleScanShutterNetworkStatic);
        server->on("/getShutterStatus", HTTP_GET, handleGetShutterStatusStatic);
        server->on("/adjustShutterStep", HTTP_POST, handleAdjustShutterStepStatic);
        
        // Mode manager routes
        server->on("/getCurrentMode", HTTP_GET, handleGetCurrentModeStatic);
        server->on("/setMode", HTTP_POST, handleSetModeStatic);
        
        // Safe mode routes
        server->on("/getSafeStatus", HTTP_GET, handleGetSafeStatusStatic);
        server->on("/saveSafePassword", HTTP_POST, handleSetSafePasswordStatic);
        server->on("/setSafePassword", HTTP_POST, handleSetSafePasswordStatic);
        server->on("/testSafeApi", HTTP_POST, handleTestSafeApiStatic);
        
        // System action routes
        server->on("/restart", HTTP_POST, handleRestartStatic);
        server->on("/factoryReset", HTTP_POST, handleFactoryResetStatic);
        
        server->onNotFound(handleNotFoundStatic);
        
        // Start server
        server->begin();
        
        DEBUG_PRINTLN("HTTP server started on port " + String(WEB_SERVER_PORT));
    }
    
    // Handle client requests
    void handleClient() {
        server->handleClient();
    }
    
    // Get server instance
    WebServer* getServer() {
        return server;
    }
};

// Initialize static member
SKWebServer* SKWebServer::instance = nullptr;

#endif // SK_WEBSERVER_H
