/**
 * SK_mode_safe_api.h
 * SmartKraft SynDimm - Safe Lock API Handler
 * Version: v1.3.0
 * 
 * ========================================
 * SAFE MOD - API TETİKLEYİCİ
 * ========================================
 * Şifre eşleştiğinde HTTP API çağrısı yapar.
 * - WiFi bağlantısı kontrolü (AP Mode'da ÇALIŞMAZ!)
 * - 1 deneme, 1 saniye timeout (watchdog güvenliği)
 * - Kendi IP'sine istek engeli
 * - LittleFS'ten lazy-load API config
 * - Sınırsız URL/Header/Body desteği
 * 
 * KRİTİK: API tetiklemesi ESP32C6 tarafından yapılır!
 * ========================================
 */

#ifndef SK_MODE_SAFE_API_H
#define SK_MODE_SAFE_API_H

#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include "SK_config.h"
#include "SK_mode_safe.h"

// ==================== API YAPILANDIRMA ====================
#define SAFE_API_RETRY_COUNT 1           // 1 deneme
#define SAFE_API_TIMEOUT_MS 1000         // 1 saniye timeout
#define SAFE_API_RETRY_DELAY_MS 0        // Deneme arası bekleme yok

// API Yanıt Durumu
enum SafeApiResponseStatus {
  SAFE_API_SUCCESS,
  SAFE_API_TIMEOUT,
  SAFE_API_ERROR,
  SAFE_API_WIFI_ERROR,
  SAFE_API_INVALID_CONFIG,
  SAFE_API_NO_WIFI,
  SAFE_API_DISABLED,
  SAFE_API_AP_MODE
};

// ==================== API YÖNETİM SINIFI ====================

class SafeLockAPIHandler {
private:
  SafeLock* safeLock;
  
  // Custom headers'ı parse et ve http'ye ekle
  void parseAndAddHeaders(HTTPClient& http, const String& customHeaders) {
    if (customHeaders.length() == 0) return;
    
    int start = 0;
    int end = customHeaders.indexOf('\n');
    
    while (start < (int)customHeaders.length()) {
      String line;
      if (end == -1) {
        line = customHeaders.substring(start);
        start = customHeaders.length();
      } else {
        line = customHeaders.substring(start, end);
        start = end + 1;
        end = customHeaders.indexOf('\n', start);
      }
      
      line.trim();
      if (line.length() == 0) continue;
      
      int colonPos = line.indexOf(':');
      if (colonPos > 0) {
        String headerName = line.substring(0, colonPos);
        String headerValue = line.substring(colonPos + 1);
        headerName.trim();
        headerValue.trim();
        http.addHeader(headerName, headerValue);
        DEBUG_PRINTF("[SafeAPI] Header: %s\n", headerName.c_str());
      }
    }
  }
  
  SafeApiResponseStatus sendHttpRequest(const SafeApiConfig& apiConfig, int& httpCode) {
    if (apiConfig.url.length() == 0) {
      return SAFE_API_INVALID_CONFIG;
    }
    
    // GÜVENLİK: URL Serial'e loglanmıyor
    DEBUG_PRINTLN("[SafeAPI] Sending request...");
    
    HTTPClient http;
    http.setTimeout(SAFE_API_TIMEOUT_MS);
    http.setConnectTimeout(SAFE_API_TIMEOUT_MS);
    
    if (!http.begin(apiConfig.url)) {
      return SAFE_API_ERROR;
    }
    
    // Authorization header
    if (apiConfig.authorization.length() > 0) {
      http.addHeader("Authorization", apiConfig.authorization);
      DEBUG_PRINTLN("[SafeAPI] Authorization header eklendi");
    }
    
    // Custom headers (çok satırlı destekli)
    parseAndAddHeaders(http, apiConfig.customHeaders);
    
    yield();
    
    // HTTP Method'a göre istek gönder
    switch (apiConfig.method) {
      case SAFE_HTTP_POST:
        if (apiConfig.contentType.length() > 0) {
          http.addHeader("Content-Type", apiConfig.contentType);
        } else {
          http.addHeader("Content-Type", "application/json");
        }
        httpCode = http.POST(apiConfig.body.length() > 0 ? apiConfig.body : "{}");
        break;
        
      case SAFE_HTTP_PUT:
        if (apiConfig.contentType.length() > 0) {
          http.addHeader("Content-Type", apiConfig.contentType);
        } else {
          http.addHeader("Content-Type", "application/json");
        }
        httpCode = http.PUT(apiConfig.body.length() > 0 ? apiConfig.body : "{}");
        break;
        
      case SAFE_HTTP_DELETE:
        httpCode = http.sendRequest("DELETE");
        break;
        
      case SAFE_HTTP_GET:
      default:
        httpCode = http.GET();
        break;
    }
    
    yield();
    
    SafeApiResponseStatus result = SAFE_API_TIMEOUT;
    
    if (httpCode > 0) {
      result = (httpCode >= 200 && httpCode < 300) ? SAFE_API_SUCCESS : SAFE_API_ERROR;
      DEBUG_PRINTF("[SafeAPI] HTTP %d - %s\n", httpCode, result == SAFE_API_SUCCESS ? "OK" : "ERROR");
    }
    
    http.end();
    return result;
  }
  
public:
  SafeLockAPIHandler() : safeLock(nullptr) {}
  
  void setSafeLock(SafeLock* sl) {
    safeLock = sl;
  }
  
  SafeApiResponseStatus trigger(const SafeApiConfig& apiConfig) {
    if (WiFi.status() != WL_CONNECTED) {
      return SAFE_API_WIFI_ERROR;
    }
    
    String myIP = WiFi.localIP().toString();
    if (myIP == "192.168.4.1") {
      return SAFE_API_AP_MODE;
    }
    
    if (!apiConfig.isValid()) {
      return SAFE_API_INVALID_CONFIG;
    }
    
    // Kendi IP kontrolü
    if (apiConfig.url.indexOf(myIP) != -1) {
      ERROR_PRINTLN("[SafeAPI] Kendi IP'sine istek engellendi!");
      return SAFE_API_ERROR;
    }
    
    int httpCode = 0;
    return sendHttpRequest(apiConfig, httpCode);
  }
  
  static void onPasswordMatch(uint8_t passwordIndex, SafeLock* safeLock, SafeLockAPIHandler* apiHandler) {
    DEBUG_PRINTF("[SafeAPI] Password #%d matched, triggering API...\n", passwordIndex);
    
    // Pointer kontrolleri
    if (!safeLock) {
      DEBUG_PRINTLN("[SafeAPI] HATA: safeLock pointer NULL!");
      return;
    }
    if (!apiHandler) {
      DEBUG_PRINTLN("[SafeAPI] HATA: apiHandler pointer NULL!");
      return;
    }
    
    // LittleFS hazır mı?
    if (!safeLock->isLittleFsReady()) {
      DEBUG_PRINTLN("[SafeAPI] HATA: LittleFS hazir degil!");
      return;
    }
    
    // LittleFS'ten lazy-load API config
    DEBUG_PRINTF("[SafeAPI] Config yuklemeden once, hasApiConfig: %d\n", safeLock->hasApiConfig(passwordIndex));
    SafeApiConfig apiConfig = safeLock->getApiConfig(passwordIndex);
    
    // GÜVENLİK: URL Serial'e loglanmıyor ama debug için URL uzunluğunu göster
    DEBUG_PRINTF("[SafeAPI] API enabled: %d, URL uzunluk: %d\n", apiConfig.enabled, apiConfig.url.length());
    
    if (!apiConfig.enabled) {
      DEBUG_PRINTLN("[SafeAPI] API devre disi, cikiliyor");
      return;
    }
    
    if (apiConfig.url.length() == 0) {
      DEBUG_PRINTLN("[SafeAPI] API URL bos, cikiliyor");
      return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      DEBUG_PRINTLN("[SafeAPI] WiFi bagli degil!");
      return;
    }
    
    String myIP = WiFi.localIP().toString();
    DEBUG_PRINTF("[SafeAPI] Cihaz IP: %s\n", myIP.c_str());
    
    if (myIP == "192.168.4.1") {
      DEBUG_PRINTLN("[SafeAPI] AP modunda, API calismiyor");
      return;
    }
    
    DEBUG_PRINTLN("[SafeAPI] API istegi gonderiliyor...");
    SafeApiResponseStatus status = apiHandler->trigger(apiConfig);
    DEBUG_PRINTF("[SafeAPI] Sonuc: %d\n", status);
  }
  
  // Test API (String parametrelerle)
  bool testApi(const String& url, SafeHttpMethod method, 
               const String& contentType = "", 
               const String& authorization = "",
               const String& customHeaders = "", 
               const String& body = "") {
    SafeApiConfig testConfig;
    
    testConfig.url = url;
    testConfig.method = method;
    testConfig.contentType = contentType;
    testConfig.authorization = authorization;
    testConfig.customHeaders = customHeaders;
    testConfig.body = body;
    testConfig.enabled = true;
    
    DEBUG_PRINTLN("[SafeAPI] Test istegi...");
    return (trigger(testConfig) == SAFE_API_SUCCESS);
  }
};

#endif // SK_MODE_SAFE_API_H
