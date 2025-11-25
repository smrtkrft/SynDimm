/**
 * SK_webserver.h
 * SmartKraft SynDimm - Web Server Management
 * Version: v0.9.1
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Web arayüzü SADECE ESP32-C6 içindir!
 * - Kullanıcı sadece cihazın kendisinden erişebilir
 * - Dışarıdan internet erişimi YOK
 * - Sadece bilgilendirme ve basit ayarlar için
 * - Hiçbir kritik kontrol web'den yapılmaz
 * ========================================
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
#include "SK_ota.h"

class SKWebServer {
private:
    WebServer* server;
    String chipID;
    class SKWiFi* wifiManager;  // Forward declaration pointer
    class SKModeManager* modeManager;  // Mode manager pointer
    
    // Handler functions
    void handleRoot() {
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
        Serial.println("\n=== /saveNetwork endpoint called ===");
        
        if (server->method() != HTTP_POST) {
            Serial.println("ERROR: Not a POST request");
            server->send(405, "application/json", "{\"success\":false,\"message\":\"Method Not Allowed\"}");
            return;
        }
        
        String body = server->arg("plain");
        Serial.println("Request body length: " + String(body.length()));
        Serial.println("Request body: " + body);
        
        if (body.length() == 0) {
            Serial.println("ERROR: Empty body");
            server->send(400, "application/json", "{\"success\":false,\"message\":\"Boş veri gönderildi\"}");
            return;
        }
        
        JsonDocument doc;
        DeserializationError error = deserializeJson(doc, body);
        
        if (error) {
            Serial.print("ERROR: JSON parse error: ");
            Serial.println(error.c_str());
            String errorMsg = "{\"success\":false,\"message\":\"JSON hatası: ";
            errorMsg += error.c_str();
            errorMsg += "\"}";
            server->send(400, "application/json", errorMsg);
            return;
        }
        
        Serial.println("JSON parsed successfully");
        
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
        
        Serial.println("Extracted data:");
        Serial.println("  Primary SSID: " + p_ssid);
        Serial.println("  Backup SSID: " + b_ssid);
        
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
            Serial.println("\n=== Saving Network Settings ===");
            Serial.println("Primary SSID: " + p_ssid);
            Serial.println("Backup SSID: " + b_ssid);
            
            bool saved = wifiManager->saveNetworkSettings(p_ssid, p_pass, p_ip, p_mdns,
                                                         b_ssid, b_pass, b_ip, b_mdns);
            
            if (saved) {
                Serial.println("Settings saved successfully!");
                server->send(200, "application/json", "{\"success\":true,\"message\":\"Ayarlar kaydedildi! Cihaz yeniden başlatılıyor...\"}");
                
                // Schedule restart after response is sent
                delay(500);
                Serial.println("Restarting ESP32...\n");
                delay(500);
                ESP.restart();
            } else {
                Serial.println("Failed to save settings!");
                server->send(500, "application/json", "{\"success\":false,\"message\":\"Ayarlar kaydedilemedi - bellek hatası\"}");
            }
        } else {
            Serial.println("ERROR: WiFi manager not found!");
            server->send(500, "application/json", "{\"success\":false,\"message\":\"WiFi yöneticisi bulunamadı\"}");
        }
    }
    
    void handleGetSettings() {
        if (wifiManager) {
            String json = wifiManager->getNetworkSettingsJSON();
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"WiFi manager not found\"}");
        }
    }
    
    void handleGetStatus() {
        if (wifiManager) {
            String json = wifiManager->getConnectionStatusJSON();
            server->send(200, "application/json", json);
        } else {
            server->send(500, "application/json", "{\"error\":\"WiFi manager not found\"}");
        }
    }
    
    // ========================================
    // OTA ENDPOINTS
    // ========================================
    
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
    
    // ========================================
    // DIMMER ENDPOINTS
    // ========================================
    
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
        Serial.println("[WEB] /scanNetwork called");
        
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
        Serial.println("[WEB] /stopScan called");
        
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
        Serial.println("[WEB] /getScanResults called");
        
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
        Serial.println("[WEB] /getScanProgress called");
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
    
    // ========================================
    // END DIMMER ENDPOINTS
    // ========================================
    
    // ========================================
    // SHUTTER ENDPOINTS
    // ========================================
    
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
        Serial.println("[WEB] /scanShutterNetwork called");
        
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
    
    // ========================================
    // END SHUTTER ENDPOINTS
    // ========================================
    
    // ========================================
    // MODE MANAGER ENDPOINTS
    // ========================================
    
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
            
            if (!error && doc.containsKey("mode")) {
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
    
    // ========================================
    // END MODE MANAGER ENDPOINTS
    // ========================================
    
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
    
public:
    SKWebServer() {
        server = new WebServer(WEB_SERVER_PORT);
        wifiManager = nullptr;
        modeManager = nullptr;
        instance = this;
    }
    
    // Set WiFi manager reference
    void setWiFiManager(SKWiFi* manager) {
        wifiManager = manager;
    }
    
    // Set mode manager reference
    void setModeManager(SKModeManager* manager) {
        modeManager = manager;
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
        
        server->onNotFound(handleNotFoundStatic);
        
        // Start server
        server->begin();
        
        Serial.println("HTTP server started on port " + String(WEB_SERVER_PORT));
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
