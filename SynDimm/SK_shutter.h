/**
 * SK_shutter.h
 * SmartKraft SynDimm - Shutter Control System
 * Version: v1.2.0
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Web arayüzü SADECE bilgilendirme içindir!
 * - Tüm shutter kontrolleri ESP32-C6 üzerinden yapılır
 * - JavaScript shutter cihazlara ASLA müdahale edemez
 * - Web sadece durum gösterir ve encoder step ayarı yapar
 * - Encoder kontrolü tamamen ESP32-C6'da çalışır
 * ========================================
 * 
 * Supported Devices:
 * - Shelly 2.5 (Gen1 REST API)
 * - Shelly Plus 2PM in Cover mode (Gen2/Gen3 RPC API)
 * 
 * Features:
 * - Single shutter connection via manual IP
 * - Network scanning for Shelly 2.5 auto-discovery
 * - Encoder control (rotate = up/down, button = stop/toggle)
 * - Calibration: Encoder Step 1-5 (position change per tick)
 * - Last direction memory (persistent)
 * - Real-time status display on web
 * ========================================
 */

#ifndef SK_SHUTTER_H
#define SK_SHUTTER_H

#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "SK_config.h"
#include "SK_scan.h"

// Shutter Settings
#define SHUTTER_MIN_POSITION 0
#define SHUTTER_MAX_POSITION 100
#define SHUTTER_DEFAULT_STEP 3        // Encoder step: 1-5 (% per tick)
#define SHUTTER_REQUEST_TIMEOUT 3000
#define SHUTTER_STATUS_UPDATE_INTERVAL 1000

// Anonymous namespace to prevent multiple definition
namespace {
    // Preferences storage
    Preferences shutterPrefs;
}

// Shutter Status Enum
enum ShutterStatus {
    SHUTTER_DISCONNECTED,
    SHUTTER_CONNECTED,
    SHUTTER_MOVING_UP,
    SHUTTER_MOVING_DOWN,
    SHUTTER_STOPPED,
    SHUTTER_OPEN,        // 100%
    SHUTTER_CLOSED,      // 0%
    SHUTTER_PARTIAL,     // 1-99%
    SHUTTER_ERROR
};

// Shutter Type (API compatibility)
enum ShutterType {
    SHUTTER_UNKNOWN = 0,
    SHUTTER_SHELLY_25_GEN1 = 100,      // SHSW-25 (REST API)
    SHUTTER_SHELLY_PLUS_2PM_GEN2 = 200, // Gen2/Gen3 RPC API
};

// Direction Enum
enum ShutterDirection {
    DIRECTION_NONE,
    DIRECTION_UP,
    DIRECTION_DOWN
};

// Shutter Device Structure
struct ShutterDevice {
    String ip;
    String modelName;
    String displayName;
    ShutterType type;
    int generation;             // 1, 2, or 3
    bool isConnected;
    bool isCalibrated;          // Whether device is calibrated
    ShutterStatus status;
    int currentPosition;        // 0-100%
    ShutterDirection lastDirection;
    bool isMoving;
    
    struct {
        String open;
        String close;
        String stop;
        String status;
        String goToPosition;    // For position control
    } endpoints;
};

// Shutter Configuration
struct ShutterConfig {
    int encoderStep;            // 1-5: % change per encoder tick
    ShutterDirection lastDirection;  // Persistent last direction
    int lastPosition;           // Last known position
    String lastConnectedIP;     // Last connected device IP
};

// Anonymous namespace to prevent multiple definition
namespace {
    // Global variables
    ShutterDevice shutterDevice;
    ShutterConfig shutterConfig;
    unsigned long lastShutterStatusUpdate = 0;
}

// ========================================
// FUNCTION DECLARATIONS
// ========================================

// Initialization
void initShutter();

// Connection Management
bool connectToShutter(String ip);
void disconnectShutter();
bool isShutterConnected();

// Shutter Control (ESP32-C6 only)
bool moveShutterUp(int steps = 0);      // steps=0: continuous, steps>0: % to move
bool moveShutterDown(int steps = 0);
bool stopShutter();
bool setShutterPosition(int position);  // 0-100%
bool toggleShutterDirection();          // Toggle based on last direction

// Status & Info
void getShutterStatus();
String getShutterStatusJSON();
ShutterStatus getCurrentShutterStatus();
int getCurrentPosition();

// Configuration
void setEncoderStep(int step);          // 1-5
int getEncoderStep();
void saveShutterConfig();
void loadShutterConfig();
void saveShutterLastIP(String ip);
void autoReconnectShutter();

// Encoder Handlers (called from SK_encoder.h)
void handleShutterEncoderRotate(int direction);  // +1 = right(down), -1 = left(up)
void handleShutterEncoderButton();               // Stop or toggle

// Update Loop (called from main loop)
void updateShutter();                            // Periodic status update

// API Detection
ShutterType detectShutterType(String ip);

// ========================================
// IMPLEMENTATION
// ========================================

// Initialize Shutter System
void initShutter() {
    DEBUG_PRINTLN("\n[SHUTTER] ========================================");
    DEBUG_PRINTLN("[SHUTTER] Initializing Shutter Control System");
    DEBUG_PRINTLN("[SHUTTER] ========================================");
    
    // Initialize preferences
    shutterPrefs.begin("shutter", false);
    
    // Load configuration
    loadShutterConfig();
    
    // Initialize device
    shutterDevice.ip = "";
    shutterDevice.modelName = "";
    shutterDevice.displayName = "No Device";
    shutterDevice.type = SHUTTER_UNKNOWN;
    shutterDevice.generation = 0;
    shutterDevice.isConnected = false;
    shutterDevice.status = SHUTTER_DISCONNECTED;
    shutterDevice.currentPosition = 0;
    shutterDevice.lastDirection = DIRECTION_NONE;
    shutterDevice.isMoving = false;
    
    DEBUG_PRINTF("[SHUTTER] Encoder Step: %d%% per tick\n", shutterConfig.encoderStep);
    DEBUG_PRINTF("[SHUTTER] Last Direction: %s\n", 
                  shutterConfig.lastDirection == DIRECTION_UP ? "UP" : 
                  shutterConfig.lastDirection == DIRECTION_DOWN ? "DOWN" : "NONE");
    DEBUG_PRINTLN("[SHUTTER] Initialization complete");
}

// Load Configuration from Preferences
void loadShutterConfig() {
    shutterConfig.encoderStep = shutterPrefs.getInt("encoder_step", SHUTTER_DEFAULT_STEP);
    shutterConfig.lastDirection = (ShutterDirection)shutterPrefs.getInt("last_dir", DIRECTION_NONE);
    shutterConfig.lastPosition = shutterPrefs.getInt("last_pos", 0);
    shutterConfig.lastConnectedIP = shutterPrefs.getString("last_ip", "");
    
    // Validate
    if (shutterConfig.encoderStep < 1 || shutterConfig.encoderStep > 5) {
        shutterConfig.encoderStep = SHUTTER_DEFAULT_STEP;
    }
    
    DEBUG_PRINTF("[SHUTTER] Loaded last IP: %s\n", shutterConfig.lastConnectedIP.c_str());
}

// Save last connected IP
void saveShutterLastIP(String ip) {
    shutterConfig.lastConnectedIP = ip;
    shutterPrefs.putString("last_ip", ip);
    DEBUG_PRINTF("[SHUTTER] Saved last IP: %s\n", ip.c_str());
}

// Auto reconnect to last shutter
void autoReconnectShutter() {
    if (shutterConfig.lastConnectedIP.length() > 0) {
        DEBUG_PRINTF("[SHUTTER] Auto-reconnecting to: %s\n", shutterConfig.lastConnectedIP.c_str());
        connectToShutter(shutterConfig.lastConnectedIP);
    }
}

// Save Configuration to Preferences
void saveShutterConfig() {
    shutterPrefs.putInt("encoder_step", shutterConfig.encoderStep);
    shutterPrefs.putInt("last_dir", shutterConfig.lastDirection);
    shutterPrefs.putInt("last_pos", shutterDevice.currentPosition);
    DEBUG_PRINTF("[SHUTTER] Config saved: Step=%d, Dir=%d, Pos=%d\n", 
                  shutterConfig.encoderStep, shutterConfig.lastDirection, shutterDevice.currentPosition);
}

// Set Encoder Step (1-5)
void setEncoderStep(int step) {
    if (step >= 1 && step <= 5) {
        shutterConfig.encoderStep = step;
        saveShutterConfig();
        DEBUG_PRINTF("[SHUTTER] Encoder step set to: %d%%\n", step);
    }
}

// Get Encoder Step
int getEncoderStep() {
    return shutterConfig.encoderStep;
}

// Detect Shutter Type from IP
ShutterType detectShutterType(String ip) {
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    
    // Try Gen1 Shelly 2.5 (REST API)
    String url = "http://" + ip + "/shelly";
    http.begin(url);
    http.setTimeout(3000);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            String type = doc["type"] | "";
            
            // Shelly 2.5 (SHSW-25)
            if (type.indexOf("SHSW-25") >= 0) {
                DEBUG_PRINTLN("[SHUTTER] Detected: Shelly 2.5 Gen1");
                return SHUTTER_SHELLY_25_GEN1;
            }
        }
    }
    
    http.end();
    
    // Try Gen2/Gen3 (RPC API)
    url = "http://" + ip + "/rpc/Shelly.GetDeviceInfo";
    http.begin(url);
    http.setTimeout(3000);
    
    httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        http.end();
        
        JsonDocument doc;
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            String model = doc["model"] | "";
            String app = doc["app"] | "";
            
            // Shelly Plus 2PM in Cover mode
            if ((model.indexOf("Plus 2PM") >= 0 || model.indexOf("SNSW-002P16EU") >= 0) && 
                app.indexOf("Cover") >= 0) {
                DEBUG_PRINTLN("[SHUTTER] Detected: Shelly Plus 2PM (Cover mode)");
                return SHUTTER_SHELLY_PLUS_2PM_GEN2;
            }
        }
    }
    
    http.end();
    return SHUTTER_UNKNOWN;
}

// Connect to Shutter Device
bool connectToShutter(String ip) {
    DEBUG_PRINTF("[SHUTTER] ========================================\n");
    DEBUG_PRINTF("[SHUTTER] Attempting connection to: %s\n", ip.c_str());
    DEBUG_PRINTF("[SHUTTER] ========================================\n");
    
    // Use unified scanner for device detection
    DiscoveredDevice device = detectDevice(ip);
    
    DEBUG_PRINTF("[SHUTTER] Detection results:\n");
    DEBUG_PRINTF("  - Found: %s\n", device.isValid ? "YES" : "NO");
    DEBUG_PRINTF("  - Model: %s\n", device.modelName.c_str());
    DEBUG_PRINTF("  - Display: %s\n", device.displayName.c_str());
    DEBUG_PRINTF("  - Generation: %d\n", device.generation);
    DEBUG_PRINTF("  - Type: %s\n", device.deviceType.c_str());
    DEBUG_PRINTF("  - Mode: %s\n", device.mode.c_str());
    DEBUG_PRINTF("  - Supports Shutter: %s\n", device.supportsShutter ? "YES" : "NO");
    DEBUG_PRINTF("  - Supports Dimming: %s\n", device.supportsDimming ? "YES" : "NO");
    
    if (!device.isValid) {
        DEBUG_PRINTLN("[SHUTTER] ERROR: Device not responding or invalid");
        return false;
    }
    
    if (!device.supportsShutter) {
        DEBUG_PRINTLN("[SHUTTER] ERROR: Device doesn't support shutter/roller mode");
        DEBUG_PRINTF("[SHUTTER] Current mode: %s\n", device.mode.c_str());
        DEBUG_PRINTLN("[SHUTTER] HINT: If this is Shelly 2.5, check if it's in 'roller' mode in Shelly app");
        DEBUG_PRINTLN("[SHUTTER] HINT: If dimmer is connected, it may be blocking roller mode");
        return false;
    }
    
    // Parse detected device info
    shutterDevice.ip = ip;
    shutterDevice.isConnected = true;
    shutterDevice.status = SHUTTER_CONNECTED;
    shutterDevice.modelName = device.modelName;
    shutterDevice.displayName = device.displayName;
    shutterDevice.generation = device.generation;
    
    // Determine type and endpoints based on generation
    if (device.generation == 1) {
        shutterDevice.type = SHUTTER_SHELLY_25_GEN1;
        // Gen1 REST API endpoints
        shutterDevice.endpoints.open = "/roller/0?go=open";
        shutterDevice.endpoints.close = "/roller/0?go=close";
        shutterDevice.endpoints.stop = "/roller/0?go=stop";
        shutterDevice.endpoints.status = "/roller/0";
        shutterDevice.endpoints.goToPosition = "/roller/0?go=to_pos&roller_pos=";
    }
    else if (device.generation == 2 || device.generation == 3) {
        shutterDevice.type = SHUTTER_SHELLY_PLUS_2PM_GEN2;
        // Gen2/Gen3 RPC API endpoints
        shutterDevice.endpoints.open = "/rpc/Cover.Open?id=0";
        shutterDevice.endpoints.close = "/rpc/Cover.Close?id=0";
        shutterDevice.endpoints.stop = "/rpc/Cover.Stop?id=0";
        shutterDevice.endpoints.status = "/rpc/Cover.GetStatus?id=0";
        shutterDevice.endpoints.goToPosition = "/rpc/Cover.GoToPosition?id=0&pos=";
    }
    else {
        // Fallback to old detection method if generation unknown
        ShutterType type = detectShutterType(ip);
        
        if (type == SHUTTER_UNKNOWN) {
            DEBUG_PRINTLN("[SHUTTER] ERROR: Device type unknown");
            return false;
        }
        
        shutterDevice.type = type;
        
        if (type == SHUTTER_SHELLY_25_GEN1) {
            shutterDevice.generation = 1;
            shutterDevice.modelName = "SHSW-25";
            shutterDevice.displayName = "Shelly 2.5";
            shutterDevice.endpoints.open = "/roller/0?go=open";
            shutterDevice.endpoints.close = "/roller/0?go=close";
            shutterDevice.endpoints.stop = "/roller/0?go=stop";
            shutterDevice.endpoints.status = "/roller/0";
            shutterDevice.endpoints.goToPosition = "/roller/0?go=to_pos&roller_pos=";
        } 
        else if (type == SHUTTER_SHELLY_PLUS_2PM_GEN2) {
            shutterDevice.generation = 2;
            shutterDevice.modelName = "SNSW-002P16EU";
            shutterDevice.displayName = "Shelly Plus 2PM (Cover)";
            shutterDevice.endpoints.open = "/rpc/Cover.Open?id=0";
            shutterDevice.endpoints.close = "/rpc/Cover.Close?id=0";
            shutterDevice.endpoints.stop = "/rpc/Cover.Stop?id=0";
            shutterDevice.endpoints.status = "/rpc/Cover.GetStatus?id=0";
            shutterDevice.endpoints.goToPosition = "/rpc/Cover.GoToPosition?id=0&pos=";
        }
    }
    
    // Get initial status
    getShutterStatus();
    
    // Save last connected IP
    saveShutterLastIP(ip);
    
    DEBUG_PRINTF("[SHUTTER] Connected: %s (Gen%d)\n", shutterDevice.displayName.c_str(), shutterDevice.generation);
    return true;
}

// Disconnect Shutter
void disconnectShutter() {
    DEBUG_PRINTLN("[SHUTTER] Disconnecting...");
    shutterDevice.isConnected = false;
    shutterDevice.status = SHUTTER_DISCONNECTED;
    shutterDevice.ip = "";
}

// Check Connection Status
bool isShutterConnected() {
    return shutterDevice.isConnected;
}

// Get Shutter Status from Device
void getShutterStatus() {
    if (!shutterDevice.isConnected) {
        return;
    }
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + shutterDevice.ip + shutterDevice.endpoints.status;
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        JsonDocument doc;
        
        if (deserializeJson(doc, payload) == DeserializationError::Ok) {
            
            if (shutterDevice.generation == 1) {
                // Gen1 REST API response
                // {"state":"open/close/stop", "current_pos":60, "power":0, "is_valid":true}
                String state = doc["state"] | "stop";
                int pos = doc["current_pos"] | 0;
                shutterDevice.currentPosition = constrain(pos, 0, 100);
                
                // Check calibration status - is_valid indicates if device is calibrated
                shutterDevice.isCalibrated = doc["is_valid"] | false;
                
                if (state == "open") {
                    shutterDevice.status = SHUTTER_MOVING_UP;
                    shutterDevice.isMoving = true;
                    shutterDevice.lastDirection = DIRECTION_UP;
                } else if (state == "close") {
                    shutterDevice.status = SHUTTER_MOVING_DOWN;
                    shutterDevice.isMoving = true;
                    shutterDevice.lastDirection = DIRECTION_DOWN;
                } else if (state == "stop") {
                    shutterDevice.isMoving = false;
                    if (shutterDevice.currentPosition >= 95) {
                        shutterDevice.status = SHUTTER_OPEN;
                    } else if (shutterDevice.currentPosition <= 5) {
                        shutterDevice.status = SHUTTER_CLOSED;
                    } else {
                        shutterDevice.status = SHUTTER_STOPPED;
                    }
                }
            } 
            else if (shutterDevice.generation == 2) {
                // Gen2/Gen3 RPC API response
                // {"state":"open/opening/closed/closing/stopped", "current_pos":60, "pos_control":true}
                String state = doc["state"] | "stopped";
                int pos = doc["current_pos"] | 0;
                shutterDevice.currentPosition = constrain(pos, 0, 100);
                
                // Check calibration status - pos_control indicates if device is calibrated
                shutterDevice.isCalibrated = doc["pos_control"] | false;
                
                if (state == "opening") {
                    shutterDevice.status = SHUTTER_MOVING_UP;
                    shutterDevice.isMoving = true;
                    shutterDevice.lastDirection = DIRECTION_UP;
                } else if (state == "closing") {
                    shutterDevice.status = SHUTTER_MOVING_DOWN;
                    shutterDevice.isMoving = true;
                    shutterDevice.lastDirection = DIRECTION_DOWN;
                } else if (state == "stopped") {
                    shutterDevice.isMoving = false;
                    if (shutterDevice.currentPosition >= 95) {
                        shutterDevice.status = SHUTTER_OPEN;
                    } else if (shutterDevice.currentPosition <= 5) {
                        shutterDevice.status = SHUTTER_CLOSED;
                    } else {
                        shutterDevice.status = SHUTTER_STOPPED;
                    }
                }
            }
            
            // Update config
            shutterConfig.lastDirection = shutterDevice.lastDirection;
            shutterConfig.lastPosition = shutterDevice.currentPosition;
        }
    } else {
        DEBUG_PRINTF("[SHUTTER] Status request failed: %d\n", httpCode);
        shutterDevice.status = SHUTTER_ERROR;
    }
    
    http.end();
}

// Get Status as JSON
String getShutterStatusJSON() {
    JsonDocument doc;
    
    doc["connected"] = shutterDevice.isConnected;
    doc["ip"] = shutterDevice.ip;
    doc["model"] = shutterDevice.displayName;
    doc["position"] = shutterDevice.currentPosition;
    doc["encoderStep"] = shutterConfig.encoderStep;
    doc["isMoving"] = shutterDevice.isMoving;
    doc["isCalibrated"] = shutterDevice.isCalibrated;
    
    // Status string (short keys for JS compatibility)
    String statusStr = "disconnected";
    if (shutterDevice.isConnected) {
        switch (shutterDevice.status) {
            case SHUTTER_MOVING_UP:    statusStr = "moving_up"; break;
            case SHUTTER_MOVING_DOWN:  statusStr = "moving_down"; break;
            case SHUTTER_STOPPED:      statusStr = "stopped"; break;
            case SHUTTER_OPEN:         statusStr = "open"; break;
            case SHUTTER_CLOSED:       statusStr = "closed"; break;
            case SHUTTER_PARTIAL:      statusStr = "partial"; break;
            case SHUTTER_ERROR:        statusStr = "error"; break;
            default:                   statusStr = "connected"; break;
        }
    }
    doc["status"] = statusStr;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// Move Shutter Up (Open)
bool moveShutterUp(int steps) {
    if (!shutterDevice.isConnected) {
        DEBUG_PRINTLN("[SHUTTER] Not connected");
        return false;
    }
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url;
    
    if (steps > 0) {
        // Move by steps (position control)
        int targetPos = min(shutterDevice.currentPosition + steps, SHUTTER_MAX_POSITION);
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.goToPosition + String(targetPos);
        DEBUG_PRINTF("[SHUTTER] Moving to position: %d%% (current: %d%%)\n", targetPos, shutterDevice.currentPosition);
        DEBUG_PRINTF("[SHUTTER] Full URL: %s\n", url.c_str());
    } else {
        // Continuous open
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.open;
        DEBUG_PRINTLN("[SHUTTER] Opening...");
        DEBUG_PRINTF("[SHUTTER] Full URL: %s\n", url.c_str());
    }
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        DEBUG_PRINTF("[SHUTTER] Response: %s\n", response.c_str());
        http.end();
        
        shutterDevice.status = SHUTTER_MOVING_UP;
        shutterDevice.lastDirection = DIRECTION_UP;
        shutterDevice.isMoving = true;
        shutterConfig.lastDirection = DIRECTION_UP;
        saveShutterConfig();
        return true;
    }
    
    String errorResponse = http.getString();
    http.end();
    DEBUG_PRINTF("[SHUTTER] Move Up failed: %d\n", httpCode);
    DEBUG_PRINTF("[SHUTTER] Error response: %s\n", errorResponse.c_str());
    return false;
}

// Move Shutter Down (Close)
bool moveShutterDown(int steps) {
    if (!shutterDevice.isConnected) {
        DEBUG_PRINTLN("[SHUTTER] Not connected");
        return false;
    }
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url;
    
    if (steps > 0) {
        // Move by steps (position control)
        int targetPos = max(shutterDevice.currentPosition - steps, SHUTTER_MIN_POSITION);
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.goToPosition + String(targetPos);
        DEBUG_PRINTF("[SHUTTER] Moving to position: %d%% (current: %d%%)\n", targetPos, shutterDevice.currentPosition);
        DEBUG_PRINTF("[SHUTTER] Full URL: %s\n", url.c_str());
    } else {
        // Continuous close
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.close;
        DEBUG_PRINTLN("[SHUTTER] Closing...");
        DEBUG_PRINTF("[SHUTTER] Full URL: %s\n", url.c_str());
    }
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    
    if (httpCode == HTTP_CODE_OK) {
        String response = http.getString();
        DEBUG_PRINTF("[SHUTTER] Response: %s\n", response.c_str());
        http.end();
        
        shutterDevice.status = SHUTTER_MOVING_DOWN;
        shutterDevice.lastDirection = DIRECTION_DOWN;
        shutterDevice.isMoving = true;
        shutterConfig.lastDirection = DIRECTION_DOWN;
        saveShutterConfig();
        return true;
    }
    
    String errorResponse = http.getString();
    http.end();
    DEBUG_PRINTF("[SHUTTER] Move Down failed: %d\n", httpCode);
    DEBUG_PRINTF("[SHUTTER] Error response: %s\n", errorResponse.c_str());
    return false;
}

// Stop Shutter
bool stopShutter() {
    if (!shutterDevice.isConnected) {
        DEBUG_PRINTLN("[SHUTTER] Not connected");
        return false;
    }
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + shutterDevice.ip + shutterDevice.endpoints.stop;
    
    DEBUG_PRINTLN("[SHUTTER] Stopping...");
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        shutterDevice.isMoving = false;
        shutterDevice.status = SHUTTER_STOPPED;
        saveShutterConfig();
        return true;
    }
    
    DEBUG_PRINTF("[SHUTTER] Stop failed: %d\n", httpCode);
    return false;
}

// Set Shutter to Specific Position (0-100%)
bool setShutterPosition(int position) {
    if (!shutterDevice.isConnected) {
        return false;
    }
    
    position = constrain(position, SHUTTER_MIN_POSITION, SHUTTER_MAX_POSITION);
    
    HTTPClient http;
    http.setReuse(false);  // Bellek sızıntısı önleme
    String url = "http://" + shutterDevice.ip + shutterDevice.endpoints.goToPosition + String(position);
    
    DEBUG_PRINTF("[SHUTTER] Going to position: %d%%\n", position);
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        // Determine direction
        if (position > shutterDevice.currentPosition) {
            shutterDevice.lastDirection = DIRECTION_UP;
            shutterDevice.status = SHUTTER_MOVING_UP;
        } else if (position < shutterDevice.currentPosition) {
            shutterDevice.lastDirection = DIRECTION_DOWN;
            shutterDevice.status = SHUTTER_MOVING_DOWN;
        }
        
        shutterDevice.isMoving = true;
        shutterConfig.lastDirection = shutterDevice.lastDirection;
        saveShutterConfig();
        return true;
    }
    
    return false;
}

// Toggle Direction (based on last direction)
bool toggleShutterDirection() {
    if (!shutterDevice.isConnected) {
        return false;
    }
    
    // If moving, opposite direction will be used
    ShutterDirection targetDirection = shutterConfig.lastDirection;
    
    // Invert direction
    if (targetDirection == DIRECTION_UP) {
        return moveShutterDown(0);  // Continuous down
    } else {
        return moveShutterUp(0);    // Continuous up (default if NONE)
    }
}

// ========================================
// UPDATE LOOP
// ========================================

// Update Shutter Status Periodically
void updateShutter() {
    if (!shutterDevice.isConnected) {
        return;
    }
    
    // Update status every SHUTTER_STATUS_UPDATE_INTERVAL (1 second)
    unsigned long currentTime = millis();
    if (currentTime - lastShutterStatusUpdate >= SHUTTER_STATUS_UPDATE_INTERVAL) {
        lastShutterStatusUpdate = currentTime;
        getShutterStatus();
    }
}

// ========================================
// ENCODER HANDLERS
// ========================================

// Handle Encoder Rotation (from SK_encoder.h)
void handleShutterEncoderRotate(int direction) {
    DEBUG_PRINTF("[SHUTTER] handleShutterEncoderRotate called with direction: %d\n", direction);
    
    if (!shutterDevice.isConnected) {
        DEBUG_PRINTLN("[SHUTTER] No device connected - cannot move");
        return;
    }
    
    // Check if device is calibrated - encoder rotation disabled if not calibrated
    if (!shutterDevice.isCalibrated) {
        DEBUG_PRINTLN("[SHUTTER] Device not calibrated - encoder rotation disabled");
        DEBUG_PRINTLN("[SHUTTER] Use button for full open/close/stop instead");
        return;
    }
    
    int step = shutterConfig.encoderStep;
    DEBUG_PRINTF("[SHUTTER] Current encoder step: %d%%\n", step);
    
    if (direction > 0) {
        // Right = Down (close)
        DEBUG_PRINTF("[SHUTTER] Encoder Right: Moving DOWN %d%%\n", step);
        moveShutterDown(step);
    } else if (direction < 0) {
        // Left = Up (open)
        DEBUG_PRINTF("[SHUTTER] Encoder Left: Moving UP %d%%\n", step);
        moveShutterUp(step);
    } else {
        DEBUG_PRINTLN("[SHUTTER] Warning: direction is 0, no action taken");
    }
}

// Handle Encoder Button (Stop or Toggle)
void handleShutterEncoderButton() {
    if (!shutterDevice.isConnected) {
        DEBUG_PRINTLN("[SHUTTER] No device connected");
        return;
    }
    
    DEBUG_PRINTLN("[SHUTTER] Encoder button pressed");
    
    if (shutterDevice.isMoving) {
        // If moving, STOP
        DEBUG_PRINTLN("[SHUTTER] → Stopping movement");
        stopShutter();
    } else {
        // If stopped, toggle direction (move opposite of last direction)
        DEBUG_PRINTF("[SHUTTER] → Toggling (last dir: %s)\n", 
                     shutterConfig.lastDirection == DIRECTION_UP ? "UP" : "DOWN");
        toggleShutterDirection();
    }
}

// Get Current Position
int getCurrentPosition() {
    return shutterDevice.currentPosition;
}

// Get Current Status
ShutterStatus getCurrentShutterStatus() {
    return shutterDevice.status;
}

#endif // SK_SHUTTER_H
