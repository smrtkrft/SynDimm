/**
 * SynDimm.ino
 * SmartKraft SynDimm - Main Application
 * Version: v1.2.0
 * 
 * ESP32-C6 with KY-040 Rotary Encoder
 * Web Interface with AP Mode
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Web arayüzü SADECE ESP32-C6 içindir!
 * - Kullanıcı sadece cihazın kendisinden erişebilir (AP mode)
 * - Dışarıdan internet erişimi YOK
 * - Sadece bilgilendirme ve basit ayarlar için
 * - Tüm kritik kontroller ENCODER ile yapılır
 * - Web sadece izleme ve ince ayar içindir
 * 
 * ========================================
 * KOD STANDARTLARI
 * ========================================
 * ❌ ASLA EMOJİ KULLANMA!
 *    - HTML/CSS/JS içinde emoji karakterleri yasak
 *    - Sebep: Flash bellek boyutu, tarayıcı uyumluluğu
 *    - Alternatif: Unicode semboller (•, ⌕, ×) veya düz metin
 * ========================================
 */

// Include all modules
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <ESPmDNS.h>
#include "SK_config.h"
#include "SKwifi.h"
#include "SK_encoder.h"
#include "SK_buzzer.h"
#include "SK_mode_manager.h"
#include "SK_webserver.h"
#include "SK_dimmer.h"
#include "SK_shutter.h"
#include "SK_mode_safe.h"
#include "SK_mode_safe_api.h"
#include "SK_ota.h"

// Create instances
SKWiFi wifi;
SKEncoder encoder;
SKBuzzer buzzer;
SKModeManager modeManager(&buzzer);
SKWebServer webServer;
SafeLock safeLock;
SafeLockAPIHandler safeApiHandler;

void setup() {
  // Initialize Serial
  Serial.begin(SERIAL_BAUD_RATE);
  delay(1000);
  
  // Başlangıç banner'ı
  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("==========================================");
  DEBUG_PRINTLN("   SmartKraft SynDimm - Starting Up");
  DEBUG_PRINTF("   Version: %s\n", VERSION);
  DEBUG_PRINTLN("==========================================");
  
  // Sistem bilgileri
  DEBUG_PRINTF("[STARTUP] Chip Model: %s\n", ESP.getChipModel());
  DEBUG_PRINTF("[STARTUP] Chip Cores: %d\n", ESP.getChipCores());
  DEBUG_PRINTF("[STARTUP] Flash Size: %lu KB\n", ESP.getFlashChipSize() / 1024);
  DEBUG_PRINTF("[STARTUP] Free Heap: %lu bytes\n", ESP.getFreeHeap());
  DEBUG_PRINTF("[STARTUP] Total Heap: %lu bytes\n", ESP.getHeapSize());
  DEBUG_PRINTF("[STARTUP] Largest Block: %lu bytes\n", ESP.getMaxAllocHeap());
  
  // Reset nedeni
  esp_reset_reason_t resetReason = esp_reset_reason();
  const char* resetReasonStr = "Unknown";
  switch(resetReason) {
    case ESP_RST_POWERON: resetReasonStr = "Power On"; break;
    case ESP_RST_EXT: resetReasonStr = "External Reset"; break;
    case ESP_RST_SW: resetReasonStr = "Software Reset"; break;
    case ESP_RST_PANIC: resetReasonStr = "Panic/Exception"; break;
    case ESP_RST_INT_WDT: resetReasonStr = "Interrupt Watchdog"; break;
    case ESP_RST_TASK_WDT: resetReasonStr = "Task Watchdog"; break;
    case ESP_RST_WDT: resetReasonStr = "Other Watchdog"; break;
    case ESP_RST_DEEPSLEEP: resetReasonStr = "Deep Sleep Wake"; break;
    case ESP_RST_BROWNOUT: resetReasonStr = "Brownout"; break;
    case ESP_RST_SDIO: resetReasonStr = "SDIO"; break;
    default: break;
  }
  DEBUG_PRINTF("[STARTUP] Reset Reason: %s (%d)\n", resetReasonStr, resetReason);
  DEBUG_PRINTLN("==========================================");
  
  // CPU frekansini maksimumda tut (160MHz for ESP32-C6)
  setCpuFrequencyMhz(160);
  DEBUG_PRINTF("[STARTUP] CPU Frequency: %d MHz\n", getCpuFrequencyMhz());
  
  // Watchdog Timer - 30 saniye timeout (donma durumunda reset)
  // ESP-IDF 5.x'te WDT zaten başlatılmış olabilir, reconfigure kullan
  esp_task_wdt_config_t wdt_config = {
    .timeout_ms = 30000,
    .idle_core_mask = (1 << 0),
    .trigger_panic = true
  };
  esp_err_t wdt_err = esp_task_wdt_reconfigure(&wdt_config);
  if (wdt_err == ESP_ERR_INVALID_STATE) {
    // WDT henüz başlatılmamış, init et
    esp_task_wdt_init(&wdt_config);
  }
  esp_task_wdt_add(NULL);
  DEBUG_PRINTLN("[STARTUP] Watchdog Timer aktif (30s)");
  
  // Get persistent device ID (NVS'te saklanır, OTA sonrası bile korunur)
  String fullChipID = getOrCreateDeviceId();
  DEBUG_PRINTF("[STARTUP] Device ID: %s\n", fullChipID.c_str());
  
  // Initialize Encoder
  encoder.begin();
  
  // Initialize Buzzer
  buzzer.begin();
  
  // Initialize Mode Manager
  modeManager.begin();
  
  // Initialize WiFi (with network scan and smart connection)
  wifi.begin();
  
  // Initialize Dimmer System
  initDimmer();
  
  // Initialize Shutter System
  initShutter();
  
  // Mod değişim callback'i - modlar arası geçişte otomatik reconnect
  // NOT: initDimmer ve initShutter'dan SONRA ayarlanmalı
  modeManager.setModeChangeCallback([](SystemMode oldMode, SystemMode newMode) {
    DEBUG_PRINTF("[MODE] Mode changed: %d -> %d\n", (int)oldMode, (int)newMode);
    
    // Dimmer moduna geçildiğinde otomatik olarak son cihaza bağlan
    if (newMode == MODE_DIMMER) {
      DEBUG_PRINTLN("[MODE] Entering DIMMER mode - triggering auto-reconnect");
      autoReconnect();
    }
    // Shutter moduna geçildiğinde otomatik olarak son cihaza bağlan
    else if (newMode == MODE_SHUTTER) {
      DEBUG_PRINTLN("[MODE] Entering SHUTTER mode - triggering auto-reconnect");
      autoReconnectShutter();
    }
  });

  // Initialize Safe Lock System
  safeLock.begin();
  safeApiHandler.setSafeLock(&safeLock);
  safeLock.setPasswordMatchCallback([](uint8_t pwdIndex) {
    DEBUG_PRINTF("[SAFE] Password #%d matched!\n", pwdIndex);
    buzzer.playDits(2);  // 2 bip = şifre eşleşti
    SafeLockAPIHandler::onPasswordMatch(pwdIndex, &safeLock, &safeApiHandler);
  });
  
  // Teaching mode callbacks - buzzer feedback
  safeLock.setTeachingCompleteCallback([](uint8_t pwdIndex, const String& pattern) {
    DEBUG_PRINTF("[SAFE] Teaching complete for slot #%d\n", pwdIndex);
    buzzer.playDits(3);  // 3 bip = teaching başarılı
  });
  safeLock.setTeachingCancelledCallback([](uint8_t pwdIndex, const char* reason) {
    DEBUG_PRINTF("[SAFE] Teaching cancelled: %s\n", reason);
    buzzer.playDits(1);  // 1 bip = iptal
  });

  // Initialize Language System
  initLanguage();
  DEBUG_PRINTF("[STARTUP] Language: %s\n", getCurrentLangCode().c_str());

  // Initialize OTA System
  initOTA();
  
  // Start Web Server
  webServer.begin(fullChipID);
  webServer.setWiFiManager(&wifi);  // Link WiFi manager to web server
  webServer.setModeManager(&modeManager);  // Link mode manager to web server
  webServer.setSafeLock(&safeLock);  // Link Safe Lock to web server
  webServer.setSafeApiHandler(&safeApiHandler);  // Link Safe API Handler (ESP32 handles all HTTP)
  webServer.setHttpRequestCallback(incrementHttpRequestCount);  // HTTP request counter
  
  // Auto-reconnect ALL modes at startup (background connections)
  SystemMode activeMode = modeManager.getActiveMode();
  DEBUG_PRINTF("[STARTUP] Active Mode: %s\n", activeMode == MODE_DIMMER ? "DIMMER" : activeMode == MODE_SHUTTER ? "SHUTTER" : "SAFE");
  
  // Always try to connect to saved devices (background, read-only for inactive modes)
  DEBUG_PRINTLN("[STARTUP] Connecting to saved devices...");
  autoReconnect();        // Dimmer - always try
  autoReconnectShutter(); // Shutter - always try
  DEBUG_PRINTLN("[STARTUP] Device connections complete");
  
  // İlk OTA kontrolü (WiFi bağlıysa, sadece bilgilendirme)
  if (wifi.isConnectedToWiFi()) {
    DEBUG_PRINTLN("[STARTUP] Guncelleme kontrolu yapiliyor...");
    startupUpdateCheck();
  }
  
  // Random seed'i chip ID ile başlat (her cihaz farklı random değer üretir)
  randomSeed(ESP.getEfuseMac() ^ millis());
}

// ========================================
// AKILLI SAĞLIK İZLEME SİSTEMİ
// ========================================
static unsigned long lastHealthCheck = 0;
static unsigned long wifiDownSince = 0;
static uint32_t minFreeHeap = UINT32_MAX;
static unsigned long systemUptime = 0;
static const unsigned long HEALTH_CHECK_INTERVAL = 60000;     // 1 dakikada bir
static const unsigned long CRITICAL_WIFI_DOWN_TIME = 600000;  // 10 dakika WiFi yoksa restart
static const uint32_t CRITICAL_HEAP_THRESHOLD = 15000;        // 15KB altı kritik
static const uint32_t WARNING_HEAP_THRESHOLD = 30000;         // 30KB altı uyarı
static const unsigned long MAX_UPTIME_BEFORE_RESTART = 172800000UL; // 2 gün = 172800000ms (48 saat)

// Detaylı izleme için ek değişkenler
static uint32_t loopCounter = 0;
static uint32_t lastLoopCounter = 0;
static unsigned long lastLoopTime = 0;
static uint32_t maxLoopTime = 0;
static uint32_t httpRequestCount = 0;
static uint32_t wifiReconnectCount = 0;
static uint32_t heapWarningCount = 0;
static uint32_t lastHeapValue = 0;
static int32_t heapTrend = 0;  // Pozitif = artıyor, negatif = azalıyor

void performHealthCheck() {
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t totalHeap = ESP.getHeapSize();
  uint8_t heapPercent = (freeHeap * 100) / totalHeap;
  uint32_t largestBlock = ESP.getMaxAllocHeap();  // En büyük allocate edilebilir blok
  
  // Heap trend hesaplama
  if (lastHeapValue > 0) {
    heapTrend = (int32_t)freeHeap - (int32_t)lastHeapValue;
  }
  lastHeapValue = freeHeap;
  
  // Minimum heap takibi
  if (freeHeap < minFreeHeap) {
    minFreeHeap = freeHeap;
    DEBUG_PRINTF("[HEALTH] Yeni minimum heap: %lu bytes\n", minFreeHeap);
  }
  
  // Kritik bellek durumu - restart
  if (freeHeap < CRITICAL_HEAP_THRESHOLD) {
    DEBUG_PRINTLN("==========================================");
    DEBUG_PRINTF("[HEALTH] !!! KRITIK BELLEK - RESTART !!!\n");
    DEBUG_PRINTF("[HEALTH] Free: %lu, Min: %lu, Largest: %lu\n", freeHeap, minFreeHeap, largestBlock);
    DEBUG_PRINTF("[HEALTH] Uptime: %lu dk, Loop: %lu, HTTP: %lu\n", millis()/60000, loopCounter, httpRequestCount);
    DEBUG_PRINTLN("==========================================");
    delay(100);
    ESP.restart();
  }
  
  // Uyarı seviyesi bellek
  if (freeHeap < WARNING_HEAP_THRESHOLD) {
    heapWarningCount++;
    DEBUG_PRINTF("[HEALTH] UYARI #%lu! Dusuk bellek: %lu bytes (%d%%), Largest: %lu\n", 
                 heapWarningCount, freeHeap, heapPercent, largestBlock);
  }
  
  // WiFi durumu kontrolü
  if (wifi.isConnectedToWiFi()) {
    wifiDownSince = 0;  // WiFi çalışıyor, sayacı sıfırla
  } else if (!wifi.isAPModeActive()) {
    // WiFi bağlı değil ve AP modunda da değil
    if (wifiDownSince == 0) {
      wifiDownSince = millis();
      wifiReconnectCount++;
      DEBUG_PRINTF("[HEALTH] WiFi baglantisi kayboldu (#%lu), izleniyor...\n", wifiReconnectCount);
    } else if (millis() - wifiDownSince > CRITICAL_WIFI_DOWN_TIME) {
      DEBUG_PRINTLN("==========================================");
      DEBUG_PRINTF("[HEALTH] !!! WIFI TIMEOUT - RESTART !!!\n");
      DEBUG_PRINTF("[HEALTH] WiFi %lu dakikadir bagli degil\n", (millis() - wifiDownSince) / 60000);
      DEBUG_PRINTF("[HEALTH] Reconnect attempts: %lu\n", wifiReconnectCount);
      DEBUG_PRINTLN("==========================================");
      delay(100);
      ESP.restart();
    }
  }
  
  // Loop performans kontrolü
  unsigned long currentTime = millis();
  uint32_t loopsSinceLastCheck = loopCounter - lastLoopCounter;
  float loopsPerSecond = (loopsSinceLastCheck * 1000.0) / (currentTime - lastLoopTime);
  lastLoopCounter = loopCounter;
  lastLoopTime = currentTime;
  
  // Periyodik durum logu (her 5 dakikada bir - detaylı)
  static unsigned long lastStatusLog = 0;
  if (millis() - lastStatusLog > 300000) {
    lastStatusLog = millis();
    DEBUG_PRINTLN("");
    DEBUG_PRINTLN("========== SISTEM DURUM RAPORU ==========");
    DEBUG_PRINTF("[UPTIME] %lu dakika (%lu saat)\n", millis()/60000, millis()/3600000);
    DEBUG_PRINTF("[HEAP] Free: %lu / %lu bytes (%d%%)\n", freeHeap, totalHeap, heapPercent);
    DEBUG_PRINTF("[HEAP] Min: %lu, Largest Block: %lu bytes\n", minFreeHeap, largestBlock);
    DEBUG_PRINTF("[HEAP] Trend: %+ld bytes (son kontrolden beri)\n", heapTrend);
    DEBUG_PRINTF("[HEAP] Warning count: %lu\n", heapWarningCount);
    DEBUG_PRINTF("[LOOP] Total: %lu, Speed: %.1f loops/sec\n", loopCounter, loopsPerSecond);
    DEBUG_PRINTF("[LOOP] Max loop time: %lu ms\n", maxLoopTime);
    DEBUG_PRINTF("[WIFI] Mode: %s\n", wifi.isConnectedToWiFi() ? "STA Connected" : wifi.isAPModeActive() ? "AP Mode" : "Disconnected");
    DEBUG_PRINTF("[WIFI] Reconnect count: %lu\n", wifiReconnectCount);
    DEBUG_PRINTF("[WIFI] Restart count: %lu, Uptime: %lu sn\n", wifi.getWiFiRestartCount(), wifi.getWiFiUptime());
    DEBUG_PRINTF("[WIFI] Ping fail: %d, Since restart: %lu sn\n", wifi.getPingFailCount(), wifi.getTimeSinceLastRestart());
    DEBUG_PRINTF("[HTTP] Request count: %lu\n", httpRequestCount);
    DEBUG_PRINTF("[MODE] Active: %d\n", (int)modeManager.getActiveMode());
    DEBUG_PRINTLN("==========================================");
    DEBUG_PRINTLN("");
  }
  
  // Her saat başı daha kısa özet
  static unsigned long lastHourlyLog = 0;
  if (millis() - lastHourlyLog > 3600000) {
    lastHourlyLog = millis();
    DEBUG_PRINTF("[HOURLY] Saat %lu - Heap: %lu/%lu, Min: %lu, Loops: %lu\n", 
                 millis()/3600000, freeHeap, totalHeap, minFreeHeap, loopCounter);
    
    // Saatlik mDNS zorla yenileme (WiFi modunda)
    if (wifi.isConnectedToWiFi() && !wifi.isAPModeActive()) {
      DEBUG_PRINTLN("[HOURLY] mDNS saatlik yenileme...");
      wifi.restartMDNS();
    }
  }
  
  // Uptime kontrolü - 2 günde bir otomatik restart (bellek fragmentasyonu önleme)
  systemUptime = millis();
  if (systemUptime > MAX_UPTIME_BEFORE_RESTART) {
    DEBUG_PRINTLN("==========================================" );
    DEBUG_PRINTLN("[HEALTH] 2 gunluk uptime - Planli restart");
    DEBUG_PRINTF("[HEALTH] Final Heap: %lu, Min: %lu\n", freeHeap, minFreeHeap);
    DEBUG_PRINTLN("==========================================");
    delay(100);
    ESP.restart();
  }
  
  // Heap fragmentasyon kontrolü - Büyük blok allocate edilemiyorsa restart
  void* testBlock = malloc(8192);  // 8KB test bloğu
  if (testBlock == NULL && freeHeap > 20000) {
    DEBUG_PRINTLN("==========================================");
    DEBUG_PRINTLN("[HEALTH] !!! HEAP FRAGMENTASYON - RESTART !!!");
    DEBUG_PRINTF("[HEALTH] Free: %lu ama 8KB allocate edilemiyor!\n", freeHeap);
    DEBUG_PRINTF("[HEALTH] Largest block: %lu bytes\n", largestBlock);
    DEBUG_PRINTLN("==========================================");
    delay(100);
    ESP.restart();
  }
  if (testBlock) free(testBlock);
}

// HTTP request sayacı (webserver'dan çağrılacak)
void incrementHttpRequestCount() {
  httpRequestCount++;
}

void loop() {
  unsigned long loopStart = millis();  // Loop timing için
  loopCounter++;  // Toplam loop sayısı
  
  // Handle WiFi status (reconnection & AP mode scanning)
  wifi.handleWiFi();
  
  // Handle web server requests
  webServer.handleClient();
  
  // Update buzzer (non-blocking)
  buzzer.update();
  
  // Update mode manager (handle timeout)
  modeManager.update();
  
  // ========================================
  // MOD İZOLASYONU - Sadece aktif mod güncellenir
  // ========================================
  SystemMode activeMode = modeManager.getActiveMode();
  
  switch(activeMode) {
    case MODE_DIMMER:
      dimmerLoop();
      break;
    case MODE_SHUTTER:
      updateShutter();
      break;
    case MODE_SAFE:
      // Safe mod loop gerektirmez, event-driven çalışır
      break;
  }
  
  // Encoder event handling
  encoder.printDebug();  // Anlık debug çıktısı
  
  if (encoder.available()) {
    char event = encoder.read();
    
    // Check if button is held while rotating - MODE CHANGE
    if (encoder.isButtonHeld() && (event == 'L' || event == 'R')) {
      // Button held + rotation = mode change
      modeManager.handleModeRotation(event);
      encoder.setModeChangeTriggered();
    }
    // Mode change confirmation (button released after rotation)
    else if (event == 'M') {
      modeManager.confirmModeChange();
    }
    // Normal event handling - route to active mode
    else {
      switch(activeMode) {
        case MODE_DIMMER:
          handleDimmerEncoderEvent(event);
          break;
        case MODE_SHUTTER:
          handleShutterEncoderEvent(event);
          break;
        case MODE_SAFE:
          handleSafeEncoderEvent(event);
          break;
      }
    }
  }
  
  // Watchdog'u besle - donma önleme
  esp_task_wdt_reset();
  
  // Akıllı sağlık kontrolü
  if (millis() - lastHealthCheck > HEALTH_CHECK_INTERVAL) {
    lastHealthCheck = millis();
    performHealthCheck();
  }
  
  // Loop timing - en uzun loop süresini kaydet
  unsigned long loopDuration = millis() - loopStart;
  if (loopDuration > maxLoopTime) {
    maxLoopTime = loopDuration;
    if (loopDuration > 100) {  // 100ms üstü anormal
      DEBUG_PRINTF("[LOOP] Uzun loop tespit: %lu ms\n", loopDuration);
    }
  }
  
  delay(10);
}

// ========================================
// MODE-SPECIFIC ENCODER HANDLERS
// ========================================

void handleDimmerEncoderEvent(char event) {
  switch(event) {
    case 'L':
      adjustBrightness(-1);
      break;
    case 'R':
      adjustBrightness(1);
      break;
    case 'B':
      toggleDimmer();
      break;
  }
}

void handleShutterEncoderEvent(char event) {
  switch(event) {
    case 'L':
      handleShutterEncoderRotate(-1);  // -1 = left = up
      break;
    case 'R':
      handleShutterEncoderRotate(1);   // +1 = right = down
      break;
    case 'B':
      handleShutterEncoderButton();
      break;
  }
}

void handleSafeEncoderEvent(char event) {
  switch(event) {
    case 'L':
      safeLock.onEncoderMove('L', 1);
      break;
    case 'R':
      safeLock.onEncoderMove('R', 1);
      break;
    case 'B':
      safeLock.onEncoderButton();
      break;
  }
}
