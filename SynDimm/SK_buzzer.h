/**
 * SK_buzzer.h
 * SmartKraft SynDimm - Buzzer Control System
 * Version: v1.0.1
 * 
 * GPIO 17 - ESP32-C6
 * Non-blocking PWM buzzer control
 * Pattern-based feedback system
 * 
 * Features:
 * - Dit patterns (1/2/3 beeps for mode feedback)
 * - Non-blocking state machine
 * - PWM 2kHz frequency
 */

#ifndef SK_BUZZER_H
#define SK_BUZZER_H

#include <Arduino.h>
#include "SK_config.h"

namespace {
    const uint8_t BUZZER_PIN = 17;
    const uint16_t BUZZER_FREQ = 2000;  // 2kHz
    const uint8_t BUZZER_RESOLUTION = 8;
    
    // Timing constants (ms)
    const uint16_t DIT_DURATION = 80;
    const uint16_t DIT_PAUSE = 100;
    
    // Buzzer state
    enum BuzzerState {
        BUZZER_IDLE = 0,
        BUZZER_BEEPING,
        BUZZER_PAUSING
    };
    
    BuzzerState buzzerState = BUZZER_IDLE;
    int remainingDits = 0;
    unsigned long stateStartTime = 0;
}

class SKBuzzer {
private:
    void beepOn() {
        ledcWrite(BUZZER_PIN, 128);  // 50% duty cycle
    }
    
    void beepOff() {
        ledcWrite(BUZZER_PIN, 0);
    }
    
public:
    void begin() {
        pinMode(BUZZER_PIN, OUTPUT);
        digitalWrite(BUZZER_PIN, LOW);
        
        // ESP32-C6 LEDC setup
        ledcAttach(BUZZER_PIN, BUZZER_FREQ, BUZZER_RESOLUTION);
        ledcWrite(BUZZER_PIN, 0);
        
        DEBUG_PRINTLN("[Buzzer] Initialized on GPIO 17 (2kHz PWM)");
    }
    
    // Non-blocking update - call in loop()
    void update() {
        if (buzzerState == BUZZER_IDLE) {
            return;
        }
        
        unsigned long now = millis();
        unsigned long elapsed = now - stateStartTime;
        
        if (buzzerState == BUZZER_BEEPING) {
            if (elapsed >= DIT_DURATION) {
                // Beep finished
                beepOff();
                remainingDits--;
                
                if (remainingDits > 0) {
                    // More dits to play - start pause
                    buzzerState = BUZZER_PAUSING;
                    stateStartTime = now;
                } else {
                    // All dits played
                    buzzerState = BUZZER_IDLE;
                }
            }
        }
        else if (buzzerState == BUZZER_PAUSING) {
            if (elapsed >= DIT_PAUSE) {
                // Pause finished - start next dit
                beepOn();
                buzzerState = BUZZER_BEEPING;
                stateStartTime = now;
            }
        }
    }
    
    // Play N dits (1, 2, or 3)
    void playDits(int count) {
        if (count < 1 || count > 3) {
            DEBUG_PRINTF("[Buzzer] Invalid dit count: %d\n", count);
            return;
        }
        
        if (buzzerState != BUZZER_IDLE) {
            DEBUG_PRINTLN("[Buzzer] Busy - ignoring new dit request");
            return;
        }
        
        DEBUG_PRINTF("[Buzzer] Playing %d dit(s)\n", count);
        
        remainingDits = count;
        buzzerState = BUZZER_BEEPING;
        stateStartTime = millis();
        beepOn();
    }
    
    bool isPlaying() {
        return buzzerState != BUZZER_IDLE;
    }
    
    void stop() {
        beepOff();
        buzzerState = BUZZER_IDLE;
        remainingDits = 0;
    }
};

#endif // SK_BUZZER_H
