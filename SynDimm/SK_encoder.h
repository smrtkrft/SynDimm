/**
 * SK_encoder.h
 * SmartKraft SynDimm - KY-040 Rotary Encoder Management
 * Version: v1.1.1
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
 * 'B' = Button press
 * 'P' = Long press (3 sec)
 * ========================================
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
  
  // Quadrature Decoder - Lookup Table
  // Index = (lastState << 2) | newState
  // +1 = sağa, -1 = sola, 0 = geçersiz/gürültü
  static constexpr int8_t ENCODER_TABLE[16] = {
     0, -1,  1,  0,
     1,  0,  0, -1,
    -1,  0,  0,  1,
     0,  1, -1,  0
  };
  
  // Encoder state variables
  volatile uint8_t encoderState;
  volatile int8_t encoderPosition;  // Ara pozisyon sayacı
  static const uint8_t STEPS_PER_DETENT = 4;  // EC11: 4 step = 1 tık
  
  // Debug sayaçları
  volatile int16_t leftCount;   // Sol tık sayısı
  volatile int16_t rightCount;  // Sağ tık sayısı
  volatile bool debugPending;   // Yeni debug çıktısı bekliyor
  
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
  
  // Last direction for tracking
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
    // Her iki pini oku ve yeni state hesapla
    uint8_t clk = digitalRead(clkPin);
    uint8_t dt = digitalRead(dtPin);
    uint8_t newState = (clk << 1) | dt;
    
    // Aynı state ise çık (gürültü filtresi)
    if (newState == encoderState) return;
    
    // Lookup table'dan yön al
    int8_t direction = ENCODER_TABLE[(encoderState << 2) | newState];
    encoderState = newState;
    
    // Pozisyon sayacını güncelle
    encoderPosition += direction;
    
    // STEPS_PER_DETENT adımda bir event üret
    if (encoderPosition >= STEPS_PER_DETENT) {
      addEvent('R');
      // Yön değiştiyse sol sayacı sıfırla
      if (lastDirection != 'R') {
        leftCount = 0;
      }
      lastDirection = 'R';
      rightCount = rightCount + 1;
      debugPending = true;
      encoderPosition = 0;
    } else if (encoderPosition <= -STEPS_PER_DETENT) {
      addEvent('L');
      // Yön değiştiyse sağ sayacı sıfırla
      if (lastDirection != 'L') {
        rightCount = 0;
      }
      lastDirection = 'L';
      leftCount = leftCount + 1;
      debugPending = true;
      encoderPosition = 0;
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
      lastButtonTime(0), buttonPressTime(0),
      bufferWriteIndex(0), bufferReadIndex(0),
      lastDirection(0), buttonWasPressed(false),
      encoderState(0), encoderPosition(0),
      leftCount(0), rightCount(0), debugPending(false) {
    lastButtonState = HIGH;
  }
  
  // Custom constructor - for manual pin assignment
  SKEncoder(uint8_t clk, uint8_t dt, uint8_t sw) 
    : clkPin(clk), dtPin(dt), swPin(sw), 
      lastButtonTime(0), buttonPressTime(0),
      bufferWriteIndex(0), bufferReadIndex(0),
      lastDirection(0), buttonWasPressed(false),
      encoderState(0), encoderPosition(0),
      leftCount(0), rightCount(0), debugPending(false) {
    lastButtonState = HIGH;
  }
  
  void begin() {
    // Set pin modes
    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin, INPUT_PULLUP);
    pinMode(swPin, INPUT_PULLUP);
    
    delay(10);  // Pull-up stabilization
    
    // Başlangıç state'ini oku
    uint8_t clk = digitalRead(clkPin);
    uint8_t dt = digitalRead(dtPin);
    encoderState = (clk << 1) | dt;
    encoderPosition = 0;
    lastButtonState = digitalRead(swPin);
    
    // Set static instance pointer
    instance = this;
    
    // Her iki pine CHANGE interrupt - tüm geçişleri yakala
    attachInterrupt(digitalPinToInterrupt(clkPin), handleEncoderStatic, CHANGE);
    attachInterrupt(digitalPinToInterrupt(dtPin), handleEncoderStatic, CHANGE);
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
          unsigned long pressDuration = currentTime - buttonPressTime;
          
          if (pressDuration >= longPressTime) {
            addEvent('P');
          } else {
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
  
  // Get last direction
  char get_last_direction() {
    return lastDirection;
  }
  
  // Reset encoder state
  void reset() {
    lastDirection = 0;
    clear();
  }
  
  // Debug: Anlık encoder durumunu yazdır (loop'tan çağrılmalı)
  void printDebug() {
    if (debugPending) {
      debugPending = false;
      if (lastDirection == 'L') {
        Serial.printf("[ENC] SOL  <- %d (Sag: %d)\n", leftCount, rightCount);
      } else if (lastDirection == 'R') {
        Serial.printf("[ENC] SAG  -> %d (Sol: %d)\n", rightCount, leftCount);
      }
    }
  }
  
  // Debug sayaçlarını sıfırla
  void resetDebugCounters() {
    leftCount = 0;
    rightCount = 0;
    Serial.println("[ENC] Sayaclar sifirlandi");
  }
  
  // Mevcut pozisyon (ara adım) - debug için
  int8_t getPosition() {
    return encoderPosition;
  }
  
  // Sayaçları al
  int16_t getLeftCount() { return leftCount; }
  int16_t getRightCount() { return rightCount; }
};

// Static member initialization
SKEncoder* SKEncoder::instance = nullptr;
constexpr int8_t SKEncoder::ENCODER_TABLE[16];

#endif // SK_ENCODER_H
