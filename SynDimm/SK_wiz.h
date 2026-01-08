/**
 * SK_wiz.h
 * SmartKraft SynDimm - Philips WiZ Bulb Control
 * Version: v1.1.1
 * 
 * ========================================
 * WiZ LAMBA KONTROLÜ
 * ========================================
 * - UDP protokolü (port 38899)
 * - Parlaklık kontrolü (0-100%)
 * - RGB renk kontrolü
 * - Hue bazlı renk çarkı
 * - Encoder ile kontrol
 * ========================================
 */

#ifndef SK_WIZ_H
#define SK_WIZ_H

#include <Arduino.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include "SK_config.h"

// WiZ Configuration
#define WIZ_UDP_PORT 38899
#define WIZ_TIMEOUT_MS 2000
#define WIZ_BROADCAST_IP "255.255.255.255"

// WiZ Device Type (150-159 aralığı)
#define DIMMER_WIZ_RGBW 150        // WiZ RGB + White bulb
#define DIMMER_WIZ_TUNABLE 151     // WiZ Tunable White bulb
#define DIMMER_WIZ_DIMMABLE 152    // WiZ Dimmable bulb

// Anonymous namespace to prevent multiple definition
namespace {
    WiFiUDP wizUDP;
    bool wizUDPInitialized = false;
    Preferences wizPrefs;
    String lastConnectedWiZIP = "";
}

// WiZ Device Structure
struct WiZDevice {
    String ip;
    String mac;
    int rssi;
    bool state;             // on/off
    int brightness;         // 0-255 (dimming)
    int colorTemp;          // Kelvin (2200-6500)
    uint8_t r, g, b;        // RGB values
    uint16_t hue;           // 0-360 derece
    uint8_t saturation;     // 0-100
    bool supportsRGB;
    bool supportsTunable;
    bool isConnected;
    unsigned long lastUpdate;
    String errorMessage;
};

// WiZ Color Mode
enum WiZColorMode {
    WIZ_MODE_BRIGHTNESS,    // Sadece parlaklık
    WIZ_MODE_COLOR,         // RGB/Hue kontrolü
    WIZ_MODE_TEMP           // Renk sıcaklığı
};

// Anonymous namespace for WiZ state
namespace {
    WiZDevice wizDevice;
    WiZColorMode wizColorMode = WIZ_MODE_BRIGHTNESS;
    bool wizColorControlActive = false;  // Encoder renk kontrolü aktif mi
}

// ========================================
// RENK DÖNÜŞÜM FONKSİYONLARI
// ========================================

// Hue (0-360) -> RGB dönüşümü (saturation=100, value=100)
void hueToRGB(uint16_t hue, uint8_t &r, uint8_t &g, uint8_t &b) {
    float h = hue / 60.0f;
    int i = (int)h;
    float f = h - i;
    
    uint8_t v = 255;
    uint8_t p = 0;
    uint8_t q = (uint8_t)(255 * (1 - f));
    uint8_t t = (uint8_t)(255 * f);
    
    switch(i % 6) {
        case 0: r = v; g = t; b = p; break;
        case 1: r = q; g = v; b = p; break;
        case 2: r = p; g = v; b = t; break;
        case 3: r = p; g = q; b = v; break;
        case 4: r = t; g = p; b = v; break;
        case 5: r = v; g = p; b = q; break;
    }
}

// RGB -> Hue dönüşümü
uint16_t rgbToHue(uint8_t r, uint8_t g, uint8_t b) {
    float rf = r / 255.0f;
    float gf = g / 255.0f;
    float bf = b / 255.0f;
    
    float maxVal = max(max(rf, gf), bf);
    float minVal = min(min(rf, gf), bf);
    float delta = maxVal - minVal;
    
    if (delta < 0.001f) return 0;
    
    float hue = 0;
    if (maxVal == rf) {
        hue = 60 * fmod((gf - bf) / delta, 6);
    } else if (maxVal == gf) {
        hue = 60 * ((bf - rf) / delta + 2);
    } else {
        hue = 60 * ((rf - gf) / delta + 4);
    }
    
    if (hue < 0) hue += 360;
    return (uint16_t)hue;
}

// ========================================
// WIZ UDP İLETİŞİM
// ========================================

// Forward declarations
bool connectToWiZ(const String& ip);
bool getWiZStatus(const String& ip);

// UDP başlat ve kayıtlı WiZ'i yükle
void initWiZUDP() {
    if (!wizUDPInitialized) {
        wizUDP.begin(WIZ_UDP_PORT);
        wizUDPInitialized = true;
        
        // Kayıtlı WiZ IP'yi yükle
        wizPrefs.begin("wiz-settings", false);
        lastConnectedWiZIP = wizPrefs.getString("last_wiz_ip", "");
        wizPrefs.end();
        
        WIZ_DEBUG("[WIZ] UDP init");
    }
}

// Son WiZ bağlantısını kaydet
void saveLastWiZConnection(const String& ip) {
    lastConnectedWiZIP = ip;
    wizPrefs.begin("wiz-settings", false);
    wizPrefs.putString("last_wiz_ip", ip);
    wizPrefs.end();
}

// Son WiZ bağlantısını temizle
void clearLastWiZConnection() {
    lastConnectedWiZIP = "";
    wizPrefs.begin("wiz-settings", false);
    wizPrefs.putString("last_wiz_ip", "");
    wizPrefs.end();
}

// WiZ auto-reconnect
void autoReconnectWiZ() {
    if (lastConnectedWiZIP.length() > 0 && !wizDevice.isConnected) {
        WIZ_DEBUGF("[WIZ] Reconnect %s\n", lastConnectedWiZIP.c_str());
        connectToWiZ(lastConnectedWiZIP);
    }
}

// WiZ cihaza UDP mesaj gönder
bool sendWiZMessage(const String& ip, const String& message) {
    if (!wizUDPInitialized) initWiZUDP();
    
    // WiFi bağlı mı kontrol et
    if (WiFi.status() != WL_CONNECTED) {
        WIZ_DEBUG("[WIZ] WiFi not connected!");
        return false;
    }
    
    IPAddress targetIP;
    if (!targetIP.fromString(ip)) {
        WIZ_DEBUGF("[WIZ] Invalid IP: %s\n", ip.c_str());
        return false;
    }
    
    WIZ_DEBUGF("[WIZ] -> %s:%d\n", ip.c_str(), WIZ_UDP_PORT);
    
    wizUDP.beginPacket(targetIP, WIZ_UDP_PORT);
    size_t written = wizUDP.print(message);
    int result = wizUDP.endPacket();
    
    WIZ_DEBUGF("[WIZ] Sent %d bytes, result=%d\n", written, result);
    return (result == 1);
}

// WiZ cihazdan yanıt al (blocking, timeout ile)
String receiveWiZResponse(int timeoutMs = WIZ_TIMEOUT_MS) {
    unsigned long startTime = millis();
    
    WIZ_DEBUGF("[WIZ] Waiting %dms...\n", timeoutMs);
    
    while (millis() - startTime < timeoutMs) {
        int packetSize = wizUDP.parsePacket();
        if (packetSize > 0) {
            char buffer[512];
            int len = wizUDP.read(buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';
            WIZ_DEBUGF("[WIZ] Got %d bytes\n", len);
            return String(buffer);
        }
        delay(10);
    }
    
    WIZ_DEBUG("[WIZ] TIMEOUT!");
    return "";
}

// ========================================
// WIZ KOMUTLARI
// ========================================

// WiZ durumunu sorgula (getPilot)
bool getWiZStatus(const String& ip) {
    String message = "{\"method\":\"getPilot\",\"params\":{}}";
    
    if (!sendWiZMessage(ip, message)) return false;
    
    String response = receiveWiZResponse();
    if (response.length() == 0) {
        wizDevice.errorMessage = "No response";
        return false;
    }
    
    // Parse JSON response
    JsonDocument doc;
    DeserializationError error = deserializeJson(doc, response);
    
    if (error) {
        wizDevice.errorMessage = "JSON parse error";
        return false;
    }
    
    // Extract state
    JsonObject result = doc["result"];
    if (!result) {
        wizDevice.errorMessage = "Invalid response";
        return false;
    }
    
    wizDevice.mac = result["mac"].as<String>();
    wizDevice.rssi = result["rssi"] | 0;
    wizDevice.state = result["state"] | false;
    wizDevice.brightness = result["dimming"] | 100;
    wizDevice.colorTemp = result["temp"] | 4000;
    
    // RGB değerleri varsa
    if (result["r"].is<int>()) {
        wizDevice.r = result["r"] | 0;
        wizDevice.g = result["g"] | 0;
        wizDevice.b = result["b"] | 0;
        wizDevice.hue = rgbToHue(wizDevice.r, wizDevice.g, wizDevice.b);
        wizDevice.supportsRGB = true;
    }
    
    wizDevice.isConnected = true;
    wizDevice.lastUpdate = millis();
    wizDevice.errorMessage = "";
    
    return true;
}

// WiZ parlaklık ayarla (0-100)
bool setWiZBrightness(const String& ip, int brightness) {
    if (brightness < 10) brightness = 10;   // WiZ minimum 10%
    if (brightness > 100) brightness = 100;
    
    String message = "{\"method\":\"setPilot\",\"params\":{\"dimming\":" + 
                     String(brightness) + ",\"state\":true}}";
    
    if (!sendWiZMessage(ip, message)) return false;
    
    String response = receiveWiZResponse();
    if (response.indexOf("success") > 0) {
        wizDevice.brightness = brightness;
        wizDevice.state = true;
        return true;
    }
    
    return false;
}

// WiZ RGB renk ayarla
bool setWiZRGB(const String& ip, uint8_t r, uint8_t g, uint8_t b) {
    String message = "{\"method\":\"setPilot\",\"params\":{\"r\":" + 
                     String(r) + ",\"g\":" + String(g) + ",\"b\":" + String(b) + 
                     ",\"dimming\":" + String(wizDevice.brightness) + ",\"state\":true}}";
    
    if (!sendWiZMessage(ip, message)) return false;
    
    String response = receiveWiZResponse();
    if (response.indexOf("success") > 0) {
        wizDevice.r = r;
        wizDevice.g = g;
        wizDevice.b = b;
        wizDevice.hue = rgbToHue(r, g, b);
        wizDevice.state = true;
        return true;
    }
    
    return false;
}

// WiZ Hue ayarla (0-360)
bool setWiZHue(const String& ip, uint16_t hue) {
    if (hue >= 360) hue = hue % 360;
    
    uint8_t r, g, b;
    hueToRGB(hue, r, g, b);
    
    return setWiZRGB(ip, r, g, b);
}

// WiZ aç/kapat
bool toggleWiZ(const String& ip) {
    bool newState = !wizDevice.state;
    
    String message = "{\"method\":\"setPilot\",\"params\":{\"state\":" + 
                     String(newState ? "true" : "false") + "}}";
    
    if (!sendWiZMessage(ip, message)) return false;
    
    String response = receiveWiZResponse();
    if (response.indexOf("success") > 0) {
        wizDevice.state = newState;
        return true;
    }
    
    return false;
}

// WiZ kapat
bool turnOffWiZ(const String& ip) {
    String message = "{\"method\":\"setPilot\",\"params\":{\"state\":false}}";
    
    if (!sendWiZMessage(ip, message)) return false;
    
    String response = receiveWiZResponse();
    if (response.indexOf("success") > 0) {
        wizDevice.state = false;
        return true;
    }
    
    return false;
}

// ========================================
// WIZ DISCOVERY (Broadcast)
// ========================================

struct DiscoveredWiZ {
    String ip;
    String mac;
    bool valid;
};

// Ağda WiZ cihaz ara (broadcast)
std::vector<DiscoveredWiZ> discoverWiZDevices(int timeoutMs = 3000) {
    std::vector<DiscoveredWiZ> devices;
    
    if (!wizUDPInitialized) initWiZUDP();
    
    // Broadcast getPilot
    String message = "{\"method\":\"getPilot\",\"params\":{}}";
    
    IPAddress broadcastIP(255, 255, 255, 255);
    wizUDP.beginPacket(broadcastIP, WIZ_UDP_PORT);
    wizUDP.print(message);
    wizUDP.endPacket();
    
    // Yanıtları topla
    unsigned long startTime = millis();
    while (millis() - startTime < timeoutMs) {
        int packetSize = wizUDP.parsePacket();
        if (packetSize > 0) {
            char buffer[512];
            int len = wizUDP.read(buffer, sizeof(buffer) - 1);
            buffer[len] = '\0';
            
            IPAddress remoteIP = wizUDP.remoteIP();
            String ip = remoteIP.toString();
            
            // Parse MAC from response
            JsonDocument doc;
            if (deserializeJson(doc, buffer) == DeserializationError::Ok) {
                JsonObject result = doc["result"];
                if (result && result["mac"].is<const char*>()) {
                    DiscoveredWiZ device;
                    device.ip = ip;
                    device.mac = result["mac"].as<String>();
                    device.valid = true;
                    
                    // Tekrar ekleme
                    bool exists = false;
                    for (auto& d : devices) {
                        if (d.mac == device.mac) {
                            exists = true;
                            break;
                        }
                    }
                    
                    if (!exists) {
                        devices.push_back(device);
                    }
                }
            }
        }
        delay(50);
    }
    
    return devices;
}

// ========================================
// ENCODER RENK KONTROLÜ
// ========================================

// Renk kontrolü aktifleştir/deaktifleştir
void setWiZColorControlActive(bool active) {
    wizColorControlActive = active;
}

bool isWiZColorControlActive() {
    return wizColorControlActive;
}

// Encoder ile Hue değiştir (direction: -1 veya +1)
void adjustWiZHue(int direction, int step = 15) {
    WIZ_DEBUGF("[WIZ] adjustHue: connected=%d, supportsRGB=%d\n", wizDevice.isConnected, wizDevice.supportsRGB);
    
    if (!wizDevice.isConnected) return;
    
    // RGB desteklemiyor olsa bile dene - çoğu WiZ lamba RGB destekler
    int newHue = wizDevice.hue + (direction * step);
    if (newHue < 0) newHue += 360;
    if (newHue >= 360) newHue -= 360;
    
    wizDevice.hue = newHue;
    
    uint8_t r, g, b;
    hueToRGB(newHue, r, g, b);
    
    WIZ_DEBUGF("[WIZ] Hue=%d -> R:%d G:%d B:%d\n", newHue, r, g, b);
    
    if (setWiZRGB(wizDevice.ip, r, g, b)) {
        wizDevice.supportsRGB = true;  // Başarılıysa RGB desteği var demek
    }
}

// Encoder ile parlaklık değiştir
void adjustWiZBrightness(int direction, int ratio = 5) {
    if (!wizDevice.isConnected) return;
    
    int newBrightness = wizDevice.brightness + (direction * ratio);
    if (newBrightness < 10) newBrightness = 10;
    if (newBrightness > 100) newBrightness = 100;
    
    setWiZBrightness(wizDevice.ip, newBrightness);
}

// ========================================
// WIZ DURUM JSON (Web için)
// ========================================

String getWiZStatusJSON() {
    JsonDocument doc;
    
    doc["connected"] = wizDevice.isConnected;
    doc["ip"] = wizDevice.ip;
    doc["mac"] = wizDevice.mac;
    doc["state"] = wizDevice.state;
    doc["brightness"] = wizDevice.brightness;
    doc["colorTemp"] = wizDevice.colorTemp;
    doc["r"] = wizDevice.r;
    doc["g"] = wizDevice.g;
    doc["b"] = wizDevice.b;
    doc["hue"] = wizDevice.hue;
    doc["supportsRGB"] = wizDevice.supportsRGB;
    doc["colorControlActive"] = wizColorControlActive;
    doc["lastUpdate"] = wizDevice.lastUpdate;
    doc["errorMessage"] = wizDevice.errorMessage;
    
    String output;
    serializeJson(doc, output);
    return output;
}

// WiZ cihaza bağlan
bool connectToWiZ(const String& ip) {
    wizDevice.ip = ip;
    wizDevice.isConnected = false;
    
    if (!wizUDPInitialized) initWiZUDP();
    
    WIZ_DEBUGF("[WIZ] Connect %s\n", ip.c_str());
    
    if (getWiZStatus(ip)) {
        wizDevice.isConnected = true;
        saveLastWiZConnection(ip);
        WIZ_DEBUGF("[WIZ] OK! %s %d%%\n", wizDevice.state?"ON":"OFF", wizDevice.brightness);
        return true;
    }
    
    wizDevice.errorMessage = "Connection failed";
    WIZ_DEBUG("[WIZ] FAILED");
    return false;
}

// WiZ bağlantısını kes
void disconnectWiZ() {
    wizDevice.isConnected = false;
    wizDevice.ip = "";
    wizColorControlActive = false;
    clearLastWiZConnection();
}

// ========================================
// WIZ LOOP (AUTO-RECONNECT & STATUS UPDATE)
// ========================================

// WiZ status update ve auto-reconnect değişkenleri
namespace {
    unsigned long lastWiZStatusUpdate = 0;
    unsigned long lastWiZReconnectAttempt = 0;
    uint8_t wizStatusFailCount = 0;
    uint8_t wizReconnectAttemptCount = 0;
    
    static const unsigned long WIZ_STATUS_UPDATE_INTERVAL = 5000;    // 5 saniyede bir durum sorgusu
    static const unsigned long WIZ_RECONNECT_INTERVAL = 5000;        // 5 saniyede bir yeniden bağlantı
    static const unsigned long WIZ_SLOW_RECONNECT_INTERVAL = 30000;  // 30 saniyede bir (yavaş mod)
    static const uint8_t WIZ_MAX_STATUS_FAILURES = 3;                // 3 başarısız sorgu = disconnect
    static const uint8_t WIZ_MAX_RECONNECT_ATTEMPTS = 12;            // 12 deneme sonra yavaş mod
}

// WiZ Loop - dimmerLoop ile aynı mantık
void wizLoop() {
    // WiFi bağlı değilse çık
    if (WiFi.status() != WL_CONNECTED) return;
    
    unsigned long currentTime = millis();
    
    // Bağlıyken periyodik durum güncellemesi
    if (wizDevice.isConnected && (currentTime - lastWiZStatusUpdate > WIZ_STATUS_UPDATE_INTERVAL)) {
        lastWiZStatusUpdate = currentTime;
        
        if (!getWiZStatus(wizDevice.ip)) {
            wizStatusFailCount++;
            WIZ_DEBUGF("[WIZ] Status fail #%d\n", wizStatusFailCount);
            
            if (wizStatusFailCount >= WIZ_MAX_STATUS_FAILURES) {
                WIZ_DEBUG("[WIZ] Device unreachable - auto disconnect");
                wizDevice.isConnected = false;
                wizDevice.errorMessage = "Connection lost";
                wizStatusFailCount = 0;
                wizReconnectAttemptCount = 0;
            }
        } else {
            wizStatusFailCount = 0;  // Başarılıysa sıfırla
        }
    }
    
    // Bağlı değilse ve kayıtlı IP varsa auto-reconnect
    if (!wizDevice.isConnected && lastConnectedWiZIP.length() > 0) {
        unsigned long reconnectInterval = (wizReconnectAttemptCount >= WIZ_MAX_RECONNECT_ATTEMPTS) 
                                          ? WIZ_SLOW_RECONNECT_INTERVAL 
                                          : WIZ_RECONNECT_INTERVAL;
        
        if (currentTime - lastWiZReconnectAttempt > reconnectInterval) {
            lastWiZReconnectAttempt = currentTime;
            wizReconnectAttemptCount++;
            
            WIZ_DEBUGF("[WIZ] Auto-reconnect #%d to %s\n", wizReconnectAttemptCount, lastConnectedWiZIP.c_str());
            
            if (connectToWiZ(lastConnectedWiZIP)) {
                WIZ_DEBUG("[WIZ] Auto-reconnect successful!");
                wizReconnectAttemptCount = 0;
            } else {
                if (wizReconnectAttemptCount >= WIZ_MAX_RECONNECT_ATTEMPTS) {
                    WIZ_DEBUGF("[WIZ] Switching to slow reconnect (every %lu sec)\n", WIZ_SLOW_RECONNECT_INTERVAL / 1000);
                }
            }
        }
    }
}

#endif // SK_WIZ_H
