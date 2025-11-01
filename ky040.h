/*
 * ========================================
 * KY-040 Rotary Encoder Library
 * ========================================
 * UYARI: BU KÜTÜPHANE ASLA DEĞİŞTİRİLMEYECEK!
 * Yeni özellikler için ayrı modül oluşturun.
 * ========================================
 * 
 * Hassas encoder okuma kütüphanesi
 * Sol tik = 'L', Sağ tik = 'R', Buton basma = 'B'
 */

#ifndef KY040_H
#define KY040_H

#include <Arduino.h>

class KY040 {
private:
  // Pin tanımlamaları
  uint8_t clkPin;
  uint8_t dtPin;
  uint8_t swPin;
  
  // Encoder state değişkenleri
  volatile bool aState;
  volatile bool aLastState;
  volatile unsigned long lastInterruptTime;
  const unsigned long debounceDelay = 1; // 1ms debounce - hassas okuma
  
  // Buton state değişkenleri
  bool lastButtonState;
  unsigned long lastButtonTime;
  const unsigned long buttonDebounceDelay = 50; // 50ms buton debounce
  
  // Event buffer
  volatile char eventBuffer[10];
  volatile uint8_t bufferWriteIndex;
  volatile uint8_t bufferReadIndex;
  
  // Matematiksel sayaçlar - SADECE dimm_sayac (0-100)
  volatile int dimm_sayac;        // 0-100 arası dimmer değeri
  volatile char lastDirection;    // Son yön ('L' veya 'R') - sadece tracking için
  
  // Static pointer for interrupt handler
  static KY040* instance;
  
  // Interrupt handler
  static void IRAM_ATTR handleEncoderStatic() {
    if (instance) {
      instance->handleEncoder();
    }
  }
  
  void IRAM_ATTR handleEncoder() {
    unsigned long currentTime = millis();
    
    // Debounce kontrolü
    if (currentTime - lastInterruptTime < debounceDelay) {
      return;
    }
    
    // CLK (A) ve DT (B) pinlerini oku
    aState = digitalRead(clkPin);
    bool bState = digitalRead(dtPin);
    
    // CLK değiştiğinde kontrol et
    if (aState != aLastState) {
      // Yön belirleme: ATAMALAR TERS ÇEVRİLDİ (L ↔ R)
      if (aState == bState) {
        // Sol dönüş (önceden R idi)
        addEvent('L');
        updateCounters('L');
      } else {
        // Sağ dönüş (önceden L idi)
        addEvent('R');
        updateCounters('R');
      }
      
      lastInterruptTime = currentTime;
      aLastState = aState;  // CRITICAL: State'i güncelle
    }
  }
  
  void updateCounters(char direction) {
    // Matematiksel işlemler modes.h'a taşındı
    // Burası artık kullanılmıyor - sadece event buffer çalışıyor
    lastDirection = direction;
  }
  
  void addEvent(char event) {
    // Buffer'a event ekle
    uint8_t nextIndex = (bufferWriteIndex + 1) % 10;
    if (nextIndex != bufferReadIndex) {
      eventBuffer[bufferWriteIndex] = event;
      bufferWriteIndex = nextIndex;
    }
  }
  
public:
  KY040(uint8_t clk, uint8_t dt, uint8_t sw) 
    : clkPin(clk), dtPin(dt), swPin(sw), 
      lastInterruptTime(0), lastButtonTime(0),
      bufferWriteIndex(0), bufferReadIndex(0),
      dimm_sayac(100), lastDirection(0) {
    lastButtonState = HIGH;
  }
  
  void begin() {
    // Pin modlarını ayarla
    pinMode(clkPin, INPUT_PULLUP);
    pinMode(dtPin, INPUT_PULLUP);
    pinMode(swPin, INPUT_PULLUP);
    
    // Pull-up stabilizasyonu için KISA bekle
    delay(10);
    
    // Başlangıç durumlarını DOĞRU oku
    aState = digitalRead(clkPin);
    aLastState = aState;
    lastButtonState = digitalRead(swPin);
    
    // Static instance pointer'ı ayarla
    instance = this;
    
    // Sadece CLK pinine interrupt ekle - CHANGE modu (eski gibi)
    attachInterrupt(digitalPinToInterrupt(clkPin), handleEncoderStatic, CHANGE);
  }
  
  // Event olup olmadığını kontrol et
  bool available() {
    // Önce encoder event'leri kontrol et
    if (bufferReadIndex != bufferWriteIndex) {
      return true;
    }
    
    // Buton kontrolü
    bool currentButtonState = digitalRead(swPin);
    if (currentButtonState != lastButtonState) {
      unsigned long currentTime = millis();
      if (currentTime - lastButtonTime > buttonDebounceDelay) {
        if (currentButtonState == LOW) {
          // Buton basıldı
          addEvent('B');
          lastButtonTime = currentTime;
        }
      }
    }
    lastButtonState = currentButtonState;
    
    return (bufferReadIndex != bufferWriteIndex);
  }
  
  // Bir sonraki eventi oku (L, R, veya B)
  char read() {
    if (bufferReadIndex != bufferWriteIndex) {
      char event = eventBuffer[bufferReadIndex];
      bufferReadIndex = (bufferReadIndex + 1) % 10;
      return event;
    }
    return 0; // Event yok
  }
  
  // Tüm buffer'ı temizle
  void clear() {
    bufferReadIndex = bufferWriteIndex;
  }
  
  // ========== MATEMATİKSEL FONKSİYONLAR ==========
  
  // dimm_sayac fonksiyonu (0-100 arası net pozisyon)
  int get_dimm_sayac() {
    return dimm_sayac;
  }
  
  // dimm_sayac'ı manuel ayarla
  void set_dimm_sayac(int value) {
    if (value >= 0 && value <= 100) {
      dimm_sayac = value;
    }
  }
  
  // Son yönü döndür ('L' veya 'R')
  char get_last_direction() {
    return lastDirection;
  }
  
  // Tüm sayaçları sıfırla
  void reset_all() {
    dimm_sayac = 100;  // 100'den başlat
    lastDirection = 0;
  }
};

// Static member initialization
KY040* KY040::instance = nullptr;

#endif // KY040_H
