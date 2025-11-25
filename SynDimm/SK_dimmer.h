/**
 * SK_dimmer.h
 * SmartKraft SynDimm - Dimmer Control System
 * Version: v0.9.1
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Web arayüzü SADECE bilgilendirme içindir!
 * - Tüm dimmer kontrolleri ESP32-C6 üzerinden yapılır
 * - JavaScript dimmer cihazlara ASLA müdahale edemez
 * - Web sadece durum gösterir ve kalibrasyon ayarı yapar
 * - Encoder kontrolü tamamen ESP32-C6'da çalışır
 * ========================================
 * 
 * Supported Protocols:
 * - HTTP REST API (Shelly, Tasmota, etc.)
 * - MQTT (Future)
 * 
 * Features:
 * - Single dimmer connection via manual IP
 * - Network scanning for auto-discovery (non-blocking)
 * - Encoder control (rotate = brightness, button = on/off)
 * - Calibration: Dimmer Ratio 1-5 (sensitivity)
 * - Real-time status display on web
 * ========================================
 */

#ifndef SK_DIMMER_H
#define SK_DIMMER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <ESPmDNS.h>
#include "SK_scan.h"  // Unified network scanner

// Dimmer Settings
#define DIMMER_MIN_BRIGHTNESS 0
#define DIMMER_MAX_BRIGHTNESS 100
#define DIMMER_DEFAULT_RATIO 3
#define DIMMER_REQUEST_TIMEOUT 5000
#define DIMMER_STATUS_UPDATE_INTERVAL 2000

// Anonymous namespace to prevent multiple definition
namespace {
    // Preferences storage
    Preferences dimmerPrefs;
}

// Dimmer Status Enum
enum DimmerStatus {
    DIMMER_IDLE,           // Not connected
    DIMMER_CONNECTING,     // Attempting connection
    DIMMER_CONNECTED,      // Connected and ready
    DIMMER_CONTROLLING,    // Actively controlling
    DIMMER_ERROR,          // Connection or control error
    DIMMER_SCANNING        // Network scanning in progress
};

// Dimmer Device Type
enum DimmerType {
    DIMMER_UNKNOWN = 0,
    
    // Shelly Dimmer Series - Gen1 (REST API)
    DIMMER_SHELLY_DIMMER_1 = 10,      // Shelly Dimmer 1 (Gen1)
    DIMMER_SHELLY_DIMMER_2 = 11,      // Shelly Dimmer 2 (Gen1)
    DIMMER_SHELLY_DIMMER_L = 12,      // Shelly Dimmer L (Gen1)
    
    // Shelly Dimmer Series - Gen2/Gen3 (RPC API)
    DIMMER_SHELLY_DIMMER_2_GEN3 = 20, // Shelly Dimmer 2 Gen3
    DIMMER_SHELLY_DIMMER_0_10V = 21,  // Shelly 0-10V Dimmer Gen3
    DIMMER_SHELLY_DIMMER_1_10V = 22,  // Shelly 1-10V Dimmer Gen3
    
    // Shelly Pro Series
    DIMMER_SHELLY_PRO_4PM = 30,       // Shelly Pro 4PM (with dimming)
    DIMMER_SHELLY_PRO_DIMMER = 31,    // Shelly Pro Dimmer series
    
    // Shelly Plus Series with Dimming
    DIMMER_SHELLY_PLUS_DIMMER = 40,   // Shelly Plus Dimmer series
    DIMMER_SHELLY_PLUS_DIMMER_PM = 41,// Shelly Plus Dimmer PM
    
    // DALI Protocol
    DIMMER_SHELLY_DALI = 50,          // Shelly DALI Dimmer Gateway
    
    // Other brands
    DIMMER_TASMOTA = 100,
    DIMMER_GENERIC = 200
};

// Endpoint configuration for different device types
struct DimmerEndpoints {
    String getStatus;        // Status endpoint
    String setBrightness;    // Set brightness endpoint
    String toggle;           // Toggle on/off endpoint
    String getInfo;          // Device info endpoint
    String getSettings;      // Device settings endpoint
    bool useRPC;             // True for Gen2/Gen3 RPC API, false for Gen1 REST
    int defaultChannel;      // Default light channel (usually 0)
};

// Model information structure
struct ShellyModelInfo {
    String modelName;        // e.g., "SHDM-2", "SNSW-001P16EU"
    String displayName;      // e.g., "Shelly Dimmer 2"
    String firmwareVersion;  // Firmware version
    DimmerType type;
    int generation;          // 1, 2, or 3
    bool supportsDimming;
    int maxChannels;         // Number of light channels
};

// Dimmer Device Structure
struct DimmerDevice {
    String ip;
    String hostname;
    String name;
    String modelName;        // e.g., "SHDM-2"
    String displayName;      // e.g., "Shelly Dimmer 2 Gen3"
    String firmwareVersion;
    String macAddress;
    DimmerType type;
    int generation;          // 1, 2, or 3
    int channel;             // Current channel (for multi-channel devices)
    int brightness;          // 0-100
    bool isOn;
    bool isConnected;
    DimmerStatus status;
    String errorMessage;
    unsigned long lastUpdate;
    DimmerEndpoints endpoints; // Model-specific endpoints
};

// Dimmer Configuration
struct DimmerConfig {
    String savedIP;
    int dimmerRatio;        // 1-5 (how much brightness changes per encoder tick)
    bool autoConnect;
    DimmerType deviceType;
};

// Anonymous namespace to prevent multiple definition
namespace {
    // Global variables
    DimmerDevice dimmerDevice;
    DimmerConfig dimmerConfig;
    unsigned long lastStatusUpdate = 0;
}

// Saved devices list (persistent) structure
struct SavedDimmerDevice {
    String ip;
    String name;
    String modelName;
    String displayName;
    String macAddress;    // Unique identifier
    DimmerType type;
    int channel;
};

// Anonymous namespace to prevent multiple definition
namespace {
    std::vector<SavedDimmerDevice> savedDevices;
    String lastConnectedIP = "";
}

// Forward declarations
bool connectToDimmer(String ip);
DimmerEndpoints getEndpointsForModel(DimmerType type);
DimmerType detectDimmerType(String response);
bool getShellyGen1Status();
bool getShellyGen3Status();
bool getShellyDALIStatus();
bool setShellyGen1Brightness(int brightness);
bool setShellyGen3Brightness(int brightness);
bool setShellyDALIBrightness(int brightness);
bool getShellyStatus();
bool setShellyBrightness(int brightness);
bool toggleShellyPower();
void adjustBrightness(int direction);
bool toggleDimmer();
void setDimmerRatio(int ratio);
void dimmerLoop();
String getDimmerStatusJSON();
void loadSavedDevices();
void saveSavedDevices();
void addDimmerDevice(String ip, String name, DimmerType type);
bool removeDimmerDevice(String ip);
String getSavedDevicesJSON();
void saveLastConnection(String ip);
void autoReconnect();

// Initialize Dimmer System
void initDimmer() {
    dimmerPrefs.begin("dimmer-settings", false);
    
    // Load saved configuration
    dimmerConfig.savedIP = dimmerPrefs.getString("dimmer_ip", "");
    dimmerConfig.dimmerRatio = dimmerPrefs.getInt("dimmer_ratio", DIMMER_DEFAULT_RATIO);
    dimmerConfig.autoConnect = dimmerPrefs.getBool("auto_connect", true);
    dimmerConfig.deviceType = (DimmerType)dimmerPrefs.getInt("device_type", DIMMER_UNKNOWN);
    lastConnectedIP = dimmerPrefs.getString("last_ip", "");
    
    // Initialize device state
    dimmerDevice.ip = dimmerConfig.savedIP;
    dimmerDevice.brightness = 0;
    dimmerDevice.isOn = false;
    dimmerDevice.isConnected = false;
    dimmerDevice.status = DIMMER_IDLE;
    dimmerDevice.type = dimmerConfig.deviceType;
    dimmerDevice.lastUpdate = 0;
    
    // Load saved devices list
    loadSavedDevices();
    
    Serial.println("[DIMMER] System initialized");
    Serial.printf("[DIMMER] Saved IP: %s\n", dimmerConfig.savedIP.c_str());
    Serial.printf("[DIMMER] Last Connected IP: %s\n", lastConnectedIP.c_str());
    Serial.printf("[DIMMER] Dimmer Ratio: %d\n", dimmerConfig.dimmerRatio);
    Serial.printf("[DIMMER] Saved devices: %d\n", savedDevices.size());
    
    // Auto-reconnect to last connected device
    if (dimmerConfig.autoConnect && lastConnectedIP.length() > 0) {
        Serial.println("[DIMMER] Auto-reconnecting to last device...");
        autoReconnect();
    }
}

// ========================================
// MODEL DETECTION & ENDPOINT MAPPING
// ========================================

// Get endpoints configuration for specific model
DimmerEndpoints getEndpointsForModel(DimmerType type) {
    DimmerEndpoints endpoints;
    
    // Gen1 models (REST API)
    if (type >= 10 && type < 20) {
        endpoints.useRPC = false;
        endpoints.getStatus = "/status";
        endpoints.getSettings = "/settings";
        endpoints.getInfo = "/shelly";
        endpoints.toggle = "/light/0?turn=toggle";
        endpoints.defaultChannel = 0;
        
        if (type == DIMMER_SHELLY_DIMMER_L) {
            endpoints.setBrightness = "/light/0?brightness="; // DALI specific
        } else {
            endpoints.setBrightness = "/light/0?brightness=";
        }
    }
    // Gen2/Gen3 models (RPC API)
    else if (type >= 20 && type < 50) {
        endpoints.useRPC = true;
        endpoints.getStatus = "/rpc/Light.GetStatus?id=0";
        endpoints.getSettings = "/rpc/Light.GetConfig?id=0";
        endpoints.getInfo = "/rpc/Shelly.GetDeviceInfo";
        endpoints.setBrightness = "/rpc/Light.Set?id=0&brightness=";
        endpoints.toggle = "/rpc/Light.Toggle?id=0";
        endpoints.defaultChannel = 0;
        
        // Special cases for analog dimmers
        if (type == DIMMER_SHELLY_DIMMER_0_10V || type == DIMMER_SHELLY_DIMMER_1_10V) {
            endpoints.getStatus = "/rpc/Light.GetStatus?id=0";
            endpoints.setBrightness = "/rpc/Light.Set?id=0&brightness=";
        }
    }
    // DALI Protocol (Gen3)
    else if (type == DIMMER_SHELLY_DALI) {
        endpoints.useRPC = true;
        endpoints.getStatus = "/rpc/Light.GetStatus?id=0";
        endpoints.setBrightness = "/rpc/Light.Set?id=0&brightness=";
        endpoints.toggle = "/rpc/Light.Toggle?id=0";
        endpoints.getInfo = "/rpc/Shelly.GetDeviceInfo";
        endpoints.getSettings = "/rpc/Light.GetConfig?id=0";
        endpoints.defaultChannel = 0;
    }
    // Pro Series
    else if (type >= 30 && type < 40) {
        endpoints.useRPC = true;
        endpoints.getStatus = "/rpc/Light.GetStatus?id=0";
        endpoints.setBrightness = "/rpc/Light.Set?id=0&brightness=";
        endpoints.toggle = "/rpc/Light.Toggle?id=0";
        endpoints.getInfo = "/rpc/Shelly.GetDeviceInfo";
        endpoints.getSettings = "/rpc/Light.GetConfig?id=0";
        endpoints.defaultChannel = 0;
    }
    // Plus Series
    else if (type >= 40 && type < 50) {
        endpoints.useRPC = true;
        endpoints.getStatus = "/rpc/Light.GetStatus?id=0";
        endpoints.setBrightness = "/rpc/Light.Set?id=0&brightness=";
        endpoints.toggle = "/rpc/Light.Toggle?id=0";
        endpoints.getInfo = "/rpc/Shelly.GetDeviceInfo";
        endpoints.getSettings = "/rpc/Light.GetConfig?id=0";
        endpoints.defaultChannel = 0;
    }
    
    return endpoints;
}

// ========================================
// LEGACY DETECTION FUNCTIONS
// ========================================

// Detect Dimmer Type from response (kept for compatibility)
DimmerType detectDimmerType(String response) {
    Serial.printf("[DIMMER] Detecting type from response (first 200 chars): %s\n", response.substring(0, 200).c_str());
    
    // Check for Shelly Gen2/Gen3 (RPC)
    if (response.indexOf("\"type\":\"SHDM-") >= 0 || response.indexOf("/rpc/") >= 0) {
        Serial.println("[DIMMER] Detected: Shelly Gen2/Gen3");
        return DIMMER_SHELLY_DIMMER_2_GEN3;
    }
    
    // Check for Shelly Gen1 (REST)
    if (response.indexOf("\"type\":") >= 0 && 
        (response.indexOf("\"lights\"") >= 0 || response.indexOf("\"brightness\"") >= 0)) {
        Serial.println("[DIMMER] Detected: Shelly Gen1");
        return DIMMER_SHELLY_DIMMER_2;
    }
    
    // Check for Tasmota
    if (response.indexOf("Tasmota") >= 0 || response.indexOf("POWER") >= 0) {
        Serial.println("[DIMMER] Detected: Tasmota");
        return DIMMER_TASMOTA;
    }
    
    Serial.println("[DIMMER] Detected: Generic/Unknown");
    return DIMMER_GENERIC;
}

// ========================================
// MODEL-SPECIFIC STATUS FUNCTIONS
// ========================================

// Shelly Gen1 - Get Status (REST API)
bool getShellyGen1Status() {
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.getStatus;
    
    Serial.printf("[DIMMER] Gen1 Status: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(10000);
    http.setConnectTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[DIMMER] Gen1 HTTP Error: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    Serial.printf("[DIMMER] Gen1 Response (300 chars): %s\n", payload.substring(0, 300).c_str());
    
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        Serial.println("[DIMMER] Gen1 parse error");
        return false;
    }
    
    // Check for lights array (Shelly Dimmer 1/2)
    if (doc["lights"].is<JsonArray>()) {
        JsonArray lights = doc["lights"].as<JsonArray>();
        if (lights.size() > dimmerDevice.channel) {
            JsonObject light = lights[dimmerDevice.channel];
            dimmerDevice.isOn = light["ison"].as<bool>();
            dimmerDevice.brightness = light["brightness"].as<int>();
            Serial.printf("[DIMMER] Gen1 - On: %d, Brightness: %d%%\n", dimmerDevice.isOn, dimmerDevice.brightness);
            return true;
        }
    }
    
    // Check for direct brightness field
    if (doc["brightness"].is<int>()) {
        dimmerDevice.brightness = doc["brightness"].as<int>();
        dimmerDevice.isOn = doc["ison"] | (dimmerDevice.brightness > 0);
        Serial.printf("[DIMMER] Gen1 Direct - On: %d, Brightness: %d%%\n", dimmerDevice.isOn, dimmerDevice.brightness);
        return true;
    }
    
    return false;
}

// Shelly Gen3 - Get Status (RPC API)
bool getShellyGen3Status() {
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.getStatus;
    
    Serial.printf("[DIMMER] Gen3 Status: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(10000);
    http.setConnectTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        Serial.printf("[DIMMER] Gen3 HTTP Error: %d\n", httpCode);
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    Serial.printf("[DIMMER] Gen3 Response: %s\n", payload.c_str());
    
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        Serial.println("[DIMMER] Gen3 parse error");
        return false;
    }
    
    dimmerDevice.isOn = doc["output"].as<bool>();
    dimmerDevice.brightness = doc["brightness"].as<int>();
    
    Serial.printf("[DIMMER] Gen3 - On: %d, Brightness: %d%%\n", dimmerDevice.isOn, dimmerDevice.brightness);
    return true;
}

// Shelly DALI - Get Status
bool getShellyDALIStatus() {
    // DALI uses same RPC as Gen3 but may have additional fields
    return getShellyGen3Status();
}

// Main Shelly Status Function (dispatches to correct model)
bool getShellyStatus() {
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 20) {
        // Gen1 models
        return getShellyGen1Status();
    } else if (dimmerDevice.type >= 20 && dimmerDevice.type < 100) {
        // Gen2/Gen3/Pro/Plus/DALI models
        return getShellyGen3Status();
    }
    
    Serial.println("[DIMMER] Unknown Shelly model type");
    return false;
}

// ========================================
// MODEL-SPECIFIC BRIGHTNESS FUNCTIONS
// ========================================

// Shelly Gen1 - Set Brightness (REST API)
bool setShellyGen1Brightness(int brightness) {
    if (brightness < DIMMER_MIN_BRIGHTNESS) brightness = DIMMER_MIN_BRIGHTNESS;
    if (brightness > DIMMER_MAX_BRIGHTNESS) brightness = DIMMER_MAX_BRIGHTNESS;
    
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.setBrightness + String(brightness);
    
    // Add turn parameter
    if (brightness > 0) {
        url += "&turn=on";
    } else {
        url += "&turn=off";
    }
    
    Serial.printf("[DIMMER] Gen1 Set Brightness: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(10000);
    http.setConnectTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("[DIMMER] Gen1 Response: %s\n", response.c_str());
        http.end();
        
        dimmerDevice.brightness = brightness;
        dimmerDevice.isOn = (brightness > 0);
        Serial.printf("[DIMMER] Gen1 brightness set to: %d%%\n", brightness);
        return true;
    }
    
    Serial.printf("[DIMMER] Gen1 Set Error: %d\n", httpCode);
    http.end();
    return false;
}

// Shelly Gen3 - Set Brightness (RPC API)
bool setShellyGen3Brightness(int brightness) {
    if (brightness < DIMMER_MIN_BRIGHTNESS) brightness = DIMMER_MIN_BRIGHTNESS;
    if (brightness > DIMMER_MAX_BRIGHTNESS) brightness = DIMMER_MAX_BRIGHTNESS;
    
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.setBrightness + String(brightness);
    
    // Turn on if brightness > 0
    if (brightness > 0 && !dimmerDevice.isOn) {
        url += "&on=true";
    } else if (brightness == 0) {
        url += "&on=false";
    }
    
    Serial.printf("[DIMMER] Gen3 Set Brightness: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(10000);
    http.setConnectTimeout(5000);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("[DIMMER] Gen3 Response: %s\n", response.c_str());
        http.end();
        
        dimmerDevice.brightness = brightness;
        dimmerDevice.isOn = (brightness > 0);
        Serial.printf("[DIMMER] Gen3 brightness set to: %d%%\n", brightness);
        return true;
    }
    
    Serial.printf("[DIMMER] Gen3 Set Error: %d\n", httpCode);
    http.end();
    return false;
}

// Shelly DALI - Set Brightness
bool setShellyDALIBrightness(int brightness) {
    // DALI uses same RPC as Gen3
    return setShellyGen3Brightness(brightness);
}

// Main Shelly Brightness Function (dispatches to correct model)
bool setShellyBrightness(int brightness) {
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 20) {
        // Gen1 models
        return setShellyGen1Brightness(brightness);
    } else if (dimmerDevice.type >= 20 && dimmerDevice.type < 100) {
        // Gen2/Gen3/Pro/Plus/DALI models
        return setShellyGen3Brightness(brightness);
    }
    
    Serial.println("[DIMMER] Unknown Shelly model type");
    return false;
}

// Shelly Dimmer - Set Brightness (OLD - keeping for compatibility)
bool setShellyBrightnessOld(int brightness) {
    if (brightness < DIMMER_MIN_BRIGHTNESS) brightness = DIMMER_MIN_BRIGHTNESS;
    if (brightness > DIMMER_MAX_BRIGHTNESS) brightness = DIMMER_MAX_BRIGHTNESS;
    
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + "/rpc/Light.Set?id=0&brightness=" + String(brightness);
    
    // Turn on if brightness > 0
    if (brightness > 0 && !dimmerDevice.isOn) {
        url += "&on=true";
    } else if (brightness == 0) {
        url += "&on=false";
    }
    
    Serial.printf("[DIMMER] Setting brightness: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(10000);
    http.setConnectTimeout(5000);
    
    int httpCode = http.GET();
    
    Serial.printf("[DIMMER] Set brightness HTTP code: %d\n", httpCode);
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("[DIMMER] Response: %s\n", response.c_str());
        http.end();
        
        dimmerDevice.brightness = brightness;
        dimmerDevice.isOn = (brightness > 0);
        Serial.printf("[DIMMER] Shelly brightness set to: %d%%\n", brightness);
        return true;
    } else {
        if (httpCode > 0) {
            Serial.printf("[DIMMER] HTTP Error: %d\n", httpCode);
        } else {
            Serial.printf("[DIMMER] Connection Error: %s\n", http.errorToString(httpCode).c_str());
        }
        http.end();
    }
    
    return false;
}

// Shelly Gen1 - Toggle On/Off (REST API)
bool toggleShellyGen1() {
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.toggle;
    
    Serial.printf("[DIMMER] Gen1 Toggle: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(DIMMER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("[DIMMER] Gen1 Toggle Response: %s\n", response.c_str());
        http.end();
        
        dimmerDevice.isOn = !dimmerDevice.isOn;
        Serial.printf("[DIMMER] Gen1 toggled: %s\n", dimmerDevice.isOn ? "ON" : "OFF");
        
        // Update status to get actual state
        delay(100);
        getShellyStatus();
        return true;
    }
    
    Serial.printf("[DIMMER] Gen1 Toggle Error: %d\n", httpCode);
    http.end();
    return false;
}

// Shelly Gen3 - Toggle On/Off (RPC API)
bool toggleShellyGen3() {
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.toggle;
    
    Serial.printf("[DIMMER] Gen3 Toggle: %s\n", url.c_str());
    
    http.begin(url);
    http.setTimeout(DIMMER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        Serial.printf("[DIMMER] Gen3 Toggle Response: %s\n", response.c_str());
        http.end();
        
        dimmerDevice.isOn = !dimmerDevice.isOn;
        Serial.printf("[DIMMER] Gen3 toggled: %s\n", dimmerDevice.isOn ? "ON" : "OFF");
        
        // Update status to get actual state
        delay(100);
        getShellyStatus();
        return true;
    }
    
    Serial.printf("[DIMMER] Gen3 Toggle Error: %d\n", httpCode);
    http.end();
    return false;
}

// Main Shelly Toggle Function (dispatches to correct model)
bool toggleShelly() {
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 20) {
        // Gen1 models
        return toggleShellyGen1();
    } else if (dimmerDevice.type >= 20 && dimmerDevice.type < 100) {
        // Gen2/Gen3/Pro/Plus/DALI models
        return toggleShellyGen3();
    }
    
    Serial.println("[DIMMER] Unknown Shelly model type for toggle");
    return false;
}

// Generic Dimmer Status Update
bool updateDimmerStatus() {
    // Allow status update during connection phase
    if (dimmerDevice.status != DIMMER_CONNECTING && !dimmerDevice.isConnected) {
        Serial.println("[DIMMER] updateDimmerStatus: Not connected");
        return false;
    }
    
    bool success = false;
    
    // Route to appropriate status function based on device type
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 100) {
        // All Shelly models
        success = getShellyStatus();
    } else if (dimmerDevice.type == DIMMER_TASMOTA) {
        // TODO: Implement Tasmota protocol
        Serial.println("[DIMMER] Tasmota not implemented yet");
        success = false;
    } else if (dimmerDevice.type == DIMMER_GENERIC) {
        // For generic devices, assume success if we got this far
        Serial.println("[DIMMER] Generic device - assuming basic connectivity");
        dimmerDevice.brightness = 0;
        dimmerDevice.isOn = false;
        dimmerDevice.name = "Generic Dimmer";
        success = true;
    } else {
        Serial.println("[DIMMER] Unknown device type");
        success = false;
    }
    
    if (success) {
        dimmerDevice.lastUpdate = millis();
        dimmerDevice.status = DIMMER_CONNECTED;
        dimmerDevice.errorMessage = "";
        Serial.println("[DIMMER] Status update successful");
    } else {
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Failed to get status";
        Serial.println("[DIMMER] Status update failed");
    }
    
    return success;
}

// Connect to Dimmer Device
bool connectToDimmer(String ip) {
    Serial.printf("[DIMMER] ========================================\n");
    Serial.printf("[DIMMER] Connecting to: %s\n", ip.c_str());
    Serial.printf("[DIMMER] ========================================\n");
    
    dimmerDevice.ip = ip;
    dimmerDevice.status = DIMMER_CONNECTING;
    dimmerDevice.errorMessage = "";
    dimmerDevice.channel = 0; // Default channel
    
    // Step 1: Detect device using unified scanner
    DiscoveredDevice device = detectDevice(ip);
    
    if (device.category == CATEGORY_DIMMER) {
        Serial.printf("[DIMMER] ✓ Detected: %s\n", device.displayName.c_str());
        Serial.printf("[DIMMER]   Model: %s\n", device.modelName.c_str());
        
        // Map DiscoveredDevice to DimmerType
        DimmerType detectedType = DIMMER_UNKNOWN;
        if (device.modelName == "SNSW-001P16EU") detectedType = DIMMER_SHELLY_DIMMER_2_GEN3;
        else if (device.modelName.indexOf("0-10V") >= 0 || device.modelName.indexOf("SNDM-0013") >= 0) detectedType = DIMMER_SHELLY_DIMMER_0_10V;
        else if (device.modelName.indexOf("1-10V") >= 0) detectedType = DIMMER_SHELLY_DIMMER_1_10V;
        else if (device.modelName.indexOf("Plus") >= 0 && device.modelName.indexOf("Dimmer") >= 0) detectedType = DIMMER_SHELLY_PLUS_DIMMER;
        else if (device.modelName.indexOf("Pro") >= 0 && device.modelName.indexOf("Dimmer") >= 0) detectedType = DIMMER_SHELLY_PRO_DIMMER;
        else if (device.modelName.indexOf("DALI") >= 0) detectedType = DIMMER_SHELLY_DALI;
        else if (device.modelName == "SHDM-2" || device.modelName.indexOf("Dimmer2") >= 0) detectedType = DIMMER_SHELLY_DIMMER_2;
        else if (device.modelName == "SHDM-1" || device.modelName.indexOf("Dimmer1") >= 0) detectedType = DIMMER_SHELLY_DIMMER_1;
        else if (device.modelName == "SHDM-L" || device.modelName.indexOf("DimmerL") >= 0) detectedType = DIMMER_SHELLY_DIMMER_L;
        else detectedType = DIMMER_SHELLY_DIMMER_2_GEN3; // Default fallback
        
        // Set device info
        dimmerDevice.type = detectedType;
        dimmerDevice.modelName = device.modelName;
        dimmerDevice.displayName = device.displayName;
        dimmerDevice.generation = (detectedType >= 20) ? 3 : 1; // Gen3 if type >= 20
        dimmerDevice.firmwareVersion = "";
        dimmerDevice.name = device.displayName;
        
        // Get model-specific endpoints
        dimmerDevice.endpoints = getEndpointsForModel(detectedType);
        
        Serial.printf("[DIMMER]   Using endpoints:\n");
        Serial.printf("[DIMMER]     Status: %s\n", dimmerDevice.endpoints.getStatus.c_str());
        Serial.printf("[DIMMER]     Brightness: %s\n", dimmerDevice.endpoints.setBrightness.c_str());
        Serial.printf("[DIMMER]     RPC Mode: %s\n", dimmerDevice.endpoints.useRPC ? "Yes" : "No");
        
        // Step 2: Get initial status
        Serial.println("[DIMMER] Getting initial device status...");
        
        if (updateDimmerStatus()) {
            dimmerDevice.isConnected = true;
            dimmerDevice.status = DIMMER_CONNECTED;
            
            // Save configuration
            dimmerConfig.savedIP = ip;
            dimmerConfig.deviceType = dimmerDevice.type;
            dimmerPrefs.putString("dimmer_ip", ip);
            dimmerPrefs.putInt("device_type", (int)dimmerDevice.type);
            
            // Save last connection for auto-reconnect
            saveLastConnection(ip);
            
            // Add to saved devices list automatically
            addDimmerDevice(ip, device.displayName, dimmerDevice.type);
            
            Serial.printf("[DIMMER] ========================================\n");
            Serial.printf("[DIMMER] ✓ CONNECTION SUCCESSFUL!\n");
            Serial.printf("[DIMMER]   Device: %s\n", dimmerDevice.displayName.c_str());
            Serial.printf("[DIMMER]   Status: %s\n", dimmerDevice.isOn ? "ON" : "OFF");
            Serial.printf("[DIMMER]   Brightness: %d%%\n", dimmerDevice.brightness);
            Serial.printf("[DIMMER] ========================================\n");
            
            return true;
        } else {
            Serial.println("[DIMMER] ✗ Failed to get initial status");
            dimmerDevice.isConnected = false;
            dimmerDevice.status = DIMMER_ERROR;
            dimmerDevice.errorMessage = "Failed to get device status";
        }
    } else {
        Serial.println("[DIMMER] ✗ Device not recognized or doesn't support dimming");
        dimmerDevice.isConnected = false;
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Unsupported device type";
    }
    
    Serial.printf("[DIMMER] ========================================\n");
    Serial.printf("[DIMMER] ✗ CONNECTION FAILED\n");
    Serial.printf("[DIMMER]   Error: %s\n", dimmerDevice.errorMessage.c_str());
    Serial.printf("[DIMMER] ========================================\n");
    
    return false;
}

// Disconnect from Dimmer
void disconnectDimmer() {
    dimmerDevice.isConnected = false;
    dimmerDevice.status = DIMMER_IDLE;
    dimmerDevice.ip = "";
    dimmerDevice.brightness = 0;
    dimmerDevice.isOn = false;
    
    Serial.println("[DIMMER] Disconnected");
}

// Set Brightness (called by encoder)
bool setDimmerBrightness(int brightness) {
    if (!dimmerDevice.isConnected) {
        Serial.println("[DIMMER] Not connected!");
        return false;
    }
    
    dimmerDevice.status = DIMMER_CONTROLLING;
    
    bool success = false;
    
    // Route to appropriate brightness function based on device type
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 100) {
        // All Shelly models
        success = setShellyBrightness(brightness);
    } else if (dimmerDevice.type == DIMMER_TASMOTA) {
        // TODO: Implement Tasmota
        Serial.println("[DIMMER] Tasmota not implemented yet");
    } else if (dimmerDevice.type == DIMMER_GENERIC) {
        // TODO: Implement generic
        Serial.println("[DIMMER] Generic not implemented yet");
    } else {
        Serial.println("[DIMMER] Unknown device type");
    }
    
    if (success) {
        dimmerDevice.status = DIMMER_CONNECTED;
    } else {
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Failed to set brightness";
    }
    
    return success;
}

// Adjust Brightness by Ratio (called by encoder rotation)
void adjustBrightness(int direction) {
    if (!dimmerDevice.isConnected) {
        return;
    }
    
    int change = dimmerConfig.dimmerRatio * direction;
    int newBrightness = dimmerDevice.brightness + change;
    
    if (newBrightness < DIMMER_MIN_BRIGHTNESS) newBrightness = DIMMER_MIN_BRIGHTNESS;
    if (newBrightness > DIMMER_MAX_BRIGHTNESS) newBrightness = DIMMER_MAX_BRIGHTNESS;
    
    Serial.printf("[DIMMER] Adjusting brightness: %d -> %d (ratio: %d)\n", 
                  dimmerDevice.brightness, newBrightness, dimmerConfig.dimmerRatio);
    
    setDimmerBrightness(newBrightness);
}

// Toggle Dimmer On/Off (called by encoder button)
bool toggleDimmer() {
    if (!dimmerDevice.isConnected) {
        Serial.println("[DIMMER] toggleDimmer: Not connected!");
        return false;
    }
    
    Serial.println("[DIMMER] ========================================");
    Serial.println("[DIMMER] Toggle button pressed!");
    Serial.printf("[DIMMER] Current state - On: %d, Brightness: %d%%\n", dimmerDevice.isOn, dimmerDevice.brightness);
    Serial.println("[DIMMER] ========================================");
    
    dimmerDevice.status = DIMMER_CONTROLLING;
    
    bool success = false;
    
    // Route to appropriate toggle function based on device type
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 100) {
        // All Shelly models
        Serial.println("[DIMMER] Calling toggleShelly()...");
        success = toggleShelly();
    } else if (dimmerDevice.type == DIMMER_TASMOTA) {
        // TODO: Implement Tasmota
        Serial.println("[DIMMER] Tasmota toggle not implemented yet");
    } else if (dimmerDevice.type == DIMMER_GENERIC) {
        // TODO: Implement generic
        Serial.println("[DIMMER] Generic toggle not implemented yet");
    } else {
        Serial.println("[DIMMER] Unknown device type");
    }
    
    if (success) {
        dimmerDevice.status = DIMMER_CONNECTED;
        Serial.println("[DIMMER] ========================================");
        Serial.printf("[DIMMER] Toggle SUCCESS! New state: %s\n", dimmerDevice.isOn ? "ON" : "OFF");
        Serial.println("[DIMMER] ========================================");
    } else {
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Failed to toggle dimmer";
        Serial.println("[DIMMER] ========================================");
        Serial.println("[DIMMER] Toggle FAILED!");
        Serial.println("[DIMMER] ========================================");
    }
    
    return success;
}

// Set Dimmer Ratio (1-5)
void setDimmerRatio(int ratio) {
    if (ratio < 1) ratio = 1;
    if (ratio > 5) ratio = 5;
    
    dimmerConfig.dimmerRatio = ratio;
    dimmerPrefs.putInt("dimmer_ratio", ratio);
    
    Serial.printf("[DIMMER] Ratio set to: %d\n", ratio);
}

// Dimmer Loop (call in main loop)
void dimmerLoop() {
    // Auto-update status every 2 seconds
    if (dimmerDevice.isConnected && (millis() - lastStatusUpdate > DIMMER_STATUS_UPDATE_INTERVAL)) {
        updateDimmerStatus();
        lastStatusUpdate = millis();
    }
}

// Get Dimmer Status as JSON
String getDimmerStatusJSON() {
    JsonDocument doc;
    
    doc["connected"] = dimmerDevice.isConnected;
    doc["status"] = (int)dimmerDevice.status;
    doc["ip"] = dimmerDevice.ip;
    doc["hostname"] = dimmerDevice.hostname;
    doc["name"] = dimmerDevice.name;
    doc["type"] = (int)dimmerDevice.type;
    doc["brightness"] = dimmerDevice.brightness;
    doc["isOn"] = dimmerDevice.isOn;
    doc["ratio"] = dimmerConfig.dimmerRatio;
    doc["errorMessage"] = dimmerDevice.errorMessage;
    doc["lastUpdate"] = dimmerDevice.lastUpdate;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// ========================================
// NETWORK SCANNING
// Now using unified SK_scan.h module
// ========================================

// ========================================
// SAVED DEVICES MANAGEMENT
// ========================================

// Load Saved Devices from Preferences
void loadSavedDevices() {
    String devicesJSON = dimmerPrefs.getString("saved_devices", "[]");
    
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, devicesJSON);
    
    if (error) {
        Serial.println("[DIMMER] Failed to parse saved devices");
        return;
    }
    
    savedDevices.clear();
    JsonArray arr = doc.as<JsonArray>();
    
    for (JsonObject obj : arr) {
        SavedDimmerDevice device;
        device.ip = obj["ip"].as<String>();
        device.name = obj["name"].as<String>();
        device.type = (DimmerType)obj["type"].as<int>();
        savedDevices.push_back(device);
    }
    
    Serial.printf("[DIMMER] Loaded %d saved devices\n", savedDevices.size());
}

// Save Saved Devices to Preferences
void saveSavedDevices() {
    JsonDocument doc;
    JsonArray arr = doc.to<JsonArray>();
    
    for (const auto& device : savedDevices) {
        JsonObject obj = arr.add<JsonObject>();
        obj["ip"] = device.ip;
        obj["name"] = device.name;
        obj["type"] = (int)device.type;
    }
    
    String output;
    serializeJson(doc, output);
    
    dimmerPrefs.putString("saved_devices", output);
    Serial.printf("[DIMMER] Saved %d devices to preferences\n", savedDevices.size());
}

// Add Dimmer Device to Saved List
void addDimmerDevice(String ip, String name, DimmerType type) {
    // Check if already exists
    for (auto& device : savedDevices) {
        if (device.ip == ip) {
            // Update existing device info
            device.name = name;
            device.type = type;
            device.modelName = dimmerDevice.modelName;
            device.displayName = dimmerDevice.displayName;
            device.macAddress = dimmerDevice.macAddress;
            device.channel = dimmerDevice.channel;
            saveSavedDevices();
            Serial.printf("[DIMMER] Updated device: %s (%s)\n", ip.c_str(), name.c_str());
            return;
        }
    }
    
    SavedDimmerDevice device;
    device.ip = ip;
    device.name = name;
    device.type = type;
    device.modelName = dimmerDevice.modelName;
    device.displayName = dimmerDevice.displayName;
    device.macAddress = dimmerDevice.macAddress;
    device.channel = dimmerDevice.channel;
    
    savedDevices.push_back(device);
    saveSavedDevices();
    
    Serial.printf("[DIMMER] Added device: %s - %s (%s)\n", ip.c_str(), device.displayName.c_str(), device.modelName.c_str());
}

// Remove Dimmer Device from Saved List
bool removeDimmerDevice(String ip) {
    for (auto it = savedDevices.begin(); it != savedDevices.end(); ++it) {
        if (it->ip == ip) {
            savedDevices.erase(it);
            saveSavedDevices();
            Serial.printf("[DIMMER] Removed device: %s\n", ip.c_str());
            return true;
        }
    }
    return false;
}

// Get Saved Devices as JSON
String getSavedDevicesJSON() {
    JsonDocument doc;
    JsonArray devices = doc["devices"].to<JsonArray>();
    
    for (const auto& device : savedDevices) {
        JsonObject obj = devices.add<JsonObject>();
        obj["ip"] = device.ip;
        obj["name"] = device.name;
        obj["type"] = (int)device.type;
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

// Save Last Connection
void saveLastConnection(String ip) {
    lastConnectedIP = ip;
    dimmerPrefs.putString("last_ip", ip);
    Serial.printf("[DIMMER] Saved last connection: %s\n", ip.c_str());
}

// Auto Reconnect to Last Device
void autoReconnect() {
    if (lastConnectedIP.length() == 0) {
        Serial.println("[DIMMER] No last connection to reconnect");
        return;
    }
    
    Serial.printf("[DIMMER] Attempting auto-reconnect to %s\n", lastConnectedIP.c_str());
    
    if (connectToDimmer(lastConnectedIP)) {
        Serial.println("[DIMMER] Auto-reconnect successful");
    } else {
        Serial.println("[DIMMER] Auto-reconnect failed");
    }
}

#endif // SK_DIMMER_H
