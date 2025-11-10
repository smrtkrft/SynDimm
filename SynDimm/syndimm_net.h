/*
 * SynDimm - Minimal Network Manager
 * ESP32C6 WiFi/AP - Stack overflow önleme
 */

#ifndef SYNDIMM_NET_H
#define SYNDIMM_NET_H

#include <NetworkClient.h>  // ESP32 3.3.x compatibility
#include <WiFi.h>
#include <Preferences.h>

class SynDimmNet {
private:
  Preferences prefs;
  String chipID;
  String apSSID;
  bool apActive;
  
  String wifi1_ssid, wifi1_pass, wifi1_ip, wifi1_local;
  String wifi2_ssid, wifi2_pass, wifi2_ip, wifi2_local;
  
public:
  SynDimmNet() : apActive(false) {}
  
  void begin() {
    // Chip ID al
    uint64_t mac = ESP.getEfuseMac();
    chipID = String((uint32_t)(mac >> 16), HEX);
    chipID.toUpperCase();
    chipID = chipID.substring(0, 6);
    apSSID = "SynDimm-" + chipID;
    
    Serial.print("Chip ID: ");
    Serial.println(chipID);
    
    // Ayarları yükle
    prefs.begin("net", true);
    wifi1_ssid = prefs.getString("w1ssid", "");
    wifi1_pass = prefs.getString("w1pass", "");
    wifi1_ip = prefs.getString("w1ip", "");
    wifi1_local = prefs.getString("w1local", "");
    wifi2_ssid = prefs.getString("w2ssid", "");
    wifi2_pass = prefs.getString("w2pass", "");
    wifi2_ip = prefs.getString("w2ip", "");
    wifi2_local = prefs.getString("w2local", "");
    prefs.end();
  }
  
  void startAP() {
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID.c_str());
    apActive = true;
    Serial.print("AP: ");
    Serial.println(WiFi.softAPIP());
  }
  
  void stopAP() {
    WiFi.softAPdisconnect(true);
    apActive = false;
  }
  
  bool connectWiFi(String ssid, String pass, String ip) {
    if (ssid.length() == 0) return false;
    
    // Debug: Şifre uzunluğunu göster
    Serial.print("  [Debug] Sifre uzunlugu: ");
    Serial.print(pass.length());
    Serial.println(" karakter");
    
    // Önce WiFi taraması yap - ağ açık mı kontrol et
    Serial.print("  Ag taraniyor: ");
    Serial.print(ssid);
    Serial.print("... ");
    
    WiFi.mode(WIFI_STA);
    int n = WiFi.scanNetworks();
    bool networkFound = false;
    
    for (int i = 0; i < n; i++) {
      if (WiFi.SSID(i) == ssid) {
        networkFound = true;
        Serial.print("Bulundu! Sinyal: ");
        Serial.print(WiFi.RSSI(i));
        Serial.println(" dBm");
        break;
      }
    }
    
    if (!networkFound) {
      Serial.println("Bulunamadi!");
      WiFi.scanDelete();
      return false;
    }
    
    WiFi.scanDelete();
    
    // Statik IP varsa ayarla
    if (ip.length() > 0 && ip != "0.0.0.0") {
      IPAddress localIP, gateway, subnet, dns1, dns2;
      if (localIP.fromString(ip)) {
        // Gateway: IP'nin son oktetini 1 yap (örn: 192.168.1.1)
        gateway = localIP;
        gateway[3] = 1;
        subnet.fromString("255.255.255.0");
        dns1.fromString("8.8.8.8");  // Google DNS
        dns2.fromString("8.8.4.4");  // Google DNS backup
        WiFi.config(localIP, gateway, subnet, dns1, dns2);
        Serial.println("  DNS: 8.8.8.8, 8.8.4.4 (Google)");
      }
    } else {
      // DHCP kullanılıyor - yine de DNS'i manuel ayarla
      IPAddress dns1, dns2;
      dns1.fromString("8.8.8.8");
      dns2.fromString("8.8.4.4");
      WiFi.config(INADDR_NONE, INADDR_NONE, INADDR_NONE, dns1, dns2);
      Serial.println("  DNS: 8.8.8.8, 8.8.4.4 (Google)");
    }
    
    Serial.print("  Baglaniyor");
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    // CRITICAL: Disable WiFi power save - "asla uyku moduna girmeyecek"
    WiFi.setSleep(false);
    Serial.println("  [WiFi] Power save disabled");
    
    int count = 0;
    while (WiFi.status() != WL_CONNECTED && count < 40) {  // 10 saniye timeout
      delay(250);
      Serial.print(".");
      yield();     // WDT reset
      count++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.print("  Baglanildi! IP: ");
      Serial.println(WiFi.localIP());
      
      // WiFi bağlıysa AP'yi kapat
      if (apActive) {
        stopAP();
        Serial.println("  AP kapatildi");
      }
      return true;
    }
    
    // Bağlantı hatası - detaylı hata kodu
    Serial.print("  Baglanti hatasi! Durum: ");
    switch (WiFi.status()) {
      case WL_NO_SSID_AVAIL:
        Serial.println("SSID bulunamadi");
        break;
      case WL_CONNECT_FAILED:
        Serial.println("Baglanti basarisiz (yanlis sifre?)");
        break;
      case WL_DISCONNECTED:
        Serial.println("Baglanti kesildi");
        break;
      case WL_IDLE_STATUS:
        Serial.println("Bos (WiFi.begin() cagrilmadi?)");
        break;
      default:
        Serial.print("Bilinmeyen hata kodu: ");
        Serial.println(WiFi.status());
        break;
    }
    
    WiFi.disconnect(true);  // Temiz disconnect
    delay(100);
    return false;
  }
  
  void autoConnect() {
    Serial.println("=== Ag Baglantisi Baslatiliyor ===");
    
    // Önce WiFi 1'i dene
    if (wifi1_ssid.length() > 0) {
      Serial.println("[WiFi 1] SSID: " + wifi1_ssid);
      if (connectWiFi(wifi1_ssid, wifi1_pass, wifi1_ip)) {
        Serial.println("[WiFi 1] Basarili!");
        return;
      }
      Serial.println("[WiFi 1] Basarisiz!");
    } else {
      Serial.println("[WiFi 1] Kayitli degil");
    }
    
    // WiFi 1 başarısızsa WiFi 2'yi dene
    if (wifi2_ssid.length() > 0) {
      Serial.println("[WiFi 2] SSID: " + wifi2_ssid);
      if (connectWiFi(wifi2_ssid, wifi2_pass, wifi2_ip)) {
        Serial.println("[WiFi 2] Basarili!");
        return;
      }
      Serial.println("[WiFi 2] Basarisiz!");
    } else {
      Serial.println("[WiFi 2] Kayitli degil");
    }
    
    // Her ikisi de başarısızsa AP başlat
    Serial.println("=== WiFi Bulunamadi - AP Mode ===");
    startAP();
  }
  
  void saveWiFi1(String ssid, String pass, String ip, String localDomain = "") {
    wifi1_ssid = ssid;
    wifi1_pass = pass;
    wifi1_ip = ip;
    wifi1_local = localDomain;
    prefs.begin("net", false);
    prefs.putString("w1ssid", ssid);
    prefs.putString("w1pass", pass);
    prefs.putString("w1ip", ip);
    prefs.putString("w1local", localDomain);
    prefs.end();
    
    // Kayıt sonrası hemen bağlan
    Serial.println("WiFi 1 kaydedildi, baglaniyor...");
    connectWiFi(ssid, pass, ip);
  }
  
  void saveWiFi2(String ssid, String pass, String ip, String localDomain = "") {
    wifi2_ssid = ssid;
    wifi2_pass = pass;
    wifi2_ip = ip;
    wifi2_local = localDomain;
    prefs.begin("net", false);
    prefs.putString("w2ssid", ssid);
    prefs.putString("w2pass", pass);
    prefs.putString("w2ip", ip);
    prefs.putString("w2local", localDomain);
    prefs.end();
    
    // Kayıt sonrası hemen bağlan
    Serial.println("WiFi 2 kaydedildi, baglaniyor...");
    connectWiFi(ssid, pass, ip);
  }
  
  String getChipID() { return chipID; }
  String getAPSSID() { return apSSID; }
  bool isAPActive() { return apActive; }
  String getWiFi1SSID() { return wifi1_ssid; }
  String getWiFi1Pass() { return wifi1_pass; }  // NEW: For WiFi watchdog
  String getWiFi1IP() { return wifi1_ip; }
  String getWiFi1Local() { return wifi1_local; }
  String getWiFi2SSID() { return wifi2_ssid; }
  String getWiFi2Pass() { return wifi2_pass; }  // NEW: For WiFi watchdog
  String getWiFi2IP() { return wifi2_ip; }
  String getWiFi2Local() { return wifi2_local; }
  
  String getStatus() {
    if (WiFi.status() == WL_CONNECTED) {
      return "WiFi: " + WiFi.localIP().toString();
    } else if (apActive) {
      return "AP: " + WiFi.softAPIP().toString();
    }
    return "Disconnected";
  }
  
  void setAP(bool enable) {
    if (enable && !apActive) startAP();
    else if (!enable && apActive) stopAP();
  }
};

#endif
