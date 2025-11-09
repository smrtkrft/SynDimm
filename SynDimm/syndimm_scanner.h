/*
 * SynDimm - Network Scanner
 * Local network'te Shelly Dimmer/DALI cihazlarını tarar
 */

#ifndef SYNDIMM_SCANNER_H
#define SYNDIMM_SCANNER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

struct DimmerDevice {
  String ip;
  String type;        // "Shelly Dimmer 2", "Shelly DALI Dimmer", vb
  String model;       // "SHDM-2", "SHDALI-1", vb
  String chipID;
  bool isDimmer;      // Dimmer/DALI mi yoksa başka bir Shelly mi?
};

class NetworkScanner {
private:
  std::vector<DimmerDevice> foundDevices;
  bool scanInProgress;
  int scannedCount;
  int totalToScan;
  TaskHandle_t scanTaskHandle;  // FreeRTOS task handle
  SemaphoreHandle_t devicesMutex;  // Mutex for thread-safe access to foundDevices
  
  static const int NUM_PARALLEL_TASKS = 5;  // 5 paralel task
  TaskHandle_t workerTasks[NUM_PARALLEL_TASKS];
  int nextIPToScan;
  String baseIP;
  IPAddress localIPCache;
  
  // Worker task parametresi
  struct WorkerParams {
    NetworkScanner* scanner;
    int workerId;
  };
  
  // Worker task wrapper
  static void workerTaskWrapper(void* parameter) {
    WorkerParams* params = (WorkerParams*)parameter;
    params->scanner->workerTask(params->workerId);
    delete params;  // Parametreyi temizle
    vTaskDelete(NULL);
  }
  
  // Her worker'ın işi
  void workerTask(int workerId) {
    Serial.printf("[Scanner] Worker %d started\n", workerId);
    
    while (true) {
      // Mutex ile sonraki IP'yi al
      int ipIndex;
      if (xSemaphoreTake(devicesMutex, portMAX_DELAY) == pdTRUE) {
        if (nextIPToScan > totalToScan) {
          xSemaphoreGive(devicesMutex);
          break;  // Tüm IP'ler tarandı
        }
        ipIndex = nextIPToScan++;
        xSemaphoreGive(devicesMutex);
      } else {
        break;
      }
      
      // Bu IP'yi tara
      String targetIP = baseIP + String(ipIndex);
      
      // Kendi IP'yi atla
      if (targetIP == localIPCache.toString()) {
        continue;
      }
      
      // Cihazı kontrol et
      DimmerDevice device;
      if (checkIfDimmer(targetIP, device)) {
        // Mutex ile listeye ekle
        if (xSemaphoreTake(devicesMutex, portMAX_DELAY) == pdTRUE) {
          foundDevices.push_back(device);
          xSemaphoreGive(devicesMutex);
        }
      }
      
      // Tarama sayacını artır (thread-safe)
      if (xSemaphoreTake(devicesMutex, portMAX_DELAY) == pdTRUE) {
        scannedCount++;
        
        // Progress göster (her 25 IP'de)
        if (scannedCount % 25 == 0) {
          Serial.printf("[Scanner] Progress: %d/%d IPs scanned, %d dimmers found (Worker %d)\n", 
                        scannedCount, totalToScan, foundDevices.size(), workerId);
        }
        
        xSemaphoreGive(devicesMutex);
      }
      
      // CPU'ya kısa nefes ver
      delay(10);
      yield();
    }
    
    Serial.printf("[Scanner] Worker %d finished\n", workerId);
  }
  
  // Task için static wrapper (coordinator)
  static void scanTaskWrapper(void* parameter) {
    NetworkScanner* scanner = (NetworkScanner*)parameter;
    scanner->coordinatorTask();
  }
  
  // Cihaz tipini kontrol et
  bool checkIfDimmer(String ip, DimmerDevice& device) {
    HTTPClient http;
    WiFiClient client;
    
    // Önce Gen2 (RPC) API'yi dene - /rpc/Shelly.GetDeviceInfo
    http.begin(client, "http://" + ip + "/rpc/Shelly.GetDeviceInfo");
    http.setTimeout(800);  // 2s → 800ms (Shelly hızlı cevap verir)
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      http.end();
      
      // JSON parse et
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        String model = doc["model"] | "";
        String app = doc["app"] | "";
        String id = doc["id"] | "";
        
        // Dimmer/DALI kontrolü
        if (model.indexOf("Dimmer") != -1 || model.indexOf("DALI") != -1 || 
            model.indexOf("SHDM") != -1 || model.indexOf("SHDALI") != -1) {
          
          device.ip = ip;
          device.model = model;
          device.chipID = id;
          device.isDimmer = true;
          
          // Tip belirle
          if (model.indexOf("DALI") != -1 || model.indexOf("SHDALI") != -1) {
            device.type = "Shelly DALI Dimmer";
          } else {
            device.type = "Shelly Dimmer 2";
          }
          
          Serial.printf("[Scanner] Found dimmer (Gen2): %s at %s\n", device.type.c_str(), ip.c_str());
          return true;
        }
      }
    } else if (httpCode <= 0) {
      // Connection failed - quick fail, hemen kes
      http.end();
      
      // Gen1 de deneme, zaten bağlantı yok
      return false;
    }
    
    http.end();
    
    // Gen1 API'yi dene - /shelly
    http.begin(client, "http://" + ip + "/shelly");
    http.setTimeout(800);  // 2s → 800ms
    
    httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      http.end();
      
      // JSON parse et
      JsonDocument doc;
      DeserializationError error = deserializeJson(doc, payload);
      
      if (!error) {
        String type = doc["type"] | "";
        String mac = doc["mac"] | "";
        
        // Dimmer/DALI kontrolü
        if (type.indexOf("SHDM") != -1 || type.indexOf("SHDALI") != -1) {
          device.ip = ip;
          device.model = type;
          device.chipID = mac;
          device.isDimmer = true;
          
          if (type.indexOf("DALI") != -1 || type.indexOf("SHDALI") != -1) {
            device.type = "Shelly DALI Dimmer";
          } else {
            device.type = "Shelly Dimmer 2";
          }
          
          Serial.printf("[Scanner] Found dimmer (Gen1): %s at %s\n", device.type.c_str(), ip.c_str());
          return true;
        }
      }
    } else if (httpCode <= 0) {
      // Connection failed - quick fail
      http.end();
      return false;
    }
    
    http.end();
    return false;
  }
  
public:
  NetworkScanner() : scanInProgress(false), scannedCount(0), totalToScan(0), 
                     scanTaskHandle(NULL), nextIPToScan(1) {
    // Mutex oluştur
    devicesMutex = xSemaphoreCreateMutex();
    
    // Worker task handle'ları sıfırla
    for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
      workerTasks[i] = NULL;
    }
  }
  
  ~NetworkScanner() {
    // Task varsa temizle
    if (scanTaskHandle != NULL) {
      vTaskDelete(scanTaskHandle);
      scanTaskHandle = NULL;
    }
    
    // Worker task'ları temizle
    for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
      if (workerTasks[i] != NULL) {
        vTaskDelete(workerTasks[i]);
        workerTasks[i] = NULL;
      }
    }
    
    // Mutex'i temizle
    if (devicesMutex != NULL) {
      vSemaphoreDelete(devicesMutex);
    }
  }
  
  // Asenkron tarama başlat (FreeRTOS Task ile)
  void startScan() {
    if (scanInProgress) {
      Serial.println("[Scanner] Scan already in progress");
      return;
    }
    
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println("[Scanner] WiFi not connected");
      return;
    }
    
    foundDevices.clear();
    scanInProgress = true;
    scannedCount = 0;
    totalToScan = 254;
    nextIPToScan = 1;  // İlk IP
    
    // IP bilgilerini cache'le
    localIPCache = WiFi.localIP();
    IPAddress gateway = WiFi.gatewayIP();
    baseIP = gateway.toString();
    int lastDot = baseIP.lastIndexOf('.');
    baseIP = baseIP.substring(0, lastDot + 1);
    
    Serial.printf("[Scanner] Starting PARALLEL network scan with %d workers...\n", NUM_PARALLEL_TASKS);
    Serial.printf("[Scanner] Scanning: %s1-%s254\n", baseIP.c_str(), baseIP.c_str());
    
    // Coordinator task oluştur - 4KB stack, öncelik 1 (düşük), CPU Core 0
    xTaskCreatePinnedToCore(
      scanTaskWrapper,      // Task fonksiyonu
      "ScanCoordinator",    // Task adı
      4096,                 // Stack size (4KB - küçük)
      this,                 // Parameter (this pointer)
      1,                    // Öncelik (düşük - web server'ı engellemesin)
      &scanTaskHandle,      // Task handle
      0                     // CPU Core 0 (Core 1 web server için)
    );
  }
  
  // Tarama durumunu kontrol et
  bool isScanning() const {
    return scanInProgress;
  }
  
  // Progress yüzdesi
  int getProgress() const {
    if (totalToScan == 0) return 0;
    return (scannedCount * 100) / totalToScan;
  }
  
  // Bulunan cihazları al
  const std::vector<DimmerDevice>& getDevices() const {
    return foundDevices;
  }
  
  // JSON formatında cihaz listesi
  String getDevicesJSON() const {
    String json = "{\"devices\":[";
    
    for (size_t i = 0; i < foundDevices.size(); i++) {
      if (i > 0) json += ",";
      
      json += "{";
      json += "\"ip\":\"" + foundDevices[i].ip + "\",";
      json += "\"type\":\"" + foundDevices[i].type + "\",";
      json += "\"model\":\"" + foundDevices[i].model + "\",";
      json += "\"chipID\":\"" + foundDevices[i].chipID + "\"";
      json += "}";
    }
    
    json += "],\"scanning\":" + String(scanInProgress ? "true" : "false");
    json += ",\"progress\":" + String(getProgress());
    json += "}";
    
    return json;
  }

private:
  // Coordinator task - worker'ları başlatır ve bekler
  void coordinatorTask() {
    Serial.println("[Scanner] Coordinator started");
    
    // 5 worker task başlat
    for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
      WorkerParams* params = new WorkerParams{this, i};
      
      char taskName[20];
      snprintf(taskName, sizeof(taskName), "ScanWorker%d", i);
      
      xTaskCreatePinnedToCore(
        workerTaskWrapper,
        taskName,
        6144,               // 6KB stack (her worker için)
        params,
        1,                  // Öncelik 1 (düşük)
        &workerTasks[i],
        0                   // CPU Core 0
      );
      
      delay(50);  // Worker'lar arasında 50ms ara (WiFi stack'e baskı yapmasın)
    }
    
    Serial.printf("[Scanner] %d workers started\n", NUM_PARALLEL_TASKS);
    
    // Worker'ların bitmesini bekle
    bool allFinished = false;
    while (!allFinished) {
      delay(1000);  // Her saniye kontrol et
      
      allFinished = true;
      for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
        if (workerTasks[i] != NULL && eTaskGetState(workerTasks[i]) != eDeleted) {
          allFinished = false;
          break;
        }
      }
    }
    
    // Tamamlandı
    scanInProgress = false;
    Serial.printf("[Scanner] Scan complete! Found %d dimmer device(s) in total\n", foundDevices.size());
    
    // Task'ı sonlandır
    scanTaskHandle = NULL;
    vTaskDelete(NULL);
  }
};

#endif
