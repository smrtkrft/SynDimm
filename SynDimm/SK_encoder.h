/**
 * SK_encoder.h
 * SmartKraft SynDimm - KY-040 Rotary Encoder Management
 * Version: v1.1.0
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
#include <LittleFS.h>
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
  
  // Debug sayaçları (int8_t for atomic read/write on RISC-V)
  volatile int8_t leftCount;   // Sol tık sayısı (max 127)
  volatile int8_t rightCount;  // Sağ tık sayısı (max 127)
  volatile bool debugPending;   // Yeni debug çıktısı bekliyor
  
  // Button state variables
  bool lastButtonState;
  unsigned long lastButtonTime;
  unsigned long buttonPressTime;
  bool buttonWasPressed;
  bool buttonIsHeld;                            // Buton şu an basılı mı
  bool modeChangeTriggered;                     // Basılı tutarken çevirme yapıldı mı
  const unsigned long buttonDebounceDelay = 50; // 50ms button debounce
  const unsigned long rebootPressTime = 5000;   // 5 seconds = reboot
  const unsigned long factoryResetTime = 10000; // 10 seconds = factory reset
  
  // Long press tracking
  bool rebootWarningShown;
  bool factoryWarningShown;
  
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
      lastDirection(0), buttonWasPressed(false), buttonIsHeld(false), modeChangeTriggered(false),
      encoderState(0), encoderPosition(0),
      leftCount(0), rightCount(0), debugPending(false),
      rebootWarningShown(false), factoryWarningShown(false) {
    lastButtonState = HIGH;
  }
  
  // Custom constructor - for manual pin assignment
  SKEncoder(uint8_t clk, uint8_t dt, uint8_t sw) 
    : clkPin(clk), dtPin(dt), swPin(sw), 
      lastButtonTime(0), buttonPressTime(0),
      bufferWriteIndex(0), bufferReadIndex(0),
      lastDirection(0), buttonWasPressed(false), buttonIsHeld(false), modeChangeTriggered(false),
      encoderState(0), encoderPosition(0),
      leftCount(0), rightCount(0), debugPending(false),
      rebootWarningShown(false), factoryWarningShown(false) {
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
    
    // Update buttonIsHeld state for mode change detection
    buttonIsHeld = (currentButtonState == LOW && buttonWasPressed);
    
    // Button pressed - track duration while held
    if (currentButtonState == LOW && buttonWasPressed) {
      unsigned long holdDuration = currentTime - buttonPressTime;
      
      // Eğer encoder döndürüldüyse (mod değişikliği), reboot/factory reset iptal
      if (modeChangeTriggered) {
        rebootWarningShown = false;
        factoryWarningShown = false;
      }
      // 10 saniye = Factory Reset (en yüksek öncelik)
      else if (holdDuration >= factoryResetTime && !factoryWarningShown) {
        factoryWarningShown = true;
        Serial.println("\n========================================");
        Serial.println("!!! FACTORY RESET HAZIR !!!");
        Serial.println("Butonu birakin - Cihaz sifirlanacak...");
        Serial.println("========================================\n");
      }
      // 5 saniye = Reboot uyarısı
      else if (holdDuration >= rebootPressTime && !rebootWarningShown) {
        rebootWarningShown = true;
        Serial.println("\n[ENCODER] 5sn - REBOOT icin birakin");
        Serial.println("[ENCODER] 10sn'ye kadar tutun = FACTORY RESET");
      }
    }
    
    if (currentButtonState != lastButtonState) {
      if (currentTime - lastButtonTime > buttonDebounceDelay) {
        
        if (currentButtonState == LOW) {
          // Button pressed down
          buttonPressTime = currentTime;
          buttonWasPressed = true;
          buttonIsHeld = true;
          modeChangeTriggered = false;  // Reset mode change flag
          rebootWarningShown = false;
          factoryWarningShown = false;
          lastButtonTime = currentTime;
          
        } else if (currentButtonState == HIGH && buttonWasPressed) {
          unsigned long pressDuration = currentTime - buttonPressTime;
          
          // Mod değişikliği yapıldıysa, reboot/factory reset iptal
          if (modeChangeTriggered) {
            addEvent('M');  // Mode confirmed
          }
          // 10+ saniye = Factory Reset (mod değişikliği yoksa)
          else if (pressDuration >= factoryResetTime) {
            Serial.println("[ENCODER] FACTORY RESET baslatiliyor...");
            performEncoderFactoryReset();
            // Cihaz resetlenecek, buraya dönmeyecek
          }
          // 5-10 saniye = Reboot (mod değişikliği yoksa)
          else if (pressDuration >= rebootPressTime) {
            Serial.println("[ENCODER] REBOOT baslatiliyor...");
            delay(100);
            ESP.restart();
          }
          // Normal kısa basış = Button press (B)
          else {
            addEvent('B');
          }
          
          buttonWasPressed = false;
          buttonIsHeld = false;
          modeChangeTriggered = false;
          rebootWarningShown = false;
          factoryWarningShown = false;
          lastButtonTime = currentTime;
        }
      }
    }
    
    lastButtonState = currentButtonState;
    
    return (bufferReadIndex != bufferWriteIndex);
  }
  
  // Check if button is currently held (for mode change detection)
  bool isButtonHeld() const {
    return buttonIsHeld;
  }
  
  // Mark that mode change was triggered while button held
  void setModeChangeTriggered() {
    modeChangeTriggered = true;
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
  // Encoder'dan Factory Reset - tüm NVS'i siler
  void performEncoderFactoryReset() {
    Serial.println("\n==========================================");
    Serial.println("     ENCODER FACTORY RESET BASLATILDI");
    Serial.println("==========================================");
    
    Preferences prefs;
    
    // 1. Sistem ayarları (first_setup dahil)
    if (prefs.begin(SYSTEM_PREFS_NAMESPACE, false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] System preferences cleared");
    }
    
    // 2. WiFi ayarları
    if (prefs.begin(PREFS_NAMESPACE, false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] WiFi settings cleared");
    }
    
    // 3. Dimmer ayarları
    if (prefs.begin("dimmer-settings", false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] Dimmer settings cleared");
    }
    
    // 4. Shutter ayarları
    if (prefs.begin("shutter", false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] Shutter settings cleared");
    }
    
    // 5. Mode ayarları
    if (prefs.begin("mode_mgr", false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] Mode settings cleared");
    }
    
    // 6. Safe Lock ayarları
    if (prefs.begin("safelock", false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] Safe Lock settings cleared");
    }
    
    // 7. OTA ayarları
    if (prefs.begin("ota-settings", false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] OTA settings cleared");
    }
    
    // 8. Dil ayarları
    if (prefs.begin("lang", false)) {
      prefs.clear();
      prefs.end();
      Serial.println("[RESET] Language settings cleared");
    }
    
    // 9. LittleFS - Safe Mode API config dosyaları
    if (LittleFS.begin(true)) {
      for (int i = 0; i < 5; i++) {
        String path = "/safe/api_" + String(i) + ".json";
        if (LittleFS.exists(path)) {
          LittleFS.remove(path);
          Serial.printf("[RESET] Removed: %s\n", path.c_str());
        }
      }
      Serial.println("[RESET] LittleFS API configs cleared");
    }
    
    Serial.println("==========================================");
    Serial.println("     FACTORY RESET TAMAMLANDI");
    Serial.println("     Cihaz yeniden baslatiliyor...");
    Serial.println("==========================================");
    
    delay(500);
    ESP.restart();
  }};

// Static member initialization
SKEncoder* SKEncoder::instance = nullptr;
constexpr int8_t SKEncoder::ENCODER_TABLE[16];

#endif // SK_ENCODER_H
