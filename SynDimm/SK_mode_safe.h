/**
 * SK_mode_safe.h
 * SmartKraft SynDimm - Safe Lock Mode
 * Version: v1.1.1
 * 
 * ========================================
 * SAFE MOD - KASA KİLİDİ MANTİĞI
 * ========================================
 * SK_encoder.h'den gelen L/R/B verilerini işleyerek şifre sistemi oluşturur.
 * - 5 farklı şifre desteği
 * - Her şifre 3-6 adım uzunluğunda (örn: R5-L3-R2-B)
 * - EEPROM'da kalıcı saklama
 * - Buzzer feedback (syndimm_buzzer.h entegrasyonu)
 * - Şifre eşleşince callback tetiklenir
 * 
 * Mimari:
 * SK_encoder (ham L/R/B) → SK_mode_safe (şifre kontrolü) → Callback (API tetikleme)
 * ========================================
 */

#ifndef SK_MODE_SAFE_H
#define SK_MODE_SAFE_H

#include <Arduino.h>
#include <EEPROM.h>
#include <Preferences.h>
#include "SK_config.h"

// ==================== YAPILANDIRMA ====================
#define SAFE_MAX_PASSWORDS 5            // Maksimum 5 şifre
#define SAFE_MAX_PASSWORD_STEPS 6       // Maksimum 6 adım
#define SAFE_MIN_PASSWORD_STEPS 3       // Minimum 3 adım
#define SAFE_MOVEMENT_BUFFER_SIZE 8     // Son 8 hareketi tut (sliding window)
#define SAFE_MAX_TICKS_PER_STEP 50      // Her adımda maksimum 50 tık
#define SAFE_MIN_TICKS_PER_STEP 1       // Her adımda minimum 1 tık

// API Konfigürasyon Limitleri
#define SAFE_API_URL_MAX 128            // Maksimum URL uzunluğu
#define SAFE_API_HEADER_MAX 64          // Maksimum header uzunluğu
#define SAFE_API_BODY_MAX 128           // Maksimum body uzunluğu

// EEPROM Magic Number (validasyon için)
#define SAFE_EEPROM_MAGIC 0x5AFE        // "SAFE" benzeri hex değer

// ==================== VERI YAPILARI ====================

// HTTP Method
enum SafeHttpMethod {
  SAFE_HTTP_GET = 0,
  SAFE_HTTP_POST = 1
};

// Hareket yönü (SK_encoder.h ile uyumlu)
enum SafeDirection {
  SAFE_DIR_LEFT = 'L',
  SAFE_DIR_RIGHT = 'R',
  SAFE_DIR_BUTTON = 'B'
};

// Tek bir şifre adımı
struct __attribute__((packed)) SafePasswordStep {
  char direction;   // L, R veya B
  uint8_t ticks;    // Tık sayısı (B için 0)
  
  SafePasswordStep() : direction(0), ticks(0) {}
  SafePasswordStep(char dir, uint8_t t) : direction(dir), ticks(t) {}
  
  bool isValid() const {
    return (direction == 'L' || direction == 'R' || direction == 'B');
  }
  
  bool equals(const SafePasswordStep& other) const {
    if (direction == 'B' && other.direction == 'B') return true;
    return (direction == other.direction && ticks == other.ticks);
  }
};

// Tek bir hareket (buffer'da saklanır)
struct __attribute__((packed)) SafeMovement {
  char direction;   // L veya R (B ayrı işlenir)
  uint8_t ticks;    // Tık sayısı
  
  SafeMovement() : direction(0), ticks(0) {}
  SafeMovement(char dir, uint8_t t) : direction(dir), ticks(t) {}
};

// Şifre yapısı
struct __attribute__((packed)) SafePassword {
  SafePasswordStep steps[SAFE_MAX_PASSWORD_STEPS];  // Şifre adımları
  uint8_t stepCount;                                // Toplam adım sayısı
  bool requireButton;                               // Son adımda B gerekli mi?
  bool isActive;                                    // Şifre aktif mi?
  
  SafePassword() : stepCount(0), requireButton(false), isActive(false) {
    for (int i = 0; i < SAFE_MAX_PASSWORD_STEPS; i++) {
      steps[i] = SafePasswordStep();
    }
  }
  
  bool isValid() const {
    if (!isActive) return false;
    if (stepCount < SAFE_MIN_PASSWORD_STEPS || stepCount > SAFE_MAX_PASSWORD_STEPS) return false;
    
    for (uint8_t i = 0; i < stepCount; i++) {
      if (!steps[i].isValid()) return false;
      if (steps[i].direction != 'B') {
        if (steps[i].ticks < SAFE_MIN_TICKS_PER_STEP || steps[i].ticks > SAFE_MAX_TICKS_PER_STEP) {
          return false;
        }
      }
    }
    return true;
  }
  
  // Şifreyi string olarak al (L3-R12-L11-R3-B)
  String toString() const {
    String result = "";
    for (uint8_t i = 0; i < stepCount; i++) {
      if (i > 0) result += "-";
      result += String(steps[i].direction);
      if (steps[i].direction != 'B') {
        result += String(steps[i].ticks);
      }
    }
    return result;
  }
  
  // String'den şifre oluştur
  bool fromString(const String& str) {
    stepCount = 0;
    requireButton = false;
    
    int startIdx = 0;
    for (int i = 0; i <= str.length(); i++) {
      if (i == str.length() || str[i] == '-') {
        if (i > startIdx) {
          String token = str.substring(startIdx, i);
          if (token.length() == 0) return false;
          
          char dir = token[0];
          if (dir != 'L' && dir != 'R' && dir != 'B') return false;
          
          if (dir == 'B') {
            steps[stepCount] = SafePasswordStep('B', 0);
            requireButton = true;
          } else {
            String numStr = token.substring(1);
            if (numStr.length() == 0) return false;
            int ticks = numStr.toInt();
            if (ticks < SAFE_MIN_TICKS_PER_STEP || ticks > SAFE_MAX_TICKS_PER_STEP) return false;
            steps[stepCount] = SafePasswordStep(dir, ticks);
          }
          
          stepCount++;
          if (stepCount > SAFE_MAX_PASSWORD_STEPS) return false;
        }
        startIdx = i + 1;
      }
    }
    
    return (stepCount >= SAFE_MIN_PASSWORD_STEPS && stepCount <= SAFE_MAX_PASSWORD_STEPS);
  }
};

// API Konfigürasyonu (her şifre için)
struct __attribute__((packed)) SafeApiConfig {
  char url[SAFE_API_URL_MAX];           // API endpoint URL
  SafeHttpMethod method;                // GET veya POST
  char header[SAFE_API_HEADER_MAX];     // Custom header (örn: "X-API-Key: abc123")
  char body[SAFE_API_BODY_MAX];         // POST body (JSON)
  bool enabled;                         // API aktif mi?
  
  SafeApiConfig() : method(SAFE_HTTP_GET), enabled(false) {
    memset(url, 0, SAFE_API_URL_MAX);
    memset(header, 0, SAFE_API_HEADER_MAX);
    memset(body, 0, SAFE_API_BODY_MAX);
  }
  
  bool isValid() const {
    if (!enabled) return false;
    if (strlen(url) == 0) return false;
    return true;
  }
};

// EEPROM Konfigürasyonu
struct __attribute__((packed)) SafeEEPROMConfig {
  uint16_t magicNumber;                              // Validasyon için
  SafePassword passwords[SAFE_MAX_PASSWORDS];        // 5 şifre
  SafeApiConfig apiConfigs[SAFE_MAX_PASSWORDS];      // Her şifre için API config
  uint8_t checksum;                                  // Veri bütünlüğü kontrolü
  
  SafeEEPROMConfig() : magicNumber(SAFE_EEPROM_MAGIC), checksum(0) {}
  
  // Checksum hesapla
  uint8_t calculateChecksum() const {
    uint8_t sum = 0;
    const uint8_t* data = (const uint8_t*)this;
    size_t size = sizeof(SafeEEPROMConfig) - sizeof(checksum);
    
    for (size_t i = 0; i < size; i++) {
      sum ^= data[i]; // XOR checksum
    }
    return sum;
  }
  
  // Checksum'ı güncelle
  void updateChecksum() {
    checksum = calculateChecksum();
  }
  
  // Checksum geçerli mi?
  bool isChecksumValid() const {
    return (checksum == calculateChecksum());
  }
  
  // Konfigürasyon geçerli mi?
  bool isValid() const {
    return (magicNumber == SAFE_EEPROM_MAGIC && isChecksumValid());
  }
};

// ==================== SAFE LOCK SINIFI ====================

class SafeLock {
private:
  // Konfigürasyon
  SafeEEPROMConfig config;
  bool initialized;
  Preferences safePrefs;
  
  // Sliding window buffer - son 8 hareket
  SafeMovement moveBuffer[SAFE_MOVEMENT_BUFFER_SIZE];
  uint8_t bufferCount;       // Buffer'daki eleman sayısı (0-8)
  
  // Geçici hareket sayacı
  char currentDirection;     // Şu anki hareket yönü
  uint8_t currentTicks;      // Şu anki tık sayısı
  bool movementStarted;      // Hareket başladı mı?
  
  // Callback fonksiyonu (şifre eşleştiğinde)
  void (*onPasswordMatchCallback)(uint8_t passwordIndex);
  
  // Buffer'a hareket ekle (sliding window)
  void addMovementToBuffer(char direction, uint8_t ticks) {
    if (ticks == 0) return;
    
    // Buffer doluysa, en eskiyi sil (kaydır)
    if (bufferCount >= SAFE_MOVEMENT_BUFFER_SIZE) {
      for (uint8_t i = 0; i < SAFE_MOVEMENT_BUFFER_SIZE - 1; i++) {
        moveBuffer[i] = moveBuffer[i + 1];
      }
      moveBuffer[SAFE_MOVEMENT_BUFFER_SIZE - 1] = SafeMovement(direction, ticks);
    } else {
      moveBuffer[bufferCount] = SafeMovement(direction, ticks);
      bufferCount++;
    }
    
    // Her ekleme sonrası buffer durumunu yazdır
    printBufferState();
  }
  
  // Buffer durumunu Serial'a yazdır
  void printBufferState() {
    DEBUG_PRINT("[SafeLock] ");
    for (uint8_t i = 0; i < bufferCount; i++) {
      if (i > 0) DEBUG_PRINT("-");
      DEBUG_PRINT(moveBuffer[i].direction);
      DEBUG_PRINT(moveBuffer[i].ticks);
    }
    DEBUG_PRINTLN();
  }
  
  // Buffer'dan belirli indeksten başlayarak N hareket al
  bool getMovementsFromIndex(SafeMovement* output, uint8_t startIdx, uint8_t count) {
    if (startIdx + count > bufferCount) return false;
    for (uint8_t i = 0; i < count; i++) {
      output[i] = moveBuffer[startIdx + i];
    }
    return true;
  }
  
  // Hareketleri şifre adımları ile karşılaştır (B hariç)
  bool matchMovements(SafeMovement* movements, uint8_t movCount, const SafePassword& pwd) {
    // Adım sayısı kontrolü (B varsa -1)
    uint8_t expectedMoves = pwd.stepCount;
    if (pwd.requireButton) expectedMoves--;
    
    if (movCount != expectedMoves) return false;
    
    // Her adımı karşılaştır
    for (uint8_t i = 0; i < expectedMoves; i++) {
      if (movements[i].direction != pwd.steps[i].direction) return false;
      if (movements[i].ticks != pwd.steps[i].ticks) return false;
    }
    return true;
  }
  
  // Tüm şifreleri tüm alt dizilerde ara (sliding window match)
  // includeCurrentMove: true ise mevcut devam eden hareketi de dahil et
  void checkAllSubsequences(bool buttonPressed, bool includeCurrentMove = false) {
    SafeMovement tempMoves[SAFE_MAX_PASSWORD_STEPS];
    
    for (uint8_t pwdIdx = 0; pwdIdx < SAFE_MAX_PASSWORDS; pwdIdx++) {
      if (!config.passwords[pwdIdx].isValid()) continue;
      
      // B ile biten şifre sadece buton basılınca kontrol edilir
      if (config.passwords[pwdIdx].requireButton && !buttonPressed) continue;
      
      uint8_t moveCount = config.passwords[pwdIdx].stepCount;
      if (config.passwords[pwdIdx].requireButton) moveCount--;
      
      // Mevcut hareketi dahil edecek miyiz?
      uint8_t effectiveBufferCount = bufferCount;
      if (includeCurrentMove && movementStarted && currentTicks > 0) {
        effectiveBufferCount++;
      }
      
      // Buffer'da yeterli hareket var mı?
      if (effectiveBufferCount < moveCount) continue;
      
      // Tüm olası başlangıç noktalarını dene
      for (uint8_t startIdx = 0; startIdx <= effectiveBufferCount - moveCount; startIdx++) {
        // Geçici buffer oluştur
        uint8_t tempIdx = 0;
        bool valid = true;
        
        for (uint8_t i = 0; i < moveCount && valid; i++) {
          uint8_t srcIdx = startIdx + i;
          if (srcIdx < bufferCount) {
            tempMoves[tempIdx++] = moveBuffer[srcIdx];
          } else if (includeCurrentMove && movementStarted) {
            // Son eleman olarak mevcut hareketi kullan
            tempMoves[tempIdx++] = SafeMovement(currentDirection, currentTicks);
          } else {
            valid = false;
          }
        }
        
        if (valid && matchMovements(tempMoves, moveCount, config.passwords[pwdIdx])) {
          DEBUG_PRINTF("[SafeLock] >>> SIFRE #%d ESLESTI! <<<\n", pwdIdx);
          onPasswordMatched(pwdIdx);
          return;
        }
      }
    }
  }
  
  // Şifre eşleştiğinde çağrılır
  void onPasswordMatched(uint8_t passwordIndex) {
    // Buffer'ı tamamen temizle
    clearBuffer();
    resetCurrentMovement();
    
    // Callback çağır
    if (onPasswordMatchCallback != nullptr) {
      onPasswordMatchCallback(passwordIndex);
    }
  }
  
public:
  SafeLock() : initialized(false), bufferCount(0), 
               currentDirection(0), currentTicks(0), movementStarted(false),
               onPasswordMatchCallback(nullptr) {}
  
  // Başlat (Preferences'dan yükle)
  void begin() {
    DEBUG_PRINTLN("[SafeLock] Baslatiliyor...");
    
    safePrefs.begin("safelock", false);
    
    // Her şifre için ayarları yükle
    for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
      String keyPwd = "pwd" + String(i);
      String keyActive = "active" + String(i);
      String keyApiEnabled = "apiEn" + String(i);
      String keyApiUrl = "apiUrl" + String(i);
      String keyApiHeader = "apiHdr" + String(i);
      String keyApiMethod = "apiMth" + String(i);
      
      String pwdStr = safePrefs.getString(keyPwd.c_str(), "");
      config.passwords[i].isActive = safePrefs.getBool(keyActive.c_str(), false);
      config.apiConfigs[i].enabled = safePrefs.getBool(keyApiEnabled.c_str(), false);
      
      String url = safePrefs.getString(keyApiUrl.c_str(), "");
      String header = safePrefs.getString(keyApiHeader.c_str(), "");
      url.toCharArray(config.apiConfigs[i].url, SAFE_API_URL_MAX);
      header.toCharArray(config.apiConfigs[i].header, SAFE_API_HEADER_MAX);
      config.apiConfigs[i].method = (SafeHttpMethod)safePrefs.getInt(keyApiMethod.c_str(), SAFE_HTTP_GET);
      
      // Şifreyi parse et
      if (pwdStr.length() > 0) {
        config.passwords[i].fromString(pwdStr);
      }
      
      if (config.passwords[i].isActive || pwdStr.length() > 0) {
        // GÜVENLİK: Şifre ve URL Serial'e loglanmıyor
        DEBUG_PRINTF("[SafeLock] Slot #%d: configured (active=%d, apiEnabled=%d)\n", 
                     i, config.passwords[i].isActive, config.apiConfigs[i].enabled);
      }
    }
    
    initialized = true;
    clearBuffer();
    resetCurrentMovement();
    DEBUG_PRINTLN("[SafeLock] Baslatma tamamlandi");
  }
  
  // Preferences'a kaydet
  bool saveToEEPROM() {
    if (!initialized) return false;
    
    for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
      String keyPwd = "pwd" + String(i);
      String keyActive = "active" + String(i);
      String keyApiEnabled = "apiEn" + String(i);
      String keyApiUrl = "apiUrl" + String(i);
      String keyApiHeader = "apiHdr" + String(i);
      String keyApiMethod = "apiMth" + String(i);
      
      safePrefs.putString(keyPwd.c_str(), config.passwords[i].toString());
      safePrefs.putBool(keyActive.c_str(), config.passwords[i].isActive);
      safePrefs.putBool(keyApiEnabled.c_str(), config.apiConfigs[i].enabled);
      safePrefs.putString(keyApiUrl.c_str(), String(config.apiConfigs[i].url));
      safePrefs.putString(keyApiHeader.c_str(), String(config.apiConfigs[i].header));
      safePrefs.putInt(keyApiMethod.c_str(), config.apiConfigs[i].method);
    }
    
    DEBUG_PRINTLN("[SafeLock] Preferences'a kaydedildi");
    return true;
  }
  
  // Buffer'ı temizle
  void clearBuffer() {
    bufferCount = 0;
    for (int i = 0; i < SAFE_MOVEMENT_BUFFER_SIZE; i++) {
      moveBuffer[i] = SafeMovement();
    }
  }
  
  // Mevcut hareketi sıfırla
  void resetCurrentMovement() {
    currentDirection = 0;
    currentTicks = 0;
    movementStarted = false;
  }
  
  // SK_encoder'dan hareket geldiğinde çağrılır (L veya R)
  void onEncoderMove(char direction, uint8_t ticks = 1) {
    if (direction != 'L' && direction != 'R') return;
    
    if (!movementStarted) {
      // İlk hareket
      currentDirection = direction;
      currentTicks = ticks;
      movementStarted = true;
    } else if (currentDirection == direction) {
      // Aynı yönde devam
      currentTicks += ticks;
      if (currentTicks > SAFE_MAX_TICKS_PER_STEP) {
        currentTicks = SAFE_MAX_TICKS_PER_STEP;
      }
    } else {
      // Yön değişti - önceki hareketi buffer'a ekle
      addMovementToBuffer(currentDirection, currentTicks);
      
      // Yeni harekete başla
      currentDirection = direction;
      currentTicks = ticks;
    }
    
    // Her harekette mevcut durumu dahil ederek kontrol et (B'siz şifreler için)
    checkAllSubsequences(false, true);
  }
  
  // SK_encoder'dan buton basımı geldiğinde çağrılır (B)
  void onEncoderButton() {
    DEBUG_PRINTLN("[SafeLock] BUTON");
    
    if (movementStarted && currentTicks > 0) {
      // Mevcut hareketi buffer'a ekle
      addMovementToBuffer(currentDirection, currentTicks);
      resetCurrentMovement();
    }
    
    // B ile biten şifreleri kontrol et
    checkAllSubsequences(true);
  }
  
  // Şifre ayarla (web arayüzünden)
  bool setPassword(uint8_t index, const String& passwordStr, const SafeApiConfig& apiConfig, bool isActive = true) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    
    // GÜVENLİK: Şifre Serial'e loglanmıyor
    DEBUG_PRINTF("[SafeLock] setPassword: index=%d, active=%d\n", index, isActive);
    
    // Boş şifre gelirse sadece API config ve active durumunu güncelle
    if (passwordStr.length() == 0) {
      config.passwords[index].isActive = isActive;
      config.apiConfigs[index] = apiConfig;
      DEBUG_PRINTF("[SafeLock] Sifre #%d sadece durum guncellendi: active=%d\n", index, isActive);
      return saveToEEPROM();
    }
    
    SafePassword newPwd;
    if (!newPwd.fromString(passwordStr)) {
      DEBUG_PRINTLN("[SafeLock] Gecersiz sifre formati!");
      return false;
    }
    
    config.passwords[index] = newPwd;
    config.passwords[index].isActive = isActive;
    config.apiConfigs[index] = apiConfig;
    
    // GÜVENLİK: Şifre ve URL Serial'e loglanmıyor
    DEBUG_PRINTF("[SafeLock] Slot #%d configured (active=%d)\n", index, isActive);
    DEBUG_PRINTF("[SafeLock] API enabled: %d\n", apiConfig.enabled);
    
    return saveToEEPROM();
  }
  
  // Şifre al
  String getPassword(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return "";
    return config.passwords[index].toString();
  }
  
  // Şifreyi aktif/pasif yap
  bool setPasswordActive(uint8_t index, bool active) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    
    config.passwords[index].isActive = active;
    return saveToEEPROM();
  }
  
  // Şifre aktif mi?
  bool isPasswordActive(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    return config.passwords[index].isActive;
  }
  
  // API konfigürasyonu al
  SafeApiConfig getApiConfig(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return SafeApiConfig();
    return config.apiConfigs[index];
  }
  
  // Callback ayarla
  void setPasswordMatchCallback(void (*callback)(uint8_t)) {
    onPasswordMatchCallback = callback;
  }
};

#endif // SK_MODE_SAFE_H
