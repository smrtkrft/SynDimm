/**
 * SK_mode_manager.h
 * SmartKraft SynDimm - Mode Management System
 * Version: v1.1.1
 * 
 * Manages system modes: DIMMER, SHUTTER, SAFE
 * Mode switching methods:
 * 1. Web interface - click on mode button
 * 2. Encoder - hold button + rotate to preview, release to confirm
 * 
 * Saves/loads active mode to Preferences
 */

#ifndef SK_MODE_MANAGER_H
#define SK_MODE_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <functional>
#include "SK_buzzer.h"
#include "SK_config.h"

namespace {
    Preferences modePrefs;
    const char* MODE_PREFS_NS = "mode_mgr";
    const char* MODE_KEY = "active_mode";
    const char* ENCODER_MODE_KEY = "enc_mode_en";  // Encoder ile mod değiştirme ayarı
}

// Mod değişim callback tipi
typedef std::function<void(SystemMode oldMode, SystemMode newMode)> ModeChangeCallback;

class SKModeManager {
private:
    SystemMode activeMode;
    SystemMode previewMode;
    bool inModeChangeMode;   // Buton basılı tutularak çevriliyor mu
    bool encoderModeChangeEnabled;  // Encoder ile mod değiştirme aktif mi
    SKBuzzer* buzzer;
    ModeChangeCallback onModeChangeCallback;  // Mod değişim callback'i
    
    const char* getModeNameStr(SystemMode mode) const {
        switch(mode) {
            case MODE_DIMMER: return "DIMMER";
            case MODE_SHUTTER: return "SHUTTER";
            case MODE_SAFE: return "SAFE";
            default: return "UNKNOWN";
        }
    }
    
    int getModeDitCount(SystemMode mode) {
        switch(mode) {
            case MODE_DIMMER: return 1;
            case MODE_SHUTTER: return 2;
            case MODE_SAFE: return 3;
            default: return 1;
        }
    }
    
    void saveActiveMode() {
        modePrefs.begin(MODE_PREFS_NS, false);
        modePrefs.putUChar(MODE_KEY, (uint8_t)activeMode);
        modePrefs.end();
        DEBUG_PRINTF("[ModeManager] Saved mode: %d (%s)\n", (uint8_t)activeMode, getModeNameStr(activeMode));
    }
    
    SystemMode loadActiveMode() {
        modePrefs.begin(MODE_PREFS_NS, true);
        uint8_t savedMode = modePrefs.getUChar(MODE_KEY, (uint8_t)MODE_DIMMER);
        modePrefs.end();
        
        if (savedMode > MODE_SAFE) {
            savedMode = (uint8_t)MODE_DIMMER;
        }
        
        DEBUG_PRINTF("[ModeManager] Loaded saved mode: %d (%s)\n", savedMode, getModeNameStr((SystemMode)savedMode));
        return (SystemMode)savedMode;
    }
    
    void activateMode(SystemMode mode) {
        SystemMode oldMode = activeMode;  // Eski modu kaydet
        activeMode = mode;
        previewMode = mode;
        saveActiveMode();
        if (buzzer) buzzer->playDits(getModeDitCount(mode));
        DEBUG_PRINTF("[ModeManager] Mode activated: %s\n", getModeNameStr(mode));
        
        // Callback çağır (eğer tanımlıysa)
        if (onModeChangeCallback) {
            onModeChangeCallback(oldMode, mode);
        }
    }
    
public:
    SKModeManager(SKBuzzer* buzzerInstance) {
        buzzer = buzzerInstance;
        inModeChangeMode = false;
        encoderModeChangeEnabled = true;  // Varsayılan: açık
        activeMode = MODE_DIMMER;
        previewMode = activeMode;
        onModeChangeCallback = nullptr;
    }
    
    void begin() {
        activeMode = loadActiveMode();
        previewMode = activeMode;
        // Encoder mod değiştirme ayarını yükle
        modePrefs.begin(MODE_PREFS_NS, true);
        encoderModeChangeEnabled = modePrefs.getBool(ENCODER_MODE_KEY, true);
        modePrefs.end();
        DEBUG_PRINTF("[ModeManager] Initialized with mode: %s, encoder mode change: %s\n", 
                     getModeNameStr(activeMode), encoderModeChangeEnabled ? "enabled" : "disabled");
    }
    
    // No timeout needed - mode change is immediate on button release
    void update() {
        // Reserved for future use
    }
    
    // Called when encoder rotates while button is held
    // Returns true if mode change was triggered (for encoder to mark)
    bool handleModeRotation(char direction) {
        // Encoder ile mod değiştirme kapalıysa işlem yapma
        if (!encoderModeChangeEnabled) {
            DEBUG_PRINTLN("[ModeManager] Encoder mode change disabled - ignoring rotation");
            return false;
        }
        
        inModeChangeMode = true;
        
        if (direction == 'R') {
            // Sağa çevirince sonraki mod
            if ((int)previewMode < 2) {
                previewMode = (SystemMode)((int)previewMode + 1);
            }
        } else if (direction == 'L') {
            // Sola çevirince önceki mod
            if ((int)previewMode > 0) {
                previewMode = (SystemMode)((int)previewMode - 1);
            }
        }
        
        // Buzzer feedback for preview mode
        if (buzzer) buzzer->playDits(getModeDitCount(previewMode));
        DEBUG_PRINTF("[ModeManager] Preview mode: %s\n", getModeNameStr(previewMode));
        
        return true;
    }
    
    // Called when button is released after mode change rotation
    void confirmModeChange() {
        if (inModeChangeMode && previewMode != activeMode) {
            activateMode(previewMode);
        } else if (inModeChangeMode) {
            // Preview was same as active, still play confirmation
            if (buzzer) buzzer->playDits(getModeDitCount(activeMode));
        }
        inModeChangeMode = false;
    }
    
    // Cancel mode change (reset preview to active)
    void cancelModeChange() {
        previewMode = activeMode;
        inModeChangeMode = false;
    }
    
    // Getters
    SystemMode getActiveMode() const {
        return activeMode;
    }
    
    SystemMode getPreviewMode() const {
        return previewMode;
    }
    
    bool isInModeChangeMode() const {
        return inModeChangeMode;
    }
    
    const char* getActiveModeName() const {
        return getModeNameStr(activeMode);
    }
    
    const char* getPreviewModeName() const {
        return getModeNameStr(previewMode);
    }
    
    // Web interface - set mode directly
    bool setMode(SystemMode mode) {
        if (mode < MODE_DIMMER || mode > MODE_SAFE) {
            return false;
        }
        activateMode(mode);
        return true;
    }
    
    // Mod değişim callback'i ayarla
    void setModeChangeCallback(ModeChangeCallback callback) {
        onModeChangeCallback = callback;
    }
    
    // Encoder ile mod değiştirme ayarı
    bool isEncoderModeChangeEnabled() const {
        return encoderModeChangeEnabled;
    }
    
    void setEncoderModeChangeEnabled(bool enabled) {
        encoderModeChangeEnabled = enabled;
        modePrefs.begin(MODE_PREFS_NS, false);
        modePrefs.putBool(ENCODER_MODE_KEY, enabled);
        modePrefs.end();
        DEBUG_PRINTF("[ModeManager] Encoder mode change: %s\n", enabled ? "enabled" : "disabled");
    }
    
    // JSON status for web (simplified - mode change is instant via encoder)
    String getStatusJSON() {
        String json = "{";
        json += "\"activeMode\":" + String((int)activeMode) + ",";
        json += "\"activeModeStr\":\"" + String(getModeNameStr(activeMode)) + "\",";
        json += "\"encoderModeChangeEnabled\":" + String(encoderModeChangeEnabled ? "true" : "false");
        json += "}";
        return json;
    }
};

#endif // SK_MODE_MANAGER_H
