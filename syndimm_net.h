/*
 * SynDimm - Minimal Network Manager
 * ESP32C6 WiFi/AP - Stack overflow önleme
 */

#ifndef SYNDIMM_NET_H
#define SYNDIMM_NET_H

#include <WiFi.h>
#include <Preferences.h>

class SynDimmNet {
private:
  Preferences prefs;
  String chipID;
  String apSSID;
  bool apActive;
  
  String wifi1_ssid, wifi1_pass, wifi1_ip;
  String wifi2_ssid, wifi2_pass, wifi2_ip;
  
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
    wifi2_ssid = prefs.getString("w2ssid", "");
    wifi2_pass = prefs.getString("w2pass", "");
    wifi2_ip = prefs.getString("w2ip", "");
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
    
    WiFi.mode(WIFI_STA);
    
    // Statik IP varsa ayarla
    if (ip.length() > 0 && ip != "0.0.0.0") {
      IPAddress localIP, gateway, subnet;
      if (localIP.fromString(ip)) {
        // Gateway: IP'nin son oktetini 1 yap (örn: 192.168.1.1)
        gateway = localIP;
        gateway[3] = 1;
        subnet.fromString("255.255.255.0");
        WiFi.config(localIP, gateway, subnet);
      }
    }
    
    WiFi.begin(ssid.c_str(), pass.c_str());
    
    int count = 0;
    while (WiFi.status() != WL_CONNECTED && count < 20) {
      delay(250);  // 500ms -> 250ms (daha kısa delay)
      yield();     // WDT reset
      count++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      // WiFi bağlıysa AP'yi kapat
      if (apActive) {
        stopAP();
        Serial.println("AP kapatildi - WiFi bagli");
      }
      return true;
    }
    
    return false;
  }
  
  void autoConnect() {
    if (connectWiFi(wifi1_ssid, wifi1_pass, wifi1_ip)) {
      Serial.println("WiFi 1 OK");
      return;
    }
    if (connectWiFi(wifi2_ssid, wifi2_pass, wifi2_ip)) {
      Serial.println("WiFi 2 OK");
      return;
    }
    Serial.println("AP mode");
    startAP();
  }
  
  void saveWiFi1(String ssid, String pass, String ip) {
    wifi1_ssid = ssid;
    wifi1_pass = pass;
    wifi1_ip = ip;
    prefs.begin("net", false);
    prefs.putString("w1ssid", ssid);
    prefs.putString("w1pass", pass);
    prefs.putString("w1ip", ip);
    prefs.end();
    
    // Kayıt sonrası hemen bağlan
    Serial.println("WiFi 1 kaydedildi, baglaniyor...");
    connectWiFi(ssid, pass, ip);
  }
  
  void saveWiFi2(String ssid, String pass, String ip) {
    wifi2_ssid = ssid;
    wifi2_pass = pass;
    wifi2_ip = ip;
    prefs.begin("net", false);
    prefs.putString("w2ssid", ssid);
    prefs.putString("w2pass", pass);
    prefs.putString("w2ip", ip);
    prefs.end();
    
    // Kayıt sonrası hemen bağlan
    Serial.println("WiFi 2 kaydedildi, baglaniyor...");
    connectWiFi(ssid, pass, ip);
  }
  
  String getChipID() { return chipID; }
  String getAPSSID() { return apSSID; }
  bool isAPActive() { return apActive; }
  String getWiFi1SSID() { return wifi1_ssid; }
  String getWiFi1IP() { return wifi1_ip; }
  String getWiFi2SSID() { return wifi2_ssid; }
  String getWiFi2IP() { return wifi2_ip; }
  
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
