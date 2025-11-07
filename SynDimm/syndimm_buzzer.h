/*
 * SynDimm Buzzer System
 * GPIO 17 - ESP32C6
 * 
 * Non-blocking buzzer kontrolü
 * Ses pattern'leri: Mod değişimi, buton feedback, WiFi durumu, API durumu
 */

#ifndef SYNDIMM_BUZZER_H
#define SYNDIMM_BUZZER_H

#include <Arduino.h>

class SynDimmBuzzer {
private:
  const uint8_t BUZZER_PIN = 17;
  
  // Pattern state
  enum Pattern {
    NONE = 0,
    SHORT_BEEP,           // Tek kısa bip - Buton basma
    DOUBLE_BEEP,          // Çift kısa bip - Safe mod
    TRIPLE_BEEP,          // 3 kısa bip - Panic mod
    LONG_BEEP,            // Tek uzun bip - Mod değişimi
    SUCCESS_BEEP,         // 2 kısa + 1 uzun - Şifre doğru
    ERROR_BEEP,           // 1 çok uzun - Hata
    WIFI_CONNECTING,      // 3 uzun bip - WiFi bağlanıyor
    API_SUCCESS,          // 2 kısa + 1 uzun - API başarılı
    API_FAIL              // 5 kısa bip - API başarısız
  };
  
  Pattern currentPattern;
  uint8_t patternStep;
  unsigned long stepStartTime;
  bool isActive;
  
  // Timing (ms)
  static const uint16_t SHORT = 80;
  static const uint16_t LONG = 300;
  static const uint16_t VERY_LONG = 500;
  static const uint16_t PAUSE_SHORT = 100;
  static const uint16_t PAUSE_LONG = 200;
  
  // PWM ayarları
  static const uint16_t FREQ = 2000;  // 2kHz
  static const uint8_t CHANNEL = 0;
  static const uint8_t RESOLUTION = 8;
  
  void beepOn() {
    ledcWrite(BUZZER_PIN, 128);  // 50% duty cycle
    isActive = true;
  }
  
  void beepOff() {
    ledcWrite(BUZZER_PIN, 0);
    isActive = false;
  }
  
  // Pattern çalıştırma mantığı
  bool executePattern() {
    if (currentPattern == NONE) return false;
    
    unsigned long now = millis();
    unsigned long elapsed = now - stepStartTime;
    
    // Pattern tanımları
    switch (currentPattern) {
      case SHORT_BEEP:
        if (patternStep == 0 && elapsed < SHORT) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case DOUBLE_BEEP:
        if (patternStep == 0 && elapsed < SHORT) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 1 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 1) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 2 && elapsed < SHORT) return true;
        if (patternStep == 2) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case TRIPLE_BEEP:
        if (patternStep == 0 && elapsed < SHORT) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 1 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 1) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 2 && elapsed < SHORT) return true;
        if (patternStep == 2) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 3 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 3) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 4 && elapsed < SHORT) return true;
        if (patternStep == 4) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case LONG_BEEP:
        if (patternStep == 0 && elapsed < LONG) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case SUCCESS_BEEP:
        // 2 kısa + 1 uzun
        if (patternStep == 0 && elapsed < SHORT) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 1 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 1) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 2 && elapsed < SHORT) return true;
        if (patternStep == 2) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 3 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 3) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 4 && elapsed < LONG) return true;
        if (patternStep == 4) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case ERROR_BEEP:
        if (patternStep == 0 && elapsed < VERY_LONG) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case WIFI_CONNECTING:
        // 3 uzun bip
        if (patternStep == 0 && elapsed < LONG) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 1 && elapsed < PAUSE_LONG) return true;
        if (patternStep == 1) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 2 && elapsed < LONG) return true;
        if (patternStep == 2) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 3 && elapsed < PAUSE_LONG) return true;
        if (patternStep == 3) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 4 && elapsed < LONG) return true;
        if (patternStep == 4) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case API_SUCCESS:
        // 2 kısa + 1 uzun (SUCCESS_BEEP ile aynı)
        if (patternStep == 0 && elapsed < SHORT) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 1 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 1) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 2 && elapsed < SHORT) return true;
        if (patternStep == 2) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 3 && elapsed < PAUSE_SHORT) return true;
        if (patternStep == 3) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 4 && elapsed < LONG) return true;
        if (patternStep == 4) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      case API_FAIL:
        // 5 kısa bip (step 0,1,2,3,4,5,6,7,8,9)
        // 0: beep, 1: pause, 2: beep, 3: pause, 4: beep, 5: pause, 6: beep, 7: pause, 8: beep, 9: done
        if (patternStep == 0 && elapsed < SHORT) return true;
        if (patternStep == 0) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 1 && elapsed < PAUSE_LONG) return true;
        if (patternStep == 1) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 2 && elapsed < SHORT) return true;
        if (patternStep == 2) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 3 && elapsed < PAUSE_LONG) return true;
        if (patternStep == 3) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 4 && elapsed < SHORT) return true;
        if (patternStep == 4) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 5 && elapsed < PAUSE_LONG) return true;
        if (patternStep == 5) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 6 && elapsed < SHORT) return true;
        if (patternStep == 6) { beepOff(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 7 && elapsed < PAUSE_LONG) return true;
        if (patternStep == 7) { beepOn(); patternStep++; stepStartTime = now; return true; }
        if (patternStep == 8 && elapsed < SHORT) return true;
        if (patternStep == 8) { beepOff(); patternStep++; stepStartTime = now; return true; }
        return false;
        
      default:
        return false;
    }
  }
  
public:
  SynDimmBuzzer() : currentPattern(NONE), patternStep(0), stepStartTime(0), isActive(false) {}
  
  void begin() {
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, LOW);
    
    // ESP32C6 LEDC setup (yeni API)
    ledcAttach(BUZZER_PIN, FREQ, RESOLUTION);
    ledcWrite(BUZZER_PIN, 0);
    
    Serial.println("Buzzer OK (GPIO 17, 2kHz PWM)");
  }
  
  // Loop içinde çağrılacak
  void update() {
    if (currentPattern == NONE) return;
    
    if (!executePattern()) {
      // Pattern tamamlandı
      beepOff();
      currentPattern = NONE;
    }
  }
  
  // Pattern çalma fonksiyonları
  void playShort() {
    currentPattern = SHORT_BEEP;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playDouble() {
    currentPattern = DOUBLE_BEEP;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playTriple() {
    currentPattern = TRIPLE_BEEP;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playLong() {
    currentPattern = LONG_BEEP;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playSuccess() {
    currentPattern = SUCCESS_BEEP;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playError() {
    currentPattern = ERROR_BEEP;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playWiFiConnecting() {
    currentPattern = WIFI_CONNECTING;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playApiSuccess() {
    currentPattern = API_SUCCESS;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  void playApiFail() {
    currentPattern = API_FAIL;
    patternStep = 0;
    stepStartTime = millis();
    beepOn();
  }
  
  bool isPlaying() {
    return currentPattern != NONE;
  }
  
  void stop() {
    beepOff();
    currentPattern = NONE;
  }
  
  // Test fonksiyonu (blocking)
  void test() {
    Serial.println("Buzzer test: 3 short beeps");
    for (int i = 0; i < 3; i++) {
      beepOn();
      delay(SHORT);
      beepOff();
      delay(PAUSE_SHORT);
    }
  }
};

#endif
