/**
 * SK_webserver.h - Web Server Management v1.1.0
 * KRITIK: Web arayüzü SADECE ESP32-C6 local erişim içindir!
 */

#ifndef SK_WEBSERVER_H
#define SK_WEBSERVER_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include <LittleFS.h>
#include "SK_config.h"
#include "SK_html.h"
#include "SK_js.h"
#include "SK_scan.h"
#include "SK_dimmer.h"
#include "SK_wiz.h"
// #include "SK_shutter.h"  // TEMP DISABLED
#include "SK_mode_safe.h"
#include "SK_mode_safe_api.h"
#include "SK_ota.h"
#include "SK_lang.h"

// TEMP: Shutter stub functions
inline void initShutter() {}
inline void autoReconnectShutter() {}
inline String getShutterStatusJSON() { return "{\"connected\":false}"; }
inline String getSavedShutterDevicesJSON() { return "{\"devices\":[]}"; }
inline bool connectToShutter(const String&) { return false; }
inline void disconnectShutter() {}
inline void setShutterPosition(int) {}
inline void stopShutter() {}
inline void shutterUp() {}
inline void shutterDown() {}
inline void setShutterEncoderStep(int) {}
inline void removeSavedShutterDevice(const String&) {}
inline void clearShutterLastConnection() {}
inline void setEncoderStep(int) {}
inline bool removeShutterDevice(const String&) { return false; }
struct ShutterDevice { bool isConnected = false; };
inline ShutterDevice shutterDevice;

class SKWebServer {
private:
    WebServer* server;
    String chipID;
    class SKWiFi* wifiManager;  // Forward declaration pointer
    class SKModeManager* modeManager;  // Mode manager pointer
    void (*httpRequestCallback)();  // HTTP request callback
    
    // ========== HELPER FUNCTIONS ==========
    
    // Send JSON success response
    void sendSuccess(const char* msg = nullptr) {
        String json = msg ? String("{\"success\":true,\"message\":\"") + msg + "\"}" 
                          : "{\"success\":true}";
        server->send(200, "application/json", json);
    }
    
    // Send JSON error response
    void sendError(int code, const char* msg) {
        String json = String("{\"success\":false,\"message\":\"") + msg + "\"}";
        server->send(code, "application/json", json);
    }
    
    // Check if request is POST method
    bool requirePost() {
        if (server->method() != HTTP_POST) {
            sendError(405, "Method Not Allowed");
            return false;
        }
        return true;
    }
    
    // Parse JSON body from POST request, returns true if successful
    // Maximum body size: 4KB to prevent heap exhaustion attacks
    static const size_t MAX_JSON_BODY_SIZE = 4096;
    
    bool parseJsonBody(JsonDocument& doc) {
        if (!requirePost()) return false;
        
        String body = server->arg("plain");
        if (body.length() == 0) {
            sendError(400, "Empty body");
            return false;
        }
        
        // Protect against oversized payloads
        if (body.length() > MAX_JSON_BODY_SIZE) {
            sendError(413, "Payload too large");
            return false;
        }
        
        DeserializationError error = deserializeJson(doc, body);
        if (error) {
            sendError(400, "Invalid JSON");
            return false;
        }
        return true;
    }
    
    // Clear a preferences namespace
    void clearPrefsNamespace(const char* ns) {
        Preferences prefs;
        prefs.begin(ns, false);
        prefs.clear();
        prefs.end();
    }
    
    // Helper function to send PROGMEM data in chunks
    void sendProgmemChunked(const char* progmemData) {
        size_t len = strlen_P(progmemData);
        size_t chunkSize = 512;
        char buffer[513];
        
        for (size_t i = 0; i < len; i += chunkSize) {
            if (!server->client().connected()) break;
            size_t remaining = len - i;
            size_t size = (remaining < chunkSize) ? remaining : chunkSize;
            memcpy_P(buffer, progmemData + i, size);
            buffer[size] = '\0';
            server->sendContent(buffer);
            yield();
        }
    }
    
    // ========== HANDLER FUNCTIONS ==========
    void handleRoot() {
        if (httpRequestCallback) httpRequestCallback();
        
        // Calculate total content length
        size_t totalLen = strlen_P(SK_HTML_PART1) + chipID.length() + 
                          strlen_P(SK_HTML_PART2) + strlen(VERSION) + 
                          strlen_P(SK_HTML_PART3);
        
        // Send headers
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "0");
        server->setContentLength(totalLen);
        server->send(200, "text/html", "");
        
        // Send HTML in parts from PROGMEM
        sendProgmemChunked(SK_HTML_PART1);
        server->sendContent(chipID);
        sendProgmemChunked(SK_HTML_PART2);
        server->sendContent(VERSION);
        sendProgmemChunked(SK_HTML_PART3);
    }
    
    void handleCSS() {
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "0");
        server->send_P(200, "text/css", SK_CSS);
    }
    
    void handleScript() {
        server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
        server->sendHeader("Pragma", "no-cache");
        server->sendHeader("Expires", "0");
        server->setContentLength(strlen_P(SK_JS));
        server->send(200, "application/javascript", "");
        
        // Send in chunks to avoid timeout
        const char* ptr = SK_JS;
        size_t len = strlen_P(SK_JS);
        size_t chunkSize = 1024;
        
        for (size_t i = 0; i < len; i += chunkSize) {
            // Check if client still connected before sending
            if (!server->client().connected()) {
                DEBUG_PRINTLN("[WEBSERVER] Client disconnected during script transfer");
                break;
            }
            
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
    
    void handleScanWiFi() {
        if (httpRequestCallback) httpRequestCallback();
        if (wifiManager) {
            String json = wifiManager->getScannedNetworksJSON();
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
    
    void handleSetAPMode() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        if (!wifiManager) {
            sendError(500, "WiFi manager not found");
            return;
        }
        
        wifiManager->setAPModeEnabled(doc["enabled"].as<bool>());
        sendSuccess();
    }
    
    // OTA Endpoints
    void handleGetOTASettings() {
        server->send(200, "application/json", getOTASettingsJSON());
    }
    
    void handleCheckOTAUpdate() {
        if (!requirePost()) return;
        
        if (checkForUpdates()) {
            server->send(200, "application/json", getOTASettingsJSON());
        } else {
            sendError(500, "Update check failed");
        }
    }
    
    void handlePerformOTAUpdate() {
        if (!requirePost()) return;
        sendSuccess("Update started");
        delay(100);
        performOTAUpdate();
    }
    
    void handleSaveOTASettings() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        saveOTASettings(doc["autoUpdate"].as<bool>());
        sendSuccess();
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
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        if (setLanguageFromCode(doc["lang"].as<String>())) {
            sendSuccess();
        } else {
            sendError(400, "Invalid language code");
        }
    }
    
    void handleGetCurrentLang() {
        String json = "{\"lang\":\"" + getCurrentLangCode() + "\"}";
        server->send(200, "application/json", json);
    }
    
    // Dimmer Endpoints
    void handleConnectDimmer() {
        Serial.println("[DIMMER] handleConnectDimmer called");
        JsonDocument doc;
        if (!parseJsonBody(doc)) {
            Serial.println("[DIMMER] parseJsonBody failed");
            return;
        }
        
        String ip = doc["ip"].as<String>();
        Serial.print("[DIMMER] IP: "); Serial.println(ip);
        
        if (ip.length() == 0) {
            sendError(400, "IP address required");
            return;
        }
        
        if (connectToDimmer(ip)) {
            Serial.println("[DIMMER] Connected OK");
            sendSuccess("Connected to dimmer");
        } else {
            Serial.println("[DIMMER] Connection FAILED");
            server->send(200, "application/json", "{\"success\":false,\"message\":\"Connection failed\"}");
        }
    }
    
    void handleDisconnectDimmer() {
        if (!requirePost()) return;
        // Kullanıcı manuel olarak disconnect dediğinde, cihazı da unut
        clearLastConnection();  // lastConnectedIP'yi temizle - otomatik reconnect olmayacak
        disconnectDimmer();
        sendSuccess("Disconnected and cleared");
    }
    
    void handleGetDimmerStatus() {
        // WiZ ve Shelly durumlarını birleştir
        JsonDocument doc;
        
        // Shelly dimmer durumu
        doc["shelly"]["connected"] = dimmerDevice.isConnected;
        doc["shelly"]["ip"] = dimmerDevice.ip;
        doc["shelly"]["name"] = dimmerDevice.name;
        doc["shelly"]["brightness"] = dimmerDevice.brightness;
        doc["shelly"]["isOn"] = dimmerDevice.isOn;
        doc["shelly"]["type"] = (int)dimmerDevice.type;
        
        // WiZ durumu
        doc["wiz"]["connected"] = wizDevice.isConnected;
        doc["wiz"]["ip"] = wizDevice.ip;
        doc["wiz"]["mac"] = wizDevice.mac;
        doc["wiz"]["brightness"] = wizDevice.brightness;
        doc["wiz"]["state"] = wizDevice.state;
        doc["wiz"]["r"] = wizDevice.r;
        doc["wiz"]["g"] = wizDevice.g;
        doc["wiz"]["b"] = wizDevice.b;
        doc["wiz"]["hue"] = wizDevice.hue;
        doc["wiz"]["supportsRGB"] = wizDevice.supportsRGB;
        doc["wiz"]["colorControlActive"] = isWiZColorControlActive();
        
        // Genel durum
        doc["ratio"] = dimmerConfig.dimmerRatio;
        doc["defaultBrightnessEnabled"] = dimmerConfig.defaultBrightnessEnabled;
        doc["defaultBrightnessValue"] = dimmerConfig.defaultBrightnessValue;
        doc["activeDevice"] = wizDevice.isConnected ? "wiz" : (dimmerDevice.isConnected ? "shelly" : "none");
        
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    }
    
    void handleSetDimmerRatio() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        int ratio = doc["ratio"].as<int>();
        if (ratio < 1 || ratio > 5) {
            sendError(400, "Ratio must be between 1-5");
            return;
        }
        
        setDimmerRatio(ratio);
        sendSuccess("Ratio saved");
    }
    
    // ========== WIZ API HANDLERS ==========
    
    void handleConnectWiZ() {
        WIZ_DEBUG("[WIZ] handleConnectWiZ called");
        JsonDocument doc;
        if (!parseJsonBody(doc)) {
            WIZ_DEBUG("[WIZ] parseJsonBody failed");
            return;
        }
        
        String ip = doc["ip"].as<String>();
        WIZ_DEBUGF("[WIZ] IP from request: %s\n", ip.c_str());
        
        if (ip.length() == 0) {
            WIZ_DEBUG("[WIZ] Empty IP!");
            sendError(400, "IP required");
            return;
        }
        
        if (connectToWiZ(ip)) {
            sendSuccess("Connected to WiZ");
        } else {
            sendError(500, "Failed to connect to WiZ");
        }
    }
    
    void handleDisconnectWiZ() {
        if (!requirePost()) return;
        disconnectWiZ();
        sendSuccess("Disconnected from WiZ");
    }
    
    void handleGetWiZStatus() {
        String json = getWiZStatusJSON();
        server->send(200, "application/json", json);
    }
    
    void handleSetWiZBrightness() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        int brightness = doc["brightness"].as<int>();
        if (brightness < 10 || brightness > 100) {
            sendError(400, "Brightness must be between 10-100");
            return;
        }
        
        if (setWiZBrightness(wizDevice.ip, brightness)) {
            sendSuccess("Brightness set");
        } else {
            sendError(500, "Failed to set brightness");
        }
    }
    
    void handleSetWiZColor() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        if (doc["hue"].is<int>()) {
            int hue = doc["hue"].as<int>();
            if (setWiZHue(wizDevice.ip, hue)) {
                sendSuccess("Color set");
            } else {
                sendError(500, "Failed to set color");
            }
        } else if (doc["r"].is<int>()) {
            uint8_t r = doc["r"] | 0;
            uint8_t g = doc["g"] | 0;
            uint8_t b = doc["b"] | 0;
            if (setWiZRGB(wizDevice.ip, r, g, b)) {
                sendSuccess("Color set");
            } else {
                sendError(500, "Failed to set color");
            }
        } else {
            sendError(400, "hue or r,g,b required");
        }
    }
    
    void handleToggleWiZ() {
        if (!requirePost()) return;
        if (toggleWiZ(wizDevice.ip)) {
            sendSuccess(wizDevice.state ? "Turned on" : "Turned off");
        } else {
            sendError(500, "Failed to toggle");
        }
    }
    
    void handleDiscoverWiZ() {
        if (!requirePost()) return;
        
        auto devices = discoverWiZDevices(3000);
        
        JsonDocument doc;
        JsonArray arr = doc.to<JsonArray>();
        
        for (auto& d : devices) {
            JsonObject obj = arr.add<JsonObject>();
            obj["ip"] = d.ip;
            obj["mac"] = d.mac;
        }
        
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    }
    
    void handleScanNetwork() {
        DEBUG_PRINTLN("[WEB] /scanNetwork called");
        if (!requirePost()) return;
        
        // Check available memory before starting scan
        if (ESP.getFreeHeap() < 25000) {
            sendError(503, "Insufficient memory for network scan");
            DEBUG_PRINTF("[WEB] Scan rejected - low heap: %lu bytes\n", ESP.getFreeHeap());
            return;
        }
        
        ScanConfig config;
        config.filters = FILTER_DIMMERS | FILTER_SHUTTERS | FILTER_WIZ;  // WiZ dahil
        config.parallelCount = 10;
        config.tcpTimeout = 500;
        config.httpTimeout = 3000;
        config.autoSave = true;
        
        startNetworkScan(config, nullptr, nullptr, nullptr);
        sendSuccess("Network scan started");
    }
    
    void handleStopScan() {
        DEBUG_PRINTLN("[WEB] /stopScan called");
        if (!requirePost()) return;
        stopNetworkScan();
        sendSuccess("Scan stopped");
    }
    
    void handleGetScanResults() {
        Serial.println("[WEB] /getScanResults called");
        
        // Get results from unified scanner
        JsonDocument doc;
        JsonArray devices = doc["devices"].to<JsonArray>();
        
        auto discoveredDevices = getDiscoveredDevices();
        Serial.printf("[WEB] Found %d devices\n", discoveredDevices.size());
        
        for (const auto& device : discoveredDevices) {
            Serial.printf("[WEB] Device: %s, cat=%d, name=%s\n", 
                device.ip.c_str(), (int)device.category, device.displayName.c_str());
            
            JsonObject obj = devices.add<JsonObject>();
            obj["ip"] = device.ip;
            obj["modelName"] = device.modelName;
            obj["displayName"] = device.displayName;
            obj["category"] = (int)device.category;
            obj["generation"] = device.generation;
            obj["supportsDimming"] = device.supportsDimming;
            obj["supportsShutter"] = device.supportsShutter;
            obj["supportsRGBW"] = device.supportsRGBW;
            obj["specificType"] = device.specificType;
            obj["macAddress"] = device.macAddress;
            obj["isWiZ"] = (device.category == CATEGORY_WIZ);
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
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        String ip = doc["ip"].as<String>();
        if (ip.length() == 0) {
            sendError(400, "No IP provided");
            return;
        }
        
        if (removeDimmerDevice(ip)) {
            sendSuccess();
        } else {
            server->send(200, "application/json", "{\"success\": false, \"message\": \"Device not found\"}");
        }
    }
    
    // Shutter Endpoints
    void handleConnectShutter() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        String ip = doc["ip"].as<String>();
        if (ip.length() == 0) {
            sendError(400, "IP address required");
            return;
        }
        
        if (connectToShutter(ip)) {
            sendSuccess("Shutter connected");
        } else {
            DiscoveredDevice device = detectDevice(ip);
            String errorMsg = "Connection failed: ";
            
            if (!device.isValid) errorMsg += "Device not found";
            else if (device.mode == "relay") errorMsg += "Device in RELAY mode";
            else if (!device.supportsShutter) errorMsg += "Device doesn't support shutter";
            else errorMsg += "Unknown error";
            
            server->send(200, "application/json", "{\"success\":false,\"message\":\"" + errorMsg + "\"}");
        }
    }
    
    void handleDisconnectShutter() {
        if (!requirePost()) return;
        // Kullanıcı manuel olarak disconnect dediğinde, cihazı da unut
        clearShutterLastConnection();  // lastConnectedIP'yi temizle - otomatik reconnect olmayacak
        disconnectShutter();
        sendSuccess("Disconnected and cleared");
    }
    
    void handleScanShutterNetwork() {
        DEBUG_PRINTLN("[WEB] /scanShutterNetwork called");
        if (!requirePost()) return;
        
        // Check available memory before starting scan
        if (ESP.getFreeHeap() < 25000) {
            sendError(503, "Insufficient memory for network scan");
            DEBUG_PRINTF("[WEB] Shutter scan rejected - low heap: %lu bytes\n", ESP.getFreeHeap());
            return;
        }
        
        ScanConfig config;
        config.filters = FILTER_SHUTTERS;
        config.parallelCount = 10;
        config.tcpTimeout = 500;
        config.httpTimeout = 3000;
        config.autoSave = true;
        
        startNetworkScan(config, nullptr, nullptr, nullptr);
        sendSuccess("Shutter network scan started");
    }
    
    void handleGetShutterStatus() {
        server->send(200, "application/json", getShutterStatusJSON());
    }
    
    void handleAdjustShutterStep() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        int step = doc["step"].as<int>();
        if (step < 1 || step > 5) {
            sendError(400, "Step must be 1-5");
            return;
        }
        
        setEncoderStep(step);
        sendSuccess();
    }
    
    void handleGetSavedShutterDevices() {
        String json = getSavedShutterDevicesJSON();
        server->send(200, "application/json", json);
    }
    
    void handleRemoveSavedShutterDevice() {
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        String ip = doc["ip"].as<String>();
        if (ip.length() == 0) {
            sendError(400, "No IP provided");
            return;
        }
        
        if (removeShutterDevice(ip)) {
            sendSuccess();
        } else {
            server->send(200, "application/json", "{\"success\": false, \"message\": \"Device not found\"}");
        }
    }
    
    // Mode Manager Endpoints
    void handleGetCurrentMode() {
        if (!modeManager) {
            sendError(500, "Mode manager not initialized");
            return;
        }
        server->send(200, "application/json", modeManager->getStatusJSON());
    }
    
    void handleSetMode() {
        if (!modeManager) {
            sendError(500, "Mode manager not initialized");
            return;
        }
        if (!requirePost()) return;
        
        // Try to get mode from form data or JSON body
        String modeStr = "";
        if (server->hasArg("mode")) {
            modeStr = server->arg("mode");
        } else {
            JsonDocument doc;
            deserializeJson(doc, server->arg("plain"));
            if (doc["mode"].is<int>()) {
                int m = doc["mode"].as<int>();
                modeStr = (m == 0) ? "DIMMER" : (m == 1) ? "SHUTTER" : (m == 2) ? "SAFE" : "";
            } else {
                modeStr = doc["mode"].as<String>();
            }
        }
        
        if (modeStr.isEmpty()) {
            sendError(400, "Mode parameter required");
            return;
        }
        
        SystemMode mode;
        if (modeStr == "DIMMER" || modeStr == "0") mode = MODE_DIMMER;
        else if (modeStr == "SHUTTER" || modeStr == "1") mode = MODE_SHUTTER;
        else if (modeStr == "SAFE" || modeStr == "2") mode = MODE_SAFE;
        else { sendError(400, "Invalid mode"); return; }
        
        if (modeManager->setMode(mode)) {
            sendSuccess();
        } else {
            sendError(500, "Failed to set mode");
        }
    }
    
    // Encoder Mode Change Toggle
    void handleSetEncoderModeChange() {
        if (!modeManager) {
            sendError(500, "Mode manager not initialized");
            return;
        }
        if (!requirePost()) return;
        
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        bool enabled = doc["enabled"].as<bool>();
        modeManager->setEncoderModeChangeEnabled(enabled);
        sendSuccess();
    }
    
    // Default Brightness Settings
    void handleSetDefaultBrightness() {
        if (!requirePost()) return;
        
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        // Check if enabling/disabling toggle
        if (!doc["enabled"].isNull()) {
            bool enabled = doc["enabled"].as<bool>();
            setDefaultBrightnessEnabled(enabled);
        }
        
        // Check if setting value
        if (!doc["value"].isNull()) {
            int value = doc["value"].as<int>();
            // Clamp to valid range
            if (value < 10) value = 10;
            if (value > 100) value = 100;
            setDefaultBrightnessValue(value);
        }
        
        sendSuccess();
    }
    
    // Safe Mode Endpoints
    SafeLock* safeLockPtr = nullptr;
    SafeLockAPIHandler* safeApiHandlerPtr = nullptr;
    
    void handleGetSafeStatus() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        JsonDocument doc;
        JsonArray passwords = doc["passwords"].to<JsonArray>();
        
        for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
            JsonObject pwd = passwords.add<JsonObject>();
            pwd["index"] = i;
            pwd["password"] = safeLockPtr->getPassword(i);
            pwd["active"] = safeLockPtr->isPasswordActive(i);
            pwd["hasApiConfig"] = safeLockPtr->hasApiConfig(i);
            
            // API config'i sadece istendiğinde yükle (lazy-load)
            // Listelemede sadece hasApiConfig kullanılır
        }
        
        doc["littleFsReady"] = safeLockPtr->isLittleFsReady();
        
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    }
    
    // Tek bir şifrenin API config'ini getir (lazy-load)
    void handleGetSafeApiConfig() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        if (!server->hasArg("index")) {
            sendError(400, "Index required");
            return;
        }
        
        uint8_t index = server->arg("index").toInt();
        if (index >= SAFE_MAX_PASSWORDS) {
            sendError(400, "Invalid index");
            return;
        }
        
        SafeApiConfig cfg = safeLockPtr->getApiConfig(index);
        String json = cfg.toJson();
        server->send(200, "application/json", json);
    }
    
    // Teaching Mode - Şifre öğretmeyi başlat
    void handleStartSafeTeaching() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        DEBUG_PRINTLN("[WebServer] /startSafeTeaching request received");
        
        uint8_t index = doc["index"] | 0;
        if (index >= SAFE_MAX_PASSWORDS) {
            sendError(400, "Invalid password index");
            return;
        }
        
        if (safeLockPtr->startTeaching(index)) {
            sendSuccess("Teaching started");
        } else {
            sendError(400, "Could not start teaching mode");
        }
    }
    
    // Teaching Mode - Şifre öğretmeyi iptal et
    void handleCancelSafeTeaching() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        DEBUG_PRINTLN("[WebServer] /cancelSafeTeaching request received");
        safeLockPtr->cancelTeaching("user_cancelled");
        sendSuccess("Teaching cancelled");
    }
    
    // Teaching Mode - Web'den şifreyi kaydet
    void handleCompleteSafeTeaching() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        DEBUG_PRINTLN("[WebServer] /completeSafeTeaching");
        
        if (safeLockPtr->completeTeachingFromWeb()) {
            String pattern = safeLockPtr->getLastCompletedPattern();
            JsonDocument resp;
            resp["success"] = true;
            resp["pattern"] = pattern;
            String output;
            serializeJson(resp, output);
            server->send(200, "application/json", output);
        } else {
            sendError(400, "Could not complete teaching - minimum 3 steps required");
        }
    }
    
    // Teaching Mode - Durumu getir
    void handleGetTeachingStatus() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        JsonDocument doc;
        bool isTeaching = safeLockPtr->isTeaching();
        doc["teaching"] = isTeaching;
        doc["index"] = safeLockPtr->getTeachingIndex();
        
        // Teaching devam ediyorsa canlı pattern, bitttiyse son tamamlanan pattern
        if (isTeaching) {
            doc["pattern"] = safeLockPtr->getTeachingPattern();
            doc["stepCount"] = safeLockPtr->getTeachingStepCount();
        } else {
            // Teaching bitti - son tamamlanan pattern'i al
            doc["pattern"] = safeLockPtr->getLastCompletedPattern();
            doc["stepCount"] = 0;
        }
        
        String output;
        serializeJson(doc, output);
        server->send(200, "application/json", output);
    }

    void handleSetSafePassword() {
        if (!safeLockPtr) {
            sendError(500, "Safe Lock not initialized");
            return;
        }
        
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        DEBUG_PRINTLN("[WebServer] /saveSafePassword request received");
        
        uint8_t index = doc["index"] | 0;
        if (index >= SAFE_MAX_PASSWORDS) {
            sendError(400, "Invalid password index");
            return;
        }
        
        // Yeni String tabanlı SafeApiConfig
        JsonObject api = doc["api"];
        SafeApiConfig apiConfig;
        apiConfig.url = api["url"] | "";
        apiConfig.contentType = api["contentType"] | "application/json";
        apiConfig.authorization = api["authorization"] | "";
        apiConfig.customHeaders = api["customHeaders"] | "";
        apiConfig.body = api["body"] | "";
        apiConfig.enabled = api["enabled"] | false;
        
        // HTTP Method parsing
        String methodStr = api["method"] | "GET";
        if (methodStr == "POST") apiConfig.method = SAFE_HTTP_POST;
        else if (methodStr == "PUT") apiConfig.method = SAFE_HTTP_PUT;
        else if (methodStr == "DELETE") apiConfig.method = SAFE_HTTP_DELETE;
        else apiConfig.method = SAFE_HTTP_GET;
        
        if (safeLockPtr->setPassword(index, doc["password"] | "", apiConfig, doc["pwdEnabled"] | false)) {
            sendSuccess("Password saved");
        } else {
            sendError(400, "Invalid password format");
        }
    }
    
    void handleTestSafeApi() {
        if (!safeApiHandlerPtr) {
            sendError(500, "Safe API Handler not initialized");
            return;
        }
        
        JsonDocument doc;
        if (!parseJsonBody(doc)) return;
        
        String url = doc["url"] | "";
        if (url.length() == 0) {
            sendError(400, "URL required");
            return;
        }
        
        // Method parsing
        String methodStr = doc["method"] | "GET";
        SafeHttpMethod method = SAFE_HTTP_GET;
        if (methodStr == "POST") method = SAFE_HTTP_POST;
        else if (methodStr == "PUT") method = SAFE_HTTP_PUT;
        else if (methodStr == "DELETE") method = SAFE_HTTP_DELETE;
        
        if (safeApiHandlerPtr->testApi(
            url, 
            method, 
            doc["contentType"] | "", 
            doc["authorization"] | "",
            doc["customHeaders"] | "", 
            doc["body"] | ""
        )) {
            sendSuccess("API test successful");
        } else {
            server->send(200, "application/json", "{\"success\":false,\"message\":\"API test failed\"}");
        }
    }
    
    // System Actions
    void handleRestart() {
        sendSuccess("Restarting...");
        delay(500);
        ESP.restart();
    }
    
    void handleFactoryReset() {
        DEBUG_PRINTLN("[FACTORY RESET] Starting factory reset...");
        
        resetFirstSetup();
        
        // Clear all preferences namespaces (except DEVICE_PREFS_NAMESPACE which keeps Device ID)
        const char* namespaces[] = {PREFS_NAMESPACE, SYSTEM_PREFS_NAMESPACE, "dimmer-settings", "shutter", "mode_mgr", "safelock", "ota-settings", "lang"};
        for (const char* ns : namespaces) {
            clearPrefsNamespace(ns);
        }
        DEBUG_PRINTLN("[FACTORY RESET] All NVS settings cleared");
        
        // Clear LittleFS safe mode API configs
        if (LittleFS.begin(true)) {
            for (int i = 0; i < 5; i++) {
                String path = String(SAFE_API_CONFIG_PREFIX) + String(i) + SAFE_API_CONFIG_SUFFIX;
                if (LittleFS.exists(path)) {
                    LittleFS.remove(path);
                    DEBUG_PRINTF("[FACTORY RESET] Removed: %s\n", path.c_str());
                }
            }
            DEBUG_PRINTLN("[FACTORY RESET] LittleFS API configs cleared");
        }
        
        sendSuccess("Factory reset complete. Restarting...");
        delay(1000);
        ESP.restart();
    }
    
    // Static instance for callbacks
    static SKWebServer* instance;
    
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
        
        // Setup routes using lambda callbacks
        server->on("/", [this]() { handleRoot(); });
        server->on("/style.css", HTTP_GET, [this]() { handleCSS(); });
        server->on("/script.js", HTTP_GET, [this]() { handleScript(); });
        server->on("/favicon.ico", HTTP_GET, [this]() { handleFavicon(); });
        server->on("/saveNetwork", HTTP_POST, [this]() { handleSaveNetwork(); });
        server->on("/getSettings", HTTP_GET, [this]() { handleGetSettings(); });
        server->on("/scanWiFi", HTTP_GET, [this]() { handleScanWiFi(); });
        server->on("/getStatus", HTTP_GET, [this]() { handleGetStatus(); });
        server->on("/setAPMode", HTTP_POST, [this]() { handleSetAPMode(); });
        
        // OTA routes
        server->on("/getOTASettings", HTTP_GET, [this]() { handleGetOTASettings(); });
        server->on("/checkOTAUpdate", HTTP_POST, [this]() { handleCheckOTAUpdate(); });
        server->on("/performOTAUpdate", HTTP_POST, [this]() { handlePerformOTAUpdate(); });
        server->on("/saveOTASettings", HTTP_POST, [this]() { handleSaveOTASettings(); });
        
        // Language routes
        server->on("/getLang", HTTP_GET, [this]() { handleGetLang(); });
        server->on("/setLang", HTTP_POST, [this]() { handleSetLang(); });
        server->on("/getCurrentLang", HTTP_GET, [this]() { handleGetCurrentLang(); });
        
        // Dimmer routes
        server->on("/connectDimmer", HTTP_POST, [this]() { handleConnectDimmer(); });
        server->on("/disconnectDimmer", HTTP_POST, [this]() { handleDisconnectDimmer(); });
        server->on("/getDimmerStatus", HTTP_GET, [this]() { handleGetDimmerStatus(); });
        server->on("/setDimmerRatio", HTTP_POST, [this]() { handleSetDimmerRatio(); });
        server->on("/scanNetwork", HTTP_POST, [this]() { handleScanNetwork(); });
        server->on("/stopScan", HTTP_POST, [this]() { handleStopScan(); });
        server->on("/getScanResults", HTTP_GET, [this]() { handleGetScanResults(); });
        server->on("/getScanProgress", HTTP_GET, [this]() { handleGetScanProgress(); });
        server->on("/getSavedDevices", HTTP_GET, [this]() { handleGetSavedDevices(); });
        server->on("/removeSavedDevice", HTTP_POST, [this]() { handleRemoveSavedDevice(); });
        
        // WiZ routes
        server->on("/connectWiZ", HTTP_POST, [this]() { handleConnectWiZ(); });
        server->on("/disconnectWiZ", HTTP_POST, [this]() { handleDisconnectWiZ(); });
        server->on("/getWiZStatus", HTTP_GET, [this]() { handleGetWiZStatus(); });
        server->on("/setWiZBrightness", HTTP_POST, [this]() { handleSetWiZBrightness(); });
        server->on("/setWiZColor", HTTP_POST, [this]() { handleSetWiZColor(); });
        server->on("/toggleWiZ", HTTP_POST, [this]() { handleToggleWiZ(); });
        server->on("/discoverWiZ", HTTP_POST, [this]() { handleDiscoverWiZ(); });
        
        // Shutter routes
        server->on("/connectShutter", HTTP_POST, [this]() { handleConnectShutter(); });
        server->on("/disconnectShutter", HTTP_POST, [this]() { handleDisconnectShutter(); });
        server->on("/scanShutterNetwork", HTTP_POST, [this]() { handleScanShutterNetwork(); });
        server->on("/getShutterStatus", HTTP_GET, [this]() { handleGetShutterStatus(); });
        server->on("/adjustShutterStep", HTTP_POST, [this]() { handleAdjustShutterStep(); });
        server->on("/getSavedShutterDevices", HTTP_GET, [this]() { handleGetSavedShutterDevices(); });
        server->on("/removeSavedShutterDevice", HTTP_POST, [this]() { handleRemoveSavedShutterDevice(); });
        
        // Mode manager routes
        server->on("/getCurrentMode", HTTP_GET, [this]() { handleGetCurrentMode(); });
        server->on("/setMode", HTTP_POST, [this]() { handleSetMode(); });
        server->on("/setEncoderModeChange", HTTP_POST, [this]() { handleSetEncoderModeChange(); });
        server->on("/setDefaultBrightness", HTTP_POST, [this]() { handleSetDefaultBrightness(); });
        
        // Safe mode routes
        server->on("/getSafeStatus", HTTP_GET, [this]() { handleGetSafeStatus(); });
        server->on("/getSafeApiConfig", HTTP_GET, [this]() { handleGetSafeApiConfig(); });
        server->on("/saveSafePassword", HTTP_POST, [this]() { handleSetSafePassword(); });
        server->on("/setSafePassword", HTTP_POST, [this]() { handleSetSafePassword(); });
        server->on("/testSafeApi", HTTP_POST, [this]() { handleTestSafeApi(); });
        
        // Safe teaching mode routes
        server->on("/startSafeTeaching", HTTP_POST, [this]() { handleStartSafeTeaching(); });
        server->on("/cancelSafeTeaching", HTTP_POST, [this]() { handleCancelSafeTeaching(); });
        server->on("/completeSafeTeaching", HTTP_POST, [this]() { handleCompleteSafeTeaching(); });
        server->on("/getTeachingStatus", HTTP_GET, [this]() { handleGetTeachingStatus(); });
        
        // System action routes
        server->on("/restart", HTTP_POST, [this]() { handleRestart(); });
        server->on("/factoryReset", HTTP_POST, [this]() { handleFactoryReset(); });
        
        server->onNotFound([this]() { handleNotFound(); });
        
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
