/**
 * SK_mode_safe_api.h
 * SmartKraft SynDimm - Safe Lock API Handler
 * Version: v1.2.0
 * 
 * ========================================
 * SAFE MOD - API TETİKLEYİCİ
 * ========================================
 * Şifre eşleştiğinde HTTP API çağrısı yapar.
 * - WiFi bağlantısı kontrolü (AP Mode'da ÇALIŞMAZ!)
 * - 1 deneme, 1 saniye timeout (watchdog güvenliği)
 * - Kendi IP'sine istek engeli
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
  
  SafeApiResponseStatus sendHttpRequest(const SafeApiConfig& apiConfig, int& httpCode) {
    if (strlen(apiConfig.url) == 0) {
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
    
    // Custom header
    if (strlen(apiConfig.header) > 0) {
      String headerStr = String(apiConfig.header);
      int colonPos = headerStr.indexOf(':');
      if (colonPos > 0) {
        String headerName = headerStr.substring(0, colonPos);
        String headerValue = headerStr.substring(colonPos + 1);
        headerName.trim();
        headerValue.trim();
        http.addHeader(headerName, headerValue);
      }
    }
    
    yield();
    
    if (apiConfig.method == SAFE_HTTP_POST) {
      http.addHeader("Content-Type", "application/json");
      String body = strlen(apiConfig.body) > 0 ? String(apiConfig.body) : "{}";
      httpCode = http.POST(body);
    } else {
      httpCode = http.GET();
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
    String url = String(apiConfig.url);
    if (url.indexOf(myIP) != -1) {
      ERROR_PRINTLN("[SafeAPI] Kendi IP'sine istek engellendi!");
      return SAFE_API_ERROR;
    }
    
    int httpCode = 0;
    return sendHttpRequest(apiConfig, httpCode);
  }
  
  static void onPasswordMatch(uint8_t passwordIndex, SafeLock* safeLock, SafeLockAPIHandler* apiHandler) {
    DEBUG_PRINTF("[SafeAPI] Password #%d matched, triggering API...\n", passwordIndex);
    
    SafeApiConfig apiConfig = safeLock->getApiConfig(passwordIndex);
    
    // GÜVENLİK: URL Serial'e loglanmıyor
    DEBUG_PRINTF("[SafeAPI] API enabled: %d\n", apiConfig.enabled);
    
    if (!apiConfig.enabled) {
      DEBUG_PRINTLN("[SafeAPI] API devre disi, cikiliyor");
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
    
    DEBUG_PRINTLN("[SafeAPI] Test istegi...");
    return (trigger(testConfig) == SAFE_API_SUCCESS);
  }
};

#endif // SK_MODE_SAFE_API_H
