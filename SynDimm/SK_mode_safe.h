/**
 * SK_mode_safe.h
 * SmartKraft SynDimm - Safe Lock Mode
 * Version: v0.9.1
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

// ==================== YAPILANDIRMA ====================
#define SAFE_MAX_PASSWORDS 5            // Maksimum 5 şifre
#define SAFE_MAX_PASSWORD_STEPS 6       // Maksimum 6 adım
#define SAFE_MIN_PASSWORD_STEPS 3       // Minimum 3 adım
#define SAFE_MOVEMENT_BUFFER_SIZE 10    // Son 10 hareketi tut
#define SAFE_MAX_TICKS_PER_STEP 50      // Her adımda maksimum 50 tık
#define SAFE_MIN_TICKS_PER_STEP 1       // Her adımda minimum 1 tık

// EEPROM Ayarları
#define SAFE_EEPROM_START 2048          // Safe Lock verileri 2048'den başlar (WiFi: 1024-2047)
#define SAFE_EEPROM_MAGIC 0x5AFE        // Validasyon için magic number

// API Konfigürasyon Limitleri
#define SAFE_API_URL_MAX 200            // Maksimum URL uzunluğu
#define SAFE_API_HEADER_MAX 100         // Maksimum header uzunluğu
#define SAFE_API_BODY_MAX 200           // Maksimum body uzunluğu

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
  // EEPROM Konfigürasyonu
  SafeEEPROMConfig config;
  bool eepromInitialized;
  
  // Hareket buffer'ı (circular buffer)
  SafeMovement moveBuffer[SAFE_MOVEMENT_BUFFER_SIZE];
  uint8_t bufferHead;        // Buffer başlangıcı
  uint8_t bufferCount;       // Buffer'daki eleman sayısı
  
  // Geçici hareket sayacı
  char currentDirection;     // Şu anki hareket yönü
  uint8_t currentTicks;      // Şu anki tık sayısı
  bool movementStarted;      // Hareket başladı mı?
  
  // Callback fonksiyonu (şifre eşleştiğinde)
  void (*onPasswordMatchCallback)(uint8_t passwordIndex);
  
  // Buffer'a hareket ekle
  void addMovementToBuffer(char direction, uint8_t ticks) {
    if (ticks == 0) return;
    
    moveBuffer[bufferHead] = SafeMovement(direction, ticks);
    bufferHead = (bufferHead + 1) % SAFE_MOVEMENT_BUFFER_SIZE;
    
    if (bufferCount < SAFE_MOVEMENT_BUFFER_SIZE) {
      bufferCount++;
    }
  }
  
  // Buffer'dan belirli sayıda son hareketi al
  bool getLastMovements(SafeMovement* output, uint8_t count) {
    if (count > bufferCount || count > SAFE_MOVEMENT_BUFFER_SIZE) return false;
    
    for (uint8_t i = 0; i < count; i++) {
      int idx = (bufferHead - count + i + SAFE_MOVEMENT_BUFFER_SIZE) % SAFE_MOVEMENT_BUFFER_SIZE;
      output[i] = moveBuffer[idx];
    }
    return true;
  }
  
  // Hareketleri şifre adımları ile karşılaştır
  bool matchMovements(SafeMovement* movements, uint8_t movCount, const SafePassword& pwd, bool buttonPressed) {
    // Buton kontrolü
    if (pwd.requireButton && !buttonPressed) return false;
    if (!pwd.requireButton && buttonPressed) return false;
    
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
  
  // Tüm şifreleri kontrol et
  void checkPasswords(bool buttonPressed) {
    SafeMovement tempMoves[SAFE_MAX_PASSWORD_STEPS];
    
    for (uint8_t pwdIdx = 0; pwdIdx < SAFE_MAX_PASSWORDS; pwdIdx++) {
      if (!config.passwords[pwdIdx].isValid()) continue;
      
      uint8_t moveCount = config.passwords[pwdIdx].stepCount;
      if (config.passwords[pwdIdx].requireButton) moveCount--;
      
      // Buffer'da yeterli hareket var mı?
      if (bufferCount < moveCount) continue;
      
      // Son N hareketi al
      if (getLastMovements(tempMoves, moveCount)) {
        if (matchMovements(tempMoves, moveCount, config.passwords[pwdIdx], buttonPressed)) {
          // Şifre eşleşti!
          onPasswordMatched(pwdIdx);
          return;
        }
      }
    }
  }
  
  // Mevcut hareket dahil kontrol et
  void checkPasswordsWithCurrentMove() {
    if (!movementStarted || currentTicks == 0) return;
    
    SafeMovement tempMoves[SAFE_MAX_PASSWORD_STEPS];
    
    for (uint8_t pwdIdx = 0; pwdIdx < SAFE_MAX_PASSWORDS; pwdIdx++) {
      if (!config.passwords[pwdIdx].isValid()) continue;
      if (config.passwords[pwdIdx].requireButton) continue;
      
      uint8_t moveCount = config.passwords[pwdIdx].stepCount;
      uint8_t totalMoves = bufferCount + 1;
      
      if (totalMoves < moveCount) continue;
      
      // Son (N-1) hareketi buffer'dan al
      if (moveCount > 1) {
        if (!getLastMovements(tempMoves, moveCount - 1)) continue;
      }
      
      // Son hareket olarak mevcut hareketi ekle
      tempMoves[moveCount - 1] = SafeMovement(currentDirection, currentTicks);
      
      // Eşleştir (buton yok)
      if (matchMovements(tempMoves, moveCount, config.passwords[pwdIdx], false)) {
        Serial.println("[SafeLock] Sifre eslesti!");
        onPasswordMatched(pwdIdx);
        return;
      }
    }
  }
  
  // Şifre eşleştiğinde çağrılır
  void onPasswordMatched(uint8_t passwordIndex) {
    Serial.print("[SafeLock] Sifre #");
    Serial.print(passwordIndex);
    Serial.println(" eslesti!");
    
    // Buffer'ı tamamen temizle
    bufferHead = 0;
    bufferCount = 0;
    currentDirection = 0;
    currentTicks = 0;
    movementStarted = false;
    
    for (uint8_t i = 0; i < SAFE_MOVEMENT_BUFFER_SIZE; i++) {
      moveBuffer[i] = SafeMovement();
    }
    
    // Callback çağır
    if (onPasswordMatchCallback != nullptr) {
      onPasswordMatchCallback(passwordIndex);
    }
  }
  
public:
  SafeLock() : eepromInitialized(false), bufferHead(0), bufferCount(0), 
               currentDirection(0), currentTicks(0), movementStarted(false),
               onPasswordMatchCallback(nullptr) {}
  
  // Başlat (EEPROM'dan yükle)
  void begin() {
    Serial.println("[SafeLock] Baslatiliyor...");
    
    // EEPROM'dan yükle
    EEPROM.get(SAFE_EEPROM_START, config);
    
    if (!config.isValid()) {
      Serial.println("[SafeLock] EEPROM'da veri yok, varsayilan degerler yukleniyor...");
      config = SafeEEPROMConfig();
      saveToEEPROM();
    } else {
      Serial.println("[SafeLock] EEPROM'dan yuklendi");
      
      // Aktif şifreleri listele
      for (uint8_t i = 0; i < SAFE_MAX_PASSWORDS; i++) {
        if (config.passwords[i].isValid()) {
          Serial.print("[SafeLock] Sifre #");
          Serial.print(i);
          Serial.print(": ");
          Serial.print(config.passwords[i].toString());
          Serial.print(" (");
          Serial.print(config.passwords[i].isActive ? "AKTIF" : "PASIF");
          Serial.println(")");
        }
      }
    }
    
    eepromInitialized = true;
    clearBuffer();
    resetCurrentMovement();
  }
  
  // EEPROM'a kaydet
  bool saveToEEPROM() {
    if (!eepromInitialized) return false;
    
    config.updateChecksum();
    EEPROM.put(SAFE_EEPROM_START, config);
    
    if (EEPROM.commit()) {
      Serial.println("[SafeLock] EEPROM'a kaydedildi");
      return true;
    } else {
      Serial.println("[SafeLock] EEPROM kaydetme HATASI!");
      return false;
    }
  }
  
  // Buffer'ı temizle
  void clearBuffer() {
    bufferHead = 0;
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
      // Yön değişti - önceki hareketi kaydet
      addMovementToBuffer(currentDirection, currentTicks);
      
      // Yeni harekete başla
      currentDirection = direction;
      currentTicks = ticks;
    }
    
    // Her hareketten sonra kontrol et
    checkPasswordsWithCurrentMove();
  }
  
  // SK_encoder'dan buton basımı geldiğinde çağrılır (B)
  void onEncoderButton() {
    if (movementStarted && currentTicks > 0) {
      // Mevcut hareketi kaydet
      addMovementToBuffer(currentDirection, currentTicks);
      resetCurrentMovement();
    }
    
    // Buton ile kontrol et
    checkPasswords(true);
  }
  
  // Şifre ayarla (web arayüzünden)
  bool setPassword(uint8_t index, const String& passwordStr, const SafeApiConfig& apiConfig) {
    if (index >= SAFE_MAX_PASSWORDS) return false;
    
    SafePassword newPwd;
    if (!newPwd.fromString(passwordStr)) {
      Serial.println("[SafeLock] Gecersiz sifre formati!");
      return false;
    }
    
    config.passwords[index] = newPwd;
    config.passwords[index].isActive = true;
    config.apiConfigs[index] = apiConfig;
    
    Serial.print("[SafeLock] Sifre #");
    Serial.print(index);
    Serial.print(" ayarlandi: ");
    Serial.println(passwordStr);
    
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
  
  // Debug: Buffer durumunu yazdır
  void printBufferStatus() {
    Serial.print("[SafeLock] Buffer (");
    Serial.print(bufferCount);
    Serial.print("/");
    Serial.print(SAFE_MOVEMENT_BUFFER_SIZE);
    Serial.print("): ");
    
    SafeMovement tempMoves[SAFE_MOVEMENT_BUFFER_SIZE];
    if (getLastMovements(tempMoves, bufferCount)) {
      for (uint8_t i = 0; i < bufferCount; i++) {
        Serial.print(tempMoves[i].direction);
        Serial.print(tempMoves[i].ticks);
        if (i < bufferCount - 1) Serial.print("-");
      }
    }
    Serial.println();
  }
};

#endif // SK_MODE_SAFE_H
