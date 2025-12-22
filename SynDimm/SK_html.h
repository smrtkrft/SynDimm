/**
 * SK_html.h
 * SmartKraft SynDimm - HTML Structure
 * Version: v1.2.0
 * 
 * ========================================
 * KRITIK KURAL - ASLA DEĞİŞTİRME!
 * ========================================
 * Web arayüzü SADECE ESP32-C6 içindir!
 * - Kullanıcı sadece cihazın kendisinden erişebilir
 * - Dışarıdan internet erişimi YOK
 * - Sadece bilgilendirme ve basit ayarlar için
 * - Hiçbir kritik kontrol web'den yapılmaz
 * ========================================
 */

#ifndef SK_HTML_H
#define SK_HTML_H

#include "SK_css.h"
#include "SK_js.h"

// Generate complete HTML page
String generateHTML(String chipID, String version) {
    String html = R"rawliteral(<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SmartKraft SynDimm</title>
    <link rel="stylesheet" href="/style.css">
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>SmartKraft SynDimm</h1>
            <div class="info-box">
                <div class="info-single">
                    <span class="info-label">ID:</span>
                    <span class="info-value">)rawliteral";
    
    html += chipID;
    
    html += R"rawliteral(</span>
                    <span class="info-separator">-</span>
                    <span class="info-label">Version:</span>
                    <span class="info-value">)rawliteral";
    
    html += version;
    
    html += R"rawliteral(</span>
                </div>
                <div class="info-single">
                    <span class="info-label" data-lang="connection.mode">Mode:</span>
                    <span class="info-value" id="header-status-mode">WiFi</span>
                    <span class="info-separator">-</span>
                    <span class="info-label" data-lang="connection.ssid">SSID:</span>
                    <span class="info-value" id="header-status-ssid">-</span>
                </div>
                <div class="info-single">
                    <span class="info-label" data-lang="connection.ip_address">IP:</span>
                    <span class="info-value" id="header-status-ip">-</span>
                    <span class="info-value info-mdns" id="header-status-mdns"></span>
                </div>
            </div>
        </div>
        
        <!-- Notification Box -->
        <div id="notification" class="notification" style="display: none;">
            <div class="notification-content">
                <span id="notification-icon" class="notification-icon"></span>
                <span id="notification-message" class="notification-message"></span>
            </div>
            <button class="notification-close" onclick="closeNotification()">×</button>
        </div>
        
        <div id="hizli-ayarlar" class="tab-content active" style="display: block;">
            <div class="modes-content">
                
                <!-- Mod Seçimi Başlığı -->
                <h2 class="mode-selection-title">
                    <span data-lang="quick.mode_selection">Mode Selection</span>
                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.mode_selection_info">Select the operating mode for the encoder. Each mode offers different functionality.</span></span>
                </h2>
                
                <!-- Mod Seçimi Butonları -->
                <div class="mode-buttons">
                    <button class="mode-btn" id="mode-btn-dimmer" onclick="selectMode('DIMMER')">
                        <span class="mode-btn-text">DIMMER</span>
                    </button>
                    <button class="mode-btn" id="mode-btn-shutter" onclick="selectMode('SHUTTER')">
                        <span class="mode-btn-text">SHUTTER</span>
                    </button>
                    <button class="mode-btn" id="mode-btn-safe" onclick="selectMode('SAFE')">
                        <span class="mode-btn-text">SAFE</span>
                    </button>
                </div>
                
                <!-- Mod Yapılandırması Akordionu -->
                <div class="accordion" id="mode-config-accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="quick.mode_config">Mode Configuration</span>
                        <span class="badge badge-info" id="mode-config-badge">DIMMER</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content" id="mode-config-content">
                        
                        <!-- DIMMER Yapılandırması -->
                        <div class="mode-config-panel" id="config-panel-dimmer">
                            <!-- Dimmer Mod Açıklama -->
                            <div class="mode-info-text" data-lang="dimmer.description">
                                Control your Shelly Dimmer with the encoder. Adjust brightness and manage on/off operations.
                            </div>
                            
                            <!-- Status Info Bar -->
                            <div class="dimmer-status-bar-new">
                                <div class="status-col">
                                    <div class="status-col-label" data-lang="dimmer.ip_address">IP ADDRESS</div>
                                    <div class="status-col-value" id="quick-dimmer-ip">-</div>
                                </div>
                                <div class="status-col">
                                    <div class="status-col-label" data-lang="dimmer.status">STATUS</div>
                                    <div class="status-col-value">
                                        <span id="quick-dimmer-brightness">0%</span>
                                        <span class="status-power-text" id="quick-dimmer-power" data-lang="common.off">Off</span>
                                    </div>
                                </div>
                                <div class="status-col">
                                    <div class="status-col-label" data-lang="dimmer.calibration">CALIBRATION</div>
                                    <div class="calibration-controls">
                                        <button class="btn-cal-up" onclick="adjustCalibration(1)" data-lang-title="common.increase">▲</button>
                                        <span class="calibration-value-display" id="quick-dimmer-calibration">3</span>
                                        <button class="btn-cal-down" onclick="adjustCalibration(-1)" data-lang-title="common.decrease">▼</button>
                                    </div>
                                </div>
                                <div class="status-col status-col-action">
                                    <div class="status-col-label" data-lang="dimmer.action">ACTION</div>
                                    <div class="status-col-value">
                                        <button class="btn-status-connect" id="quick-btn-connect" onclick="connectDimmer()" data-lang="common.connect">Connect</button>
                                        <button class="btn-status-disconnect" id="quick-btn-disconnect" onclick="disconnectDimmer()" style="display: none;" data-lang="common.disconnect">Disconnect</button>
                                    </div>
                                </div>
                            </div>
                            
                            <!-- Connection Section -->
                            <div class="dimmer-config-section">
                                <h4 class="dimmer-section-title" data-lang="dimmer.device_connection">Device Connection</h4>
                                <div class="form-group">
                                    <label data-lang="dimmer.manual_ip">Manual IP Entry</label>
                                    <input type="text" id="quick-dimmer-ip-input" placeholder="192.168.1.100" class="input-full">
                                </div>
                                <div class="form-actions-row">
                                    <button class="btn-primary" onclick="connectDimmerFromQuick()" data-lang="common.connect">Connect</button>
                                    <button class="btn-secondary" onclick="startNetworkScan()" data-lang="dimmer.scan_network">Scan Network</button>
                                </div>
                            </div>
                            
                            <!-- Saved Devices -->
                            <div class="dimmer-config-section">
                                <h4 class="dimmer-section-title" data-lang="dimmer.saved_devices">Saved Devices</h4>
                                <div class="saved-devices-list" id="quick-saved-devices-list">
                                    <div class="saved-device-empty" data-lang="dimmer.no_devices">No saved devices yet.</div>
                                </div>
                            </div>
                        </div>
                        
                        <!-- SHUTTER Yapılandırması -->
                        <div class="mode-config-panel" id="config-panel-shutter" style="display: none;">
                            <!-- Shutter Mod Açıklama -->
                            <div class="mode-info-text" data-lang="shutter.description">
                                Control your Shelly Shutter with the encoder. Easily adjust curtain and blind positions.
                            </div>
                            
                            <!-- Status Info Bar -->
                            <div class="shutter-status-bar">
                                <div class="status-col">
                                    <div class="status-col-label" data-lang="shutter.ip_address">IP ADDRESS</div>
                                    <div class="status-col-value" id="quick-shutter-ip">-</div>
                                </div>
                                <div class="status-col">
                                    <div class="status-col-label" data-lang="shutter.status">STATUS</div>
                                    <div class="status-col-value">
                                        <span class="shutter-status-text" id="quick-shutter-status" data-lang="shutter.not_connected">Not Connected</span>
                                    </div>
                                </div>
                                <div class="status-col">
                                    <div class="status-col-label" data-lang="shutter.position">POSITION</div>
                                    <div class="status-col-value">
                                        <div class="position-display">
                                            <div class="position-bar-container">
                                                <div class="position-bar" id="quick-shutter-position-bar" style="width: 0%;"></div>
                                            </div>
                                            <span class="position-percent" id="quick-shutter-position">0%</span>
                                        </div>
                                    </div>
                                </div>
                                <div class="status-col status-col-action">
                                    <div class="status-col-label" data-lang="shutter.encoder_step">ENCODER STEP</div>
                                    <div class="calibration-controls">
                                        <button class="btn-cal-up" onclick="adjustShutterStep(1)" data-lang-title="common.increase">▲</button>
                                        <span class="calibration-value-display" id="quick-shutter-step">3</span>
                                        <button class="btn-cal-down" onclick="adjustShutterStep(-1)" data-lang-title="common.decrease">▼</button>
                                    </div>
                                </div>
                            </div>
                            
                            <!-- Connection Section -->
                            <div class="shutter-config-section">
                                <h4 class="shutter-section-title" data-lang="shutter.device_connection">Device Connection</h4>
                                <div class="form-group">
                                    <label data-lang="shutter.manual_ip">Manual IP Entry</label>
                                    <input type="text" id="quick-shutter-ip-input" placeholder="192.168.1.100" class="input-full">
                                </div>
                                <div class="form-actions-row">
                                    <button class="btn-primary" onclick="connectShutterFromQuick()" data-lang="common.connect">Connect</button>
                                    <button class="btn-secondary" onclick="startShutterNetworkScan()" data-lang="shutter.scan_network">Scan Network</button>
                                </div>
                            </div>
                            
                            <div class="shutter-info-text warning">
                                <span class="warning-icon">&#9888;</span> <span data-lang="shutter.info_text">Shutter is controlled by encoder.</span>
                            </div>
                        </div>
                        
                        <!-- SAFE Yapılandırması -->
                        <div class="mode-config-panel" id="config-panel-safe" style="display: none;">
                            <!-- Safe Mod Açıklama -->
                            <div class="mode-info-text" data-lang="safe.description">
                                Password entry is done with encoder movements. You can define an API endpoint for each password.
                            </div>
                            
                            <!-- Şifre Tab'ları -->
                            <div class="safe-tabs">
                                <button class="safe-tab active" onclick="openQuickSafeTab(event, 0)" data-lang="safe.password_1">Password 1</button>
                                <button class="safe-tab" onclick="openQuickSafeTab(event, 1)" data-lang="safe.password_2">Password 2</button>
                                <button class="safe-tab" onclick="openQuickSafeTab(event, 2)" data-lang="safe.password_3">Password 3</button>
                                <button class="safe-tab" onclick="openQuickSafeTab(event, 3)" data-lang="safe.password_4">Password 4</button>
                                <button class="safe-tab" onclick="openQuickSafeTab(event, 4)" data-lang="safe.password_5">Password 5</button>
                            </div>
                            
                            <!-- Quick Safe Password Panels -->
                            <div class="quick-safe-tab-content active" id="quick-safe-tab-0">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-0-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label data-lang="safe.password_label">Password</label>
                                        <input type="text" id="quick-safe-pwd-0" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <div class="form-group">
                                        <label data-lang="safe.api_url">API URL</label>
                                        <input type="text" id="quick-safe-api-0-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                    </div>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(0)" data-lang="safe.save_password">Save Password</button>
                                    <button class="btn-test" onclick="testSafeApi(0)" data-lang="safe.test_api">Test API</button>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-1" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-1-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label data-lang="safe.password_label">Password</label>
                                        <input type="text" id="quick-safe-pwd-1" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <div class="form-group">
                                        <label data-lang="safe.api_url">API URL</label>
                                        <input type="text" id="quick-safe-api-1-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                    </div>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(1)" data-lang="safe.save_password">Save Password</button>
                                    <button class="btn-test" onclick="testSafeApi(1)" data-lang="safe.test_api">Test API</button>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-2" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-2-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label data-lang="safe.password_label">Password</label>
                                        <input type="text" id="quick-safe-pwd-2" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <div class="form-group">
                                        <label data-lang="safe.api_url">API URL</label>
                                        <input type="text" id="quick-safe-api-2-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                    </div>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(2)" data-lang="safe.save_password">Save Password</button>
                                    <button class="btn-test" onclick="testSafeApi(2)" data-lang="safe.test_api">Test API</button>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-3" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-3-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label data-lang="safe.password_label">Password</label>
                                        <input type="text" id="quick-safe-pwd-3" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <div class="form-group">
                                        <label data-lang="safe.api_url">API URL</label>
                                        <input type="text" id="quick-safe-api-3-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                    </div>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(3)" data-lang="safe.save_password">Save Password</button>
                                    <button class="btn-test" onclick="testSafeApi(3)" data-lang="safe.test_api">Test API</button>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-4" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-4-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label data-lang="safe.password_label">Password</label>
                                        <input type="text" id="quick-safe-pwd-4" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <div class="form-group">
                                        <label data-lang="safe.api_url">API URL</label>
                                        <input type="text" id="quick-safe-api-4-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                    </div>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(4)" data-lang="safe.save_password">Save Password</button>
                                    <button class="btn-test" onclick="testSafeApi(4)" data-lang="safe.test_api">Test API</button>
                                </div>
                            </div>
                        </div>
                        
                    </div>
                </div>
                
                <!-- Ayırıcı Çizgi -->
                <div class="section-divider"></div>
                
                <!-- Cihaz Ayarları Başlığı -->
                <h2 class="device-settings-title">
                    <span data-lang="quick.device_settings">Device Settings</span>
                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.device_settings_info">Configure theme, language and network settings for your device.</span></span>
                </h2>
                
                <!-- Tema ve Dil Seçimi -->
                <div class="settings-row">
                    <!-- Sol: Tema -->
                    <div class="settings-group">
                        <h4 data-lang="quick.theme">Theme</h4>
                        <div class="theme-selector">
                            <label class="theme-option">
                                <input type="radio" name="theme" value="dark" checked onclick="setTheme('dark')">
                                <span class="theme-option-label">
                                    <span class="theme-name" data-lang="quick.theme_dark">Dark</span>
                                </span>
                            </label>
                            <label class="theme-option">
                                <input type="radio" name="theme" value="light" onclick="setTheme('light')">
                                <span class="theme-option-label">
                                    <span class="theme-name" data-lang="quick.theme_light">Light</span>
                                </span>
                            </label>
                        </div>
                    </div>
                    
                    <!-- Sağ: Dil -->
                    <div class="settings-group">
                        <h4 data-lang="quick.language">Language</h4>
                        <div class="language-selector">
                            <button class="lang-btn lang-btn-en active" onclick="setLanguage('en')">EN</button>
                            <button class="lang-btn lang-btn-de" onclick="setLanguage('de')">DE</button>
                            <button class="lang-btn lang-btn-tr" onclick="setLanguage('tr')">TR</button>
                        </div>
                    </div>
                </div>
                
                <!-- WiFi Bağlantı Açıklaması -->
                <div class="wifi-connection-info">
                    <p class="wifi-info-text">
                        <span data-lang="quick.wifi_settings">WiFi Settings</span>
                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.wifi_settings_info">Configure WiFi networks and AP Mode fallback settings.</span></span>
                    </p>
                    
                    <!-- AP Mode Card -->
                    <div class="ap-mode-card" id="ap-mode-card">
                        <div class="ap-mode-header">
                            <div class="ap-mode-title-section">
                                <div class="ap-mode-title-text">
                                    <h4 data-lang="connection.ap_mode_title">Access Point (AP) Mode</h4>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="connection.ap_mode_info">When WiFi is unavailable, the device creates its own network so you can still access the web interface.</span></span>
                                </div>
                            </div>
                            <div class="ap-mode-toggle-section">
                                <span class="ap-mode-status" id="ap-mode-status" data-lang="common.enabled">Enabled</span>
                                <label class="toggle-switch">
                                    <input type="checkbox" id="ap-mode-enabled" checked onchange="toggleAPMode(this.checked)">
                                    <span class="toggle-slider"></span>
                                </label>
                            </div>
                        </div>
                        
                        <div class="ap-mode-credentials">
                            <div class="ap-credential-row">
                                <div class="ap-credential-item-compact">
                                    <span class="ap-credential-label" data-lang="connection.network_name">SSID</span>
                                    <span class="ap-credential-value">SynDimm-SK[ChipID]</span>
                                </div>
                                <div class="ap-credential-item-compact">
                                    <span class="ap-credential-label" data-lang="connection.password">Password</span>
                                    <span class="ap-credential-value" data-lang="connection.no_password">None</span>
                                </div>
                                <div class="ap-credential-item-compact">
                                    <span class="ap-credential-label">IP</span>
                                    <span class="ap-credential-value">192.168.4.1</span>
                                </div>
                                <div class="ap-credential-item-compact">
                                    <span class="ap-credential-label">mDNS</span>
                                    <span class="ap-credential-value">dimm.local</span>
                                </div>
                            </div>
                        </div>
                        
                        <div class="ap-mode-warning" id="ap-mode-warning" style="display: none;">
                            <span class="warning-icon">&#9888;</span>
                            <span data-lang="connection.ap_disabled_warning">Warning: If AP Mode is disabled and WiFi is unavailable, device will be inaccessible!</span>
                        </div>
                    </div>
                    
                    <!-- Primary WiFi -->
                    <div class="accordion wifi-accordion-first">
                        <div class="accordion-header" onclick="toggleAccordion(this)">
                            <span class="accordion-title-text" data-lang="connection.primary_wifi">PRIMARY WIFI</span>
                            <span class="badge badge-not-configured" id="quick-primary-badge" data-lang="common.not_configured">Not Configured</span>
                            <span class="accordion-icon">▼</span>
                        </div>
                        <div class="accordion-content">
                            <div class="form-group">
                                <label data-lang="connection.ssid">SSID</label>
                                <input type="text" id="quick-primary-ssid" data-lang-placeholder="connection.enter_ssid">
                            </div>
                            <div class="form-group">
                                <label data-lang="connection.password_optional">Password (optional)</label>
                                <input type="password" id="quick-primary-password" data-lang-placeholder="connection.leave_empty_open">
                            </div>
                            <div class="form-group">
                                <label data-lang="connection.static_ip">Static IP (optional)</label>
                                <input type="text" id="quick-primary-static" data-lang-placeholder="connection.leave_empty_dhcp">
                            </div>
                            <div class="form-group">
                                <label data-lang="connection.local_domain">.local Domain Name (optional)</label>
                                <input type="text" id="quick-primary-mdns" placeholder="e.g: dimm">
                                <small class="form-hint" data-lang="connection.mdns_hint">Will be accessible as [name].local on the network</small>
                            </div>
                        </div>
                    </div>

                    <!-- Backup WiFi -->
                    <div class="accordion">
                        <div class="accordion-header" onclick="toggleAccordion(this)">
                            <span class="accordion-title-text" data-lang="connection.backup_wifi">BACKUP WIFI</span>
                            <span class="badge badge-not-configured" id="quick-backup-badge" data-lang="common.not_configured">Not Configured</span>
                            <span class="accordion-icon">▼</span>
                        </div>
                        <div class="accordion-content">
                            <div class="form-group">
                                <label data-lang="connection.ssid">SSID</label>
                                <input type="text" id="quick-backup-ssid" data-lang-placeholder="connection.enter_ssid">
                            </div>
                            <div class="form-group">
                                <label data-lang="connection.password_optional">Password (optional)</label>
                                <input type="password" id="quick-backup-password" data-lang-placeholder="connection.leave_empty_open">
                            </div>
                            <div class="form-group">
                                <label data-lang="connection.static_ip">Static IP (optional)</label>
                                <input type="text" id="quick-backup-static" data-lang-placeholder="connection.leave_empty_dhcp">
                            </div>
                            <div class="form-group">
                                <label data-lang="connection.local_domain">.local Domain Name (optional)</label>
                                <input type="text" id="quick-backup-mdns" placeholder="e.g: mysyndimm">
                                <small class="form-hint" data-lang="connection.mdns_hint">Will be accessible as [name].local on the network</small>
                            </div>
                        </div>
                    </div>
                </div>
                
                <!-- Cihaz Ayarlarını Kaydet Butonu -->
                <div class="save-button-container">
                    <button class="save-button save-device-settings" onclick="saveDeviceSettings()" data-lang="quick.save_device_settings">Save Device Settings</button>
                </div>
                
                <!-- Ayırıcı Çizgi -->
                <div class="section-divider"></div>
                
                <!-- OTA Update Section -->
                <div class="ota-quick-section">
                    <div class="ota-quick-header">
                        <div class="ota-quick-info">
                            <span class="ota-quick-label">
                                <span data-lang="info.current_version">Current Version</span>
                                <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.version_info">Check for firmware updates over the air (OTA).</span></span>
                            </span>
                            <span class="ota-quick-version" id="quick-ota-version">-</span>
                        </div>
                        <button class="btn-check-update" onclick="checkOTAUpdate()" data-lang="info.check_update">Check for Update</button>
                    </div>
                    <div id="quick-ota-status" class="ota-quick-status" style="display: none;"></div>
                    <div id="quick-ota-update-available" class="ota-update-available" style="display: none;">
                        <div class="ota-new-version">
                            <span data-lang="info.new_version">New Version</span>
                            <span id="quick-ota-new-version" class="version-badge">-</span>
                        </div>
                        <button class="btn-ota-install" onclick="startOTAUpdate()" data-lang="info.update_now">Update Now</button>
                    </div>
                    <div id="quick-ota-progress" class="ota-progress-container" style="display: none;">
                        <div class="ota-progress-bar">
                            <div id="quick-ota-progress-fill" class="ota-progress-fill" style="width: 0%"></div>
                        </div>
                        <span id="quick-ota-progress-text" class="ota-progress-text">0%</span>
                    </div>
                </div>
                
                <!-- Ayırıcı Çizgi -->
                <div class="section-divider"></div>
                
                <!-- System Operations -->
                <div class="system-operations">
                    <div class="system-buttons">
                        <button class="btn-restart" onclick="restartDevice()" data-lang="info.restart">Restart</button>
                        <button class="btn-factory-reset" onclick="showFactoryResetConfirm()">
                            <span data-lang="info.factory_reset">Factory Reset</span>
                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.factory_reset_info">Resets all settings to factory defaults. This action cannot be undone.</span></span>
                        </button>
                    </div>
                    <div id="factory-reset-confirm" class="factory-reset-confirm" style="display: none;">
                        <p data-lang="info.factory_reset_warning">Type 'Yes' to confirm factory reset:</p>
                        <input type="text" id="factory-reset-input" placeholder="Yes" autocomplete="off">
                        <button class="btn-confirm-reset" onclick="confirmFactoryReset()" data-lang="info.confirm">Confirm</button>
                        <button class="btn-cancel-reset" onclick="hideFactoryResetConfirm()" data-lang="info.cancel">Cancel</button>
                    </div>
                </div>
                
                <!-- Footer / Imza -->
                <div class="info-footer">
                    <h3 data-lang="info.support_docs">Support and Documentation</h3>
                    <p data-lang="info.support_desc">For detailed user guide, example scenarios and updates:</p>
                    <div class="button-group">
                        <a href="https://smartkraft.ch/syndimm" target="_blank" class="info-button">SmartKraft.ch</a>
                        <a href="https://github.com/smrtkrft/SynDimm" target="_blank" class="info-button">GitHub</a>
                    </div>
                </div>
                
            </div>
        </div>
    </div>
    
    <script src="/script.js"></script>
</body>
</html>)rawliteral";
    
    return html;
}

#endif // SK_HTML_H
