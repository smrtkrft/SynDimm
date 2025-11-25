/**
 * SK_shutter.h
 * SmartKraft SynDimm - Shutter Control System
 * Version: v0.9.1
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
#include "SK_scan.h"

// Shutter Settings
#define SHUTTER_MIN_POSITION 0
#define SHUTTER_MAX_POSITION 100
#define SHUTTER_DEFAULT_STEP 3        // Encoder step: 1-5 (% per tick)
#define SHUTTER_REQUEST_TIMEOUT 5000
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
    Serial.println("\n[SHUTTER] ========================================");
    Serial.println("[SHUTTER] Initializing Shutter Control System");
    Serial.println("[SHUTTER] ========================================");
    
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
    
    Serial.printf("[SHUTTER] Encoder Step: %d%% per tick\n", shutterConfig.encoderStep);
    Serial.printf("[SHUTTER] Last Direction: %s\n", 
                  shutterConfig.lastDirection == DIRECTION_UP ? "UP" : 
                  shutterConfig.lastDirection == DIRECTION_DOWN ? "DOWN" : "NONE");
    Serial.println("[SHUTTER] Initialization complete");
}

// Load Configuration from Preferences
void loadShutterConfig() {
    shutterConfig.encoderStep = shutterPrefs.getInt("encoder_step", SHUTTER_DEFAULT_STEP);
    shutterConfig.lastDirection = (ShutterDirection)shutterPrefs.getInt("last_dir", DIRECTION_NONE);
    shutterConfig.lastPosition = shutterPrefs.getInt("last_pos", 0);
    
    // Validate
    if (shutterConfig.encoderStep < 1 || shutterConfig.encoderStep > 5) {
        shutterConfig.encoderStep = SHUTTER_DEFAULT_STEP;
    }
}

// Save Configuration to Preferences
void saveShutterConfig() {
    shutterPrefs.putInt("encoder_step", shutterConfig.encoderStep);
    shutterPrefs.putInt("last_dir", shutterConfig.lastDirection);
    shutterPrefs.putInt("last_pos", shutterDevice.currentPosition);
    Serial.printf("[SHUTTER] Config saved: Step=%d, Dir=%d, Pos=%d\n", 
                  shutterConfig.encoderStep, shutterConfig.lastDirection, shutterDevice.currentPosition);
}

// Set Encoder Step (1-5)
void setEncoderStep(int step) {
    if (step >= 1 && step <= 5) {
        shutterConfig.encoderStep = step;
        saveShutterConfig();
        Serial.printf("[SHUTTER] Encoder step set to: %d%%\n", step);
    }
}

// Get Encoder Step
int getEncoderStep() {
    return shutterConfig.encoderStep;
}

// Detect Shutter Type from IP
ShutterType detectShutterType(String ip) {
    HTTPClient http;
    
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
                Serial.println("[SHUTTER] Detected: Shelly 2.5 Gen1");
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
                Serial.println("[SHUTTER] Detected: Shelly Plus 2PM (Cover mode)");
                return SHUTTER_SHELLY_PLUS_2PM_GEN2;
            }
        }
    }
    
    http.end();
    return SHUTTER_UNKNOWN;
}

// Connect to Shutter Device
bool connectToShutter(String ip) {
    Serial.printf("[SHUTTER] ========================================\n");
    Serial.printf("[SHUTTER] Attempting connection to: %s\n", ip.c_str());
    Serial.printf("[SHUTTER] ========================================\n");
    
    // Use unified scanner for device detection
    DiscoveredDevice device = detectDevice(ip);
    
    Serial.printf("[SHUTTER] Detection results:\n");
    Serial.printf("  - Found: %s\n", device.isValid ? "YES" : "NO");
    Serial.printf("  - Model: %s\n", device.modelName.c_str());
    Serial.printf("  - Display: %s\n", device.displayName.c_str());
    Serial.printf("  - Generation: %d\n", device.generation);
    Serial.printf("  - Type: %s\n", device.deviceType.c_str());
    Serial.printf("  - Mode: %s\n", device.mode.c_str());
    Serial.printf("  - Supports Shutter: %s\n", device.supportsShutter ? "YES" : "NO");
    Serial.printf("  - Supports Dimming: %s\n", device.supportsDimming ? "YES" : "NO");
    
    if (!device.isValid) {
        Serial.println("[SHUTTER] ERROR: Device not responding or invalid");
        return false;
    }
    
    if (!device.supportsShutter) {
        Serial.println("[SHUTTER] ERROR: Device doesn't support shutter/roller mode");
        Serial.printf("[SHUTTER] Current mode: %s\n", device.mode.c_str());
        Serial.println("[SHUTTER] HINT: If this is Shelly 2.5, check if it's in 'roller' mode in Shelly app");
        Serial.println("[SHUTTER] HINT: If dimmer is connected, it may be blocking roller mode");
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
            Serial.println("[SHUTTER] ERROR: Device type unknown");
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
    
    Serial.printf("[SHUTTER] ✓ Connected: %s (Gen%d)\n", shutterDevice.displayName.c_str(), shutterDevice.generation);
    return true;
}

// Disconnect Shutter
void disconnectShutter() {
    Serial.println("[SHUTTER] Disconnecting...");
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
                shutterDevice.currentPosition = doc["current_pos"] | 0;
                
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
                // {"state":"open/opening/closed/closing/stopped", "current_pos":60}
                String state = doc["state"] | "stopped";
                shutterDevice.currentPosition = doc["current_pos"] | 0;
                
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
        Serial.printf("[SHUTTER] Status request failed: %d\n", httpCode);
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
    
    // Status string
    String statusStr = "Disconnected";
    if (shutterDevice.isConnected) {
        switch (shutterDevice.status) {
            case SHUTTER_MOVING_UP:    statusStr = "Moving Up ↑"; break;
            case SHUTTER_MOVING_DOWN:  statusStr = "Moving Down ↓"; break;
            case SHUTTER_STOPPED:      statusStr = "Stopped"; break;
            case SHUTTER_OPEN:         statusStr = "Open (100%)"; break;
            case SHUTTER_CLOSED:       statusStr = "Closed (0%)"; break;
            case SHUTTER_PARTIAL:      statusStr = "Partial"; break;
            case SHUTTER_ERROR:        statusStr = "Error"; break;
            default:                   statusStr = "Connected"; break;
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
        Serial.println("[SHUTTER] Not connected");
        return false;
    }
    
    HTTPClient http;
    String url;
    
    if (steps > 0) {
        // Move by steps (position control)
        int targetPos = min(shutterDevice.currentPosition + steps, SHUTTER_MAX_POSITION);
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.goToPosition + String(targetPos);
        Serial.printf("[SHUTTER] Moving to position: %d%%\n", targetPos);
    } else {
        // Continuous open
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.open;
        Serial.println("[SHUTTER] Opening...");
    }
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        shutterDevice.status = SHUTTER_MOVING_UP;
        shutterDevice.lastDirection = DIRECTION_UP;
        shutterDevice.isMoving = true;
        shutterConfig.lastDirection = DIRECTION_UP;
        saveShutterConfig();
        
        // Update status after short delay
        delay(100);
        getShutterStatus();
        return true;
    }
    
    Serial.printf("[SHUTTER] Move Up failed: %d\n", httpCode);
    return false;
}

// Move Shutter Down (Close)
bool moveShutterDown(int steps) {
    if (!shutterDevice.isConnected) {
        Serial.println("[SHUTTER] Not connected");
        return false;
    }
    
    HTTPClient http;
    String url;
    
    if (steps > 0) {
        // Move by steps (position control)
        int targetPos = max(shutterDevice.currentPosition - steps, SHUTTER_MIN_POSITION);
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.goToPosition + String(targetPos);
        Serial.printf("[SHUTTER] Moving to position: %d%%\n", targetPos);
    } else {
        // Continuous close
        url = "http://" + shutterDevice.ip + shutterDevice.endpoints.close;
        Serial.println("[SHUTTER] Closing...");
    }
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        shutterDevice.status = SHUTTER_MOVING_DOWN;
        shutterDevice.lastDirection = DIRECTION_DOWN;
        shutterDevice.isMoving = true;
        shutterConfig.lastDirection = DIRECTION_DOWN;
        saveShutterConfig();
        
        // Update status after short delay
        delay(100);
        getShutterStatus();
        return true;
    }
    
    Serial.printf("[SHUTTER] Move Down failed: %d\n", httpCode);
    return false;
}

// Stop Shutter
bool stopShutter() {
    if (!shutterDevice.isConnected) {
        Serial.println("[SHUTTER] Not connected");
        return false;
    }
    
    HTTPClient http;
    String url = "http://" + shutterDevice.ip + shutterDevice.endpoints.stop;
    
    Serial.println("[SHUTTER] Stopping...");
    
    http.begin(url);
    http.setTimeout(SHUTTER_REQUEST_TIMEOUT);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        shutterDevice.isMoving = false;
        shutterDevice.status = SHUTTER_STOPPED;
        
        // Update status
        delay(100);
        getShutterStatus();
        saveShutterConfig();
        return true;
    }
    
    Serial.printf("[SHUTTER] Stop failed: %d\n", httpCode);
    return false;
}

// Set Shutter to Specific Position (0-100%)
bool setShutterPosition(int position) {
    if (!shutterDevice.isConnected) {
        return false;
    }
    
    position = constrain(position, SHUTTER_MIN_POSITION, SHUTTER_MAX_POSITION);
    
    HTTPClient http;
    String url = "http://" + shutterDevice.ip + shutterDevice.endpoints.goToPosition + String(position);
    
    Serial.printf("[SHUTTER] Going to position: %d%%\n", position);
    
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
        
        delay(100);
        getShutterStatus();
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
    if (!shutterDevice.isConnected) {
        Serial.println("[SHUTTER] No device connected");
        return;
    }
    
    int step = shutterConfig.encoderStep;
    
    if (direction > 0) {
        // Right = Down (close)
        Serial.printf("[SHUTTER] Encoder Right: Moving DOWN %d%%\n", step);
        moveShutterDown(step);
    } else if (direction < 0) {
        // Left = Up (open)
        Serial.printf("[SHUTTER] Encoder Left: Moving UP %d%%\n", step);
        moveShutterUp(step);
    }
}

// Handle Encoder Button (Stop or Toggle)
void handleShutterEncoderButton() {
    if (!shutterDevice.isConnected) {
        Serial.println("[SHUTTER] No device connected");
        return;
    }
    
    Serial.println("[SHUTTER] Encoder button pressed");
    
    if (shutterDevice.isMoving) {
        // If moving, STOP
        Serial.println("[SHUTTER] → Stopping movement");
        stopShutter();
    } else {
        // If stopped, toggle direction (move opposite of last direction)
        Serial.printf("[SHUTTER] → Toggling (last dir: %s)\n", 
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
