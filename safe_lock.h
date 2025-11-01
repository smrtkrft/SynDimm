/*
 * Safe Lock System - Rotary Combination Lock
 * 
 * Kasa kilidi mantığı ile çalışan şifre sistemi
 * - 5 farklı şifre desteği
 * - Son 10 hareketi buffer'da tutar
 * - Akıllı eşleştirme algoritması
 * - Sınırsız deneme hakkı
 * - Her şifre kendi API'sini tetikler
 */

#ifndef SAFE_LOCK_H
#define SAFE_LOCK_H

#include <Arduino.h>

// ==================== YAPILANDIRMA ====================
// BUZZER KALDIRILDI
// #define BUZZER_PIN 13              // GPIO13 (D7)

#define MAX_PASSWORDS 5            // Maksimum 5 şifre
#define MAX_PASSWORD_STEPS 6       // Maksimum 6 adım
#define MIN_PASSWORD_STEPS 3       // Minimum 3 adım
#define MOVEMENT_BUFFER_SIZE 10    // Son 10 hareketi tut
#define MAX_TICKS_PER_STEP 50      // Her adımda maksimum 50 tık
#define MIN_TICKS_PER_STEP 1       // Her adımda minimum 1 tık

// BUZZER TANIMLARI KALDIRILDI
/*
#define BEEP_SUCCESS_FREQ 1000     // Başarı: 1000Hz
#define BEEP_SUCCESS_DURATION 500  // 500ms uzun dit
#define BEEP_SHORT_DURATION 100    // 100ms kısa dit
#define BEEP_ERROR_FREQ 2000       // Hata: 2000Hz
#define BEEP_API_SUCCESS_FREQ 1500 // API başarı: 1500Hz
#define BEEP_WIFI_ERROR_FREQ 500   // WiFi hatası: 500Hz (düşük)
*/
#define BEEP_API_SUCCESS_FREQ 1500 // API başarı: 1500Hz
#define BEEP_WIFI_ERROR_FREQ 800   // WiFi hata: 800Hz

// ==================== VERI YAPILARI ====================

// Hareket yönü
enum Direction {
  DIR_LEFT = 'L',   // Sola dönüş
  DIR_RIGHT = 'R',  // Sağa dönüş
  DIR_BUTTON = 'B'  // Buton basımı
};

// Tek bir şifre adımı
struct __attribute__((packed)) PasswordStep {
  char direction;   // L, R veya B
  uint8_t ticks;    // Tık sayısı (B için 0)
  
  PasswordStep() : direction(0), ticks(0) {}
  
  PasswordStep(char dir, uint8_t t) : direction(dir), ticks(t) {}
  
  bool isValid() const {
    return (direction == 'L' || direction == 'R' || direction == 'B');
  }
  
  bool equals(const PasswordStep& other) const {
    if (direction == 'B' && other.direction == 'B') return true;
    return (direction == other.direction && ticks == other.ticks);
  }
};

// Tek bir hareket (buffer'da saklanır)
struct __attribute__((packed)) Movement {
  char direction;   // L veya R (B ayrı işlenir)
  uint8_t ticks;    // Tık sayısı
  
  Movement() : direction(0), ticks(0) {}
  
  Movement(char dir, uint8_t t) : direction(dir), ticks(t) {}
};

// Şifre yapısı
struct __attribute__((packed)) Password {
  PasswordStep steps[MAX_PASSWORD_STEPS];  // Şifre adımları
  uint8_t stepCount;                       // Toplam adım sayısı
  bool requireButton;                      // Son adımda B gerekli mi?
  bool isActive;                           // Şifre aktif mi?
  
  Password() : stepCount(0), requireButton(false), isActive(false) {
    for (int i = 0; i < MAX_PASSWORD_STEPS; i++) {
      steps[i] = PasswordStep();
    }
  }
  
  bool isValid() const {
    if (!isActive) return false;
    if (stepCount < MIN_PASSWORD_STEPS || stepCount > MAX_PASSWORD_STEPS) return false;
    
    for (uint8_t i = 0; i < stepCount; i++) {
      if (!steps[i].isValid()) return false;
      if (steps[i].direction != 'B') {
        if (steps[i].ticks < MIN_TICKS_PER_STEP || steps[i].ticks > MAX_TICKS_PER_STEP) {
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
            steps[stepCount] = PasswordStep('B', 0);
            requireButton = true;
          } else {
            String numStr = token.substring(1);
            if (numStr.length() == 0) return false;
            int ticks = numStr.toInt();
            if (ticks < MIN_TICKS_PER_STEP || ticks > MAX_TICKS_PER_STEP) return false;
            steps[stepCount] = PasswordStep(dir, ticks);
          }
          
          stepCount++;
          if (stepCount > MAX_PASSWORD_STEPS) return false;
        }
        startIdx = i + 1;
      }
    }
    
    return (stepCount >= MIN_PASSWORD_STEPS && stepCount <= MAX_PASSWORD_STEPS);
  }
};

// ==================== SAFE LOCK SINIFI ====================

class SafeLock {
private:
  // Şifreler
  Password passwords[MAX_PASSWORDS];
  
  // Hareket buffer'ı (circular buffer)
  Movement moveBuffer[MOVEMENT_BUFFER_SIZE];
  uint8_t bufferHead;        // Buffer başlangıcı
  uint8_t bufferCount;       // Buffer'daki eleman sayısı
  
  // Geçici hareket sayacı
  char currentDirection;     // Şu anki hareket yönü
  uint8_t currentTicks;      // Şu anki tık sayısı
  bool movementStarted;      // Hareket başladı mı?
  
  // Callback fonksiyonları
  void (*onPasswordMatch)(uint8_t passwordIndex);  // Şifre eşleştiğinde
  
  // BUZZER FONKSİYONLARI KALDIRILDI
  
  // Buffer'a hareket ekle
  void addMovementToBuffer(char direction, uint8_t ticks) {
    if (ticks == 0) return; // 0 tıklı hareket ekleme
    
    moveBuffer[bufferHead] = Movement(direction, ticks);
    bufferHead = (bufferHead + 1) % MOVEMENT_BUFFER_SIZE;
    
    if (bufferCount < MOVEMENT_BUFFER_SIZE) {
      bufferCount++;
    }
  }
  
  // Buffer'dan belirli sayıda son hareketi al
  bool getLastMovements(Movement* output, uint8_t count) {
    if (count > bufferCount || count > MOVEMENT_BUFFER_SIZE) return false;
    
    for (uint8_t i = 0; i < count; i++) {
      int idx = (bufferHead - count + i + MOVEMENT_BUFFER_SIZE) % MOVEMENT_BUFFER_SIZE;
      output[i] = moveBuffer[idx];
    }
    return true;
  }
  
  // Hareketleri şifre adımları ile karşılaştır
  bool matchMovements(Movement* movements, uint8_t movCount, const Password& pwd, bool buttonPressed) {
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
    Movement tempMoves[MAX_PASSWORD_STEPS];
    
    for (uint8_t pwdIdx = 0; pwdIdx < MAX_PASSWORDS; pwdIdx++) {
      if (!passwords[pwdIdx].isValid()) continue;
      
      uint8_t moveCount = passwords[pwdIdx].stepCount;
      if (passwords[pwdIdx].requireButton) moveCount--;
      
      // Buffer'da yeterli hareket var mı?
      if (bufferCount < moveCount) continue;
      
      // Son N hareketi al
      if (getLastMovements(tempMoves, moveCount)) {
        if (matchMovements(tempMoves, moveCount, passwords[pwdIdx], buttonPressed)) {
          // Şifre eşleşti!
          onPasswordMatched(pwdIdx);
          return; // İlk eşleşeni bul ve çık
        }
      }
    }
  }
  
  // Mevcut hareket dahil kontrol et (geçici olarak)
  void checkPasswordsWithCurrentMove() {
    if (!movementStarted || currentTicks == 0) return;
    
    // DEBUG LOG'LARI DEVRE DIŞI (crash önleme)
    // Serial.println("=== checkPasswordsWithCurrentMove ===");
    
    Movement tempMoves[MAX_PASSWORD_STEPS];
    
    for (uint8_t pwdIdx = 0; pwdIdx < MAX_PASSWORDS; pwdIdx++) {
      if (!passwords[pwdIdx].isValid()) continue;
      if (passwords[pwdIdx].requireButton) continue;
      
      uint8_t moveCount = passwords[pwdIdx].stepCount;
      uint8_t totalMoves = bufferCount + 1;
      
      if (totalMoves < moveCount) continue;
      
      // Son (N-1) hareketi buffer'dan al
      if (moveCount > 1) {
        if (!getLastMovements(tempMoves, moveCount - 1)) continue;
      }
      
      // Son hareket olarak mevcut hareketi ekle
      tempMoves[moveCount - 1] = Movement(currentDirection, currentTicks);
      
      // Eşleştir (buton yok)
      if (matchMovements(tempMoves, moveCount, passwords[pwdIdx], false)) {
        // Şifre eşleşti!
        Serial.println("[MATCH] Sifre eslesti!");
        Serial.flush();
        onPasswordMatched(pwdIdx);
        return;
      }
    }
  }
  
  void onPasswordMatched(uint8_t passwordIndex) {
    Serial.println("[onPasswordMatched] Basladi");
    Serial.flush();
    
    // Buffer'ı tamamen temizle
    Serial.println("[onPasswordMatched] Buffer temizleniyor...");
    Serial.flush();
    
    bufferHead = 0;
    bufferCount = 0;
    
    // Devam eden hareketi de sıfırla (ÖNEMLİ!)
    currentDirection = 0;
    currentTicks = 0;
    movementStarted = false;
    
    // Buffer içeriğini de sıfırla
    for (uint8_t i = 0; i < MOVEMENT_BUFFER_SIZE; i++) {
      moveBuffer[i] = Movement();
    }
    
    Serial.println("[onPasswordMatched] Buffer ve aktif hareket temizlendi");
    Serial.flush();
    
    // BUZZER DEVRE DIŞI - Crash yapıyor!
    // beepLong(BEEP_SUCCESS_FREQ);
    
    // Callback çağır
    if (onPasswordMatch != nullptr) {
      Serial.println("[onPasswordMatched] Callback cagriliyor");
      Serial.flush();
      
      onPasswordMatch(passwordIndex);
      
      Serial.println("[onPasswordMatched] Callback bitti");
      Serial.flush();
    }
    
    Serial.println("[onPasswordMatched] Tamamlandi");
    Serial.flush();
  }
  
public:
  SafeLock() : bufferHead(0), bufferCount(0), currentDirection(0), 
               currentTicks(0), movementStarted(false), onPasswordMatch(nullptr) {
    // BUZZER KALDIRILDI
    // pinMode(BUZZER_PIN, OUTPUT);
  }
  
  // Başlat
  void begin() {
    clearBuffer();
    resetCurrentMovement();
  }
  
  // Buffer'ı temizle
  void clearBuffer() {
    bufferHead = 0;
    bufferCount = 0;
    for (int i = 0; i < MOVEMENT_BUFFER_SIZE; i++) {
      moveBuffer[i] = Movement();
    }
  }
  
  // Mevcut hareketi sıfırla
  void resetCurrentMovement() {
    currentDirection = 0;
    currentTicks = 0;
    movementStarted = false;
  }
  
  // Encoder hareketi geldiğinde çağrılır
  void onEncoderMove(bool clockwise) {
    char newDirection = clockwise ? 'R' : 'L';
    
    if (!movementStarted) {
      // İlk hareket
      currentDirection = newDirection;
      currentTicks = 1;
      movementStarted = true;
    } else if (currentDirection == newDirection) {
      // Aynı yönde devam
      currentTicks++;
      if (currentTicks > MAX_TICKS_PER_STEP) {
        currentTicks = MAX_TICKS_PER_STEP; // Taşmayı önle
      }
    } else {
      // Yön değişti - önceki hareketi kaydet
      addMovementToBuffer(currentDirection, currentTicks);
      
      // Yeni harekete başla
      currentDirection = newDirection;
      currentTicks = 1;
    }
    
    // Her hareketten sonra kontrol et (mevcut hareket dahil)
    checkPasswordsWithCurrentMove();
  }
  
  // Buton basıldığında çağrılır
  void onButtonPress() {
    if (movementStarted && currentTicks > 0) {
      // Mevcut hareketi kaydet
      addMovementToBuffer(currentDirection, currentTicks);
      resetCurrentMovement();
    }
    
    // Buton ile kontrol et
    checkPasswords(true);
  }
  
  // Şifre ayarla
  bool setPassword(uint8_t index, const String& passwordStr) {
    if (index >= MAX_PASSWORDS) return false;
    
    Password newPwd;
    if (!newPwd.fromString(passwordStr)) return false;
    
    passwords[index] = newPwd;
    passwords[index].isActive = true;
    return true;
  }
  
  // Şifre al
  String getPassword(uint8_t index) {
    if (index >= MAX_PASSWORDS) return "";
    return passwords[index].toString();
  }
  
  // Şifreyi aktif/pasif yap
  void setPasswordActive(uint8_t index, bool active) {
    if (index < MAX_PASSWORDS) {
      passwords[index].isActive = active;
    }
  }
  
  // Şifre aktif mi?
  bool isPasswordActive(uint8_t index) {
    if (index >= MAX_PASSWORDS) return false;
    return passwords[index].isActive;
  }
  
  // Şifre geçerli mi?
  bool isPasswordValid(uint8_t index) {
    if (index >= MAX_PASSWORDS) return false;
    return passwords[index].isValid();
  }
  
  // Şifre callback'i ayarla
  void setPasswordMatchCallback(void (*callback)(uint8_t)) {
    onPasswordMatch = callback;
  }
  
  // BUZZER SES FONKSİYONLARI TAMAMEN KALDIRILDI - Sistem stabilitesi için
  
  // Debug: Buffer durumunu yazdır
  void printBufferStatus() {
    Serial.print("Buffer (");
    Serial.print(bufferCount);
    Serial.print("/");
    Serial.print(MOVEMENT_BUFFER_SIZE);
    Serial.print("): ");
    
    Movement tempMoves[MOVEMENT_BUFFER_SIZE];
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

#endif // SAFE_LOCK_H
