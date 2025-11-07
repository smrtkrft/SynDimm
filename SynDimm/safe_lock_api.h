/*
 * Safe Lock API Module
 * 
 * HTTP API tetikleme sistemi
 * - GET/POST desteği
 * - 3 deneme mekanizması
 * - 2 saniye timeout
 * - Buzzer feedback
 */

#ifndef SAFE_LOCK_API_H
#define SAFE_LOCK_API_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "safe_lock.h"
#include "safe_lock_eeprom.h"

// ==================== API YAPILANDIRMA ====================
#define API_RETRY_COUNT 1           // 1 DENEME (watchdog timeout önleme için azaltıldı)
#define API_TIMEOUT_MS 1000         // 1 saniye timeout (daha da azaltıldı)
#define API_RETRY_DELAY_MS 0        // Deneme arası bekleme yok (zaten 1 deneme)

// API Yanıt Durumu
enum ApiResponseStatus {
  API_SUCCESS,              // Başarılı (200-299)
  API_TIMEOUT,              // Zaman aşımı
  API_ERROR,                // HTTP hatası (400+, 500+)
  API_WIFI_ERROR,           // WiFi bağlantısı yok
  API_INVALID_CONFIG,       // Geçersiz konfigürasyon
  API_NO_WIFI,              // WiFi bağlı değil
  API_DISABLED              // API devre dışı
};

// ==================== API YÖNETİM SINIFI ====================

class SafeLockAPI {
private:
  SafeLock* safeLock;       // SafeLock referansı (buzzer için)
  
  // HTTP isteği gönder (tek deneme)
  ApiResponseStatus sendHttpRequest(const ApiConfig& apiConfig, int& httpCode) {
    Serial.println("[API] sendHttpRequest başladı");
    Serial.print("[API] Free heap: ");
    Serial.println(ESP.getFreeHeap());
    Serial.flush();
    
    // URL validasyonu
    if (strlen(apiConfig.url) == 0) {
      Serial.println("[API] URL boş!");
      return API_INVALID_CONFIG;
    }
    
    Serial.print("[API] URL: ");
    Serial.println(apiConfig.url);
    Serial.flush();
    
    HTTPClient http;
    
    Serial.println("[API] HTTPClient oluşturuldu");
    Serial.flush();
    
    // Timeout ayarla
    http.setTimeout(API_TIMEOUT_MS);
    http.setConnectTimeout(API_TIMEOUT_MS);
    
    Serial.println("[API] Timeout ayarlandı");
    Serial.flush();
    
    // URL'yi kontrol et (ESP32)
    Serial.println("[API] http.begin() çağrılıyor...");
    Serial.flush();
    
    if (!http.begin(apiConfig.url)) {
      Serial.println("[API] HTTP begin başarısız!");
      return API_ERROR;
    }
    
    Serial.println("[API] http.begin() başarılı");
    Serial.flush();
    
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
        
        Serial.print("[API] Header: ");
        Serial.print(headerName);
        Serial.print(" = ");
        Serial.println(headerValue);
      }
    }
    
    // İstek gönder
    Serial.println("[API] İstek gönderiliyor...");
    Serial.flush();
    
    if (apiConfig.method == SAFE_HTTP_POST) {
      // POST isteği
      http.addHeader("Content-Type", "application/json");
      
      String body = strlen(apiConfig.body) > 0 ? String(apiConfig.body) : "{}";
      
      Serial.print("[API] POST: ");
      Serial.print(apiConfig.url);
      Serial.print(" Body: ");
      Serial.println(body);
      Serial.flush();
      
      httpCode = http.POST(body);
    } else {
      // GET isteği
      Serial.print("[API] GET: ");
      Serial.println(apiConfig.url);
      Serial.flush();
      
      yield();  // Watchdog'u besle
      httpCode = http.GET();
      yield();  // Watchdog'u besle
    }
    
    Serial.print("[API] İstek tamamlandı, kod: ");
    Serial.println(httpCode);
    Serial.flush();
    
    // Yanıt kontrolü
    ApiResponseStatus result = API_TIMEOUT;
    
    if (httpCode > 0) {
      Serial.print("[API] HTTP Code: ");
      Serial.println(httpCode);
      Serial.flush();
      
      if (httpCode >= 200 && httpCode < 300) {
        result = API_SUCCESS;
        Serial.println("[API] SUCCESS");
        Serial.flush();
      } else {
        result = API_ERROR;
        Serial.println("[API] ERROR");
        Serial.flush();
      }
    } else {
      Serial.print("[API] Error: ");
      Serial.println(http.errorToString(httpCode));
      Serial.flush();
      result = API_TIMEOUT;
    }
    
    // HTTPClient'ı temizle
    http.end();
    Serial.println("[API] Cleaned up");
    Serial.flush();
    
    return result;
  }
  
public:
  SafeLockAPI() : safeLock(nullptr) {}
  
  // SafeLock referansını ayarla
  void setSafeLock(SafeLock* sl) {
    safeLock = sl;
  }
  
  // API tetikle (retry mekanizması ile)
  ApiResponseStatus trigger(const ApiConfig& apiConfig) {
    // WiFi bağlantısı kontrolü
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[API] WiFi bağlantısı yok!");
      // BUZZER KALDIRILDI
      return API_WIFI_ERROR;
    }
    
    // Konfigürasyon kontrolü
    if (!apiConfig.isValid()) {
      Serial.println("[API] Geçersiz API konfigürasyonu!");
      return API_INVALID_CONFIG;
    }
    
    // Retry döngüsü
    for (int attempt = 1; attempt <= API_RETRY_COUNT; attempt++) {
      Serial.print("[API] Deneme ");
      Serial.print(attempt);
      Serial.print("/");
      Serial.println(API_RETRY_COUNT);
      
      int httpCode = 0;
      ApiResponseStatus status = sendHttpRequest(apiConfig, httpCode);
      
      if (status == API_SUCCESS) {
        // Başarılı!
        Serial.println("[API] İstek başarılı!");
        // BUZZER KALDIRILDI
        return API_SUCCESS;
      }
      
      // Başarısız - tekrar dene
      if (attempt < API_RETRY_COUNT) {
        Serial.print("[API] Başarısız, ");
        Serial.print(API_RETRY_DELAY_MS);
        Serial.println("ms sonra tekrar denenecek...");
        
        // Non-blocking delay with yield
        unsigned long startWait = millis();
        while (millis() - startWait < API_RETRY_DELAY_MS) {
          yield();  // Watchdog'u besle, WiFi stack'i çalıştır
        }
      }
    }
    
    // Tüm denemeler başarısız
    Serial.println("[API] Tüm denemeler başarısız!");
    // BUZZER KALDIRILDI
    return API_TIMEOUT;
  }
  
  // Şifre eşleştiğinde tetiklenecek callback
  static ApiResponseStatus onPasswordMatch(SafeLockEEPROM* eeprom, SafeLockAPI* api, uint8_t passwordIndex) {
    Serial.print("[SafeLock] Sifre eslesti: #");
    Serial.println(passwordIndex);
    Serial.flush();
    
    // API konfigürasyonunu al
    ApiConfig apiConfig = eeprom->getApiConfig(passwordIndex);
    
    Serial.print("[API] URL: ");
    Serial.println(apiConfig.url);
    Serial.print("[API] Enabled: ");
    Serial.println(apiConfig.enabled ? "Yes" : "No");
    Serial.flush();
    
    if (!apiConfig.enabled) {
      Serial.println("[API] API devre disi");
      return API_DISABLED;
    }
    
    // KENDİ IP'SİNE İSTEK ENGELLE (infinite loop önleme)
    String url = String(apiConfig.url);
    String myIP = WiFi.localIP().toString();
    if (url.indexOf(myIP) != -1) {
      Serial.println("[API] HATA: Kendi IP'sine istek gonderilemez!");
      Serial.print("[API] Kendi IP: ");
      Serial.println(myIP);
      return API_ERROR;
    }
    
    // WiFi kontrolü
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[API] WiFi bagli degil");
      return API_NO_WIFI;
    }
    
    Serial.println("[API] API tetikleniyor...");
    Serial.flush();
    
    ApiResponseStatus status = api->trigger(apiConfig);
    
    Serial.print("[API] Sonuc: ");
    Serial.println(status);
    Serial.flush();
    
    return status;
  }
  
  // Test API isteği (web arayüzünden test için)
  bool testApi(const String& url, SafeHttpMethod method, const String& header = "", const String& body = "") {
    ApiConfig testConfig;
    
    url.toCharArray(testConfig.url, API_URL_MAX_LENGTH);
    testConfig.method = method;
    
    if (header.length() > 0) {
      header.toCharArray(testConfig.header, API_HEADER_MAX_LENGTH);
    }
    
    if (body.length() > 0) {
      body.toCharArray(testConfig.body, API_BODY_MAX_LENGTH);
    }
    
    testConfig.enabled = true;
    
    Serial.println("[API] Test isteği gönderiliyor...");
    ApiResponseStatus status = trigger(testConfig);
    
    return (status == API_SUCCESS);
  }
};

#endif // SAFE_LOCK_API_H
