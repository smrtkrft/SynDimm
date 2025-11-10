/*
 * SynDimm - Modes Library
 * Mode management: Dimmer, Shutter, Safe and Panic modes
 * Powered by SEU - Emek - SmartKraft
 */

#ifndef MODES_H
#define MODES_H

#include "encoder.h"
#include <Preferences.h>
#include "safe_lock.h"  // Include SafeLock class
#include "syndimm_buzzer.h"  // For buzzer
#include "shutter.h"  // Include Shutter class

// Mode enumeration (order matters: Left to Right)
enum OperationMode {
  MODE_DIMMER = 1,   // 1 beep
  MODE_SHUTTER = 2,  // 2 beeps
  MODE_SAFE = 3,     // 3 beeps
  MODE_PANIC = 4     // 4 beeps (future implementation)
};

class ModeManager {
private:
  Encoder* encoder;
  SafeLock* safeLock;  // Safe Lock reference
  Shutter* shutter;    // Shutter reference
  SynDimmBuzzer* buzzer;  // Buzzer reference (for mode change beep)
  
  // Current active mode
  OperationMode currentMode;
  OperationMode pendingMode;  // Temporary mode during selection
  
  // Legacy mode flags (for backward compatibility during transition)
  bool dimmerMode;
  bool safeMode;
  
  // Mathematical counters (moved from encoder)
  long L_deger;          // Left direction consecutive counter
  long R_deger;          // Right direction consecutive counter
  char lastDirection;    // Last direction ('L' or 'R')
  
  // Mode change logic (moved from encoder.h)
  bool modeSelectActive;        // Is mode select active
  char modeChangeDirection;     // Last direction during mode change
  bool encoderRotatedDuringPress;  // Did encoder rotate after P event
  
  // Helper: Play buzzer based on mode number
  void playModeBuzzer(OperationMode mode) {
    if (!buzzer) return;
    
    switch(mode) {
      case MODE_DIMMER:
        buzzer->playShort();  // 1 beep
        break;
      case MODE_SHUTTER:
        buzzer->playDouble();  // 2 beeps
        break;
      case MODE_SAFE:
        buzzer->playTriple();  // 3 beeps
        break;
      case MODE_PANIC:
        // Future: 4 beeps
        break;
    }
  }
  
public:
  ModeManager(Encoder* enc) : encoder(enc), safeLock(nullptr), shutter(nullptr), buzzer(nullptr), 
                            currentMode(MODE_DIMMER), pendingMode(MODE_DIMMER),
                            dimmerMode(true), safeMode(false), 
                            L_deger(0), R_deger(0), lastDirection(0), 
                            modeSelectActive(false), modeChangeDirection(0), encoderRotatedDuringPress(false) {}
  
  void setBuzzer(SynDimmBuzzer* bz) {
    buzzer = bz;
  }
  
  void setShutter(Shutter* sh) {
    shutter = sh;
  }
  
  void begin() {
    // Initial settings
    L_deger = 0;
    R_deger = 0;
    lastDirection = 0;
    
    // Load last active mode (continue after power loss)
    Preferences prefs;
    prefs.begin("syndimm", true);
    int savedMode = prefs.getInt("currentMode", MODE_DIMMER);
    currentMode = (OperationMode)savedMode;
    pendingMode = currentMode;
    
    // Update legacy flags for compatibility
    dimmerMode = (currentMode == MODE_DIMMER);
    safeMode = (currentMode == MODE_SAFE);
    
    prefs.end();
    
    Serial.println("[Modes] Initialized - Mode: " + getModeName(currentMode));
  }
  
  // Get mode name as string
  String getModeName(OperationMode mode) {
    switch(mode) {
      case MODE_DIMMER: return "Dimmer";
      case MODE_SHUTTER: return "Shutter";
      case MODE_SAFE: return "Safe";
      case MODE_PANIC: return "Panic";
      default: return "Unknown";
    }
  }
  
  // Mode selection (legacy compatibility)
  // CRITICAL: Mode changes NEVER disconnect the device
  // Device connection persists across all modes and reboots
  void setDimmerMode(bool enable) {
    if (enable) {
      currentMode = MODE_DIMMER;
      dimmerMode = true;
      safeMode = false;
      // DEBUG REMOVED
      
      // Save to EEPROM
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("currentMode", currentMode);
      prefs.end();
    }
  }
  
  void setShutterMode(bool enable) {
    if (enable) {
      currentMode = MODE_SHUTTER;
      dimmerMode = false;
      safeMode = false;
      // DEBUG REMOVED
      
      // Save to EEPROM
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("currentMode", currentMode);
      prefs.end();
    }
  }
  
  void setSafeMode(bool enable) {
    if (enable) {
      currentMode = MODE_SAFE;
      safeMode = true;
      dimmerMode = false;
      // DEBUG REMOVED
      
      // Save to EEPROM
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("currentMode", currentMode);
      prefs.end();
    }
  }
  
  // Mod değiştirme (toggle)
  void toggleMode() {
    if (dimmerMode) {
      setSafeMode(true);
      // DEBUG REMOVED
    } else {
      setDimmerMode(true);
      // DEBUG REMOVED
    }
  }
  
  bool isDimmerMode() { return dimmerMode; }
  bool isSafeMode() { return safeMode; }
  
  // Safe Lock referansını ayarla
  void setSafeLock(SafeLock* sl) {
    safeLock = sl;
  }
  
  // Direction counters
  void processEncoderEvent(char event) {
    if (event == 0) return;  // Empty event
    
    // DEBUG REMOVED: Event received
    
    // === NEW MODE CHANGE LOGIC: DIMMER(1) -> SHUTTER(2) -> SAFE(3) -> PANIC(4) ===
    
    // 'P' event = Long press (3+ seconds)
    if (event == 'P') {
      // If mode select already active, this is CONFIRM
      if (modeSelectActive && encoderRotatedDuringPress) {
        Serial.println("[Mode] *** MODE CONFIRMED *** - Switching to: " + getModeName(pendingMode));
        
        // Apply the pending mode
        currentMode = pendingMode;
        
        // Update legacy flags
        dimmerMode = (currentMode == MODE_DIMMER);
        safeMode = (currentMode == MODE_SAFE);
        
        // Save to EEPROM
        Preferences prefs;
        prefs.begin("syndimm", false);
        prefs.putInt("currentMode", currentMode);
        prefs.end();
        
        // Play final buzzer
        playModeBuzzer(currentMode);
        
        // DEBUG REMOVED: Active mode
        
        // Exit mode select
        modeSelectActive = false;
        encoderRotatedDuringPress = false;
        return;
      }
      
      // First 'P' press - START mode select
      modeSelectActive = true;
      pendingMode = currentMode;  // Start from current mode
      encoderRotatedDuringPress = false;
      
      Serial.println("[Mode] *** MODE SELECT ACTIVE ***");
      Serial.print("[Mode] Current: ");
      Serial.print(getModeName(currentMode));
      Serial.println(" - Rotate to select, hold 3s to confirm");
      return;
    }
    
    // In mode select: handle LEFT/RIGHT rotation
    if (modeSelectActive) {
      if (event == 'L') {
        // LEFT: previous mode (with boundary check)
        OperationMode newMode = pendingMode;
        
        if (pendingMode == MODE_SHUTTER) {
          newMode = MODE_DIMMER;  // 2 -> 1
        } else if (pendingMode == MODE_SAFE) {
          newMode = MODE_SHUTTER;  // 3 -> 2
        } else if (pendingMode == MODE_PANIC) {
          newMode = MODE_SAFE;  // 4 -> 3
        }
        // If MODE_DIMMER, stay at MODE_DIMMER (boundary)
        
        if (newMode != pendingMode) {
          pendingMode = newMode;
          encoderRotatedDuringPress = true;
          playModeBuzzer(pendingMode);  // Play beeps for target mode
          // DEBUG REMOVED: LEFT
        } else {
          // DEBUG REMOVED: Boundary
        }
        return;
        
      } else if (event == 'R') {
        // RIGHT: next mode (with boundary check)
        OperationMode newMode = pendingMode;
        
        if (pendingMode == MODE_DIMMER) {
          newMode = MODE_SHUTTER;  // 1 -> 2
        } else if (pendingMode == MODE_SHUTTER) {
          newMode = MODE_SAFE;  // 2 -> 3
        } else if (pendingMode == MODE_SAFE) {
          newMode = MODE_PANIC;  // 3 -> 4 (not implemented yet)
        }
        // If MODE_PANIC or MODE_SAFE (until panic ready), stay at MODE_SAFE
        
        // Block transition to PANIC (not implemented)
        if (newMode == MODE_PANIC) {
          // DEBUG REMOVED: PANIC not implemented
          return;
        }
        
        if (newMode != pendingMode) {
          pendingMode = newMode;
          encoderRotatedDuringPress = true;
          playModeBuzzer(pendingMode);  // Play beeps for target mode
          // DEBUG REMOVED: RIGHT
        } else {
          // DEBUG REMOVED: Boundary
        }
        return;
        
      } else if (event == 'B') {
        // Short press - CANCEL mode select
        // DEBUG REMOVED: Cancelled
        pendingMode = currentMode;  // Reset to current
        modeSelectActive = false;
        encoderRotatedDuringPress = false;
      }
      return;  // In mode select, don't process normal operations
    }
    
    // === NORMAL MODE OPERATIONS (based on active mode) ===
    
    // Update direction counters (moved from encoder)
    if (event == 'L') {
      if (lastDirection != 'L') {
        R_deger = 0;  // Direction changed, reset opposite counter
      }
      L_deger++;
      lastDirection = 'L';
      
    } else if (event == 'R') {
      if (lastDirection != 'R') {
        L_deger = 0;  // Direction changed, reset opposite counter
      }
      R_deger++;
      lastDirection = 'R';
    }
    
    // Process based on current mode
    switch(currentMode) {
      case MODE_DIMMER:
        // Dimmer mode is now handled by DimmerControl class
        // This switch case kept for future extension
        break;
      case MODE_SHUTTER:
        if (shutter) shutter->processEncoderEvent(event);
        break;
      case MODE_SAFE:
        processSafeMode(event);
        break;
      case MODE_PANIC:
        // Future implementation
        break;
    }
  }
  
  // Direction counters
  long getLeftCount() { return L_deger; }
  long getRightCount() { return R_deger; }
  
  // Current mode getter
  OperationMode getCurrentMode() { return currentMode; }
  String getCurrentModeName() { return getModeName(currentMode); }
  
private:
  void processSafeMode(char event) {
    // Safe mod: Encoder hareketlerini Safe Lock'a yönlendir
    if (safeLock == nullptr) {
      Serial.println("[ERROR] Safe Lock not initialized!");
      return;
    }
    
    if (event == 'L' || event == 'R') {
      // Encoder hareketi - Safe Lock'a bildir
      bool clockwise = (event == 'R');
      safeLock->onEncoderMove(clockwise);
      
      // Debug: L ve R sayaçlarını yazdır
      Serial.print(event);
      Serial.print(event == 'L' ? L_deger : R_deger);
      Serial.print(" ");
      safeLock->printBufferStatus();
      
    } else if (event == 'B') {
      // Buton basıldı - Safe Lock'a bildir
      safeLock->onButtonPress();
      // DEBUG REMOVED: Button pressed
    }
  }
};

#endif // MODES_H
