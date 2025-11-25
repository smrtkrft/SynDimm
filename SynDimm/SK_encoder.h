/**
 * SK_encoder.h
 * SmartKraft SynDimm - KY-040 Rotary Encoder Management
 * Version: v0.9.1
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * ENCODER ana kontrol mekanizmasıdır!
 * - Tüm kritik işlemler ENCODER ile yapılır
 * - Web arayüzü sadece yardımcı/bilgilendirme
 * - Encoder bağımsız çalışmalıdır
 * - Event-based mimari korunmalıdır
 * ========================================
 */

/*
 * ========================================
 * Rotary Encoder Library (Cleaned)
 * ========================================
 * Clean encoder library - ONLY event detection
 * Business logic should be in modes.h!
 * ========================================
 * 
 * Event Types:
 * 'L' = Left rotation
 * 'R' = Right rotation
 * 'B' = Button short press
 * 'P' = Long press (3+ seconds) (for mode select)
 * 
 * Author: SEU
 */

#ifndef SK_ENCODER_H
#define SK_ENCODER_H

#include <Arduino.h>
#include "SK_config.h"

class SKEncoder {
private:
  // Pin definitions
  uint8_t clkPin;
  uint8_t dtPin;
  uint8_t swPin;
  
  // Encoder state variables
  volatile bool aState;
  volatile bool aLastState;
  volatile unsigned long lastInterruptTime;
  const unsigned long debounceDelay = 1; // 1ms debounce - precise reading
  
  // Button state variables
  bool lastButtonState;
  unsigned long lastButtonTime;
  unsigned long buttonPressTime;
  bool buttonWasPressed;
  const unsigned long buttonDebounceDelay = 50; // 50ms button debounce
  const unsigned long longPressTime = 3000; // 3 seconds long press - Emek
  
  // Event buffer
  volatile char eventBuffer[10];
  volatile uint8_t bufferWriteIndex;
  volatile uint8_t bufferReadIndex;
  
  // Mathematical counters - ONLY dimm_sayac (0-100)
  volatile int dimm_sayac;
  volatile char lastDirection;
  
  // Static pointer for interrupt handler
  static SKEncoder* instance;
  
  // Interrupt handler
  static void IRAM_ATTR handleEncoderStatic() {
    if (instance) {
      instance->handleEncoder();
    }
  }
  
  void IRAM_ATTR handleEncoder() {
    unsigned long currentTime = millis();
    
    // Debounce control
    if (currentTime - lastInterruptTime < debounceDelay) {
      return;
    }
    
    // Read CLK (A) and DT (B) pins
    aState = digitalRead(clkPin);
    bool bState = digitalRead(dtPin);
    
    // Check when CLK changes
    if (aState != aLastState) {
      // Direction determination - SmartKraft
      if (aState == bState) {
        // Left rotation
        addEvent('L');
        lastDirection = 'L';
      } else {
        // Right rotation
        addEvent('R');
        lastDirection = 'R';
      }
      
      lastInterruptTime = currentTime;
      aLastState = aState;
    }
  }
  
  void addEvent(char event) {
    // Add event to buffer
    uint8_t nextIndex = (bufferWriteIndex + 1) % 10;
    if (nextIndex != bufferReadIndex) {
      eventBuffer[bufferWriteIndex] = event;
      bufferWriteIndex = nextIndex;
    }
  }
  
public:
  // Default constructor - uses SK_config.h pins
  SKEncoder() 
    : clkPin(ENCODER_CLK), dtPin(ENCODER_DT), swPin(ENCODER_SW), 
      lastInterruptTime(0), lastButtonTime(0), buttonPressTime(0),
      bufferWriteIndex(0), bufferReadIndex(0),
      dimm_sayac(100), lastDirection(0), buttonWasPressed(false) {
    lastButtonState = HIGH;
  }
  
  // Custom constructor - for manual pin assignment
  SKEncoder(uint8_t clk, uint8_t dt, uint8_t sw) 
    : clkPin(clk), dtPin(dt), swPin(sw), 
      lastInterruptTime(0), lastButtonTime(0), buttonPressTime(0),
      bufferWriteIndex(0), bufferReadIndex(0),
      dimm_sayac(100), lastDirection(0), buttonWasPressed(false) {
    lastButtonState = HIGH;
  }
  
  void begin() {
    // Set pin modes
    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin, INPUT_PULLUP);
    pinMode(swPin, INPUT_PULLUP);
    
    delay(10);  // Pull-up stabilization
    
    // Read initial states
    aState = digitalRead(clkPin);
    aLastState = aState;
    lastButtonState = digitalRead(swPin);
    
    // Set static instance pointer
    instance = this;
    
    // Add interrupt to CLK pin
    attachInterrupt(digitalPinToInterrupt(clkPin), handleEncoderStatic, CHANGE);
  }
  
  // Check if event is available
  bool available() {
    // First check encoder events
    if (bufferReadIndex != bufferWriteIndex) {
      return true;
    }
    
    // Button control
    bool currentButtonState = digitalRead(swPin);
    unsigned long currentTime = millis();
    
    if (currentButtonState != lastButtonState) {
      if (currentTime - lastButtonTime > buttonDebounceDelay) {
        
        if (currentButtonState == LOW) {
          // Button pressed
          buttonPressTime = currentTime;
          buttonWasPressed = true;
          lastButtonTime = currentTime;
          
        } else if (currentButtonState == HIGH && buttonWasPressed) {
          // Button released
          unsigned long pressDuration = currentTime - buttonPressTime;
          
          Serial.print("[Encoder] Button released - Duration: ");
          Serial.print(pressDuration);
          Serial.println("ms");
          
          if (pressDuration >= longPressTime) {
            // Long press - 'P' event (for mode select)
            Serial.println("[Encoder] LONG PRESS detected - Sending 'P' event");
            addEvent('P');
          } else {
            // Short press - 'B' event
            Serial.println("[Encoder] Short press - Sending 'B' event");
            addEvent('B');
          }
          
          buttonWasPressed = false;
          lastButtonTime = currentTime;
        }
      }
    }
    
    lastButtonState = currentButtonState;
    
    return (bufferReadIndex != bufferWriteIndex);
  }
  
  // Read next event
  char read() {
    if (bufferReadIndex != bufferWriteIndex) {
      char event = eventBuffer[bufferReadIndex];
      bufferReadIndex = (bufferReadIndex + 1) % 10;
      return event;
    }
    return 0;
  }
  
  // Clear all buffer
  void clear() {
    bufferReadIndex = bufferWriteIndex;
  }
  
  // ========== MATHEMATICAL FUNCTIONS ==========
  
  int get_dimm_sayac() {
    return dimm_sayac;
  }
  
  void set_dimm_sayac(int value) {
    if (value >= 0 && value <= 100) {
      dimm_sayac = value;
    }
  }
  
  char get_last_direction() {
    return lastDirection;
  }
  
  void reset_all() {
    dimm_sayac = 100;
    lastDirection = 0;
  }
};

// Static member initialization
SKEncoder* SKEncoder::instance = nullptr;

#endif // SK_ENCODER_H
