/**
 * SK_mode_safe.h
 * SmartKraft SynDimm - Safe Lock Mode
 * Version: v1.3.0
 * 
 * ========================================
 * SAFE MOD - KASA KİLİDİ MANTİĞI
 * ========================================
 * SK_encoder.h'den gelen L/R/B verilerini işleyerek şifre sistemi oluşturur.
 * - 5 farklı şifre desteği
 * - Her şifre 3-6 adım uzunluğunda (örn: R5-L3-R2-B)
 * - Şifre pattern'leri NVS'te (küçük), API config'ler LittleFS'te (sınırsız)
 * - Buzzer feedback (syndimm_buzzer.h entegrasyonu)
 * - Şifre eşleşince lazy-loading ile API config okunur
 * 
 * Mimari:
 * SK_encoder (ham L/R/B) → SK_mode_safe (şifre kontrolü) → Callback (API tetikleme)
 * 
 * v1.3.0 - LittleFS tabanlı sınırsız API config desteği
 * ========================================
 */

#ifndef SK_MODE_SAFE_H
#define SK_MODE_SAFE_H

#include <Arduino.h>
#include <Preferences.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include "SK_config.h"

// ==================== YAPILANDIRMA ====================
#define SAFE_MAX_PASSWORDS 5            // Maksimum 5 şifre
#define SAFE_MAX_PASSWORD_STEPS 6       // Maksimum 6 adım
#define SAFE_MIN_PASSWORD_STEPS 3       // Minimum 3 adım
#define SAFE_MOVEMENT_BUFFER_SIZE 8     // Son 8 hareketi tut (sliding window)
#define SAFE_MAX_TICKS_PER_STEP 50      // Her adımda maksimum 50 tık
#define SAFE_MIN_TICKS_PER_STEP 1       // Her adımda minimum 1 tık

// LittleFS API Config Dizini
#define SAFE_API_CONFIG_DIR "/safe"
#define SAFE_API_CONFIG_PREFIX "/safe/api_"
#define SAFE_API_CONFIG_SUFFIX ".json"

// ==================== VERI YAPILARI ====================

// HTTP Method
enum SafeHttpMethod {
  SAFE_HTTP_GET = 0,
  SAFE_HTTP_POST = 1,
  SAFE_HTTP_PUT = 2,
  SAFE_HTTP_DELETE = 3
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
  bool hasApiConfig;                                // LittleFS'te API config var mı?
  
  SafePassword() : stepCount(0), requireButton(false), isActive(false), hasApiConfig(false) {
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

// API Konfigürasyonu - LittleFS'ten lazy-load edilir (sınırsız boyut)
struct SafeApiConfig {
  String url;                           // API endpoint URL (sınırsız)
  SafeHttpMethod method;                // GET, POST, PUT, DELETE
  String contentType;                   // Content-Type header
  String authorization;                 // Authorization header (Bearer token vb.)
  String customHeaders;                 // Ek headerlar (satır satır: "X-Key: value\nX-Key2: value2")
  String body;                          // Request body (JSON - sınırsız)
  bool enabled;                         // API aktif mi?
  
  SafeApiConfig() : method(SAFE_HTTP_GET), contentType("application/json"), enabled(false) {}
  
  bool isValid() const {
    if (!enabled) return false;
    if (url.length() == 0) return false;
    return true;
  }
  
  // JSON'a çevir
  String toJson() const {
    JsonDocument doc;
    doc["enabled"] = enabled;
    doc["method"] = (int)method;
    doc["url"] = url;
    doc["contentType"] = contentType;
    doc["authorization"] = authorization;
    doc["customHeaders"] = customHeaders;
    doc["body"] = body;
    
    String output;
    serializeJson(doc, output);
    return output;
  }
  
  // JSON'dan oluştur
  bool fromJson(const String& json) {
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, json);
    if (error) {
      DEBUG_PRINTF("[SafeApiConfig] JSON parse error: %s\n", error.c_str());
      return false;
    }
    
    enabled = doc["enabled"] | false;
    method = (SafeHttpMethod)(doc["method"] | 0);
    url = doc["url"] | "";
    contentType = doc["contentType"] | "application/json";
    authorization = doc["authorization"] | "";
    customHeaders = doc["customHeaders"] | "";
    body = doc["body"] | "";
    
    return true;
  }
};

// ==================== SAFE LOCK SINIFI ====================

class SafeLock {
private:
  // Konfigürasyon - sadece şifre pattern'leri NVS'te
  SafePassword passwords[SAFE_MAX_PASSWORDS];
  bool initialized;
  bool littleFsReady;
  Preferences safePrefs;
  
  // Sliding window buffer - son 8 hareket
  SafeMovement moveBuffer[SAFE_MOVEMENT_BUFFER_SIZE];
  uint8_t bufferCount;       // Buffer'daki eleman sayısı (0-8)
  
  // Geçici hareket sayacı
  char currentDirection;     // Şu anki hareket yönü
  uint8_t currentTicks;      // Şu anki tık sayısı
  bool movementStarted;      // Hareket başladı mı?
  
  // Teaching Mode (Şifre Öğretme)
  bool teachingActive;                              // Teaching mode aktif mi?
  uint8_t teachingPasswordIndex;                    // Hangi şifre öğretiliyor?
  unsigned long teachingStartTime;                  // Teaching başlangıç zamanı
  SafeMovement teachBuffer[SAFE_MAX_PASSWORD_STEPS]; // Öğretilen hareketler
  uint8_t teachBufferCount;                         // Öğretilen hareket sayısı
  bool teachingRequiresButton;                      // Teaching B ile mi bitti?
  String lastCompletedPattern;                      // Son tamamlanan pattern
  static const unsigned long TEACHING_TIMEOUT = 15000; // 15 saniye timeout
  
  // Callback fonksiyonları
  void (*onPasswordMatchCallback)(uint8_t passwordIndex);
  void (*onTeachingCompleteCallback)(uint8_t passwordIndex, const String& pattern);
  void (*onTeachingCancelledCallback)(uint8_t passwordIndex, const char* reason);
  
  // API Config dosya yolunu oluştur
  String getApiConfigPath(uint8_t index) {
    return String(SAFE_API_CONFIG_PREFIX) + String(index) + String(SAFE_API_CONFIG_SUFFIX);
  }
  
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
      if (!passwords[pwdIdx].isValid()) continue;
      
      // B ile biten şifre sadece buton basılınca kontrol edilir
      if (passwords[pwdIdx].requireButton && !buttonPressed) continue;
      
      uint8_t moveCount = passwords[pwdIdx].stepCount;
      if (passwords[pwdIdx].requireButton) moveCount--;
      
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
        
        if (valid && matchMovements(tempMoves, moveCount, passwords[pwdIdx])) {
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
  SafeLock() : initialized(false), littleFsReady(false), bufferCount(0), 
               currentDirection(0), currentTicks(0), movementStarted(false),
               teachingActive(false), teachingPasswordIndex(0), teachingStartTime(0),
               teachBufferCount(0), teachingRequiresButton(false), lastCompletedPattern(""),
               onPasswordMatchCallback(nullptr), onTeachingCompleteCallback(nullptr),
               onTeachingCancelledCallback(nullptr) {}
  
  // Başlat (NVS'den şifreleri, LittleFS'ten API config'leri yükle)
  void begin() {
    DEBUG_PRINTLN("[SafeLock] Baslatiliyor...");
    
    // LittleFS başlat
    if (!LittleFS.begin(true)) {
      DEBUG_PRINTLN("[SafeLock] LittleFS baslatilamadi!");
      littleFsReady = false;
    } else {
      littleFsReady = true;
      DEBUG_PRINTLN("[SafeLock] LittleFS hazir");
      
      // /safe dizinini oluştur
      if (!LittleFS.exists(SAFE_API_CONFIG_DIR)) {
        LittleFS.mkdir(SAFE_API_CONFIG_DIR);
        DEBUG_PRINTLN("[SafeLock] /safe dizini olusturuldu");
      }
    }
    
    // NVS'ten şifre pattern'lerini yükle
    safePrefs.begin("safelock", false);
    
    for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
      String keyPwd = "pwd" + String(i);
      String keyActive = "active" + String(i);
      String keyHasApi = "hasApi" + String(i);
      
      String pwdStr = safePrefs.getString(keyPwd.c_str(), "");
      passwords[i].isActive = safePrefs.getBool(keyActive.c_str(), false);
      passwords[i].hasApiConfig = safePrefs.getBool(keyHasApi.c_str(), false);
      
      // Şifreyi parse et
      if (pwdStr.length() > 0) {
        passwords[i].fromString(pwdStr);
      }
      
      if (passwords[i].isActive || pwdStr.length() > 0) {
        DEBUG_PRINTF("[SafeLock] Slot #%d: configured (active=%d, hasApi=%d)\n", 
                     i, passwords[i].isActive, passwords[i].hasApiConfig);
      }
    }
    
    initialized = true;
    clearBuffer();
    resetCurrentMovement();
    DEBUG_PRINTLN("[SafeLock] Baslatma tamamlandi");
  }
  
  // NVS'e şifreleri kaydet
  bool savePasswords() {
    if (!initialized) return false;
    
    for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
      String keyPwd = "pwd" + String(i);
      String keyActive = "active" + String(i);
      String keyHasApi = "hasApi" + String(i);
      
      safePrefs.putString(keyPwd.c_str(), passwords[i].toString());
      safePrefs.putBool(keyActive.c_str(), passwords[i].isActive);
      safePrefs.putBool(keyHasApi.c_str(), passwords[i].hasApiConfig);
    }
    
    DEBUG_PRINTLN("[SafeLock] NVS'e kaydedildi");
    return true;
  }
  
  // LittleFS'e API config kaydet
  bool saveApiConfig(uint8_t index, const SafeApiConfig& config) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    if (!littleFsReady) {
      DEBUG_PRINTLN("[SafeLock] LittleFS hazir degil!");
      return false;
    }
    
    String path = getApiConfigPath(index);
    String json = config.toJson();
    
    File file = LittleFS.open(path, "w");
    if (!file) {
      DEBUG_PRINTF("[SafeLock] Dosya acilamadi: %s\n", path.c_str());
      return false;
    }
    
    size_t written = file.print(json);
    file.close();
    
    if (written > 0) {
      passwords[index].hasApiConfig = config.enabled;
      savePasswords();
      DEBUG_PRINTF("[SafeLock] API config kaydedildi: %s (%d byte)\n", path.c_str(), written);
      return true;
    }
    
    return false;
  }
  
  // LittleFS'ten API config oku (lazy-load)
  SafeApiConfig loadApiConfig(uint8_t index) {
    SafeApiConfig config;
    
    if (index >= SAFE_MAX_PASSWORDS) return config;
    if (!littleFsReady) return config;
    if (!passwords[index].hasApiConfig) return config;
    
    String path = getApiConfigPath(index);
    
    if (!LittleFS.exists(path)) {
      DEBUG_PRINTF("[SafeLock] API config dosyasi yok: %s\n", path.c_str());
      return config;
    }
    
    File file = LittleFS.open(path, "r");
    if (!file) {
      DEBUG_PRINTF("[SafeLock] Dosya acilamadi: %s\n", path.c_str());
      return config;
    }
    
    String json = file.readString();
    file.close();
    
    if (config.fromJson(json)) {
      DEBUG_PRINTF("[SafeLock] API config yuklendi: %s (%d byte)\n", path.c_str(), json.length());
    }
    
    return config;
  }
  
  // API config sil
  bool deleteApiConfig(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    if (!littleFsReady) return false;
    
    String path = getApiConfigPath(index);
    
    if (LittleFS.exists(path)) {
      LittleFS.remove(path);
    }
    
    passwords[index].hasApiConfig = false;
    savePasswords();
    
    DEBUG_PRINTF("[SafeLock] API config silindi: %s\n", path.c_str());
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
    
    // Teaching Mode aktifse
    if (teachingActive) {
      // Timeout kontrolü
      if (millis() - teachingStartTime > TEACHING_TIMEOUT) {
        cancelTeaching("timeout");
        return;
      }
      
      // Teaching buffer'a hareket ekle
      if (!movementStarted) {
        currentDirection = direction;
        currentTicks = ticks;
        movementStarted = true;
      } else if (currentDirection == direction) {
        currentTicks += ticks;
        if (currentTicks > SAFE_MAX_TICKS_PER_STEP) {
          currentTicks = SAFE_MAX_TICKS_PER_STEP;
        }
      } else {
        // Yön değişti - teaching buffer'a ekle
        if (teachBufferCount < SAFE_MAX_PASSWORD_STEPS) {
          teachBuffer[teachBufferCount++] = SafeMovement(currentDirection, currentTicks);
        }
        currentDirection = direction;
        currentTicks = ticks;
      }
      return; // Teaching modda normal şifre kontrolü yapma
    }
    
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
    
    // Teaching Mode aktifse - buton teaching'i tamamlar
    if (teachingActive) {
      // Timeout kontrolü
      if (millis() - teachingStartTime > TEACHING_TIMEOUT) {
        cancelTeaching("timeout");
        return;
      }
      
      // Mevcut hareketi teaching buffer'a ekle
      if (movementStarted && currentTicks > 0) {
        if (teachBufferCount < SAFE_MAX_PASSWORD_STEPS) {
          teachBuffer[teachBufferCount++] = SafeMovement(currentDirection, currentTicks);
        }
        resetCurrentMovement();
      }
      
      // Teaching'i tamamla
      teachingRequiresButton = true;
      completeTeaching();
      return;
    }
    
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
      passwords[index].isActive = isActive;
      
      // API config varsa LittleFS'e kaydet
      if (apiConfig.enabled) {
        saveApiConfig(index, apiConfig);
      } else {
        deleteApiConfig(index);
      }
      
      DEBUG_PRINTF("[SafeLock] Sifre #%d sadece durum guncellendi: active=%d\n", index, isActive);
      return savePasswords();
    }
    
    SafePassword newPwd;
    if (!newPwd.fromString(passwordStr)) {
      DEBUG_PRINTLN("[SafeLock] Gecersiz sifre formati!");
      return false;
    }
    
    passwords[index] = newPwd;
    passwords[index].isActive = isActive;
    
    // API config varsa LittleFS'e kaydet
    if (apiConfig.enabled) {
      passwords[index].hasApiConfig = true;
      saveApiConfig(index, apiConfig);
    } else {
      deleteApiConfig(index);
    }
    
    // GÜVENLİK: Şifre ve URL Serial'e loglanmıyor
    DEBUG_PRINTF("[SafeLock] Slot #%d configured (active=%d)\n", index, isActive);
    DEBUG_PRINTF("[SafeLock] API enabled: %d\n", apiConfig.enabled);
    
    return savePasswords();
  }
  
  // Şifre al
  String getPassword(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return "";
    return passwords[index].toString();
  }
  
  // Şifreyi aktif/pasif yap
  bool setPasswordActive(uint8_t index, bool active) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    
    passwords[index].isActive = active;
    return savePasswords();
  }
  
  // Şifre aktif mi?
  bool isPasswordActive(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    return passwords[index].isActive;
  }
  
  // API konfigürasyonu al (lazy-load from LittleFS)
  SafeApiConfig getApiConfig(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return SafeApiConfig();
    return loadApiConfig(index);
  }
  
  // API config var mı? (dosya kontrolü yapmadan)
  bool hasApiConfig(uint8_t index) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    return passwords[index].hasApiConfig;
  }
  
  // LittleFS hazır mı?
  bool isLittleFsReady() { return littleFsReady; }
  
  // Callback ayarla
  void setPasswordMatchCallback(void (*callback)(uint8_t)) {
    onPasswordMatchCallback = callback;
  }
  
  // Teaching Mode callback'leri
  void setTeachingCompleteCallback(void (*callback)(uint8_t, const String&)) {
    onTeachingCompleteCallback = callback;
  }
  
  void setTeachingCancelledCallback(void (*callback)(uint8_t, const char*)) {
    onTeachingCancelledCallback = callback;
  }
  
  // ==================== TEACHING MODE FUNCTIONS ====================
  
  // Teaching mode'u başlat
  bool startTeaching(uint8_t passwordIndex) {
    if (passwordIndex >= SAFE_MAX_PASSWORDS) {
      DEBUG_PRINTLN("[SafeLock] Teaching: Invalid password index");
      return false;
    }
    
    if (teachingActive) {
      DEBUG_PRINTLN("[SafeLock] Teaching: Already in teaching mode");
      return false;
    }
    
    // Teaching buffer'ı temizle
    teachBufferCount = 0;
    for (int i = 0; i < SAFE_MAX_PASSWORD_STEPS; i++) {
      teachBuffer[i] = SafeMovement();
    }
    
    // Mevcut hareketi sıfırla
    resetCurrentMovement();
    
    // Normal buffer'ı temizle (karışmaması için)
    clearBuffer();
    
    teachingActive = true;
    teachingPasswordIndex = passwordIndex;
    teachingStartTime = millis();
    teachingRequiresButton = false;
    
    DEBUG_PRINTF("[SafeLock] Teaching started for password #%d\n", passwordIndex);
    return true;
  }
  
  // Teaching mode'u iptal et
  void cancelTeaching(const char* reason = "cancelled") {
    if (!teachingActive) return;
    
    uint8_t index = teachingPasswordIndex;
    
    teachingActive = false;
    teachBufferCount = 0;
    resetCurrentMovement();
    
    DEBUG_PRINTF("[SafeLock] Teaching cancelled: %s\n", reason);
    
    if (onTeachingCancelledCallback != nullptr) {
      onTeachingCancelledCallback(index, reason);
    }
  }
  
  // Teaching mode'u tamamla
  void completeTeaching() {
    if (!teachingActive) return;
    
    uint8_t index = teachingPasswordIndex;
    
    // Minimum adım kontrolü
    if (teachBufferCount < SAFE_MIN_PASSWORD_STEPS - (teachingRequiresButton ? 1 : 0)) {
      cancelTeaching("too_short");
      return;
    }
    
    // Şifre string'i oluştur
    String pattern = "";
    for (uint8_t i = 0; i < teachBufferCount; i++) {
      if (i > 0) pattern += "-";
      pattern += String(teachBuffer[i].direction);
      pattern += String(teachBuffer[i].ticks);
    }
    
    // Eğer butonla tamamlandıysa B ekle
    if (teachingRequiresButton) {
      pattern += "-B";
    }
    
    DEBUG_PRINTF("[SafeLock] Teaching complete: %s\n", pattern.c_str());
    
    // Pattern'i sakla (client alana kadar)
    lastCompletedPattern = pattern;
    
    teachingActive = false;
    teachBufferCount = 0;
    
    if (onTeachingCompleteCallback != nullptr) {
      onTeachingCompleteCallback(index, pattern);
    }
  }
  
  // Teaching mode durumu
  bool isTeaching() const { return teachingActive; }
  uint8_t getTeachingIndex() const { return teachingPasswordIndex; }
  
  // Son tamamlanan pattern'i al ve temizle
  String getLastCompletedPattern() {
    String pattern = lastCompletedPattern;
    lastCompletedPattern = "";
    return pattern;
  }
  
  // Teaching timeout kontrolü (loop'ta çağrılmalı)
  void checkTeachingTimeout() {
    if (teachingActive && millis() - teachingStartTime > TEACHING_TIMEOUT) {
      cancelTeaching("timeout");
    }
  }
  
  // Teaching buffer'daki mevcut pattern
  String getTeachingPattern() const {
    if (!teachingActive) return "";
    
    String pattern = "";
    for (uint8_t i = 0; i < teachBufferCount; i++) {
      if (i > 0) pattern += "-";
      pattern += String(teachBuffer[i].direction);
      pattern += String(teachBuffer[i].ticks);
    }
    
    // Devam eden hareket varsa onu da göster
    if (movementStarted && currentTicks > 0) {
      if (pattern.length() > 0) pattern += "-";
      pattern += String(currentDirection);
      pattern += String(currentTicks);
    }
    
    return pattern;
  }
  
  uint8_t getTeachingStepCount() const {
    return teachBufferCount + (movementStarted ? 1 : 0);
  }
};

#endif // SK_MODE_SAFE_H
