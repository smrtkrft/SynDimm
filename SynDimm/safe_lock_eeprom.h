/*
 * Safe Lock EEPROM Management
 * 
 * Şifre ve API bilgilerini EEPROM'da saklar
 * - 5 şifre + API konfigürasyonu
 * - Validasyon ve checksum kontrolü
 */

#ifndef SAFE_LOCK_EEPROM_H
#define SAFE_LOCK_EEPROM_H

#include <Arduino.h>
#include <EEPROM.h>
#include "safe_lock.h"

// ==================== EEPROM YAPILANDIRMA ====================
#define EEPROM_SIZE 4096                    // ESP32 için artırıldı (2048 → 4096)
#define EEPROM_SAFE_LOCK_START 1024         // Safe Lock verileri 1024'ten başlar
#define EEPROM_MAGIC_NUMBER 0x5AFE          // Veri geçerlilik kontrolü (SAFE)

// ==================== API YAPILANDIRMA ====================
#define API_URL_MAX_LENGTH 200              // Maksimum URL uzunluğu
#define API_HEADER_MAX_LENGTH 100           // Maksimum header uzunluğu
#define API_BODY_MAX_LENGTH 200             // Maksimum body uzunluğu

// HTTP Method (ESP32'deki HTTP_GET/POST ile çakışmayı önlemek için)
enum SafeHttpMethod {
  SAFE_HTTP_GET = 0,
  SAFE_HTTP_POST = 1
};

// ==================== VERI YAPILARI ====================

// API Konfigürasyonu
struct __attribute__((packed)) ApiConfig {
  char url[API_URL_MAX_LENGTH];           // API endpoint URL
  SafeHttpMethod method;                   // GET veya POST
  char header[API_HEADER_MAX_LENGTH];     // Custom header (örn: "X-API-Key: abc123")
  char body[API_BODY_MAX_LENGTH];         // POST body (JSON)
  bool enabled;                            // API aktif mi?
  
  ApiConfig() : method(SAFE_HTTP_GET), enabled(false) {
    memset(url, 0, API_URL_MAX_LENGTH);
    memset(header, 0, API_HEADER_MAX_LENGTH);
    memset(body, 0, API_BODY_MAX_LENGTH);
  }
  
  bool isValid() const {
    if (!enabled) return false;
    if (strlen(url) == 0) return false;
    return true;
  }
};

// Safe Lock Konfigürasyonu (EEPROM'da saklanır)
struct __attribute__((packed)) SafeLockConfig {
  uint16_t magicNumber;                    // Validasyon için
  Password passwords[MAX_PASSWORDS];       // 5 şifre
  ApiConfig apiConfigs[MAX_PASSWORDS];     // Her şifre için API config
  uint8_t checksum;                        // Veri bütünlüğü kontrolü
  
  SafeLockConfig() : magicNumber(EEPROM_MAGIC_NUMBER), checksum(0) {}
  
  // Checksum hesapla
  uint8_t calculateChecksum() const {
    uint8_t sum = 0;
    const uint8_t* data = (const uint8_t*)this;
    size_t size = sizeof(SafeLockConfig) - sizeof(checksum); // checksum hariç
    
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
    return (magicNumber == EEPROM_MAGIC_NUMBER && isChecksumValid());
  }
};

// ==================== EEPROM YÖNETİM SINIFI ====================

class SafeLockEEPROM {
private:
  SafeLockConfig config;
  bool initialized;
  
public:
  SafeLockEEPROM() : initialized(false) {}
  
  // EEPROM'u başlat
  void begin() {
    // ESP32 için EEPROM boyutu kontrolü
    Serial.print("[SafeLock] SafeLockConfig struct boyutu: ");
    Serial.print(sizeof(SafeLockConfig));
    Serial.println(" bytes");
    Serial.print("[SafeLock] EEPROM başlangıç: ");
    Serial.print(EEPROM_SAFE_LOCK_START);
    Serial.print(", Toplam: ");
    Serial.println(EEPROM_SAFE_LOCK_START + sizeof(SafeLockConfig));
    
    EEPROM.begin(EEPROM_SIZE);
    initialized = true;
    
    // Konfigürasyonu yükle
    if (!load()) {
      // İlk kurulum - varsayılan değerlerle başlat
      Serial.println("[SafeLock] EEPROM'da veri bulunamadı, varsayılan değerler yükleniyor...");
      reset();
      save();
    } else {
      Serial.println("[SafeLock] EEPROM'dan konfigürasyon yüklendi");
    }
  }
  
  // EEPROM'dan yükle
  bool load() {
    if (!initialized) return false;
    
    EEPROM.get(EEPROM_SAFE_LOCK_START, config);
    
    // Debug: Magic number ve checksum kontrol
    Serial.print("[SafeLock] EEPROM'dan okunan magic: 0x");
    Serial.print(config.magicNumber, HEX);
    Serial.print(", Beklenen: 0x");
    Serial.println(EEPROM_MAGIC_NUMBER, HEX);
    Serial.print("[SafeLock] Checksum valid: ");
    Serial.println(config.isChecksumValid() ? "Evet" : "Hayır");
    
    if (!config.isValid()) {
      Serial.println("[SafeLock] EEPROM verisi geçersiz!");
      return false;
    }
    
    Serial.println("[SafeLock] EEPROM yükleme başarılı");
    return true;
  }
  
  // EEPROM'a kaydet
  bool save() {
    if (!initialized) return false;
    
    config.updateChecksum();
    EEPROM.put(EEPROM_SAFE_LOCK_START, config);
    
    if (EEPROM.commit()) {
      Serial.println("[SafeLock] EEPROM kaydetme başarılı");
      return true;
    } else {
      Serial.println("[SafeLock] EEPROM kaydetme HATASI!");
      return false;
    }
  }
  
  // Varsayılan değerlere sıfırla
  void reset() {
    config = SafeLockConfig();
    
    // Tüm şifreleri pasif yap
    for (int i = 0; i < MAX_PASSWORDS; i++) {
      config.passwords[i].isActive = false;
      config.apiConfigs[i].enabled = false;
    }
    
    Serial.println("[SafeLock] Konfigürasyon sıfırlandı");
  }
  
  // Şifre ayarla
  bool setPassword(uint8_t index, const String& passwordStr, const String& oldPassword = "") {
    if (index >= MAX_PASSWORDS) return false;
    
    // Eğer şifre zaten varsa, eski şifreyi kontrol et
    if (config.passwords[index].isActive && config.passwords[index].isValid()) {
      if (oldPassword.length() > 0) {
        String currentPwd = config.passwords[index].toString();
        if (currentPwd != oldPassword) {
          Serial.println("[SafeLock] Eski şifre yanlış!");
          return false;
        }
      } else {
        Serial.println("[SafeLock] Şifre değiştirmek için eski şifre gerekli!");
        return false;
      }
    }
    
    Password newPwd;
    if (!newPwd.fromString(passwordStr)) {
      Serial.println("[SafeLock] Geçersiz şifre formatı!");
      return false;
    }
    
    config.passwords[index] = newPwd;
    config.passwords[index].isActive = true;
    
    Serial.print("[SafeLock] Şifre ");
    Serial.print(index);
    Serial.print(" ayarlandı: ");
    Serial.println(passwordStr);
    
    return save();
  }
  
  // Şifre + API ayarla (overload)
  bool setPassword(uint8_t index, const String& passwordStr, const ApiConfig& apiConfig, const String& oldPassword = "") {
    if (index >= MAX_PASSWORDS) return false;
    
    // ESKİ ŞİFRE KONTROLÜ KALDIRILDI - Web arayüzünden rahat güncelleme için
    // Sadece geçerli şifre formatını kontrol et
    Password newPwd;
    if (!newPwd.fromString(passwordStr)) {
      Serial.println("[SafeLock] Geçersiz şifre formatı!");
      return false;
    }
    
    config.passwords[index] = newPwd;
    config.passwords[index].isActive = true;
    config.apiConfigs[index] = apiConfig;
    
    Serial.print("[SafeLock] Şifre ");
    Serial.print(index);
    Serial.print(" + API ayarlandı: ");
    Serial.println(passwordStr);
    
    return save();
  }
  
  // Şifre al
  String getPassword(uint8_t index) {
    if (index >= MAX_PASSWORDS) return "";
    return config.passwords[index].toString();
  }
  
  // Şifreyi aktif/pasif yap
  bool setPasswordActive(uint8_t index, bool active) {
    if (index >= MAX_PASSWORDS) return false;
    
    config.passwords[index].isActive = active;
    return save();
  }
  
  // Şifre aktif mi?
  bool isPasswordActive(uint8_t index) {
    if (index >= MAX_PASSWORDS) return false;
    return config.passwords[index].isActive;
  }
  
  // API konfigürasyonu ayarla
  bool setApiConfig(uint8_t index, const String& url, SafeHttpMethod method, 
                    const String& header = "", const String& body = "") {
    if (index >= MAX_PASSWORDS) return false;
    
    ApiConfig newConfig;
    
    // URL
    if (url.length() >= API_URL_MAX_LENGTH) {
      Serial.println("[SafeLock] URL çok uzun!");
      return false;
    }
    url.toCharArray(newConfig.url, API_URL_MAX_LENGTH);
    
    // Method
    newConfig.method = method;
    
    // Header
    if (header.length() >= API_HEADER_MAX_LENGTH) {
      Serial.println("[SafeLock] Header çok uzun!");
      return false;
    }
    if (header.length() > 0) {
      header.toCharArray(newConfig.header, API_HEADER_MAX_LENGTH);
    }
    
    // Body
    if (body.length() >= API_BODY_MAX_LENGTH) {
      Serial.println("[SafeLock] Body çok uzun!");
      return false;
    }
    if (body.length() > 0) {
      body.toCharArray(newConfig.body, API_BODY_MAX_LENGTH);
    }
    
    newConfig.enabled = true;
    config.apiConfigs[index] = newConfig;
    
    Serial.print("[SafeLock] API ");
    Serial.print(index);
    Serial.print(" ayarlandı: ");
    Serial.println(url);
    
    return save();
  }
  
  // API konfigürasyonu al
  ApiConfig getApiConfig(uint8_t index) {
    if (index >= MAX_PASSWORDS) return ApiConfig();
    return config.apiConfigs[index];
  }
  
  // API aktif/pasif
  bool setApiEnabled(uint8_t index, bool enabled) {
    if (index >= MAX_PASSWORDS) return false;
    
    config.apiConfigs[index].enabled = enabled;
    return save();
  }
  
  // Tüm konfigürasyonu SafeLock nesnesine yükle
  void loadToSafeLock(SafeLock& safeLock) {
    for (uint8_t i = 0; i < MAX_PASSWORDS; i++) {
      // Şifre var mı kontrol et (stepCount > 0)
      if (config.passwords[i].stepCount > 0) {
        String pwdStr = config.passwords[i].toString();
        safeLock.setPassword(i, pwdStr);
        
        // isActive durumunu ayarla (pasif şifreler de yüklenir)
        safeLock.setPasswordActive(i, config.passwords[i].isActive);
        
        Serial.print("[SafeLock] Şifre ");
        Serial.print(i);
        Serial.print(" yüklendi: ");
        Serial.print(pwdStr);
        Serial.print(" (Aktif: ");
        Serial.print(config.passwords[i].isActive ? "Evet" : "Hayır");
        Serial.println(")");
        
        // API bilgisini de yazdır
        Serial.print("  -> API Enabled: ");
        Serial.print(config.apiConfigs[i].enabled ? "Evet" : "Hayır");
        Serial.print(", URL: ");
        Serial.println(config.apiConfigs[i].url);
      }
    }
  }
  
  // Debug: Tüm şifreleri yazdır
  void printAllPasswords() {
    Serial.println("=== Safe Lock Şifreleri ===");
    for (uint8_t i = 0; i < MAX_PASSWORDS; i++) {
      Serial.print("Şifre ");
      Serial.print(i);
      Serial.print(": ");
      
      if (config.passwords[i].isValid()) {
        Serial.print(config.passwords[i].toString());
        Serial.print(" - ");
        Serial.print(config.passwords[i].isActive ? "AKTİF" : "PASİF");
        
        if (config.apiConfigs[i].isValid()) {
          Serial.print(" - API: ");
          Serial.print(config.apiConfigs[i].url);
          Serial.print(" (");
          Serial.print(config.apiConfigs[i].method == HTTP_GET ? "GET" : "POST");
          Serial.print(")");
        }
        Serial.println();
      } else {
        Serial.println("Ayarlanmamış");
      }
    }
    Serial.println("===========================");
  }
};

#endif // SAFE_LOCK_EEPROM_H
