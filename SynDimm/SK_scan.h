/**
 * SK_scan.h
 * SmartKraft SynDimm - Unified Network Scanner
 * Version: v0.9.1
 * 
 * ========================================
 * UNIFIED NETWORK DISCOVERY ENGINE
 * ========================================
 * Tüm modlar için tek scan sistemi:
 * - Dimmer devices (Shelly Dimmer serisi)
 * - Shutter devices (Shelly 2.5, Plus 2PM Cover)
 * - Future: RGB, Switch, Relay...
 * 
 * Features:
 * - Bitwise filter system
 * - Parallel scanning (10 threads)
 * - Fast TCP Port 80 check
 * - Comprehensive Shelly detection (Gen1/2/3)
 * - Callback-based architecture
 * - FreeRTOS task-based (non-blocking)
 * ========================================
 */

#ifndef SK_SCAN_H
#define SK_SCAN_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <vector>

// ========================================
// DEVICE TYPE FILTERS (Bitwise)
// ========================================
#define FILTER_NONE        0
#define FILTER_DIMMERS     1    // Shelly Dimmer 1/2/L/Gen3, 0-10V, 1-10V, DALI
#define FILTER_SHUTTERS    2    // Shelly 2.5, Plus 2PM (Cover mode)
#define FILTER_SWITCHES    4    // Future: Shelly 1/1PM/Plus 1PM
#define FILTER_RGBW        8    // Future: Shelly RGBW2, Duo RGBW
#define FILTER_RELAYS      16   // Future: Shelly 4Pro
#define FILTER_ALL         255  // All devices

// ========================================
// SCAN CONFIGURATION
// ========================================
#define SCAN_TCP_TIMEOUT_MS     500    // TCP Port 80 check
#define SCAN_HTTP_TIMEOUT_MS    3000   // HTTP device info
#define SCAN_PARALLEL_THREADS   10     // Parallel IP scanning
#define SCAN_TASK_STACK_SIZE    8192   // FreeRTOS stack
#define SCAN_TASK_PRIORITY      1      // Background priority

// ========================================
// DEVICE TYPE ENUMS
// ========================================
enum DeviceCategory {
    CATEGORY_UNKNOWN = 0,
    CATEGORY_DIMMER = 1,
    CATEGORY_SHUTTER = 2,
    CATEGORY_SWITCH = 3,
    CATEGORY_RGBW = 4,
    CATEGORY_RELAY = 5
};

// ========================================
// DISCOVERED DEVICE STRUCTURE
// ========================================
struct DiscoveredDevice {
    String ip;
    String modelName;        // e.g., "SHDM-2", "SHSW-25", "SNSW-001P16EU"
    String displayName;      // e.g., "Shelly Dimmer 2", "Shelly 2.5"
    String macAddress;
    String firmwareVersion;
    String mode;             // e.g., "roller", "relay", "white", "color" (for Shelly 2.5/RGBW2)
    String deviceType;       // Raw device type string
    
    DeviceCategory category;
    int specificType;        // DIMMER_SHELLY_DIMMER_2, SHUTTER_SHELLY_25, etc.
    int generation;          // 1, 2, 3
    
    // Capabilities
    bool supportsDimming;
    bool supportsShutter;
    bool supportsSwitch;
    bool supportsRGBW;
    bool isValid;            // Device responded correctly
    
    int maxChannels;
    
    // Constructor
    DiscoveredDevice() {
        ip = "";
        modelName = "";
        displayName = "";
        macAddress = "";
        firmwareVersion = "";
        mode = "";
        deviceType = "";
        category = CATEGORY_UNKNOWN;
        specificType = 0;
        generation = 0;
        supportsDimming = false;
        supportsShutter = false;
        supportsSwitch = false;
        supportsRGBW = false;
        isValid = false;
        maxChannels = 0;
    }
};

// ========================================
// SCAN PROGRESS TRACKING
// ========================================
struct ScanProgress {
    bool isScanning;
    int totalIPs;
    int scannedIPs;
    int aliveIPs;
    int devicesFound;
    unsigned long startTime;
    unsigned long endTime;
};

// ========================================
// SCAN CONFIGURATION
// ========================================
struct ScanConfig {
    int filters;              // Bitwise: FILTER_DIMMERS | FILTER_SHUTTERS
    int parallelCount;        // Number of parallel scans
    int tcpTimeout;           // TCP check timeout (ms)
    int httpTimeout;          // HTTP request timeout (ms)
    bool autoSave;            // Auto-save discovered devices
    
    // Constructor with defaults
    ScanConfig() {
        filters = FILTER_ALL;
        parallelCount = SCAN_PARALLEL_THREADS;
        tcpTimeout = SCAN_TCP_TIMEOUT_MS;
        httpTimeout = SCAN_HTTP_TIMEOUT_MS;
        autoSave = false;
    }
};

// ========================================
// CALLBACK TYPES
// ========================================
typedef std::function<void(DiscoveredDevice)> OnDeviceFoundCallback;
typedef std::function<void(int progress, int total)> OnProgressCallback;
typedef std::function<void(bool success, String message)> OnCompleteCallback;

// Anonymous namespace to prevent multiple definition
namespace {
    // Scan state
    std::vector<DiscoveredDevice> discoveredDevices;
    ScanProgress scanProgress;
    TaskHandle_t scanTaskHandle = NULL;
    SemaphoreHandle_t scanMutex = NULL;
    
    // Callbacks
    OnDeviceFoundCallback deviceFoundCallback = nullptr;
    OnProgressCallback progressCallback = nullptr;
    OnCompleteCallback completeCallback = nullptr;
    
    // Current scan config
    ScanConfig currentScanConfig;
}

// ========================================
// FUNCTION DECLARATIONS
// ========================================

// Main API
void startNetworkScan(ScanConfig config, 
                     OnDeviceFoundCallback onFound = nullptr,
                     OnProgressCallback onProgress = nullptr,
                     OnCompleteCallback onComplete = nullptr);
void stopNetworkScan();
bool isNetworkScanning();
String getNetworkScanProgressJSON();
std::vector<DiscoveredDevice> getDiscoveredDevices();
void clearDiscoveredDevices();

// Detection functions
DiscoveredDevice detectDevice(String ip);
bool checkTCPPort(String ip, int port, int timeout);
bool isIPAlive(String ip);

// FreeRTOS scan task
void networkScanTask(void* parameter);

// ========================================
// IMPLEMENTATION
// ========================================

// Check TCP Port (Fast alive check)
bool checkTCPPort(String ip, int port, int timeout) {
    WiFiClient client;
    client.setTimeout(timeout);
    
    if (client.connect(ip.c_str(), port, timeout)) {
        client.stop();
        return true;
    }
    return false;
}

// Check if IP is alive (TCP Port 80)
bool isIPAlive(String ip) {
    return checkTCPPort(ip, 80, SCAN_TCP_TIMEOUT_MS);
}

// Detect Device Type and Capabilities
DiscoveredDevice detectDevice(String ip) {
    DiscoveredDevice device;
    device.ip = ip;
    
    HTTPClient http;
    
    // ========================================
    // Try Gen2/Gen3 Device Info (RPC API)
    // ========================================
    String url = "http://" + ip + "/rpc/Shelly.GetDeviceInfo";
    http.begin(url);
    http.setTimeout(SCAN_HTTP_TIMEOUT_MS);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            String model = doc["model"] | "";
            String app = doc["app"] | "";
            String fw = doc["fw_id"] | "";
            String mac = doc["mac"] | "";
            
            device.modelName = model;
            device.firmwareVersion = fw;
            device.macAddress = mac;
            device.generation = 3;
            device.isValid = true;
            device.deviceType = app;
            
            // ========== DIMMER DEVICES ==========
            
            // Shelly Dimmer 2 Gen3 (SNSW-001P16EU)
            if (model.indexOf("SNSW-001P16EU") >= 0 || (app.indexOf("Dimmer") >= 0 && model.indexOf("Gen3") >= 0)) {
                device.displayName = "Shelly Dimmer 2 Gen3";
                device.category = CATEGORY_DIMMER;
                device.specificType = 20; // DIMMER_SHELLY_DIMMER_2_GEN3
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // Dimmer 0-10V PM Gen3 (SNDM-0013A1)
            else if (model.indexOf("SNDM-0013") >= 0 || 
                     (model.indexOf("0-10V") >= 0 && model.indexOf("PM") >= 0) ||
                     (app.indexOf("Dimmer") >= 0 && app.indexOf("0-10V") >= 0)) {
                device.displayName = "Shelly Dimmer 0-10V PM Gen3";
                device.category = CATEGORY_DIMMER;
                device.specificType = 21; // DIMMER_SHELLY_DIMMER_0_10V
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // Dimmer 1-10V PM Gen3
            else if ((model.indexOf("1-10V") >= 0 && model.indexOf("PM") >= 0) ||
                     (app.indexOf("Dimmer") >= 0 && app.indexOf("1-10V") >= 0)) {
                device.displayName = "Shelly Dimmer 1-10V PM Gen3";
                device.category = CATEGORY_DIMMER;
                device.specificType = 22; // DIMMER_SHELLY_DIMMER_1_10V
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // Generic 0-10V/1-10V
            else if (model.indexOf("0-10V") >= 0 || model.indexOf("S0-10V") >= 0) {
                device.displayName = "Shelly 0-10V Dimmer Gen3";
                device.category = CATEGORY_DIMMER;
                device.specificType = 21;
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            else if (model.indexOf("1-10V") >= 0 || model.indexOf("S1-10V") >= 0) {
                device.displayName = "Shelly 1-10V Dimmer Gen3";
                device.category = CATEGORY_DIMMER;
                device.specificType = 22;
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // Shelly Plus Dimmer 0-10V PM
            else if ((model.indexOf("Plus") >= 0 || model.indexOf("SNDM-0010") >= 0) && model.indexOf("Dimmer") >= 0) {
                device.displayName = "Shelly Plus Dimmer 0-10V PM";
                device.category = CATEGORY_DIMMER;
                device.specificType = 40; // DIMMER_SHELLY_PLUS_DIMMER
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // Shelly Plus Dimmer PM (SNDM-00100WW)
            else if (model.indexOf("SNDM-00100WW") >= 0 || model.indexOf("PlusDimmerPM") >= 0) {
                device.displayName = "Shelly Plus Dimmer PM";
                device.category = CATEGORY_DIMMER;
                device.specificType = 41; // DIMMER_SHELLY_PLUS_DIMMER_PM
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // Shelly Pro Dimmer
            else if ((model.indexOf("Pro") >= 0 && model.indexOf("Dimmer") >= 0) || model.indexOf("SPDM-") >= 0) {
                device.displayName = "Shelly Pro Dimmer";
                device.category = CATEGORY_DIMMER;
                device.specificType = 31; // DIMMER_SHELLY_PRO_DIMMER
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            // DALI Dimmer Gateway
            else if (model.indexOf("DALI") >= 0 || model.indexOf("SHDALI") >= 0) {
                device.displayName = "Shelly DALI Dimmer Gateway";
                device.category = CATEGORY_DIMMER;
                device.specificType = 50; // DIMMER_SHELLY_DALI
                device.supportsDimming = true;
                device.maxChannels = 1;
            }
            
            // ========== SHUTTER DEVICES ==========
            
            // Shelly Plus 2PM in Cover mode
            else if ((model.indexOf("Plus 2PM") >= 0 || model.indexOf("SNSW-002P16EU") >= 0) && app.indexOf("Cover") >= 0) {
                device.displayName = "Shelly Plus 2PM (Cover)";
                device.category = CATEGORY_SHUTTER;
                device.specificType = 200; // SHUTTER_SHELLY_PLUS_2PM_GEN2
                device.supportsShutter = true;
                device.maxChannels = 1;
                device.mode = "cover";
            }
            
            // Generic Dimmer fallback
            else if (app.indexOf("Dimmer") >= 0) {
                device.displayName = "Shelly Dimmer (Gen3)";
                device.category = CATEGORY_DIMMER;
                device.specificType = 20;
                device.supportsDimming = true;
                device.maxChannels = 1;
                device.mode = "dimmer";
            }
            
            return device;
        }
    }
    
    http.end();
    
    // ========================================
    // Try Gen1 Device Info (REST API)
    // ========================================
    url = "http://" + ip + "/shelly";
    http.begin(url);
    http.setTimeout(SCAN_HTTP_TIMEOUT_MS);
    
    httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            String type = doc["type"] | "";
            String fw = doc["fw"] | "";
            String mac = doc["mac"] | "";
            
            device.modelName = type;
            device.firmwareVersion = fw;
            device.macAddress = mac;
            device.generation = 1;
            device.isValid = true;
            device.deviceType = type;
            
            // ========== DIMMER DEVICES (Gen1) ==========
            
            // Shelly Dimmer 2 (Gen1)
            if (type == "SHDM-2" || type.indexOf("Dimmer2") >= 0) {
                device.displayName = "Shelly Dimmer 2";
                device.category = CATEGORY_DIMMER;
                device.specificType = 11; // DIMMER_SHELLY_DIMMER_2
                device.supportsDimming = true;
                device.maxChannels = 1;
                device.mode = "dimmer";
            }
            // Shelly Dimmer 1 (Gen1)
            else if (type == "SHDM-1" || type.indexOf("Dimmer1") >= 0) {
                device.displayName = "Shelly Dimmer 1";
                device.category = CATEGORY_DIMMER;
                device.specificType = 10; // DIMMER_SHELLY_DIMMER_1
                device.supportsDimming = true;
                device.maxChannels = 1;
                device.mode = "dimmer";
            }
            // Shelly Dimmer L (Gen1 - DALI)
            else if (type == "SHDM-L" || type.indexOf("DimmerL") >= 0) {
                device.displayName = "Shelly Dimmer L";
                device.category = CATEGORY_DIMMER;
                device.specificType = 12; // DIMMER_SHELLY_DIMMER_L
                device.supportsDimming = true;
                device.maxChannels = 1;
                device.mode = "dimmer";
            }
            // Shelly RGBW2 (can work in dimmer mode)
            else if (type == "SHRGBW2" || type.indexOf("RGBW2") >= 0) {
                device.displayName = "Shelly RGBW2 (White Mode)";
                device.category = CATEGORY_DIMMER;
                device.specificType = 10;
                device.supportsDimming = true;
                device.maxChannels = 4;
                device.mode = "white";
            }
            
            // ========== SHUTTER DEVICES (Gen1) ==========
            
            // Shelly 2.5 (SHSW-25) - KRITIK: Mode kontrolü yapmalıyız!
            else if (type.indexOf("SHSW-25") >= 0 || type.indexOf("Shelly2.5") >= 0) {
                // Shelly 2.5 bulundu - şimdi mode'unu kontrol et
                String settingsUrl = "http://" + ip + "/settings";
                HTTPClient httpSettings;
                httpSettings.begin(settingsUrl);
                httpSettings.setTimeout(SCAN_HTTP_TIMEOUT_MS);
                
                int settingsCode = httpSettings.GET();
                if (settingsCode == HTTP_CODE_OK) {
                    String settingsPayload = httpSettings.getString();
                    JsonDocument settingsDoc;
                    
                    if (deserializeJson(settingsDoc, settingsPayload) == DeserializationError::Ok) {
                        String mode = settingsDoc["mode"] | "";
                        device.mode = mode;
                        
                        if (mode == "roller") {
                            // ROLLER MODE - Shutter olarak kullanılıyor
                            device.displayName = "Shelly 2.5 (Roller Mode)";
                            device.category = CATEGORY_SHUTTER;
                            device.specificType = 100; // SHUTTER_SHELLY_25_GEN1
                            device.supportsShutter = true;
                            device.maxChannels = 1;
                            Serial.printf("[SCAN] Shelly 2.5 detected in ROLLER mode: %s\n", ip.c_str());
                        } else if (mode == "relay") {
                            // RELAY MODE - Switch olarak kullanılıyor
                            device.displayName = "Shelly 2.5 (Relay Mode)";
                            device.category = CATEGORY_SWITCH;
                            device.specificType = 101;
                            device.supportsSwitch = true;
                            device.maxChannels = 2;
                            Serial.printf("[SCAN] Shelly 2.5 detected in RELAY mode: %s (not suitable for shutter)\n", ip.c_str());
                        } else {
                            // Bilinmeyen mode
                            device.displayName = "Shelly 2.5 (Unknown Mode)";
                            device.category = CATEGORY_UNKNOWN;
                            Serial.printf("[SCAN] Shelly 2.5 detected with UNKNOWN mode '%s': %s\n", mode.c_str(), ip.c_str());
                        }
                    } else {
                        // Settings parse edilemedi - varsayılan roller
                        device.displayName = "Shelly 2.5 (Mode Unknown)";
                        device.category = CATEGORY_SHUTTER;
                        device.specificType = 100;
                        device.supportsShutter = true;
                        device.maxChannels = 1;
                        device.mode = "unknown";
                        Serial.printf("[SCAN] Shelly 2.5 detected but mode check failed: %s\n", ip.c_str());
                    }
                } else {
                    // Settings endpoint'e erişilemedi
                    device.displayName = "Shelly 2.5 (Mode Check Failed)";
                    device.category = CATEGORY_SHUTTER;
                    device.specificType = 100;
                    device.supportsShutter = true;
                    device.maxChannels = 1;
                    device.mode = "unknown";
                    Serial.printf("[SCAN] Shelly 2.5 detected but /settings unreachable: %s\n", ip.c_str());
                }
                
                httpSettings.end();
            }
            
            return device;
        }
    }
    
    http.end();
    
    // Unknown device
    return device;
}

// FreeRTOS Network Scan Task
void networkScanTask(void* parameter) {
    Serial.println("[SCAN] ========================================");
    Serial.println("[SCAN] Starting unified network scan...");
    Serial.println("[SCAN] ========================================");
    
    // Get local IP
    IPAddress localIP = WiFi.localIP();
    String subnet = String(localIP[0]) + "." + String(localIP[1]) + "." + String(localIP[2]) + ".";
    
    Serial.printf("[SCAN] Subnet: %s0/24\n", subnet.c_str());
    Serial.printf("[SCAN] Filters: 0x%02X\n", currentScanConfig.filters);
    Serial.printf("[SCAN] Parallel threads: %d\n", currentScanConfig.parallelCount);
    
    // Clear previous results
    if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
        discoveredDevices.clear();
        scanProgress.totalIPs = 254;
        scanProgress.scannedIPs = 0;
        scanProgress.aliveIPs = 0;
        scanProgress.devicesFound = 0;
        xSemaphoreGive(scanMutex);
    }
    
    // Phase 1: Fast TCP Port 80 check (parallel)
    Serial.println("[SCAN] Phase 1: TCP Port 80 scanning...");
    std::vector<String> aliveIPs;
    
    for (int batch = 1; batch <= 254; batch += currentScanConfig.parallelCount) {
        // Check if scan was stopped
        if (!scanProgress.isScanning) {
            Serial.println("[SCAN] Scan stopped by user");
            break;
        }
        
        // Parallel check
        for (int i = 0; i < currentScanConfig.parallelCount && (batch + i) <= 254; i++) {
            int ipSuffix = batch + i;
            String testIP = subnet + String(ipSuffix);
            
            if (isIPAlive(testIP)) {
                aliveIPs.push_back(testIP);
                if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
                    scanProgress.aliveIPs++;
                    xSemaphoreGive(scanMutex);
                }
                Serial.printf("[SCAN] ✓ Alive: %s\n", testIP.c_str());
            }
            
            // Update progress
            if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
                scanProgress.scannedIPs++;
                xSemaphoreGive(scanMutex);
            }
            
            // Callback progress
            if (progressCallback) {
                progressCallback(scanProgress.scannedIPs, scanProgress.totalIPs);
            }
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    Serial.printf("[SCAN] Phase 1 Complete: %d alive IPs found\n", aliveIPs.size());
    
    // Phase 2: Device detection and filtering
    Serial.println("[SCAN] Phase 2: Detecting devices...");
    
    for (const String& ip : aliveIPs) {
        // Check if scan was stopped
        if (!scanProgress.isScanning) {
            Serial.println("[SCAN] Scan stopped by user");
            break;
        }
        
        Serial.printf("[SCAN] Detecting: %s\n", ip.c_str());
        
        DiscoveredDevice device = detectDevice(ip);
        
        // Check if device matches filters
        bool matchesFilter = false;
        
        if (currentScanConfig.filters & FILTER_DIMMERS) {
            if (device.category == CATEGORY_DIMMER && device.supportsDimming) {
                matchesFilter = true;
            }
        }
        
        if (currentScanConfig.filters & FILTER_SHUTTERS) {
            if (device.category == CATEGORY_SHUTTER && device.supportsShutter) {
                matchesFilter = true;
            }
        }
        
        if (currentScanConfig.filters & FILTER_SWITCHES) {
            if (device.category == CATEGORY_SWITCH && device.supportsSwitch) {
                matchesFilter = true;
            }
        }
        
        // Add if matches filter
        if (matchesFilter) {
            if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
                discoveredDevices.push_back(device);
                scanProgress.devicesFound++;
                xSemaphoreGive(scanMutex);
            }
            
            Serial.printf("[SCAN] ✓✓ FOUND: %s - %s (Gen%d, Category: %d)\n", 
                         ip.c_str(), 
                         device.displayName.c_str(),
                         device.generation,
                         device.category);
            
            // Callback device found
            if (deviceFoundCallback) {
                deviceFoundCallback(device);
            }
        } else if (device.category != CATEGORY_UNKNOWN) {
            Serial.printf("[SCAN] ⊗ Skipped (filtered out): %s - %s\n", ip.c_str(), device.displayName.c_str());
        }
        
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
    
    // Complete
    if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
        scanProgress.isScanning = false;
        scanProgress.endTime = millis();
        xSemaphoreGive(scanMutex);
    }
    
    unsigned long duration = (scanProgress.endTime - scanProgress.startTime) / 1000;
    Serial.println("[SCAN] ========================================");
    Serial.printf("[SCAN] Scan complete in %lu seconds\n", duration);
    Serial.printf("[SCAN] Devices found: %d\n", scanProgress.devicesFound);
    Serial.println("[SCAN] ========================================");
    
    // Callback complete
    if (completeCallback) {
        String msg = "Found " + String(scanProgress.devicesFound) + " devices in " + String(duration) + "s";
        completeCallback(true, msg);
    }
    
    // Delete task
    scanTaskHandle = NULL;
    vTaskDelete(NULL);
}

// Start Network Scan
void startNetworkScan(ScanConfig config, 
                     OnDeviceFoundCallback onFound,
                     OnProgressCallback onProgress,
                     OnCompleteCallback onComplete) {
    
    // Check if already scanning
    if (scanProgress.isScanning) {
        Serial.println("[SCAN] Scan already in progress");
        if (onComplete) {
            onComplete(false, "Scan already in progress");
        }
        return;
    }
    
    // Create mutex if not exists
    if (scanMutex == NULL) {
        scanMutex = xSemaphoreCreateMutex();
    }
    
    // Store config and callbacks
    currentScanConfig = config;
    deviceFoundCallback = onFound;
    progressCallback = onProgress;
    completeCallback = onComplete;
    
    // Initialize progress
    if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
        scanProgress.isScanning = true;
        scanProgress.startTime = millis();
        scanProgress.totalIPs = 254;
        scanProgress.scannedIPs = 0;
        scanProgress.aliveIPs = 0;
        scanProgress.devicesFound = 0;
        xSemaphoreGive(scanMutex);
    }
    
    // Create FreeRTOS task
    xTaskCreate(
        networkScanTask,
        "NetworkScan",
        SCAN_TASK_STACK_SIZE,
        NULL,
        SCAN_TASK_PRIORITY,
        &scanTaskHandle
    );
    
    Serial.println("[SCAN] Network scan started (background task)");
}

// Stop Network Scan
void stopNetworkScan() {
    if (!scanProgress.isScanning) {
        Serial.println("[SCAN] No scan in progress");
        return;
    }
    
    Serial.println("[SCAN] Stopping network scan...");
    
    if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
        scanProgress.isScanning = false;
        xSemaphoreGive(scanMutex);
    }
    
    Serial.println("[SCAN] Scan stop requested");
}

// Check if scanning
bool isNetworkScanning() {
    return scanProgress.isScanning;
}

// Get Scan Progress as JSON
String getNetworkScanProgressJSON() {
    JsonDocument doc;
    
    if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
        doc["scanning"] = scanProgress.isScanning;
        doc["totalIPs"] = scanProgress.totalIPs;
        doc["scannedIPs"] = scanProgress.scannedIPs;
        doc["aliveIPs"] = scanProgress.aliveIPs;
        doc["devicesFound"] = scanProgress.devicesFound;
        
        int percentage = (scanProgress.scannedIPs * 100) / scanProgress.totalIPs;
        doc["percentage"] = percentage;
        
        // Count device types
        int dimmerCount = 0;
        int shutterCount = 0;
        int switchCount = 0;
        
        for (const auto& device : discoveredDevices) {
            if (device.supportsDimming) dimmerCount++;
            if (device.supportsShutter) shutterCount++;
            if (device.supportsSwitch) switchCount++;
        }
        
        doc["dimmerCount"] = dimmerCount;
        doc["shutterCount"] = shutterCount;
        doc["switchCount"] = switchCount;
        
        xSemaphoreGive(scanMutex);
    }
    
    String output;
    serializeJson(doc, output);
    return output;
}

// Get Discovered Devices
std::vector<DiscoveredDevice> getDiscoveredDevices() {
    return discoveredDevices;
}

// Clear Discovered Devices
void clearDiscoveredDevices() {
    if (xSemaphoreTake(scanMutex, portMAX_DELAY)) {
        discoveredDevices.clear();
        xSemaphoreGive(scanMutex);
    }
}

#endif // SK_SCAN_H
