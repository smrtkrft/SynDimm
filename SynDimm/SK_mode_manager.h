/**
 * SK_mode_manager.h
 * SmartKraft SynDimm - Mode Management System
 * Version: v1.3.0
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
#include "SK_buzzer.h"
#include "SK_config.h"

namespace {
    Preferences modePrefs;
    const char* MODE_PREFS_NS = "mode_mgr";
    const char* MODE_KEY = "active_mode";
}

class SKModeManager {
private:
    SystemMode activeMode;
    SystemMode previewMode;
    bool inModeChangeMode;   // Buton basılı tutularak çevriliyor mu
    SKBuzzer* buzzer;
    
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
        activeMode = mode;
        previewMode = mode;
        saveActiveMode();
        if (buzzer) buzzer->playDits(getModeDitCount(mode));
        DEBUG_PRINTF("[ModeManager] Mode activated: %s\n", getModeNameStr(mode));
    }
    
public:
    SKModeManager(SKBuzzer* buzzerInstance) {
        buzzer = buzzerInstance;
        inModeChangeMode = false;
        activeMode = MODE_DIMMER;
        previewMode = activeMode;
    }
    
    void begin() {
        activeMode = loadActiveMode();
        previewMode = activeMode;
        DEBUG_PRINTF("[ModeManager] Initialized with mode: %s\n", getModeNameStr(activeMode));
    }
    
    // No timeout needed - mode change is immediate on button release
    void update() {
        // Reserved for future use
    }
    
    // Called when encoder rotates while button is held
    // Returns true if mode change was triggered (for encoder to mark)
    bool handleModeRotation(char direction) {
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
    
    // JSON status for web (simplified - mode change is instant via encoder)
    String getStatusJSON() {
        String json = "{";
        json += "\"activeMode\":" + String((int)activeMode) + ",";
        json += "\"activeModeStr\":\"" + String(getModeNameStr(activeMode)) + "\"";
        json += "}";
        return json;
    }
};

#endif // SK_MODE_MANAGER_H
