/**
 * SK_mode_safe_api.h
 * SmartKraft SynDimm - Safe Lock API Handler
 * Version: v0.9.1
 * 
 * ========================================
 * SAFE MOD - API TETİKLEYİCİ
 * ========================================
 * Şifre eşleştiğinde HTTP API çağrısı yapar.
 * - WiFi bağlantısı kontrolü (AP Mode'da ÇALIŞMAZ!)
 * - Buzzer feedback (syndimm_buzzer.h)
 * - 1 deneme, 1 saniye timeout (watchdog güvenliği)
 * - Kendi IP'sine istek engeli
 * 
 * KRİTİK: API tetiklemesi ESP32C6 tarafından yapılır, web arayüzü sadece ayar yapar!
 * ========================================
 */

#ifndef SK_MODE_SAFE_API_H
#define SK_MODE_SAFE_API_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "SK_mode_safe.h"

// Forward declaration (buzzer isteğe bağlı)
class SynDimmBuzzer;

// ==================== API YAPILANDIRMA ====================
#define SAFE_API_RETRY_COUNT 1           // 1 deneme
#define SAFE_API_TIMEOUT_MS 1000         // 1 saniye timeout
#define SAFE_API_RETRY_DELAY_MS 0        // Deneme arası bekleme yok

// API Yanıt Durumu
enum SafeApiResponseStatus {
  SAFE_API_SUCCESS,              // Başarılı (200-299)
  SAFE_API_TIMEOUT,              // Zaman aşımı
  SAFE_API_ERROR,                // HTTP hatası (400+, 500+)
  SAFE_API_WIFI_ERROR,           // WiFi bağlantısı yok
  SAFE_API_INVALID_CONFIG,       // Geçersiz konfigürasyon
  SAFE_API_NO_WIFI,              // WiFi bağlı değil
  SAFE_API_DISABLED,             // API devre dışı
  SAFE_API_AP_MODE               // AP Mode'da (internet yok)
};

// ==================== API YÖNETİM SINIFI ====================

class SafeLockAPIHandler {
private:
  SafeLock* safeLock;              // SafeLock referansı
  SynDimmBuzzer* buzzer;           // Buzzer referansı (opsiyonel)
  
  // HTTP isteği gönder
  SafeApiResponseStatus sendHttpRequest(const SafeApiConfig& apiConfig, int& httpCode) {
    Serial.println("[SafeAPI] sendHttpRequest basladi");
    
    // URL validasyonu
    if (strlen(apiConfig.url) == 0) {
      Serial.println("[SafeAPI] URL bos!");
      return SAFE_API_INVALID_CONFIG;
    }
    
    Serial.print("[SafeAPI] URL: ");
    Serial.println(apiConfig.url);
    
    HTTPClient http;
    
    // Timeout ayarla
    http.setTimeout(SAFE_API_TIMEOUT_MS);
    http.setConnectTimeout(SAFE_API_TIMEOUT_MS);
    
    // URL'yi kontrol et
    if (!http.begin(apiConfig.url)) {
      Serial.println("[SafeAPI] HTTP begin basarisiz!");
      return SAFE_API_ERROR;
    }
    
    // Custom header ekle
    if (strlen(apiConfig.header) > 0) {
      String headerStr = String(apiConfig.header);
      int colonPos = headerStr.indexOf(':');
      if (colonPos > 0) {
        String headerName = headerStr.substring(0, colonPos);
        String headerValue = headerStr.substring(colonPos + 1);
        headerName.trim();
        headerValue.trim();
        http.addHeader(headerName, headerValue);
        
        Serial.print("[SafeAPI] Header: ");
        Serial.print(headerName);
        Serial.print(" = ");
        Serial.println(headerValue);
      }
    }
    
    // İstek gönder
    Serial.println("[SafeAPI] Istek gonderiliyor...");
    
    if (apiConfig.method == SAFE_HTTP_POST) {
      // POST isteği
      http.addHeader("Content-Type", "application/json");
      
      String body = strlen(apiConfig.body) > 0 ? String(apiConfig.body) : "{}";
      
      Serial.print("[SafeAPI] POST: ");
      Serial.print(apiConfig.url);
      Serial.print(" Body: ");
      Serial.println(body);
      
      yield();
      httpCode = http.POST(body);
      yield();
    } else {
      // GET isteği
      Serial.print("[SafeAPI] GET: ");
      Serial.println(apiConfig.url);
      
      yield();
      httpCode = http.GET();
      yield();
    }
    
    Serial.print("[SafeAPI] Istek tamamlandi, kod: ");
    Serial.println(httpCode);
    
    // Yanıt kontrolü
    SafeApiResponseStatus result = SAFE_API_TIMEOUT;
    
    if (httpCode > 0) {
      Serial.print("[SafeAPI] HTTP Code: ");
      Serial.println(httpCode);
      
      if (httpCode >= 200 && httpCode < 300) {
        result = SAFE_API_SUCCESS;
        Serial.println("[SafeAPI] SUCCESS");
      } else {
        result = SAFE_API_ERROR;
        Serial.println("[SafeAPI] ERROR");
      }
    } else {
      Serial.print("[SafeAPI] Error: ");
      Serial.println(http.errorToString(httpCode));
      result = SAFE_API_TIMEOUT;
    }
    
    // HTTPClient'ı temizle
    http.end();
    Serial.println("[SafeAPI] Cleaned up");
    
    return result;
  }
  
public:
  SafeLockAPIHandler() : safeLock(nullptr), buzzer(nullptr) {}
  
  // SafeLock referansını ayarla
  void setSafeLock(SafeLock* sl) {
    safeLock = sl;
  }
  
  // Buzzer referansını ayarla (opsiyonel)
  void setBuzzer(SynDimmBuzzer* bz) {
    buzzer = bz;
  }
  
  // API tetikle
  SafeApiResponseStatus trigger(const SafeApiConfig& apiConfig) {
    // WiFi bağlantısı kontrolü
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[SafeAPI] WiFi baglantisi yok!");
      
      // Buzzer feedback
      if (buzzer != nullptr) {
        buzzer->playError();
      }
      
      return SAFE_API_WIFI_ERROR;
    }
    
    // AP Mode kontrolü (192.168.4.1 = AP Mode)
    String myIP = WiFi.localIP().toString();
    if (myIP == "192.168.4.1") {
      Serial.println("[SafeAPI] AP Mode'da - API devre disi");
      
      // Buzzer feedback
      if (buzzer != nullptr) {
        buzzer->playError();
      }
      
      return SAFE_API_AP_MODE;
    }
    
    // Konfigürasyon kontrolü
    if (!apiConfig.isValid()) {
      Serial.println("[SafeAPI] Gecersiz API konfigurasyonu!");
      return SAFE_API_INVALID_CONFIG;
    }
    
    // KENDİ IP'SİNE İSTEK ENGELLE
    String url = String(apiConfig.url);
    if (url.indexOf(myIP) != -1) {
      Serial.println("[SafeAPI] HATA: Kendi IP'sine istek gonderilemez!");
      Serial.print("[SafeAPI] Kendi IP: ");
      Serial.println(myIP);
      return SAFE_API_ERROR;
    }
    
    // Retry döngüsü
    for (int attempt = 1; attempt <= SAFE_API_RETRY_COUNT; attempt++) {
      Serial.print("[SafeAPI] Deneme ");
      Serial.print(attempt);
      Serial.print("/");
      Serial.println(SAFE_API_RETRY_COUNT);
      
      int httpCode = 0;
      SafeApiResponseStatus status = sendHttpRequest(apiConfig, httpCode);
      
      if (status == SAFE_API_SUCCESS) {
        // Başarılı!
        Serial.println("[SafeAPI] Istek basarili!");
        
        // Buzzer feedback
        if (buzzer != nullptr) {
          buzzer->playApiSuccess();
        }
        
        return SAFE_API_SUCCESS;
      }
      
      // Başarısız - tekrar dene
      if (attempt < SAFE_API_RETRY_COUNT) {
        Serial.print("[SafeAPI] Basarisiz, ");
        Serial.print(SAFE_API_RETRY_DELAY_MS);
        Serial.println("ms sonra tekrar denenecek...");
        
        unsigned long startWait = millis();
        while (millis() - startWait < SAFE_API_RETRY_DELAY_MS) {
          yield();
        }
      }
    }
    
    // Tüm denemeler başarısız
    Serial.println("[SafeAPI] Tum denemeler basarisiz!");
    
    // Buzzer feedback
    if (buzzer != nullptr) {
      buzzer->playApiFail();
    }
    
    return SAFE_API_TIMEOUT;
  }
  
  // Şifre eşleştiğinde çağrılacak callback (ESP32C6 tarafından)
  static void onPasswordMatch(uint8_t passwordIndex, SafeLock* safeLock, SafeLockAPIHandler* apiHandler, SynDimmBuzzer* buzzer) {
    Serial.print("[SafeAPI] Sifre eslesti: #");
    Serial.println(passwordIndex);
    
    // Buzzer feedback (şifre doğru)
    if (buzzer != nullptr) {
      buzzer->playSuccess();
    }
    
    // API konfigürasyonunu al
    SafeApiConfig apiConfig = safeLock->getApiConfig(passwordIndex);
    
    Serial.print("[SafeAPI] URL: ");
    Serial.println(apiConfig.url);
    Serial.print("[SafeAPI] Enabled: ");
    Serial.println(apiConfig.enabled ? "Yes" : "No");
    
    if (!apiConfig.enabled) {
      Serial.println("[SafeAPI] API devre disi");
      return;
    }
    
    // WiFi kontrolü
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[SafeAPI] WiFi bagli degil");
      return;
    }
    
    // AP Mode kontrolü
    String myIP = WiFi.localIP().toString();
    if (myIP == "192.168.4.1") {
      Serial.println("[SafeAPI] AP Mode'da - API devre disi");
      return;
    }
    
    Serial.println("[SafeAPI] API tetikleniyor...");
    
    SafeApiResponseStatus status = apiHandler->trigger(apiConfig);
    
    Serial.print("[SafeAPI] Sonuc: ");
    Serial.println(status);
  }
  
  // Test API isteği (web arayüzünden test için)
  bool testApi(const String& url, SafeHttpMethod method, const String& header = "", const String& body = "") {
    SafeApiConfig testConfig;
    
    url.toCharArray(testConfig.url, SAFE_API_URL_MAX);
    testConfig.method = method;
    
    if (header.length() > 0) {
      header.toCharArray(testConfig.header, SAFE_API_HEADER_MAX);
    }
    
    if (body.length() > 0) {
      body.toCharArray(testConfig.body, SAFE_API_BODY_MAX);
    }
    
    testConfig.enabled = true;
    
    Serial.println("[SafeAPI] Test istegi gonderiliyor...");
    SafeApiResponseStatus status = trigger(testConfig);
    
    return (status == SAFE_API_SUCCESS);
  }
};

#endif // SK_MODE_SAFE_API_H
