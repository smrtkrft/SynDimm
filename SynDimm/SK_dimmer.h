/**
 * SK_dimmer.h
 * SmartKraft SynDimm - Dimmer Control System
 * Version: v1.2.0
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
#include "SK_config.h"
#include "SK_scan.h"  // Unified network scanner

// Dimmer Settings
#define DIMMER_MIN_BRIGHTNESS 0
#define DIMMER_MAX_BRIGHTNESS 100
#define DIMMER_DEFAULT_RATIO 3
#define DIMMER_REQUEST_TIMEOUT 3000
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
    
    // Shelly Gen4 Series (RPC API - same as Gen3)
    DIMMER_SHELLY_DIMMER_GEN4 = 60,   // Shelly Dimmer Gen4
    DIMMER_SHELLY_PLUS_1_GEN4 = 61,   // Shelly Plus 1 Gen4 (with dimming)
    DIMMER_SHELLY_PM_MINI_GEN4 = 62,  // Shelly PM Mini Gen4
    DIMMER_SHELLY_1_MINI_GEN4 = 63,   // Shelly 1 Mini Gen4
    DIMMER_SHELLY_DIMMER_0_10V_GEN4 = 64, // Shelly 0-10V Dimmer Gen4
    
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
void disconnectDimmer();
void clearLastConnection();

// Initialize Dimmer System
void initDimmer() {
    dimmerPrefs.begin("dimmer-settings", false);
    
    dimmerConfig.savedIP = dimmerPrefs.getString("dimmer_ip", "");
    dimmerConfig.dimmerRatio = dimmerPrefs.getInt("dimmer_ratio", DIMMER_DEFAULT_RATIO);
    dimmerConfig.autoConnect = dimmerPrefs.getBool("auto_connect", true);
    dimmerConfig.deviceType = (DimmerType)dimmerPrefs.getInt("device_type", DIMMER_UNKNOWN);
    lastConnectedIP = dimmerPrefs.getString("last_ip", "");
    
    dimmerDevice.ip = dimmerConfig.savedIP;
    dimmerDevice.brightness = 0;
    dimmerDevice.isOn = false;
    dimmerDevice.isConnected = false;
    dimmerDevice.status = DIMMER_IDLE;
    dimmerDevice.type = dimmerConfig.deviceType;
    dimmerDevice.lastUpdate = 0;
    
    loadSavedDevices();
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
    // Gen4 Series (same RPC API as Gen3)
    else if (type >= 60 && type < 70) {
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
    DEBUG_PRINTF("[DIMMER] Detecting type from response (first 200 chars): %s\n", response.substring(0, 200).c_str());
    
    // Check for Shelly Gen2/Gen3 (RPC)
    if (response.indexOf("\"type\":\"SHDM-") >= 0 || response.indexOf("/rpc/") >= 0) {
        DEBUG_PRINTLN("[DIMMER] Detected: Shelly Gen2/Gen3");
        return DIMMER_SHELLY_DIMMER_2_GEN3;
    }
    
    // Check for Shelly Gen1 (REST)
    if (response.indexOf("\"type\":") >= 0 && 
        (response.indexOf("\"lights\"") >= 0 || response.indexOf("\"brightness\"") >= 0)) {
        DEBUG_PRINTLN("[DIMMER] Detected: Shelly Gen1");
        return DIMMER_SHELLY_DIMMER_2;
    }
    
    // Check for Tasmota
    if (response.indexOf("Tasmota") >= 0 || response.indexOf("POWER") >= 0) {
        DEBUG_PRINTLN("[DIMMER] Detected: Tasmota");
        return DIMMER_TASMOTA;
    }
    
    DEBUG_PRINTLN("[DIMMER] Detected: Generic/Unknown");
    return DIMMER_GENERIC;
}

// ========================================
// MODEL-SPECIFIC STATUS FUNCTIONS
// ========================================

// Shelly Gen1 - Get Status (REST API)
bool getShellyGen1Status() {
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.getStatus;
    
    http.begin(url);
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        return false;
    }
    
    if (doc["lights"].is<JsonArray>()) {
        JsonArray lights = doc["lights"].as<JsonArray>();
        if (lights.size() > dimmerDevice.channel) {
            JsonObject light = lights[dimmerDevice.channel];
            dimmerDevice.isOn = light["ison"].as<bool>();
            dimmerDevice.brightness = light["brightness"].as<int>();
            return true;
        }
    }
    
    if (doc["brightness"].is<int>()) {
        dimmerDevice.brightness = doc["brightness"].as<int>();
        dimmerDevice.isOn = doc["ison"] | (dimmerDevice.brightness > 0);
        return true;
    }
    
    return false;
}

// Shelly Gen3 - Get Status (RPC API)
bool getShellyGen3Status() {
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.getStatus;
    
    http.begin(url);
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    
    int httpCode = http.GET();
    
    if (httpCode != HTTP_CODE_OK) {
        http.end();
        return false;
    }
    
    String payload = http.getString();
    http.end();
    
    JsonDocument doc;
    if (deserializeJson(doc, payload) != DeserializationError::Ok) {
        return false;
    }
    
    dimmerDevice.isOn = doc["output"].as<bool>();
    dimmerDevice.brightness = doc["brightness"].as<int>();
    
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
        return getShellyGen1Status();
    } else if (dimmerDevice.type >= 20 && dimmerDevice.type < 100) {
        return getShellyGen3Status();
    }
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
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.setBrightness + String(brightness);
    
    if (brightness > 0) {
        url += "&turn=on";
    } else {
        url += "&turn=off";
    }
    
    http.begin(url);
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        dimmerDevice.brightness = brightness;
        dimmerDevice.isOn = (brightness > 0);
        return true;
    }
    
    return false;
}

// Shelly Gen3 - Set Brightness (RPC API)
bool setShellyGen3Brightness(int brightness) {
    if (brightness < DIMMER_MIN_BRIGHTNESS) brightness = DIMMER_MIN_BRIGHTNESS;
    if (brightness > DIMMER_MAX_BRIGHTNESS) brightness = DIMMER_MAX_BRIGHTNESS;
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.setBrightness + String(brightness);
    
    if (brightness > 0 && !dimmerDevice.isOn) {
        url += "&on=true";
    } else if (brightness == 0) {
        url += "&on=false";
    }
    
    http.begin(url);
    http.setTimeout(5000);
    http.setConnectTimeout(3000);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        dimmerDevice.brightness = brightness;
        dimmerDevice.isOn = (brightness > 0);
        return true;
    }
    
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
        return setShellyGen1Brightness(brightness);
    } else if (dimmerDevice.type >= 20 && dimmerDevice.type < 100) {
        return setShellyGen3Brightness(brightness);
    }
    return false;
}

// Shelly Gen1 - Toggle On/Off (REST API)
bool toggleShellyGen1() {
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.toggle;
    
    http.begin(url);
    http.setTimeout(DIMMER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        dimmerDevice.isOn = !dimmerDevice.isOn;
        delay(100);
        getShellyStatus();
        return true;
    }
    return false;
}

bool toggleShellyGen3() {
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + dimmerDevice.ip + dimmerDevice.endpoints.toggle;
    
    http.begin(url);
    http.setTimeout(DIMMER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        dimmerDevice.isOn = !dimmerDevice.isOn;
        delay(100);
        getShellyStatus();
        return true;
    }
    return false;
}

bool toggleShelly() {
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 20) {
        return toggleShellyGen1();
    } else if (dimmerDevice.type >= 20 && dimmerDevice.type < 100) {
        return toggleShellyGen3();
    }
    return false;
}

// Generic Dimmer Status Update
bool updateDimmerStatus() {
    if (dimmerDevice.status != DIMMER_CONNECTING && !dimmerDevice.isConnected) {
        return false;
    }
    
    bool success = false;
    
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 100) {
        success = getShellyStatus();
    } else if (dimmerDevice.type == DIMMER_TASMOTA) {
        success = false;
    } else if (dimmerDevice.type == DIMMER_GENERIC) {
        dimmerDevice.brightness = 0;
        dimmerDevice.isOn = false;
        dimmerDevice.name = "Generic Dimmer";
        success = true;
    } else {
        success = false;
    }
    
    if (success) {
        dimmerDevice.lastUpdate = millis();
        dimmerDevice.status = DIMMER_CONNECTED;
        dimmerDevice.errorMessage = "";
    } else {
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Failed to get status";
    }
    
    return success;
}

// Connect to Dimmer Device
bool connectToDimmer(String ip) {
    dimmerDevice.ip = ip;
    dimmerDevice.status = DIMMER_CONNECTING;
    dimmerDevice.errorMessage = "";
    dimmerDevice.channel = 0;
    
    DiscoveredDevice device = detectDevice(ip);
    
    if (device.category == CATEGORY_DIMMER) {
        DimmerType detectedType = DIMMER_UNKNOWN;
        
        // Gen4 detection (check for g4 suffix or Gen4 in name)
        bool isGen4 = device.modelName.indexOf("g4") >= 0 || 
                      device.modelName.indexOf("G4") >= 0 || 
                      device.modelName.indexOf("Gen4") >= 0 ||
                      device.displayName.indexOf("Gen4") >= 0;
        
        if (isGen4) {
            // Gen4 devices
            if (device.modelName.indexOf("dimmer") >= 0 || device.modelName.indexOf("Dimmer") >= 0) {
                if (device.modelName.indexOf("0-10") >= 0) detectedType = DIMMER_SHELLY_DIMMER_0_10V_GEN4;
                else detectedType = DIMMER_SHELLY_DIMMER_GEN4;
            }
            else if (device.modelName.indexOf("plus1") >= 0 || device.modelName.indexOf("Plus 1") >= 0) detectedType = DIMMER_SHELLY_PLUS_1_GEN4;
            else if (device.modelName.indexOf("pmminig4") >= 0 || device.modelName.indexOf("PM Mini") >= 0) detectedType = DIMMER_SHELLY_PM_MINI_GEN4;
            else if (device.modelName.indexOf("1minig4") >= 0 || device.modelName.indexOf("1 Mini") >= 0) detectedType = DIMMER_SHELLY_1_MINI_GEN4;
            else detectedType = DIMMER_SHELLY_DIMMER_GEN4; // Default Gen4
        }
        else if (device.modelName == "SNSW-001P16EU") detectedType = DIMMER_SHELLY_DIMMER_2_GEN3;
        else if (device.modelName.indexOf("0-10V") >= 0 || device.modelName.indexOf("SNDM-0013") >= 0) detectedType = DIMMER_SHELLY_DIMMER_0_10V;
        else if (device.modelName.indexOf("1-10V") >= 0) detectedType = DIMMER_SHELLY_DIMMER_1_10V;
        else if (device.modelName.indexOf("Plus") >= 0 && device.modelName.indexOf("Dimmer") >= 0) detectedType = DIMMER_SHELLY_PLUS_DIMMER;
        else if (device.modelName.indexOf("Pro") >= 0 && device.modelName.indexOf("Dimmer") >= 0) detectedType = DIMMER_SHELLY_PRO_DIMMER;
        else if (device.modelName.indexOf("DALI") >= 0) detectedType = DIMMER_SHELLY_DALI;
        else if (device.modelName == "SHDM-2" || device.modelName.indexOf("Dimmer2") >= 0) detectedType = DIMMER_SHELLY_DIMMER_2;
        else if (device.modelName == "SHDM-1" || device.modelName.indexOf("Dimmer1") >= 0) detectedType = DIMMER_SHELLY_DIMMER_1;
        else if (device.modelName == "SHDM-L" || device.modelName.indexOf("DimmerL") >= 0) detectedType = DIMMER_SHELLY_DIMMER_L;
        else detectedType = DIMMER_SHELLY_DIMMER_2_GEN3;
        
        dimmerDevice.type = detectedType;
        dimmerDevice.modelName = device.modelName;
        dimmerDevice.displayName = device.displayName;
        // Generation detection: 60+ = Gen4, 20-59 = Gen3/Gen2, 10-19 = Gen1
        dimmerDevice.generation = (detectedType >= 60) ? 4 : (detectedType >= 20) ? 3 : 1;
        dimmerDevice.firmwareVersion = "";
        dimmerDevice.name = device.displayName;
        dimmerDevice.endpoints = getEndpointsForModel(detectedType);
        
        if (updateDimmerStatus()) {
            dimmerDevice.isConnected = true;
            dimmerDevice.status = DIMMER_CONNECTED;
            
            dimmerConfig.savedIP = ip;
            dimmerConfig.deviceType = dimmerDevice.type;
            dimmerPrefs.putString("dimmer_ip", ip);
            dimmerPrefs.putInt("device_type", (int)dimmerDevice.type);
            
            saveLastConnection(ip);
            addDimmerDevice(ip, device.displayName, dimmerDevice.type);
            
            return true;
        } else {
            dimmerDevice.isConnected = false;
            dimmerDevice.status = DIMMER_ERROR;
            dimmerDevice.errorMessage = "Failed to get device status";
        }
    } else {
        dimmerDevice.isConnected = false;
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Unsupported device type";
    }
    
    return false;
}

// Disconnect from Dimmer (keeps lastConnectedIP for auto-reconnect)
void disconnectDimmer() {
    dimmerDevice.isConnected = false;
    dimmerDevice.status = DIMMER_IDLE;
    // NOT: IP ve lastConnectedIP silinmiyor - otomatik yeniden bağlanma için korunuyor
    dimmerDevice.brightness = 0;
    dimmerDevice.isOn = false;
    DEBUG_PRINTF("[DIMMER] Disconnected. Last IP preserved: %s\n", lastConnectedIP.c_str());
}

// Clear last connection (only when user explicitly wants to forget device)
void clearLastConnection() {
    lastConnectedIP = "";
    dimmerDevice.ip = "";
    dimmerPrefs.putString("last_ip", "");
    dimmerPrefs.putString("dimmer_ip", "");
    DEBUG_PRINTLN("[DIMMER] Last connection cleared by user");
}

// Set Brightness (called by encoder)
bool setDimmerBrightness(int brightness) {
    if (!dimmerDevice.isConnected) {
        return false;
    }
    
    dimmerDevice.status = DIMMER_CONTROLLING;
    bool success = false;
    
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 100) {
        success = setShellyBrightness(brightness);
    } else if (dimmerDevice.type == DIMMER_TASMOTA) {
        // TODO
    } else if (dimmerDevice.type == DIMMER_GENERIC) {
        // TODO
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
    
    setDimmerBrightness(newBrightness);
}

// Toggle Dimmer On/Off (called by encoder button)
bool toggleDimmer() {
    if (!dimmerDevice.isConnected) {
        return false;
    }
    
    dimmerDevice.status = DIMMER_CONTROLLING;
    bool success = false;
    
    if (dimmerDevice.type >= 10 && dimmerDevice.type < 100) {
        success = toggleShelly();
    } else if (dimmerDevice.type == DIMMER_TASMOTA) {
        // TODO
    } else if (dimmerDevice.type == DIMMER_GENERIC) {
        // TODO
    }
    
    if (success) {
        dimmerDevice.status = DIMMER_CONNECTED;
    } else {
        dimmerDevice.status = DIMMER_ERROR;
        dimmerDevice.errorMessage = "Failed to toggle dimmer";
    }
    
    return success;
}

// Set Dimmer Ratio (1-5)
void setDimmerRatio(int ratio) {
    if (ratio < 1) ratio = 1;
    if (ratio > 5) ratio = 5;
    
    dimmerConfig.dimmerRatio = ratio;
    dimmerPrefs.putInt("dimmer_ratio", ratio);
}

// Dimmer Loop (call in main loop)
// Status update failure counter for auto-disconnect on unreachable device
static uint8_t dimmerStatusFailCount = 0;
static const uint8_t MAX_STATUS_FAILURES = 5;

// Auto-reconnect timing
static unsigned long lastReconnectAttempt = 0;
static const unsigned long RECONNECT_INTERVAL = 5000;  // 5 saniyede bir yeniden bağlanma denemesi
static uint8_t reconnectAttemptCount = 0;
static const uint8_t MAX_RECONNECT_ATTEMPTS = 12;  // 12 deneme = 1 dakika sonra daha seyrek
static const unsigned long SLOW_RECONNECT_INTERVAL = 30000;  // 30 saniyede bir (yavaş mod)

void dimmerLoop() {
    // Auto-update status every 2 seconds (if connected)
    if (dimmerDevice.isConnected && (millis() - lastStatusUpdate > DIMMER_STATUS_UPDATE_INTERVAL)) {
        bool success = updateDimmerStatus();
        lastStatusUpdate = millis();
        
        // Track consecutive failures and auto-disconnect if device unreachable
        if (!success) {
            dimmerStatusFailCount++;
            if (dimmerStatusFailCount >= MAX_STATUS_FAILURES) {
                DEBUG_PRINTLN("[DIMMER] Device unreachable - auto disconnecting (will retry)");
                disconnectDimmer();
                dimmerStatusFailCount = 0;
                reconnectAttemptCount = 0;  // Reset reconnect counter
            }
        } else {
            dimmerStatusFailCount = 0;  // Reset on success
        }
    }
    
    // AUTO-RECONNECT: If not connected but have a saved IP, try to reconnect
    if (!dimmerDevice.isConnected && lastConnectedIP.length() > 0) {
        unsigned long currentInterval = (reconnectAttemptCount >= MAX_RECONNECT_ATTEMPTS) 
                                        ? SLOW_RECONNECT_INTERVAL 
                                        : RECONNECT_INTERVAL;
        
        if (millis() - lastReconnectAttempt > currentInterval) {
            lastReconnectAttempt = millis();
            reconnectAttemptCount++;
            
            DEBUG_PRINTF("[DIMMER] Auto-reconnect attempt #%d to %s\n", reconnectAttemptCount, lastConnectedIP.c_str());
            
            if (connectToDimmer(lastConnectedIP)) {
                DEBUG_PRINTLN("[DIMMER] Auto-reconnect successful!");
                reconnectAttemptCount = 0;  // Reset on success
            } else {
                if (reconnectAttemptCount >= MAX_RECONNECT_ATTEMPTS) {
                    DEBUG_PRINTF("[DIMMER] Switching to slow reconnect mode (every %lu sec)\n", SLOW_RECONNECT_INTERVAL / 1000);
                }
            }
        }
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
    doc["deviceType"] = dimmerDevice.displayName.length() > 0 ? dimmerDevice.displayName : dimmerDevice.name;
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
        return;
    }
    
    // Vector'u temizle ve belleği geri ver
    savedDevices.clear();
    savedDevices.shrink_to_fit();
    
    JsonArray arr = doc.as<JsonArray>();
    
    // Maksimum 10 cihaz - bellek sınırı
    int count = 0;
    for (JsonObject obj : arr) {
        if (count >= 10) break;  // Maksimum 10 cihaz
        SavedDimmerDevice device;
        device.ip = obj["ip"].as<String>();
        device.name = obj["name"].as<String>();
        device.type = (DimmerType)obj["type"].as<int>();
        savedDevices.push_back(device);
        count++;
    }
}

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
}

// Add Dimmer Device to Saved List
void addDimmerDevice(String ip, String name, DimmerType type) {
    for (auto& device : savedDevices) {
        if (device.ip == ip) {
            device.name = name;
            device.type = type;
            device.modelName = dimmerDevice.modelName;
            device.displayName = dimmerDevice.displayName;
            device.macAddress = dimmerDevice.macAddress;
            device.channel = dimmerDevice.channel;
            saveSavedDevices();
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
}

bool removeDimmerDevice(String ip) {
    for (auto it = savedDevices.begin(); it != savedDevices.end(); ++it) {
        if (it->ip == ip) {
            savedDevices.erase(it);
            saveSavedDevices();
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
}

void autoReconnect() {
    if (lastConnectedIP.length() == 0) {
        return;
    }
    connectToDimmer(lastConnectedIP);
}

#endif // SK_DIMMER_H
