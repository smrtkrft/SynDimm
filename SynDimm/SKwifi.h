/**
 * SKwifi.h
 * SmartKraft SynDimm - WiFi & AP Mode Management
 * Version: v1.0.2
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
#include <Preferences.h>
#include <esp_wifi.h>
#include "SK_config.h"

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
    unsigned long lastScanTime;
    
    // Scan for available networks and check if saved networks exist
    bool scanForSavedNetworks(String& foundSSID, bool& isPrimary) {
        DEBUG_PRINTLN("\n=== Scanning WiFi Networks ===");
        int networksFound = WiFi.scanNetworks();
        
        if (networksFound == 0) {
            DEBUG_PRINTLN("No networks found");
            return false;
        }
        
        DEBUG_PRINTLN("Networks found: " + String(networksFound));
        
        // Check if Primary network is available
        if (primaryNetwork.enabled && primaryNetwork.ssid.length() > 0) {
            for (int i = 0; i < networksFound; i++) {
                if (WiFi.SSID(i) == primaryNetwork.ssid) {
                    DEBUG_PRINTLN("[OK] Found Primary WiFi: " + primaryNetwork.ssid);
                    foundSSID = primaryNetwork.ssid;
                    isPrimary = true;
                    WiFi.scanDelete();
                    return true;
                }
            }
        }
        
        // Check if Backup network is available
        if (backupNetwork.enabled && backupNetwork.ssid.length() > 0) {
            for (int i = 0; i < networksFound; i++) {
                if (WiFi.SSID(i) == backupNetwork.ssid) {
                    DEBUG_PRINTLN("[OK] Found Backup WiFi: " + backupNetwork.ssid);
                    foundSSID = backupNetwork.ssid;
                    isPrimary = false;
                    WiFi.scanDelete();
                    return true;
                }
            }
        }
        
        DEBUG_PRINTLN("No saved networks found in scan");
        WiFi.scanDelete();
        return false;
    }
    
public:
    SKWiFi() {
        chipID = "";
        apSSID = "";
        connectedToWiFi = false;
        apModeActive = false;
        lastScanTime = 0;
        lastKeepAliveTime = 0;
        lastReconnectAttempt = 0;
        reconnectFailCount = 0;
        apScanCount = 0;
    }
    
    // WiFi Power Save Mode'u devre dışı bırak - her zaman tam güç
    void disablePowerSave() {
        esp_wifi_set_ps(WIFI_PS_NONE);
        DEBUG_PRINTLN("[WiFi] Power Save Mode DEVRE DISI - Tam performans");
    }
    
    // Get Chip ID (last 6 characters)
    String getChipID() {
        if (chipID.length() == 0) {
            uint64_t chipid = ESP.getEfuseMac();
            char chipIDStr[13];
            sprintf(chipIDStr, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
            String fullID = String(chipIDStr);
            chipID = fullID.substring(fullID.length() - 6);  // Son 6 hane
            DEBUG_PRINTLN("Full Chip ID length: " + String(fullID.length()) + " - Full ID: " + fullID);
        }
        return chipID;
    }
    
    // Setup Access Point Mode
    void setupAP() {
        apSSID = "SynDimm-SK" + getChipID();
        
        DEBUG_PRINTLN("\n=== WiFi Access Point ===");
        DEBUG_PRINTLN("Starting Access Point...");
        DEBUG_PRINTLN("SSID: " + apSSID);
        DEBUG_PRINTLN("Password: None (Open Network)");
        
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSSID.c_str()); // No password - open network
        
        // Power Save'i devre dışı bırak
        disablePowerSave();
        
        IPAddress IP = WiFi.softAPIP();
        DEBUG_PRINT("AP IP address: ");
        DEBUG_PRINTLN(IP);
        
        // Start mDNS for AP Mode
        if (MDNS.begin("dimm")) {
            DEBUG_PRINTLN("[OK] mDNS started: dimm.local");
            MDNS.addService("http", "tcp", 80);
        } else {
            DEBUG_PRINTLN("[ERROR] mDNS failed to start");
        }
        
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
        
        prefs.end();
        
        DEBUG_PRINTLN("Network settings loaded from memory");
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
            
            // Start mDNS if configured
            if (network.mdns.length() > 0) {
                if (MDNS.begin(network.mdns.c_str())) {
                    DEBUG_PRINTLN("[OK] mDNS started: " + network.mdns + ".local");
                    MDNS.addService("http", "tcp", 80);
                } else {
                    DEBUG_PRINTLN("[ERROR] mDNS failed to start");
                }
            } else {
                DEBUG_PRINTLN("[INFO] mDNS not configured (using default: dimm.local)");
                if (MDNS.begin("dimm")) {
                    DEBUG_PRINTLN("[OK] mDNS started with default: dimm.local");
                    MDNS.addService("http", "tcp", 80);
                }
            }
            
            connectedToWiFi = true;
            apModeActive = false;
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
        }
        
        // No saved networks found or connection failed - Start AP Mode
        DEBUG_PRINTLN("\nNo available saved networks - Starting AP Mode");
        setupAP();
        lastScanTime = millis();
    }
    
    // Check WiFi status and handle reconnection
    void handleWiFi() {
        unsigned long currentTime = millis();
        
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
            
            if (scanForSavedNetworks(foundSSID, isPrimary)) {
                NetworkConfig& network = isPrimary ? primaryNetwork : backupNetwork;
                
                if (connectToWiFi(network, 15000)) {
                    DEBUG_PRINTLN("[OK] Yeniden baglandi: " + foundSSID);
                    reconnectFailCount = 0;
                    apScanCount = 0;
                    return;
                }
            }
            
            // 5 başarısız deneme - AP Mode'a geç
            if (reconnectFailCount >= MAX_RECONNECT_ATTEMPTS) {
                DEBUG_PRINTF("[WiFi] %d deneme basarisiz - AP Mode baslatiliyor\n", MAX_RECONNECT_ATTEMPTS);
                setupAP();
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
        
        // Configuration status
        json += "\"primaryConfigured\":" + String(primaryNetwork.enabled ? "true" : "false") + ",";
        json += "\"backupConfigured\":" + String(backupNetwork.enabled ? "true" : "false");
        
        json += "}";
        return json;
    }
};

#endif // SKWIFI_H
