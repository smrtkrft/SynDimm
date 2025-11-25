/**
 * SKwifi.h
 * SmartKraft SynDimm - WiFi & AP Mode Management
 * Version: v0.9.1
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
#include "SK_config.h"

class SKWiFi {
private:
    String chipID;
    String apSSID;
    Preferences prefs;
    
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
        Serial.println("\n=== Scanning WiFi Networks ===");
        int networksFound = WiFi.scanNetworks();
        
        if (networksFound == 0) {
            Serial.println("No networks found");
            return false;
        }
        
        Serial.println("Networks found: " + String(networksFound));
        
        // Check if Primary network is available
        if (primaryNetwork.enabled && primaryNetwork.ssid.length() > 0) {
            for (int i = 0; i < networksFound; i++) {
                if (WiFi.SSID(i) == primaryNetwork.ssid) {
                    Serial.println("[OK] Found Primary WiFi: " + primaryNetwork.ssid);
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
                    Serial.println("[OK] Found Backup WiFi: " + backupNetwork.ssid);
                    foundSSID = backupNetwork.ssid;
                    isPrimary = false;
                    WiFi.scanDelete();
                    return true;
                }
            }
        }
        
        Serial.println("No saved networks found in scan");
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
    }
    
    // Get Chip ID (last 6 characters)
    String getChipID() {
        if (chipID.length() == 0) {
            uint64_t chipid = ESP.getEfuseMac();
            char chipIDStr[13];
            sprintf(chipIDStr, "%04X%08X", (uint16_t)(chipid >> 32), (uint32_t)chipid);
            String fullID = String(chipIDStr);
            chipID = fullID.substring(fullID.length() - 6);  // Son 6 hane
            Serial.println("Full Chip ID length: " + String(fullID.length()) + " - Full ID: " + fullID);
        }
        return chipID;
    }
    
    // Setup Access Point Mode
    void setupAP() {
        apSSID = "SynDimm-SK" + getChipID();
        
        Serial.println("\n=== WiFi Access Point ===");
        Serial.println("Starting Access Point...");
        Serial.println("SSID: " + apSSID);
        Serial.println("Password: None (Open Network)");
        
        WiFi.mode(WIFI_AP);
        WiFi.softAP(apSSID.c_str()); // No password - open network
        
        IPAddress IP = WiFi.softAPIP();
        Serial.print("AP IP address: ");
        Serial.println(IP);
        
        // Start mDNS for AP Mode
        if (MDNS.begin("dimm")) {
            Serial.println("[OK] mDNS started: dimm.local");
            MDNS.addService("http", "tcp", 80);
        } else {
            Serial.println("[ERROR] mDNS failed to start");
        }
        
        Serial.println("========================\n");
        
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
        
        Serial.println("Network settings loaded from memory");
        if (primaryNetwork.enabled) {
            Serial.println("Primary WiFi: " + primaryNetwork.ssid);
        }
        if (backupNetwork.enabled) {
            Serial.println("Backup WiFi: " + backupNetwork.ssid);
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
        
        Serial.println("Network settings saved to memory");
        
        // Reload settings
        loadNetworkSettings();
        
        return true;
    }
    
    // Try to connect to WiFi network
    bool connectToWiFi(NetworkConfig& network, unsigned long timeout = WIFI_CONNECT_TIMEOUT_MS) {
        if (!network.enabled || network.ssid.length() == 0) {
            return false;
        }
        
        Serial.println("\nConnecting to: " + network.ssid);
        
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
                    Serial.println("Static IP configured: " + network.staticIP);
                } else {
                    Serial.println("Failed to configure static IP");
                }
            } else {
                Serial.println("Invalid static IP format: " + network.staticIP);
            }
        } else {
            Serial.println("Using DHCP");
        }
        
        WiFi.begin(network.ssid.c_str(), network.password.c_str());
        
        unsigned long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && (millis() - startTime) < timeout) {
            delay(500);
            Serial.print(".");
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println("\n[OK] Connected to WiFi!");
            Serial.print("IP Address: ");
            Serial.println(WiFi.localIP());
            
            // Start mDNS if configured
            if (network.mdns.length() > 0) {
                if (MDNS.begin(network.mdns.c_str())) {
                    Serial.println("[OK] mDNS started: " + network.mdns + ".local");
                    MDNS.addService("http", "tcp", 80);
                } else {
                    Serial.println("[ERROR] mDNS failed to start");
                }
            } else {
                Serial.println("[INFO] mDNS not configured (using default: dimm.local)");
                if (MDNS.begin("dimm")) {
                    Serial.println("[OK] mDNS started with default: dimm.local");
                    MDNS.addService("http", "tcp", 80);
                }
            }
            
            connectedToWiFi = true;
            apModeActive = false;
            return true;
        }
        
        Serial.println("\n[ERROR] Failed to connect to: " + network.ssid);
        return false;
    }
    
    // Initialize WiFi with network scan
    void begin() {
        loadNetworkSettings();
        
        // Check if any network is configured
        if (!primaryNetwork.enabled && !backupNetwork.enabled) {
            Serial.println("\nNo saved networks found - Starting AP Mode");
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
                Serial.println("Using " + String(isPrimary ? "Primary" : "Backup") + " WiFi");
                return;
            }
        }
        
        // No saved networks found or connection failed - Start AP Mode
        Serial.println("\nNo available saved networks - Starting AP Mode");
        setupAP();
        lastScanTime = millis();
    }
    
    // Check WiFi status and handle reconnection
    void handleWiFi() {
        // If connected to WiFi, check if still connected
        if (connectedToWiFi && !apModeActive) {
            if (WiFi.status() != WL_CONNECTED) {
                Serial.println("\n⚠ WiFi connection lost!");
                connectedToWiFi = false;
                
                // Try to scan and reconnect before starting AP
                String foundSSID;
                bool isPrimary;
                
                Serial.println("Attempting to reconnect...");
                if (scanForSavedNetworks(foundSSID, isPrimary)) {
                    NetworkConfig& network = isPrimary ? primaryNetwork : backupNetwork;
                    
                    if (connectToWiFi(network)) {
                        Serial.println("[OK] Reconnected to " + foundSSID);
                        return;
                    }
                }
                
                // Could not reconnect - Start AP Mode
                Serial.println("Could not reconnect - Starting AP Mode");
                setupAP();
                lastScanTime = millis();
            }
            return; // Connected and working - do nothing
        }
        
        // If in AP Mode, periodically scan for saved networks
        if (apModeActive) {
            if (millis() - lastScanTime >= WIFI_SCAN_INTERVAL_MS) {
                Serial.println("\n[AP Mode] Checking for saved networks...");
                lastScanTime = millis();
                
                String foundSSID;
                bool isPrimary;
                
                if (scanForSavedNetworks(foundSSID, isPrimary)) {
                    NetworkConfig& network = isPrimary ? primaryNetwork : backupNetwork;
                    
                    if (connectToWiFi(network)) {
                        Serial.println("[OK] Connected to WiFi - Closing AP Mode");
                        WiFi.softAPdisconnect(true);
                        apModeActive = false;
                        return;
                    }
                }
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
