/*
 * SynDimm - Network Scanner
 * Local network'te Shelly Dimmer/DALI cihazlarını tarar
 */

#ifndef SYNDIMM_SCANNER_H
#define SYNDIMM_SCANNER_H

#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

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
  std::vector<DimmerDevice> currentScanDevices;  // Temporary list for current scan
  bool scanInProgress;
  int scannedCount;
  int totalToScan;
  TaskHandle_t scanTaskHandle;  // FreeRTOS task handle
  SemaphoreHandle_t devicesMutex;  // Mutex for thread-safe access to foundDevices
  
  static const int NUM_PARALLEL_TASKS = 5;  // 5 paralel task
  TaskHandle_t workerTasks[NUM_PARALLEL_TASKS];
  int nextIPToScan;
  char baseIP[16];  // Static buffer instead of String (prevent heap fragmentation)
  IPAddress localIPCache;
  
  // Adaptive timeout - öğrenen sistem
  uint16_t adaptiveTimeout;  // Başlangıç 500ms, başarılı cihazlardan öğrenir
  
  // Persistent HTTP clients per worker (connection reuse)
  HTTPClient* workerHTTPClients[NUM_PARALLEL_TASKS];
  WiFiClient* workerWiFiClients[NUM_PARALLEL_TASKS];
  
  // EEPROM persistent device list
  Preferences prefs;
  
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
    int devicesFound = 0;
    
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
      
      // Bu IP'yi tara (statik buffer kullan - heap fragmentation önleme)
      char targetIP[16];
      snprintf(targetIP, sizeof(targetIP), "%s%d", baseIP, ipIndex);
      
      // Kendi IP'yi atla
      String targetIPStr = String(targetIP);
      if (targetIPStr == localIPCache.toString()) {
        continue;
      }
      
      // Cihazı kontrol et (worker ID ile persistent HTTP client)
      DimmerDevice device;
      if (checkIfDimmer(targetIPStr, device, workerId)) {
        devicesFound++;
        Serial.printf("[Scanner] Worker %d FOUND DEVICE: %s (%s)\n", workerId, device.ip.c_str(), device.type.c_str());
        
        // Mutex ile geçici tarama listesine ekle
        if (xSemaphoreTake(devicesMutex, portMAX_DELAY) == pdTRUE) {
          currentScanDevices.push_back(device);
          xSemaphoreGive(devicesMutex);
        }
      }
      
      // Tarama sayacını artır (thread-safe)
      if (xSemaphoreTake(devicesMutex, portMAX_DELAY) == pdTRUE) {
        scannedCount++;
        
        // Her 50 IP'de progress göster
        if (scannedCount % 50 == 0) {
          Serial.printf("[Scanner] Progress: %d/%d IPs scanned\n", scannedCount, totalToScan);
        }
        
        xSemaphoreGive(devicesMutex);
      }
      
      // CPU'ya kısa nefes ver
      delay(10);
      yield();
    }
    
    Serial.printf("[Scanner] Worker %d finished. Found %d device(s)\n", workerId, devicesFound);
  }
  
  // Task için static wrapper (coordinator)
  static void scanTaskWrapper(void* parameter) {
    NetworkScanner* scanner = (NetworkScanner*)parameter;
    scanner->coordinatorTask();
  }
  
  // Cihaz tipini kontrol et (persistent HTTP client ile)
  bool checkIfDimmer(String ip, DimmerDevice& device, int workerId) {
    // Worker'a özel persistent client kullan (connection reuse)
    if (workerHTTPClients[workerId] == nullptr) {
      workerHTTPClients[workerId] = new HTTPClient();
      workerWiFiClients[workerId] = new WiFiClient();
      workerHTTPClients[workerId]->setReuse(true);  // HTTP keep-alive
    }
    
    HTTPClient* http = workerHTTPClients[workerId];
    WiFiClient* client = workerWiFiClients[workerId];
    
    // Adaptive timeout kullan (başarılı cihazlardan öğrenir)
    unsigned long startTime = millis();
    
    // Önce Gen2 (RPC) API'yi dene - /rpc/Shelly.GetDeviceInfo
    http->begin(*client, "http://" + ip + "/rpc/Shelly.GetDeviceInfo");
    http->setTimeout(adaptiveTimeout);  // Öğrenen timeout
    
    int httpCode = http->GET();
    
    if (httpCode == 200) {
      unsigned long responseTime = millis() - startTime;
      String payload = http->getString();
      // NOT: http->end() KALDIRILDI - keep-alive için bağlantı açık kalacak
      
      // Timeout'u öğren: başarılı response time * 1.5, max 800ms
      uint16_t newTimeout = min((uint16_t)(responseTime * 1.5), (uint16_t)800);
      if (newTimeout > adaptiveTimeout && newTimeout < 800) {
        adaptiveTimeout = newTimeout;
      }
      
      // JSON parse et (statik bellek - heap fragmentation önleme)
      StaticJsonDocument<512> doc;  // Shelly Gen2 response max ~400 bytes
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
          
          return true;  // Dimmer found (Gen2)
        }
      }
    } else if (httpCode <= 0) {
      // Connection failed - quick fail
      return false;  // Bağlantı yok, Gen1 denemeye gerek yok
    }
    
    // Gen2 değilse Gen1 API'yi dene - /shelly
    http->begin(*client, "http://" + ip + "/shelly");
    http->setTimeout(adaptiveTimeout);  // Aynı adaptive timeout
    
    httpCode = http->GET();
    
    if (httpCode == 200) {
      String payload = http->getString();
      // NOT: http->end() KALDIRILDI - keep-alive
      
      // JSON parse et (statik bellek - heap fragmentation önleme)
      StaticJsonDocument<256> doc;  // Shelly Gen1 response max ~200 bytes
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
          
          return true;  // Dimmer found (Gen1)
        }
      }
    }
    
    return false;  // Dimmer değil
  }
  
public:
  NetworkScanner() : scanInProgress(false), scannedCount(0), totalToScan(0), 
                     scanTaskHandle(NULL), nextIPToScan(1) {
    // Mutex oluştur
    devicesMutex = xSemaphoreCreateMutex();
    
    // Worker task handle'ları ve HTTP client'ları sıfırla
    for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
      workerTasks[i] = NULL;
      workerHTTPClients[i] = nullptr;
      workerWiFiClients[i] = nullptr;
    }
    
    // Adaptive timeout başlangıç değeri
    adaptiveTimeout = 500;  // 500ms başla, öğren
    
    // Load saved devices from EEPROM on boot
    loadDevicesFromEEPROM();
  }
  
  ~NetworkScanner() {
    // Task varsa temizle
    if (scanTaskHandle != NULL) {
      vTaskDelete(scanTaskHandle);
      scanTaskHandle = NULL;
    }
    
    // Worker task'ları ve HTTP client'ları temizle
    for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
      if (workerTasks[i] != NULL) {
        vTaskDelete(workerTasks[i]);
        workerTasks[i] = NULL;
      }
      if (workerHTTPClients[i] != nullptr) {
        workerHTTPClients[i]->end();
        delete workerHTTPClients[i];
        workerHTTPClients[i] = nullptr;
      }
      if (workerWiFiClients[i] != nullptr) {
        workerWiFiClients[i]->stop();
        delete workerWiFiClients[i];
        workerWiFiClients[i] = nullptr;
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
      Serial.println("[Scanner] ERROR: WiFi not connected!");
      return;
    }
    
    // Clear temporary scan list for new scan
    currentScanDevices.clear();
    scanInProgress = true;
    scannedCount = 0;
    totalToScan = 254;
    nextIPToScan = 1;  // İlk IP
    
    // IP bilgilerini cache'le (statik buffer - heap fragmentation önleme)
    localIPCache = WiFi.localIP();
    IPAddress gateway = WiFi.gatewayIP();
    String gatewayStr = gateway.toString();
    int lastDot = gatewayStr.lastIndexOf('.');
    String baseIPStr = gatewayStr.substring(0, lastDot + 1);
    strncpy(baseIP, baseIPStr.c_str(), sizeof(baseIP) - 1);
    baseIP[sizeof(baseIP) - 1] = '\0';  // Null terminate
    
    Serial.printf("[Scanner] Starting PARALLEL network scan with %d workers...\n", NUM_PARALLEL_TASKS);
    Serial.printf("[Scanner] Local IP: %s\n", localIPCache.toString().c_str());
    Serial.printf("[Scanner] Gateway: %s\n", gatewayStr.c_str());
    Serial.printf("[Scanner] Scanning: %s1-%s254\n", baseIP, baseIP);
    Serial.printf("[Scanner] Current device list has %d device(s)\n", foundDevices.size());
    
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
  
  // Taramayı durdur
  void stopScan() {
    if (!scanInProgress) {
      Serial.println("[Scanner] No scan in progress");
      return;
    }
    
    Serial.println("[Scanner] Stopping scan...");
    
    // Coordinator task'ı durdur
    if (scanTaskHandle != NULL) {
      vTaskDelete(scanTaskHandle);
      scanTaskHandle = NULL;
    }
    
    // Worker task'ları durdur
    for (int i = 0; i < NUM_PARALLEL_TASKS; i++) {
      if (workerTasks[i] != NULL) {
        vTaskDelete(workerTasks[i]);
        workerTasks[i] = NULL;
      }
    }
    
    scanInProgress = false;
    Serial.printf("[Scanner] Scan stopped. Found %d device(s) so far\n", foundDevices.size());
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
  
  // Manuel IP ile cihaz ekle (Web UI'den gelen manuel bağlantılar için)
  bool addManualDevice(const String& ip, const String& type) {
    Serial.printf("[Scanner] Adding manual device: %s (%s)\n", ip.c_str(), type.c_str());
    
    // Check if already exists
    for (const auto& dev : foundDevices) {
      if (dev.ip == ip) {
        Serial.println("[Scanner] Device already in list");
        return true;  // Zaten listede, başarılı sayalım
      }
    }
    
    // Create device entry
    DimmerDevice device;
    device.ip = ip;
    device.type = type;
    device.model = type;  // Tip olarak model'i de set et
    device.chipID = "manual";  // Manuel eklendi işareti
    
    // Add to list
    foundDevices.push_back(device);
    Serial.printf("[Scanner] Manual device added. Total devices: %d\n", foundDevices.size());
    
    // Save to EEPROM
    saveDevicesToEEPROM();
    
    return true;
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
    Serial.printf("[Scanner] Current scan found %d dimmer device(s)\n", currentScanDevices.size());
    
    // Merge current scan with existing devices
    mergeDevices();
    
    Serial.printf("[Scanner] Total devices after merge: %d\n", foundDevices.size());
    
    // Save devices to EEPROM
    saveDevicesToEEPROM();
    
    // Task'ı sonlandır
    scanTaskHandle = NULL;
    vTaskDelete(NULL);
  }
  
  // ========== EEPROM PERSISTENCE ==========
  
  void mergeDevices() {
    // Merge currentScanDevices into foundDevices
    // Keep old devices from different networks
    // Update/add devices from current network
    
    IPAddress gateway = WiFi.gatewayIP();
    String currentBaseIP = gateway.toString();
    int lastDot = currentBaseIP.lastIndexOf('.');
    currentBaseIP = currentBaseIP.substring(0, lastDot + 1);
    
    // Remove old devices from current network that weren't found
    auto it = foundDevices.begin();
    while (it != foundDevices.end()) {
      if (it->ip.startsWith(currentBaseIP)) {
        // Check if this device was found in current scan
        bool foundInScan = false;
        for (const auto& scanned : currentScanDevices) {
          if (it->ip == scanned.ip) {
            foundInScan = true;
            break;
          }
        }
        
        if (!foundInScan) {
          Serial.printf("[Scanner] Removing unreachable device: %s\n", it->ip.c_str());
          it = foundDevices.erase(it);
        } else {
          ++it;
        }
      } else {
        // Keep devices from other networks
        ++it;
      }
    }
    
    // Add new devices from current scan
    for (const auto& scanned : currentScanDevices) {
      bool exists = false;
      for (const auto& existing : foundDevices) {
        if (existing.ip == scanned.ip) {
          exists = true;
          break;
        }
      }
      
      if (!exists) {
        Serial.printf("[Scanner] Adding new device: %s (%s)\n", scanned.ip.c_str(), scanned.type.c_str());
        foundDevices.push_back(scanned);
      }
    }
  }
  
  // EEPROM operations
  void saveDevicesToEEPROM() {
    prefs.begin("scanner", false);
    
    // Save device count
    int count = foundDevices.size();
    prefs.putInt("deviceCount", count);
    
    // Save each device (max 20 to prevent EEPROM overflow)
    for (int i = 0; i < count && i < 20; i++) {
      String prefix = "dev" + String(i) + "_";
      prefs.putString((prefix + "ip").c_str(), foundDevices[i].ip);
      prefs.putString((prefix + "type").c_str(), foundDevices[i].type);
      prefs.putString((prefix + "model").c_str(), foundDevices[i].model);
      prefs.putString((prefix + "chip").c_str(), foundDevices[i].chipID);
    }
    
    prefs.end();
    Serial.printf("[Scanner] Saved %d devices to EEPROM\n", count);
  }
  
  void loadDevicesFromEEPROM() {
    prefs.begin("scanner", true);  // Read-only
    
    int count = prefs.getInt("deviceCount", 0);
    
    if (count > 0) {
      foundDevices.clear();
      
      for (int i = 0; i < count && i < 20; i++) {
        String prefix = "dev" + String(i) + "_";
        DimmerDevice dev;
        dev.ip = prefs.getString((prefix + "ip").c_str(), "");
        dev.type = prefs.getString((prefix + "type").c_str(), "");
        dev.model = prefs.getString((prefix + "model").c_str(), "");
        dev.chipID = prefs.getString((prefix + "chip").c_str(), "");
        dev.isDimmer = true;  // Saved devices are always dimmers
        
        if (dev.ip != "") {
          foundDevices.push_back(dev);
        }
      }
      
      Serial.printf("[Scanner] Loaded %d devices from EEPROM\n", foundDevices.size());
    } else {
      Serial.println("[Scanner] No saved devices in EEPROM");
    }
    
    prefs.end();
  }
};

#endif
