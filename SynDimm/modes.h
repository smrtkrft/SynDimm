/*
 * SynDimm - Modes Library
 * Dimmer ve Safe modları için yönetim
 */

#ifndef MODES_H
#define MODES_H

#include "ky040.h"
#include <HTTPClient.h>
#include <Preferences.h>
#include "safe_lock.h"  // SafeLock sınıfını dahil et

class ModeManager {
private:
  KY040* encoder;
  SafeLock* safeLock;  // Safe Lock referansı
  
  // Mod ayarları
  bool dimmerMode;
  bool safeMode;
  int dimmRatio;  // 1-5 arası, her encoder tıkında kaç birim değişecek
  
  // Matematiksel sayaçlar (KY040'dan taşındı)
  long L_deger;          // Sol yönde ardışık sayaç
  long R_deger;          // Sağ yönde ardışık sayaç
  char lastDirection;    // Son yön ('L' veya 'R')
  
  // ========== DIMMER MODE - CİHAZ ENTEGRASYONU ==========
  String deviceIP;                    // Bağlı cihaz IP adresi
  String deviceType;                  // Cihaz tipi (shelly-dimmer, shelly-dali, vb)
  bool deviceConnected;               // Bağlantı durumu
  bool deviceIson;                    // Cihaz açık/kapalı durumu
  int deviceBrightness;               // Cihaz dimm değeri (0-100)
  unsigned long lastDeviceSync;       // Son senkronizasyon zamanı
  unsigned long lastDimmChange;       // Son dimm_sayac değişim zamanı
  int lastSentBrightness;             // Cihaza son gönderilen değer
  const unsigned long deviceSyncInterval = 1000;  // 1 saniyede bir polling
  const unsigned long dimmerSendDelay = 150;      // Encoder durduğunda 150ms sonra gönder (daha responsive)
  
public:
  ModeManager(KY040* enc) : encoder(enc), safeLock(nullptr), dimmerMode(true), safeMode(false), dimmRatio(3), 
                            L_deger(0), R_deger(0), lastDirection(0),
                            deviceIP(""), deviceType(""), deviceConnected(false), deviceIson(false), 
                            deviceBrightness(0), lastDeviceSync(0), lastDimmChange(0), 
                            lastSentBrightness(-1) {}
  
  void begin() {
    // Başlangıç ayarları
    L_deger = 0;
    R_deger = 0;
    lastDirection = 0;
    
    // Kaydedilmiş ayarları yükle
    Preferences prefs;
    prefs.begin("syndimm", true);
    dimmRatio = prefs.getInt("dimmRatio", 1);  // Varsayılan 1
    
    // Son aktif modu yükle (elektrik gidip gelse devam etsin)
    String lastMode = prefs.getString("lastMode", "dimmer");
    if (lastMode == "safe") {
      dimmerMode = false;
      safeMode = true;
      Serial.println("Son mod yüklendi: Safe Mode");
    } else {
      dimmerMode = true;
      safeMode = false;
      Serial.println("Son mod yüklendi: Dimmer Mode");
    }
    
    prefs.end();
    
    // dimm_sayac başlangıç değerini 100 yap
    encoder->set_dimm_sayac(100);
    
    Serial.println("Dimmer başlangıç: 100");
    Serial.print("Dimm Ratio: ");
    Serial.println(dimmRatio);
    Serial.println("Matematiksel sayaçlar sıfırlandı (L_deger, R_deger)");
    
    // Kaydedilmiş cihazı yükle
    loadDevice();
  }
  
  // Mod seçimi
  void setDimmerMode(bool enable) {
    if (enable) {
      dimmerMode = true;
      safeMode = false;
      Serial.println("Mode: Dimmer");
      
      // Mod değişikliğini EEPROM'a kaydet
      saveModeToEEPROM("dimmer");
    }
  }
  
  void setSafeMode(bool enable) {
    if (enable) {
      safeMode = true;
      dimmerMode = false;
      Serial.println("Mode: Safe");
      
      // Mod değişikliğini EEPROM'a kaydet
      saveModeToEEPROM("safe");
    }
  }
  
  // Mod değiştirme (toggle)
  void toggleMode() {
    if (dimmerMode) {
      setSafeMode(true);
      Serial.println("[Mode] Dimmer -> Safe (Long Press)");
    } else {
      setDimmerMode(true);
      Serial.println("[Mode] Safe -> Dimmer (Long Press)");
    }
  }
  
  bool isDimmerMode() { return dimmerMode; }
  bool isSafeMode() { return safeMode; }
  
  // Safe Lock referansını ayarla
  void setSafeLock(SafeLock* sl) {
    safeLock = sl;
  }
  
  // Dimm oranı ayarları
  void setDimmRatio(int ratio) {
    if (ratio >= 1 && ratio <= 5) {
      dimmRatio = ratio;
      
      // Preferences'a kaydet
      Preferences prefs;
      prefs.begin("syndimm", false);
      prefs.putInt("dimmRatio", ratio);
      prefs.end();
      
      Serial.print("Dimm ratio kaydedildi: ");
      Serial.println(dimmRatio);
    }
  }
  
  int getDimmRatio() { return dimmRatio; }
  
  // Encoder event'lerini işle ve matematiksel işlemleri yap
  void processEncoderEvent(char event) {
    if (event == 0) return;  // Boş event
    
    // Mod değiştirme eventi (3+ saniye basılı tutup döndür ve bırak)
    if (event == 'M') {
      // Hangi yöne çevrildiğini kontrol et
      char direction = encoder->getModeSelectDirection();
      
      if (direction == 'L') {
        // Sol çevirme: Safe moduna geç (zaten Safe'deysen hiçbir şey yapma)
        if (!safeMode) {
          setSafeMode(true);
          Serial.println("[Mode] Dimmer -> Safe (Left turn)");
        } else {
          Serial.println("[Mode] Already in Safe mode (Left turn ignored)");
        }
      } else if (direction == 'R') {
        // Sağ çevirme: Dimmer moduna geç (zaten Dimmer'daysan hiçbir şey yapma)
        if (!dimmerMode) {
          setDimmerMode(true);
          Serial.println("[Mode] Safe -> Dimmer (Right turn)");
        } else {
          Serial.println("[Mode] Already in Dimmer mode (Right turn ignored)");
        }
      }
      return;
    }
    
    // Mod seçme modunda encoder eventlerini yok say
    if (encoder->isModeSelectActive()) {
      Serial.println("[Mode] Mode select active - Encoder disabled");
      return;
    }
    
    // Yön sayaçlarını güncelle (KY040'dan taşındı)
    if (event == 'L') {
      if (lastDirection != 'L') {
        R_deger = 0;  // Yön değişti, karşı sayacı sıfırla
      }
      L_deger++;
      lastDirection = 'L';
      
    } else if (event == 'R') {
      if (lastDirection != 'R') {
        L_deger = 0;  // Yön değişti, karşı sayacı sıfırla
      }
      R_deger++;
      lastDirection = 'R';
    }
    
    // Mod'a göre işlem yap
    if (dimmerMode) {
      processDimmerMode(event);
    } else if (safeMode) {
      processSafeMode(event);
    }
  }
  
  // Yön sayaçlarını oku
  long getLeftCount() { return L_deger; }
  long getRightCount() { return R_deger; }
  
private:
  void processDimmerMode(char event) {
    int currentValue = encoder->get_dimm_sayac();
    
    if (event == 'L') {
      // Sol: azalt
      int newValue = currentValue - dimmRatio;
      if (newValue < 0) newValue = 0;
      encoder->set_dimm_sayac(newValue);
      lastDimmChange = millis();  // Değişiklik zamanını kaydet
      Serial.print("Dimmer: ");
      Serial.print(newValue);
      Serial.print(" (L x");
      Serial.print(dimmRatio);
      Serial.println(")");
      
    } else if (event == 'R') {
      // Sağ: arttır
      int newValue = currentValue + dimmRatio;
      if (newValue > 100) newValue = 100;
      encoder->set_dimm_sayac(newValue);
      lastDimmChange = millis();  // Değişiklik zamanını kaydet
      Serial.print("Dimmer: ");
      Serial.print(newValue);
      Serial.print(" (R x");
      Serial.print(dimmRatio);
      Serial.println(")");
      
    } else if (event == 'B') {
      // Buton: Cihaz toggle (açık/kapalı)
      if (deviceConnected) {
        toggleDevice();
      } else {
        Serial.println("Dimmer Button: Cihaz bagli degil");
      }
    }
  }
  
  void processSafeMode(char event) {
    // Safe mod: Encoder hareketlerini Safe Lock'a yönlendir
    if (safeLock == nullptr) {
      Serial.println("Safe Lock not initialized!");
      return;
    }
    
    if (event == 'L' || event == 'R') {
      // Encoder hareketi - Safe Lock'a bildir
      bool clockwise = (event == 'R');
      safeLock->onEncoderMove(clockwise);
      
      // Debug: L ve R sayaçlarını yazdır
      Serial.print(event);
      Serial.print(event == 'L' ? L_deger : R_deger);
      Serial.print(" ");
      safeLock->printBufferStatus();
      
    } else if (event == 'B') {
      // Buton basıldı - Safe Lock'a bildir
      safeLock->onButtonPress();
      Serial.println("Safe: Button pressed");
    }
  }

public:
  // ========== CİHAZ YÖNETİMİ FONKSİYONLARI ==========
  
  // Cihaza bağlan (Shelly Dimmer, Shelly DALI, vb)
  bool connectDevice(String ip, String type = "shelly-dimmer") {
    deviceIP = ip;
    deviceType = type;
    deviceConnected = false;
    
    // Test bağlantısı yap
    HTTPClient http;
    String url = "http://" + ip + "/light/0";
    http.begin(url);
    http.setTimeout(2000);  // 3000ms → 2000ms (ilk bağlantıda timeout biraz daha uzun olabilir)
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      deviceConnected = true;
      Serial.println("Cihaz baglandi: " + ip + " (" + type + ")");
      Serial.println("Response: " + payload);
      
      // İlk durumu oku
      parseDeviceStatus(payload);
      
      // Cihaz bilgilerini kaydet
      saveDevice(ip, type);
      
      http.end();
      return true;
    } else {
      Serial.println("Cihaz baglanti hatasi: " + String(httpCode));
      http.end();
      return false;
    }
  }
  
  // Geriye uyumluluk için (eski API çağrıları için)
  bool connectShelly(String ip) {
    return connectDevice(ip, "shelly-dimmer");
  }
  
  // Cihaz bağlantısını kes
  void disconnectDevice() {
    deviceConnected = false;
    deviceIP = "";
    deviceType = "";
    deviceIson = false;
    deviceBrightness = 0;
    
    // Kaydedilmiş cihaz bilgisini sil
    Preferences prefs;
    prefs.begin("syndimm", false);
    prefs.remove("deviceIP");
    prefs.remove("deviceType");
    prefs.end();
    
    Serial.println("Cihaz baglanti kesildi ve kayit silindi");
  }
  
  // Geriye uyumluluk için
  void disconnectShelly() {
    disconnectDevice();
  }
  
  // Cihaz durumunu oku (polling)
  void syncFromDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    HTTPClient http;
    String url = "http://" + deviceIP + "/light/0";
    http.begin(url);
    http.setTimeout(1500);  // 2000ms → 1500ms (daha hızlı)
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      String payload = http.getString();
      parseDeviceStatus(payload);
    } else {
      Serial.println("Cihaz okuma hatasi: " + String(httpCode));
      // 3 başarısız denemeden sonra bağlantıyı kes
      static int failCount = 0;
      failCount++;
      if (failCount > 3) {
        disconnectDevice();
        failCount = 0;
      }
    }
    
    http.end();
  }
  
  // Geriye uyumluluk için
  void syncFromShelly() {
    syncFromDevice();
  }
  
  // dimm_sayac'ı cihaza gönder
  void syncToDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    int currentBrightness = encoder->get_dimm_sayac();
    
    // Değişiklik yoksa gönderme
    if (currentBrightness == lastSentBrightness) return;
    
    HTTPClient http;
    String url;
    
    // Brightness = 0 ise cihazı kapat, değilse brightness gönder
    if (currentBrightness == 0) {
      // Shelly brightness=0 kabul etmiyor, turn=off kullan
      url = "http://" + deviceIP + "/light/0?turn=off&transition=0";
      if (deviceIson) {  // Sadece açıksa kapat
        deviceIson = false;
        Serial.println("Cihaz kapatiliyor (brightness=0)");
      } else {
        http.end();
        return;  // Zaten kapalı, istek gönderme
      }
    } else {
      // Cihaz kapalıysa önce aç, sonra brightness ayarla
      if (!deviceIson) {
        url = "http://" + deviceIP + "/light/0?turn=on&brightness=" + String(currentBrightness) + "&transition=0";
        deviceIson = true;
        Serial.println("Cihaz aciliyor ve brightness ayarlaniyor: " + String(currentBrightness));
      } else {
        // Zaten açık, sadece brightness ayarla
        url = "http://" + deviceIP + "/light/0?brightness=" + String(currentBrightness) + "&transition=0";
      }
    }
    
    http.begin(url);
    http.setTimeout(1500);
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      lastSentBrightness = currentBrightness;
      Serial.println("Cihaz brightness gonderildi: " + String(currentBrightness));
    } else {
      Serial.println("Cihaz brightness gonderme hatasi: " + String(httpCode));
    }
    
    http.end();
  }
  
  // Geriye uyumluluk için
  void syncToShelly() {
    syncToDevice();
  }
  
  // Cihaz açma/kapama toggle
  void toggleDevice() {
    if (!deviceConnected || deviceIP == "") return;
    
    String command = deviceIson ? "off" : "on";
    
    HTTPClient http;
    // transition=0 ile anında değişim
    String url = "http://" + deviceIP + "/light/0?turn=" + command + "&transition=0";
    
    // Açarken %80 brightness ayarla
    if (!deviceIson) {
      url += "&brightness=80";
    }
    
    http.begin(url);
    http.setTimeout(1500);  // 2000ms → 1500ms
    
    int httpCode = http.GET();
    
    if (httpCode == 200) {
      deviceIson = !deviceIson;
      Serial.println("Cihaz toggle: " + command + " (" + deviceType + ")");
      
      // Açıldıysa mevcut dimm_sayac'ı gönder (delay yerine flag kullan)
      if (deviceIson) {
        lastDimmChange = millis() - dimmerSendDelay + 100; // 100ms sonra gönderilecek
      }
    } else {
      Serial.println("Cihaz toggle hatasi: " + String(httpCode));
    }
    
    http.end();
  }
  
  // Geriye uyumluluk için
  void toggleShelly() {
    toggleDevice();
  }
  
  // Cihaz status JSON parse (basit)
  void parseDeviceStatus(String json) {
    // {"ison":true,"brightness":50,...}
    int isonIndex = json.indexOf("\"ison\":");
    int brightnessIndex = json.indexOf("\"brightness\":");
    
    if (isonIndex > 0) {
      String isonStr = json.substring(isonIndex + 7, isonIndex + 12);
      deviceIson = (isonStr.indexOf("true") >= 0);
    }
    
    if (brightnessIndex > 0) {
      int startIdx = brightnessIndex + 13;
      int endIdx = json.indexOf(",", startIdx);
      if (endIdx < 0) endIdx = json.indexOf("}", startIdx);
      String brightStr = json.substring(startIdx, endIdx);
      int newBrightness = brightStr.toInt();
      
      // DeviceBrightness'ı HER ZAMAN güncelle (web arayüzü için)
      deviceBrightness = newBrightness;
      
      // Encoder'ı SADECE cihazdan farklı bir değer geldiğinde ve encoder durgunken güncelle
      if (newBrightness != encoder->get_dimm_sayac() && deviceIson && lastDimmChange == 0) {
        encoder->set_dimm_sayac(newBrightness);
        Serial.println("Encoder cihazla senkronize edildi: " + String(newBrightness));
      }
    }
  }
  
  // Geriye uyumluluk için
  void parseShellyStatus(String json) {
    parseDeviceStatus(json);
  }
  
  // Loop içinde çağrılacak (senkronizasyon)
  void updateDevice() {
    if (!deviceConnected) {
      // Eğer IP var ama bağlı değilse, otomatik bağlanmayı dene
      static unsigned long lastConnectAttempt = 0;
      if (deviceIP != "" && millis() - lastConnectAttempt > 5000) {
        Serial.println("Kaydedilmis cihaza baglaniyor: " + deviceIP);
        connectDevice(deviceIP, deviceType);
        lastConnectAttempt = millis();
      }
      return;
    }
    
    unsigned long now = millis();
    
    // Encoder durdu mu? 150ms sonra cihaza gönder
    if (lastDimmChange > 0 && now - lastDimmChange > dimmerSendDelay) {
      syncToDevice();
      lastDimmChange = 0;  // Sıfırla
    }
    
    // Cihazdan durum oku - AMA SADECE ENCODER DURGUNKEN!
    // (Encoder aktifken cihazdan gelen değer encoder'ı override etmesin)
    if (lastDimmChange == 0 && now - lastDeviceSync > deviceSyncInterval) {
      syncFromDevice();
      lastDeviceSync = now;
    }
  }
  
  // Geriye uyumluluk için
  void updateShelly() {
    updateDevice();
  }
  
  // Getter'lar
  bool isDeviceConnected() { return deviceConnected; }
  String getDeviceIP() { return deviceIP; }
  String getDeviceType() { return deviceType; }
  bool getDeviceIson() { return deviceIson; }
  int getDeviceBrightness() { return deviceBrightness; }
  
  // Web arayüzünden değişiklik yapıldığında çağrılacak
  void triggerDimmChange() {
    lastDimmChange = millis();
  }
  
  // Geriye uyumluluk için (eski API)
  bool isShellyConnected() { return deviceConnected; }
  String getShellyIP() { return deviceIP; }
  bool getShellyIson() { return deviceIson; }
  int getShellyBrightness() { return deviceBrightness; }

private:
  // ========== PREFERENCES FONKSİYONLARI ==========
  
  // Aktif modu kaydet (elektrik gidip gelse devam etsin)
  void saveModeToEEPROM(String mode) {
    Preferences prefs;
    prefs.begin("syndimm", false);
    prefs.putString("lastMode", mode);
    prefs.end();
    Serial.println("Aktif mod kaydedildi: " + mode);
  }
  
  // Cihaz bilgilerini kaydet
  void saveDevice(String ip, String type) {
    Preferences prefs;
    prefs.begin("syndimm", false);
    prefs.putString("deviceIP", ip);
    prefs.putString("deviceType", type);
    prefs.end();
    Serial.println("Cihaz kaydedildi: " + ip + " (" + type + ")");
  }
  
  // Kaydedilmiş cihazı yükle (otomatik bağlantı loop'ta yapılacak)
  void loadDevice() {
    Preferences prefs;
    prefs.begin("syndimm", true);  // Read-only
    String savedIP = prefs.getString("deviceIP", "");
    String savedType = prefs.getString("deviceType", "shelly-dimmer");
    prefs.end();
    
    if (savedIP != "") {
      Serial.println("Kaydedilmis cihaz bulundu: " + savedIP + " (" + savedType + ")");
      deviceIP = savedIP;
      deviceType = savedType;
      // Otomatik bağlantı için deviceConnected false kalacak
      // İlk updateDevice() çağrısında bağlanmaya çalışacak
    } else {
      Serial.println("Kaydedilmis cihaz yok");
    }
  }
};

#endif // MODES_H
