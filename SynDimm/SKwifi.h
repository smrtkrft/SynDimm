/**
 * SKwifi.h
 * SmartKraft SynDimm - WiFi & AP Mode Management
 * Version: v1.3.0
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * WiFi sadece AP (Access Point) modunda!
 * - Kullanıcı sadece cihaza direkt bağlanır
 * - İnternet bağlantısı YOK
 * - Dış ağ erişimi YOK
 * - Sadece lokal ayar arayüzü için
 * ========================================
 */

#ifndef SKWIFI_H
#define SKWIFI_H

#include <WiFi.h>
#include <DNSServer.h>
#include <ESPmDNS.h>
#include <Preferences.h>
#include <esp_wifi.h>
#include <ArduinoJson.h>
#include "SK_config.h"

// DNS Server for Captive Portal (AP mode mDNS alternative)
static DNSServer dnsServer;
static const byte DNS_PORT = 53;

class SKWiFi {
private:
    String chipID;
    String apSSID;
    Preferences prefs;
    unsigned long lastKeepAliveTime;
    unsigned long lastReconnectAttempt;
    uint8_t reconnectFailCount;
    uint8_t apScanCount;
    static const unsigned long KEEP_ALIVE_INTERVAL = 30000;      // 30 saniyede bir kontrol
    static const unsigned long RECONNECT_COOLDOWN = 10000;       // 10 saniye bekleme
    static const unsigned long AP_SCAN_INTERVAL_FAST = 60000;    // İlk 5 deneme: 1 dakika
    static const unsigned long AP_SCAN_INTERVAL_SLOW = 3600000;  // 5+ deneme: 1 saat
    static const uint8_t MAX_RECONNECT_ATTEMPTS = 5;             // 5 deneme sonrası AP mode
    
    // WiFi Restart Sabitleri (WiFi donması önleme)
    static const unsigned long WIFI_PING_CHECK_INTERVAL = 120000;      // 2 dakikada bir ping kontrolü
    static const unsigned long WIFI_FORCE_RESTART_INTERVAL = 14400000; // 4 saatte bir zorunlu restart (6 saat -> 4 saat)
    static const uint8_t MAX_PING_FAILS = 3;                           // 3 başarısız ping sonrası restart
    
    // Network configuration
    struct NetworkConfig {
        String ssid;
        String password;
        String staticIP;
        String mdns;
        bool enabled;
    };
    
    NetworkConfig primaryNetwork;
    NetworkConfig backupNetwork;
    bool connectedToWiFi;
    bool apModeActive;
    bool apModeEnabled;     // Kullanıcı tarafından AP Mode açık/kapalı ayarı
    bool dnsServerActive;   // DNS Server durumu (AP mode captive portal için)
    unsigned long lastScanTime;
    
    // mDNS yönetimi
    bool mdnsActive;                        // mDNS çalışıyor mu?
    String currentMdnsHostname;             // Şu anki hostname
    unsigned long lastMdnsCheck;            // Son sağlık kontrolü
    uint8_t mdnsFailCount;                  // Ardışık başarısız deneme sayısı
    static const unsigned long MDNS_CHECK_INTERVAL = 30000;  // 30 saniyede bir kontrol
    static const uint8_t MDNS_MAX_RETRIES = 5;               // Max retry sayısı
    
    // WiFi restart değişkenleri
    unsigned long lastPingCheckTime;
    unsigned long lastWiFiRestartTime;
    unsigned long wifiConnectedSince;      // WiFi bağlantı başlangıç zamanı
    uint8_t pingFailCount;
    uint32_t wifiRestartCount;             // Toplam restart sayısı (debug için)
    
    // mDNS başlatma (internal)
    bool startMDNS(const String& hostname) {
        MDNS.end();  // Önce temizle
        delay(50);
        
        if (MDNS.begin(hostname.c_str())) {
            MDNS.addService("http", "tcp", 80);
            MDNS.addService("syndimm", "tcp", 80);  // Custom service for discovery
            mdnsActive = true;
            currentMdnsHostname = hostname;
            mdnsFailCount = 0;
            DEBUG_PRINTLN("[mDNS] Started: " + hostname + ".local");
            return true;
        }
        
        mdnsActive = false;
        mdnsFailCount++;
        DEBUG_PRINTLN("[mDNS] Failed to start: " + hostname);
        return false;
    }
    
    // mDNS sağlık kontrolü
    bool checkMDNSHealth() {
        if (!mdnsActive) return false;
        
        // WiFi bağlantısı yoksa mDNS çalışamaz
        if (!connectedToWiFi || WiFi.status() != WL_CONNECTED) {
            return false;
        }
        
        // IP adresi yoksa mDNS çalışamaz
        if (WiFi.localIP() == IPAddress(0, 0, 0, 0)) {
            DEBUG_PRINTLN("[mDNS] IP adresi yok - mDNS calismiyor");
            return false;
        }
        
        // ESP32 mDNS kütüphanesi durumu sorgulama API'si sağlamıyor
        // Dolayısıyla WiFi sağlıklıysa mDNS'in de sağlıklı olduğunu varsayıyoruz
        // Periyodik yenileme (saatlik) bu varsayımın riskini azaltır
        return true;
    }
    
    // WiFi stack sağlık kontrolü - İNTERNET GEREKTİRMEZ!
    // Sadece WiFi.status() ve temel network stack kontrolü
    bool checkWiFiHealth() {
        // 1. Temel WiFi durumu
        if (WiFi.status() != WL_CONNECTED) {
            DEBUG_PRINTLN("[WiFi] Status: NOT CONNECTED");
            return false;
        }
        
        // 2. IP adresi kontrolü - geçerli IP var mı?
        IPAddress localIP = WiFi.localIP();
        if (localIP == IPAddress(0, 0, 0, 0)) {
            DEBUG_PRINTLN("[WiFi] IP adresi alinamadi");
            return false;
        }
        
        // 3. Gateway IP kontrolü - DHCP veya static config çalışıyor mu?
        IPAddress gateway = WiFi.gatewayIP();
        if (gateway == IPAddress(0, 0, 0, 0)) {
            DEBUG_PRINTLN("[WiFi] Gateway IP alinamadi");
            return false;
        }
        
        // 4. RSSI kontrolü - sinyal var mı?
        int rssi = WiFi.RSSI();
        if (rssi == 0) {
            DEBUG_PRINTLN("[WiFi] RSSI alinamadi - WiFi stack sorunu");
            return false;
        }
        
        // 5. BSSID kontrolü - AP'ye bağlı mı?
        uint8_t* bssid = WiFi.BSSID();
        if (bssid == nullptr) {
            DEBUG_PRINTLN("[WiFi] BSSID alinamadi");
            return false;
        }
        
        // Tüm kontroller başarılı
        DEBUG_PRINTF("[WiFi] Health OK - IP: %s, RSSI: %d dBm\n", 
                     localIP.toString().c_str(), rssi);
        return true;
    }
    
    // Gateway'e ARP/ping testi (opsiyonel, sadece debug için)
    // NOT: Bu fonksiyon internet gerektirmez, sadece lokal ağ kontrolü yapar
    bool pingGateway() {
        if (WiFi.status() != WL_CONNECTED) {
            return false;
        }
        
        IPAddress gateway = WiFi.gatewayIP();
        if (gateway == IPAddress(0, 0, 0, 0)) {
            return false;
        }
        
        // Kısa timeout ile gateway'e TCP bağlantısı dene
        // Bu sadece gateway erişilebilirlik testi - internet gerektirmez
        WiFiClient testClient;
        testClient.setTimeout(2000);  // 2 saniye timeout (kısaltıldı)
        
        bool success = testClient.connect(gateway, 80);
        testClient.stop();
        
        // Port 80 kapalı olabilir - bu normal
        // Önemli olan WiFi stack'in çalışması, internet değil!
        if (!success) {
            // Gateway port 80 kapalı olabilir, bu hata değil
            // checkWiFiHealth() zaten WiFi stack'i kontrol ediyor
            DEBUG_PRINTF("[WiFi] Gateway port 80 kapali (normal olabilir)\n");
        }
        
        return success;
    }
    
    // WiFi donanımını yeniden başlat
    void restartWiFiHardware() {
        if (!connectedToWiFi || apModeActive) {
            return;  // Sadece WiFi bağlıyken çalış
        }
        
        wifiRestartCount++;
        unsigned long uptime = (millis() - wifiConnectedSince) / 1000;
        
        DEBUG_PRINTLN("\n========================================");
        DEBUG_PRINTLN("      WIFI DONANIM YENIDEN BASLATMA");
        DEBUG_PRINTLN("========================================");
        DEBUG_PRINTF("Restart #%lu - Uptime: %lu saniye\n", wifiRestartCount, uptime);
        DEBUG_PRINTF("Ping fail count: %d\n", pingFailCount);
        
        // Mevcut ağ bilgilerini kaydet
        String currentSSID = WiFi.SSID();
        
        // WiFi'yi tamamen kapat
        WiFi.disconnect(true);
        delay(100);
        
        esp_wifi_stop();
        delay(500);
        
        esp_wifi_start();
        delay(100);
        
        // WiFi'yi yeniden başlat
        WiFi.mode(WIFI_STA);
        delay(100);
        
        // Kayıtlı ağa yeniden bağlan
        NetworkConfig* targetNetwork = nullptr;
        if (currentSSID == primaryNetwork.ssid) {
            targetNetwork = &primaryNetwork;
        } else if (currentSSID == backupNetwork.ssid) {
            targetNetwork = &backupNetwork;
        }
        
        if (targetNetwork != nullptr && targetNetwork->enabled) {
            DEBUG_PRINTLN("Yeniden baglaniliyor: " + targetNetwork->ssid);
            
            // Static IP varsa ayarla
            if (targetNetwork->staticIP.length() > 0) {
                IPAddress ip, gateway, subnet, dns1, dns2;
                if (ip.fromString(targetNetwork->staticIP)) {
                    String gatewayStr = targetNetwork->staticIP.substring(0, targetNetwork->staticIP.lastIndexOf('.')) + ".1";
                    gateway.fromString(gatewayStr);
                    subnet.fromString("255.255.255.0");
                    dns1.fromString("8.8.8.8");
                    dns2.fromString("8.8.4.4");
                    WiFi.config(ip, gateway, subnet, dns1, dns2);
                }
            }
            
            WiFi.begin(targetNetwork->ssid.c_str(), targetNetwork->password.c_str());
            
            // Bağlantıyı bekle (max 15 saniye)
            unsigned long startTime = millis();
            while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < 15000) {
                delay(500);
                DEBUG_PRINT(".");
            }
            
            if (WiFi.status() == WL_CONNECTED) {
                DEBUG_PRINTLN("\n[OK] WiFi yeniden baglandi!");
                DEBUG_PRINTLN("IP: " + WiFi.localIP().toString());
                
                // Power save kapat
                disablePowerSave();
                
                // mDNS yenile
                refreshMDNS(targetNetwork->mdns.length() > 0 ? targetNetwork->mdns : "dimm");
                
                // Değişkenleri sıfırla
                wifiConnectedSince = millis();
                lastWiFiRestartTime = millis();
                lastPingCheckTime = millis();
                lastMdnsCheck = millis();  // mDNS check timer da sıfırla
                pingFailCount = 0;
                
                DEBUG_PRINTLN("========================================\n");
            } else {
                DEBUG_PRINTLN("\n[ERROR] WiFi yeniden baglanti BASARISIZ!");
                connectedToWiFi = false;
                reconnectFailCount = 0;
                DEBUG_PRINTLN("========================================\n");
            }
        } else {
            DEBUG_PRINTLN("[ERROR] Hedef ag bulunamadi!");
            connectedToWiFi = false;
        }
    }

    // Scan for available networks and check if saved networks exist
    bool scanForSavedNetworks(String& foundSSID, bool& isPrimary) {
        DEBUG_PRINTLN("\n=== Scanning WiFi Networks ===");
        int networksFound = WiFi.scanNetworks();
        
        // Hata kontrolü
        if (networksFound < 0) {
            DEBUG_PRINTF("WiFi scan error: %d\n", networksFound);
            WiFi.scanDelete();  // Her durumda temizle
            return false;
        }
        
        if (networksFound == 0) {
            DEBUG_PRINTLN("No networks found");
            WiFi.scanDelete();  // Her durumda temizle
            return false;
        }
        
        DEBUG_PRINTLN("Networks found: " + String(networksFound));
        
        bool found = false;
        
        // Check if Primary network is available
        if (!found && primaryNetwork.enabled && primaryNetwork.ssid.length() > 0) {
            for (int i = 0; i < networksFound; i++) {
                if (WiFi.SSID(i) == primaryNetwork.ssid) {
                    DEBUG_PRINTLN("[OK] Found Primary WiFi: " + primaryNetwork.ssid);
                    foundSSID = primaryNetwork.ssid;
                    isPrimary = true;
                    found = true;
                    break;
                }
            }
        }
        
        // Check if Backup network is available
        if (!found && backupNetwork.enabled && backupNetwork.ssid.length() > 0) {
            for (int i = 0; i < networksFound; i++) {
                if (WiFi.SSID(i) == backupNetwork.ssid) {
                    DEBUG_PRINTLN("[OK] Found Backup WiFi: " + backupNetwork.ssid);
                    foundSSID = backupNetwork.ssid;
                    isPrimary = false;
                    found = true;
                    break;
                }
            }
        }
        
        if (!found) {
            DEBUG_PRINTLN("No saved networks found in scan");
        }
        
        // Her zaman temizle - bellek sızıntısı önleme
        WiFi.scanDelete();
        return found;
    }
    
public:
    SKWiFi() {
        chipID = "";
        apSSID = "";
        connectedToWiFi = false;
        apModeActive = false;
        dnsServerActive = false;
        mdnsActive = false;
        currentMdnsHostname = "dimm";
        lastMdnsCheck = 0;
        mdnsFailCount = 0;
        lastScanTime = 0;
        lastKeepAliveTime = 0;
        lastReconnectAttempt = 0;
        reconnectFailCount = 0;
        apScanCount = 0;
        lastPingCheckTime = 0;
        lastWiFiRestartTime = 0;
        wifiConnectedSince = 0;
        pingFailCount = 0;
        wifiRestartCount = 0;
    }
    
    // WiFi Power Save Mode'u devre dışı bırak - her zaman tam güç
    void disablePowerSave() {
        esp_wifi_set_ps(WIFI_PS_NONE);
        DEBUG_PRINTLN("[WiFi] Power Save Mode DEVRE DISI - Tam performans");
    }
    
    // Scan available WiFi networks and return as JSON
    String getScannedNetworksJSON() {
        DEBUG_PRINTLN("[WiFi] Scanning available networks...");
        
        // AP modundayken tarama yapabilmek için STA modunu da aktifleştir
        wifi_mode_t currentMode;
        esp_wifi_get_mode(&currentMode);
        if (currentMode == WIFI_MODE_AP) {
            WiFi.mode(WIFI_AP_STA);
            delay(100);
            DEBUG_PRINTLN("[WiFi] Switched to AP+STA mode for scanning");
        }
        
        // Senkron tarama yap (true = gizli ağları da tara)
        int networksFound = WiFi.scanNetworks(false, true);
        
        JsonDocument doc;
        JsonArray networks = doc["networks"].to<JsonArray>();
        
        if (networksFound > 0) {
            DEBUG_PRINTF("[WiFi] Found %d networks\n", networksFound);
            for (int i = 0; i < networksFound; i++) {
                String ssid = WiFi.SSID(i);
                if (ssid.length() > 0) {  // Boş SSID'leri atla
                    JsonObject net = networks.add<JsonObject>();
                    net["ssid"] = ssid;
                    net["rssi"] = WiFi.RSSI(i);
                    net["encryption"] = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "open" : "secured";
                }
            }
        } else if (networksFound == 0) {
            DEBUG_PRINTLN("[WiFi] No networks found");
        } else {
            DEBUG_PRINTF("[WiFi] Scan error: %d\n", networksFound);
        }
        
        doc["count"] = networks.size();
        WiFi.scanDelete();
        
        // Eğer sadece AP modundaydık, geri dön
        if (currentMode == WIFI_MODE_AP) {
            WiFi.mode(WIFI_MODE_AP);
            DEBUG_PRINTLN("[WiFi] Switched back to AP mode");
        }
        
        String output;
        serializeJson(doc, output);
        DEBUG_PRINTLN("[WiFi] Scan result: " + output);
        return output;
    }
    
    // Get Device ID (son 6 karakter - kalıcı, NVS'te saklanır)
    String getChipID() {
        if (chipID.length() == 0) {
            chipID = getDeviceIdShort();  // SK_config.h'daki kalıcı ID sistemi
            DEBUG_PRINTLN("[WiFi] Device ID (short): " + chipID);
        }
        return chipID;
    }
    
    // Get Full Device ID (12 karakter)
    String getFullDeviceID() {
        return getOrCreateDeviceId();  // SK_config.h'daki kalıcı ID sistemi
    }
    
    // Setup Access Point Mode
    void setupAP() {
        apSSID = "SynDimm-" + getFullDeviceID();  // 12 haneli tam ID
        
        DEBUG_PRINTLN("\n=== WiFi Access Point ===");
        DEBUG_PRINTLN("Starting Access Point...");
        DEBUG_PRINTLN("SSID: " + apSSID);
        DEBUG_PRINTLN("Password: None (Open Network)");
        
        WiFi.mode(WIFI_AP);
        
        // AP IP ve subnet ayarla (captive portal için gerekli)
        IPAddress apIP(192, 168, 4, 1);
        IPAddress subnet(255, 255, 255, 0);
        WiFi.softAPConfig(apIP, apIP, subnet);  // Gateway = AP IP (captive portal)
        
        WiFi.softAP(apSSID.c_str()); // No password - open network
        
        // Power Save'i devre dışı bırak
        disablePowerSave();
        
        IPAddress IP = WiFi.softAPIP();
        DEBUG_PRINT("AP IP address: ");
        DEBUG_PRINTLN(IP);
        
        // Start DNS Server for Captive Portal
        // Tüm DNS sorgularını AP IP'sine yönlendir (dimm.local dahil)
        dnsServer.start(DNS_PORT, "*", IP);
        dnsServerActive = true;
        DEBUG_PRINTLN("[OK] DNS Server started (Captive Portal)");
        
        // Start mDNS for AP Mode (yedek olarak - bazı cihazlarda çalışır)
        startMDNS("dimm");
        
        DEBUG_PRINTLN("[INFO] Access via: http://192.168.4.1 or http://dimm.local");
        DEBUG_PRINTLN("========================\n");
        
        apModeActive = true;
        connectedToWiFi = false;
    }
    
    // Get AP SSID
    String getAPSSID() {
        return apSSID;
    }
    
    // Get AP IP Address
    String getAPIP() {
        return WiFi.softAPIP().toString();
    }
    
    // Get number of connected clients
    int getConnectedClients() {
        return WiFi.softAPgetStationNum();
    }
    
    // mDNS durumu
    bool isMDNSActive() {
        return mdnsActive;
    }
    
    // mDNS hostname
    String getMDNSHostname() {
        return currentMdnsHostname;
    }
    
    // mDNS'i yeniden başlat (public API)
    bool refreshMDNS(const String& hostname = "") {
        String hn = hostname.length() > 0 ? hostname : currentMdnsHostname;
        if (hn.length() == 0) hn = "dimm";
        mdnsFailCount = 0;  // Retry sayacını sıfırla (manuel çağrılarda)
        return startMDNS(hn);
    }
    
    // mDNS'i zorla yeniden başlat (health check sonrası)
    bool restartMDNS() {
        DEBUG_PRINTLN("[mDNS] Zorla yeniden baslatiliyor...");
        mdnsFailCount = 0;  // Retry sayacını sıfırla
        return startMDNS(currentMdnsHostname.length() > 0 ? currentMdnsHostname : "dimm");
    }
    
    // mDNS'i durdur
    void stopMDNS() {
        MDNS.end();
        mdnsActive = false;
        currentMdnsHostname = "";  // Hostname'i temizle
        DEBUG_PRINTLN("[mDNS] Durduruldu");
    }
    
    // mDNS sağlık kontrolü ve otomatik restart (handleWiFi içinden çağrılır)
    void handleMDNS() {
        unsigned long now = millis();
        
        // AP modunda sadece DNS captive portal kullanılıyor, mDNS yedek
        // AP modunda mDNS kontrolü yapmaya gerek yok
        if (apModeActive) {
            return;
        }
        
        // WiFi bağlı değilse mDNS'i durdur
        if (!connectedToWiFi || WiFi.status() != WL_CONNECTED) {
            if (mdnsActive) {
                DEBUG_PRINTLN("[mDNS] WiFi baglantisi kesildi - mDNS durduruluyor");
                stopMDNS();
            }
            mdnsFailCount = 0;  // Yeni bağlantıda tekrar deneyebilsin
            return;
        }
        
        // Periyodik kontrol (30 saniyede bir)
        if (now - lastMdnsCheck < MDNS_CHECK_INTERVAL) return;
        lastMdnsCheck = now;
        
        // mDNS aktif değilse başlat
        if (!mdnsActive) {
            if (mdnsFailCount < MDNS_MAX_RETRIES) {
                // Hangi hostname kullanılacak?
                String hostname = currentMdnsHostname;
                if (hostname.length() == 0) {
                    // Bağlı olduğumuz ağın mDNS ayarını kullan
                    if (WiFi.SSID() == primaryNetwork.ssid && primaryNetwork.mdns.length() > 0) {
                        hostname = primaryNetwork.mdns;
                    } else if (WiFi.SSID() == backupNetwork.ssid && backupNetwork.mdns.length() > 0) {
                        hostname = backupNetwork.mdns;
                    } else {
                        hostname = "dimm";
                    }
                }
                
                DEBUG_PRINTF("[mDNS] Aktif degil, baslatiliyor: %s (deneme %d/%d)\n", 
                            hostname.c_str(), mdnsFailCount + 1, MDNS_MAX_RETRIES);
                startMDNS(hostname);
            } else if (mdnsFailCount == MDNS_MAX_RETRIES) {
                // Sadece bir kez log bas
                DEBUG_PRINTF("[mDNS] %d deneme basarisiz - mDNS devre disi!\n", MDNS_MAX_RETRIES);
                DEBUG_PRINTLN("[mDNS] IP adresi ile erisim hala mumkun: " + WiFi.localIP().toString());
                mdnsFailCount++;  // Bir daha log basma
            }
            return;
        }
        
        // mDNS sağlık kontrolü
        if (!checkMDNSHealth()) {
            DEBUG_PRINTLN("[mDNS] Saglik kontrolu BASARISIZ - yeniden baslatiliyor");
            restartMDNS();
        }
    }
        // Load network settings from preferences
    void loadNetworkSettings() {
        prefs.begin(PREFS_NAMESPACE, true); // Read-only mode
        
        // Load Primary Network
        primaryNetwork.ssid = prefs.getString("p_ssid", "");
        primaryNetwork.password = prefs.getString("p_pass", "");
        primaryNetwork.staticIP = prefs.getString("p_ip", "");
        primaryNetwork.mdns = prefs.getString("p_mdns", "dimm");
        primaryNetwork.enabled = primaryNetwork.ssid.length() > 0;
        
        // Load Backup Network
        backupNetwork.ssid = prefs.getString("b_ssid", "");
        backupNetwork.password = prefs.getString("b_pass", "");
        backupNetwork.staticIP = prefs.getString("b_ip", "");
        backupNetwork.mdns = prefs.getString("b_mdns", "dimm");
        backupNetwork.enabled = backupNetwork.ssid.length() > 0;
        
        // Load AP Mode setting - İlk kurulumda true (WiFi yokken AP açık)
        apModeEnabled = prefs.getBool("ap_enabled", true);
        
        prefs.end();
        
        DEBUG_PRINTLN("Network settings loaded from memory");
        DEBUG_PRINTLN("AP Mode enabled: " + String(apModeEnabled ? "Yes" : "No"));
        if (primaryNetwork.enabled) {
            DEBUG_PRINTLN("Primary WiFi: " + primaryNetwork.ssid);
        }
        if (backupNetwork.enabled) {
            DEBUG_PRINTLN("Backup WiFi: " + backupNetwork.ssid);
        }
    }
    
    // Save network settings to preferences
    bool saveNetworkSettings(String p_ssid, String p_pass, String p_ip, String p_mdns,
                            String b_ssid, String b_pass, String b_ip, String b_mdns) {
        prefs.begin(PREFS_NAMESPACE, false); // Read-write mode
        
        // Save Primary Network
        prefs.putString("p_ssid", p_ssid);
        prefs.putString("p_pass", p_pass);
        prefs.putString("p_ip", p_ip);
        prefs.putString("p_mdns", p_mdns);
        
        // Save Backup Network
        prefs.putString("b_ssid", b_ssid);
        prefs.putString("b_pass", b_pass);
        prefs.putString("b_ip", b_ip);
        prefs.putString("b_mdns", b_mdns);
        
        prefs.end();
        
        DEBUG_PRINTLN("Network settings saved to memory");
        
        // Reload settings
        loadNetworkSettings();
        
        return true;
    }
    
    // Try to connect to WiFi network
    bool connectToWiFi(NetworkConfig& network, unsigned long timeout = WIFI_CONNECT_TIMEOUT_MS) {
        if (!network.enabled || network.ssid.length() == 0) {
            return false;
        }
        
        DEBUG_PRINTLN("\nConnecting to: " + network.ssid);
        
        WiFi.mode(WIFI_STA);
        
        // Apply static IP if configured
        if (network.staticIP.length() > 0) {
            IPAddress ip, gateway, subnet, dns1, dns2;
            
            if (ip.fromString(network.staticIP)) {
                // Parse gateway from IP (x.x.x.1)
                String ipStr = network.staticIP;
                int lastDot = ipStr.lastIndexOf('.');
                String gatewayStr = ipStr.substring(0, lastDot) + ".1";
                gateway.fromString(gatewayStr);
                
                subnet.fromString("255.255.255.0");
                dns1.fromString("8.8.8.8");
                dns2.fromString("8.8.4.4");
                
                if (WiFi.config(ip, gateway, subnet, dns1, dns2)) {
                    DEBUG_PRINTLN("Static IP configured: " + network.staticIP);
                } else {
                    DEBUG_PRINTLN("Failed to configure static IP");
                }
            } else {
                DEBUG_PRINTLN("Invalid static IP format: " + network.staticIP);
            }
        } else {
            DEBUG_PRINTLN("Using DHCP");
        }
        
        WiFi.begin(network.ssid.c_str(), network.password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
            delay(500);
            DEBUG_PRINT(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            DEBUG_PRINTLN("\n[OK] Connected to WiFi!");
            DEBUG_PRINT("IP Address: ");
            DEBUG_PRINTLN(WiFi.localIP());
            
            // Power Save'i devre dışı bırak - her zaman erişilebilir ol
            disablePowerSave();
            
            // Start mDNS with configured or default hostname
            String mdnsHostname = network.mdns.length() > 0 ? network.mdns : "dimm";
            startMDNS(mdnsHostname);
            
            // DNS Server'ı kapat (AP mode captive portal için kullanılıyordu)
            if (dnsServerActive) {
                dnsServer.stop();
                dnsServerActive = false;
                DEBUG_PRINTLN("[OK] DNS Server stopped (switching to STA mode)");
            }
            
            connectedToWiFi = true;
            apModeActive = false;
            
            // WiFi restart zamanlarını başlat
            wifiConnectedSince = millis();
            lastWiFiRestartTime = millis();
            lastPingCheckTime = millis();
            lastMdnsCheck = millis();  // mDNS check timer da sıfırla
            pingFailCount = 0;
            
            return true;
        }
        
        DEBUG_PRINTLN("\n[ERROR] Failed to connect to: " + network.ssid);
        return false;
    }
    
    // Initialize WiFi with network scan
    void begin() {
        loadNetworkSettings();
        
        // Check if any network is configured
        if (!primaryNetwork.enabled && !backupNetwork.enabled) {
            DEBUG_PRINTLN("\nNo saved networks found - Starting AP Mode");
            setupAP();
            lastScanTime = millis();
            return;
        }
        
        // Try to connect with multiple retries if network is visible
        const int MAX_INITIAL_RETRIES = 3;
        
        for (int attempt = 1; attempt <= MAX_INITIAL_RETRIES; attempt++) {
            DEBUG_PRINTF("\n=== Connection Attempt %d/%d ===\n", attempt, MAX_INITIAL_RETRIES);
            
            // Scan for saved networks
            String foundSSID;
            bool isPrimary;
            
            if (scanForSavedNetworks(foundSSID, isPrimary)) {
                // Found a saved network, try to connect
                NetworkConfig& network = isPrimary ? primaryNetwork : backupNetwork;
                
                if (connectToWiFi(network)) {
                    DEBUG_PRINTLN("Using " + String(isPrimary ? "Primary" : "Backup") + " WiFi");
                    return;
                }
                
                // Network visible but connection failed - retry after delay
                if (attempt < MAX_INITIAL_RETRIES) {
                    DEBUG_PRINTF("[WiFi] Network '%s' visible but connection failed - retrying in 3 seconds...\n", foundSSID.c_str());
                    delay(3000);
                }
            } else {
                // No networks found in scan - no point retrying immediately
                DEBUG_PRINTLN("No saved networks visible in scan");
                break;
            }
        }
        
        // All retries exhausted or no networks visible - Start AP Mode (if enabled)
        if (apModeEnabled) {
            DEBUG_PRINTLN("\nNo available saved networks or connection failed - Starting AP Mode");
            setupAP();
        } else {
            DEBUG_PRINTLN("\nNo available saved networks and AP Mode is disabled - No connection!");
            connectedToWiFi = false;
            apModeActive = false;
        }
        lastScanTime = millis();
    }
    
    // Check WiFi status and handle reconnection
    void handleWiFi() {
        unsigned long currentTime = millis();
        
        // DNS Server işleme (AP modunda captive portal için)
        if (dnsServerActive && apModeActive) {
            dnsServer.processNextRequest();
        }
        
        // mDNS sağlık kontrolü ve otomatik restart
        handleMDNS();
        
        // ========================================
        // DURUM 1: WiFi'ye bağlıyken
        // ========================================
        if (connectedToWiFi && !apModeActive) {
            // Keep-alive kontrolü
            if (currentTime - lastKeepAliveTime >= KEEP_ALIVE_INTERVAL) {
                lastKeepAliveTime = currentTime;
                esp_wifi_set_ps(WIFI_PS_NONE);  // Power save kapalı tut
                
                // Sinyal gücü kontrolü
                if (WiFi.status() == WL_CONNECTED) {
                    int rssi = WiFi.RSSI();
                    if (rssi < -85) {
                        DEBUG_PRINTF("[WiFi] Cok zayif sinyal: %d dBm\n", rssi);
                    }
                }
            }
            
            // ========================================
            // WiFi DONMA KORUMASI - Periyodik Restart
            // ========================================
            
            // 1. Zorunlu restart kontrolü (4 saatte bir)
            if (currentTime - lastWiFiRestartTime >= WIFI_FORCE_RESTART_INTERVAL) {
                DEBUG_PRINTLN("\n[WiFi] 4 saat doldu - Zorunlu WiFi restart");
                restartWiFiHardware();
                return;
            }
            
            // 2. WiFi sağlık kontrolü (2 dakikada bir) - İNTERNET GEREKTİRMEZ!
            if (currentTime - lastPingCheckTime >= WIFI_PING_CHECK_INTERVAL) {
                lastPingCheckTime = currentTime;
                
                // WiFi stack sağlığını kontrol et (internet bağımsız)
                if (!checkWiFiHealth()) {
                    pingFailCount++;
                    DEBUG_PRINTF("[WiFi] Saglik kontrolu BASARISIZ! (%d/%d)\n", pingFailCount, MAX_PING_FAILS);
                    
                    // 3 başarısız kontrol = WiFi stack sorunu, restart et
                    if (pingFailCount >= MAX_PING_FAILS) {
                        DEBUG_PRINTLN("[WiFi] WiFi stack sorunu - Restart gerekli!");
                        restartWiFiHardware();
                        return;
                    }
                } else {
                    // Kontrol başarılı - sayacı sıfırla
                    if (pingFailCount > 0) {
                        DEBUG_PRINTF("[WiFi] WiFi saglikli - Fail count sifirlandi (onceki: %d)\n", pingFailCount);
                        pingFailCount = 0;
                    }
                }
            }
            
            // Bağlantı koptu mu?
            if (WiFi.status() != WL_CONNECTED) {
                DEBUG_PRINTLN("\n[WiFi] Baglanti kesildi!");
                connectedToWiFi = false;
                reconnectFailCount = 0;
                lastReconnectAttempt = 0;  // Hemen ilk denemeyi yap
            }
            return;  // Bağlıyken başka bir şey yapma
        }
        
        // ========================================
        // DURUM 2: Bağlı değil, AP Mode da değil - Yeniden bağlanmaya çalış
        // ========================================
        if (!connectedToWiFi && !apModeActive) {
            // Cooldown kontrolü (ilk deneme hariç)
            if (reconnectFailCount > 0 && (currentTime - lastReconnectAttempt < RECONNECT_COOLDOWN)) {
                return;
            }
            lastReconnectAttempt = currentTime;
            
            reconnectFailCount++;
            DEBUG_PRINTF("[WiFi] Yeniden baglanti denemesi %d/%d\n", reconnectFailCount, MAX_RECONNECT_ATTEMPTS);
            
            // Kayıtlı ağları ara
            String foundSSID;
            bool isPrimary;
            bool networkVisible = scanForSavedNetworks(foundSSID, isPrimary);
            
            if (networkVisible) {
                NetworkConfig& network = isPrimary ? primaryNetwork : backupNetwork;
                
                if (connectToWiFi(network, 15000)) {
                    DEBUG_PRINTLN("[OK] Yeniden baglandi: " + foundSSID);
                    reconnectFailCount = 0;
                    apScanCount = 0;
                    return;
                }
                
                // Ağ görünüyor ama bağlanamadık - denemeye devam et, AP Mode'a geçme!
                if (reconnectFailCount >= MAX_RECONNECT_ATTEMPTS) {
                    DEBUG_PRINTF("[WiFi] Ag '%s' gorunuyor ama baglanilamiyor - denemeye devam\n", foundSSID.c_str());
                    // Sayacı sıfırla ama cooldown'u artır (her 5 denemede bir log bas)
                    reconnectFailCount = 0;
                }
                return;
            }
            
            // Ağ görünmüyor - 5 başarısız deneme sonra AP Mode'a geç
            if (reconnectFailCount >= MAX_RECONNECT_ATTEMPTS) {
                if (apModeEnabled) {
                    DEBUG_PRINTF("[WiFi] %d deneme basarisiz, ag gorunmuyor - AP Mode baslatiliyor\n", MAX_RECONNECT_ATTEMPTS);
                    setupAP();
                } else {
                    DEBUG_PRINTF("[WiFi] %d deneme basarisiz, ag gorunmuyor - AP Mode kapali, baglanti yok!\n", MAX_RECONNECT_ATTEMPTS);
                }
                lastScanTime = currentTime;
                apScanCount = 0;
            }
            return;
        }
        
        // ========================================
        // DURUM 3: AP Mode aktif - Periyodik olarak kayıtlı ağları ara
        // ========================================
        if (apModeActive) {
            // İlk 5 deneme: dakikada bir, sonra: saatte bir
            unsigned long scanInterval = (apScanCount < 5) ? AP_SCAN_INTERVAL_FAST : AP_SCAN_INTERVAL_SLOW;
            
            if (currentTime - lastScanTime >= scanInterval) {
                lastScanTime = currentTime;
                apScanCount++;
                
                if (apScanCount <= 5) {
                    DEBUG_PRINTF("[AP Mode] Ag taramasi %d/5 (dakikada bir)\n", apScanCount);
                } else {
                    DEBUG_PRINTF("[AP Mode] Ag taramasi #%d (saatte bir)\n", apScanCount);
                }
                
                String foundSSID;
                bool isPrimary;
                
                if (scanForSavedNetworks(foundSSID, isPrimary)) {
                    NetworkConfig& network = isPrimary ? primaryNetwork : backupNetwork;
                    
                    if (connectToWiFi(network, 15000)) {
                        DEBUG_PRINTLN("[OK] WiFi baglandi - AP Mode kapatiliyor");
                        WiFi.softAPdisconnect(true);
                        if (dnsServerActive) {
                            dnsServer.stop();
                            dnsServerActive = false;
                        }
                        apModeActive = false;
                        reconnectFailCount = 0;
                        apScanCount = 0;
                        return;
                    }
                }
                
                DEBUG_PRINTLN("[AP Mode] Kayitli ag bulunamadi veya baglanilamadi");
            }
        }
    }
    
    // Check if connected to WiFi
    bool isConnectedToWiFi() {
        return connectedToWiFi && WiFi.status() == WL_CONNECTED;
    }
    
    // Check if AP mode is active
    bool isAPModeActive() {
        return apModeActive;
    }
    
    // Check if AP mode is enabled by user
    bool isAPModeEnabled() {
        return apModeEnabled;
    }
    
    // Set AP mode enabled/disabled
    bool setAPModeEnabled(bool enabled) {
        apModeEnabled = enabled;
        prefs.begin(PREFS_NAMESPACE, false);
        prefs.putBool("ap_enabled", enabled);
        prefs.end();
        DEBUG_PRINTLN("AP Mode enabled set to: " + String(enabled ? "Yes" : "No"));
        
        // Eğer AP mode kapatıldıysa ve şu an aktifse, kapat
        if (!enabled && apModeActive && connectedToWiFi) {
            WiFi.softAPdisconnect(true);
            if (dnsServerActive) {
                dnsServer.stop();
                dnsServerActive = false;
            }
            apModeActive = false;
            DEBUG_PRINTLN("AP Mode deactivated");
        }
        // Eğer AP mode açıldıysa ve WiFi bağlı değilse, aç
        else if (enabled && !connectedToWiFi && !apModeActive) {
            setupAP();
        }
        
        return true;
    }
    
    // WiFi restart istatistikleri - debug için
    uint32_t getWiFiRestartCount() {
        return wifiRestartCount;
    }
    
    uint8_t getPingFailCount() {
        return pingFailCount;
    }
    
    unsigned long getWiFiUptime() {
        if (connectedToWiFi && wifiConnectedSince > 0) {
            return (millis() - wifiConnectedSince) / 1000;  // saniye olarak
        }
        return 0;
    }
    
    unsigned long getTimeSinceLastRestart() {
        if (lastWiFiRestartTime > 0) {
            return (millis() - lastWiFiRestartTime) / 1000;  // saniye olarak
        }
        return 0;
    }
    
    // Get current network info
    String getCurrentNetworkInfo() {
        if (isConnectedToWiFi()) {
            return "WiFi: " + WiFi.SSID() + " (" + WiFi.localIP().toString() + ")";
        } else if (isAPModeActive()) {
            return "AP Mode: " + apSSID + " (" + getAPIP() + ")";
        } else {
            return "Disconnected";
        }
    }
    
    // Get saved network settings as JSON
    String getNetworkSettingsJSON() {
        String json = "{";
        json += "\"primary\":{";
        json += "\"ssid\":\"" + primaryNetwork.ssid + "\",";
        json += "\"staticIP\":\"" + primaryNetwork.staticIP + "\",";
        json += "\"mdns\":\"" + primaryNetwork.mdns + "\"";
        json += "},";
        json += "\"backup\":{";
        json += "\"ssid\":\"" + backupNetwork.ssid + "\",";
        json += "\"staticIP\":\"" + backupNetwork.staticIP + "\",";
        json += "\"mdns\":\"" + backupNetwork.mdns + "\"";
        json += "}";
        json += "}";
        return json;
    }
    
    // Get connection status as JSON
    String getConnectionStatusJSON() {
        String json = "{";
        
        // Mode
        if (isConnectedToWiFi()) {
            json += "\"mode\":\"WiFi\",";
            json += "\"ssid\":\"" + WiFi.SSID() + "\",";
            json += "\"ip\":\"" + WiFi.localIP().toString() + "\",";
            
            // Determine which network is connected
            if (WiFi.SSID() == primaryNetwork.ssid) {
                json += "\"connectedTo\":\"primary\",";
                json += "\"mdns\":\"" + primaryNetwork.mdns + "\",";
            } else if (WiFi.SSID() == backupNetwork.ssid) {
                json += "\"connectedTo\":\"backup\",";
                json += "\"mdns\":\"" + backupNetwork.mdns + "\",";
            } else {
                json += "\"connectedTo\":\"unknown\",";
                json += "\"mdns\":\"\",";
            }
        } else if (isAPModeActive()) {
            json += "\"mode\":\"AP Mode\",";
            json += "\"ssid\":\"" + apSSID + "\",";
            json += "\"ip\":\"" + getAPIP() + "\",";
            json += "\"connectedTo\":\"ap\",";
            json += "\"mdns\":\"dimm\",";
        } else {
            json += "\"mode\":\"Disconnected\",";
            json += "\"ssid\":\"-\",";
            json += "\"ip\":\"-\",";
            json += "\"connectedTo\":\"none\",";
            json += "\"mdns\":\"\",";
        }
        
        // mDNS durumu
        json += "\"mdnsActive\":" + String(mdnsActive ? "true" : "false") + ",";
        json += "\"mdnsHostname\":\"" + currentMdnsHostname + "\",";
        json += "\"dnsServerActive\":" + String(dnsServerActive ? "true" : "false") + ",";
        
        // Configuration status
        json += "\"primaryConfigured\":" + String(primaryNetwork.enabled ? "true" : "false") + ",";
        json += "\"backupConfigured\":" + String(backupNetwork.enabled ? "true" : "false") + ",";
        json += "\"apModeEnabled\":" + String(apModeEnabled ? "true" : "false");
        
        json += "}";
        return json;
    }
};

#endif // SKWIFI_H
