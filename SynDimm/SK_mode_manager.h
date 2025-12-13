/**
 * SK_mode_manager.h
 * SmartKraft SynDimm - Mode Management System
 * Version: v1.1.1
 * 
 * Manages system modes: DIMMER, SHUTTER, SAFE
 * Handles mode switching via encoder long press (3s)
 * Saves/loads active mode to Preferences
 * 
 * Mode Selection Flow:
 * 1. Long press (3s) → Enter selection mode
 * 2. Rotate encoder → Preview modes (dit feedback)
 * 3. Long press (3s) again → Activate selected mode
 * 4. Timeout (15s) → Cancel, revert to previous mode
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
    const unsigned long MODE_SELECT_TIMEOUT = 15000; // 15 seconds
}

class SKModeManager {
private:
    SystemMode activeMode;
    SystemMode previewMode;
    bool inSelectionMode;
    unsigned long selectionStartTime;
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
        saveActiveMode();
        buzzer->playDits(getModeDitCount(mode));
    }
    
public:
    SKModeManager(SKBuzzer* buzzerInstance) {
        buzzer = buzzerInstance;
        inSelectionMode = false;
        selectionStartTime = 0;
        
        // Default to DIMMER, will be loaded in begin()
        activeMode = MODE_DIMMER;
        previewMode = activeMode;
    }
    
    void begin() {
        // Load saved mode from Preferences
        activeMode = loadActiveMode();
        previewMode = activeMode;
        DEBUG_PRINTF("[ModeManager] Initialized with mode: %s\n", getModeNameStr(activeMode));
    }
    
    // Call in loop() to handle timeout
    void update() {
        if (inSelectionMode) {
            unsigned long elapsed = millis() - selectionStartTime;
            if (elapsed >= MODE_SELECT_TIMEOUT) {
                inSelectionMode = false;
                previewMode = activeMode;
            }
        }
    }
    
    // Handle encoder events
    void handleEncoderEvent(char event) {
        if (event == 'P') {
            if (!inSelectionMode) {
                inSelectionMode = true;
                selectionStartTime = millis();
                previewMode = activeMode;
            } else {
                inSelectionMode = false;
                activateMode(previewMode);
            }
        }
        else if (inSelectionMode && (event == 'L' || event == 'R')) {
            if (event == 'R') {
                // Sağa çevirince sonraki mod, ama SAFE(2)'den öteye geçemez
                if ((int)previewMode < 2) {
                    previewMode = (SystemMode)((int)previewMode + 1);
                }
            } else {
                // Sola çevirince önceki mod, ama DIMMER(0)'dan öteye geçemez
                if ((int)previewMode > 0) {
                    previewMode = (SystemMode)((int)previewMode - 1);
                }
            }
            buzzer->playDits(getModeDitCount(previewMode));
        }
    }
    
    // Getters
    SystemMode getActiveMode() const {
        return activeMode;
    }
    
    SystemMode getPreviewMode() const {
        return previewMode;
    }
    
    bool isInSelectionMode() const {
        return inSelectionMode;
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
        
        if (inSelectionMode) {
            // Cancel selection mode first
            inSelectionMode = false;
        }
        
        activateMode(mode);
        return true;
    }
    
    // JSON status for web
    String getStatusJSON() {
        String json = "{";
        json += "\"activeMode\":" + String((int)activeMode) + ",";
        json += "\"activeModeStr\":\"" + String(getModeNameStr(activeMode)) + "\",";
        json += "\"previewMode\":" + String((int)previewMode) + ",";
        json += "\"previewModeStr\":\"" + String(getModeNameStr(previewMode)) + "\",";
        json += "\"inSelectionMode\":" + String(inSelectionMode ? "true" : "false");
        json += "}";
        return json;
    }
};

#endif // SK_MODE_MANAGER_H
