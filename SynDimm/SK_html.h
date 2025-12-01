/**
 * SK_html.h
 * SmartKraft SynDimm - HTML Structure
 * Version: v1.0.2
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
            </div>
        </div>
        
        <div class="tabs">
            <button class="tab active" onclick="openTab(event, 'hizli-ayarlar')" data-lang="tabs.quick">Quick Control</button>
            <button class="tab" onclick="openTab(event, 'modlar')" data-lang="tabs.modes">Modes</button>
            <button class="tab" onclick="openTab(event, 'baglanti')" data-lang="tabs.connection">Connection</button>
            <button class="tab" onclick="openTab(event, 'info')" data-lang="tabs.info">Info</button>
        </div>
        
        <div id="hizli-ayarlar" class="tab-content active">
            <div class="modes-content">
                
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
                
                <!-- Alt Satır: Tema -->
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
                
            </div>
        </div>
        
        <div id="modlar" class="tab-content">
            <div class="modes-content">
                
                <!-- Dimmer Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="dimmer.title">DIMMER</span>
                        <span class="badge badge-not-configured" id="dimmer-badge" data-lang="common.passive">Passive</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Status Info Bar (Top) - 4 Column Layout -->
                        <div class="dimmer-status-bar-new">
                            <div class="status-col">
                                <div class="status-col-label" data-lang="dimmer.ip_address">IP ADDRESS</div>
                                <div class="status-col-value" id="dimmer-status-ip">-</div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label" data-lang="dimmer.status">STATUS</div>
                                <div class="status-col-value">
                                    <span id="dimmer-status-brightness">0%</span>
                                    <span class="status-power-text" id="dimmer-status-power" data-lang="common.off">Off</span>
                                </div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label" data-lang="dimmer.calibration">CALIBRATION</div>
                                <div class="calibration-controls">
                                    <button class="btn-cal-up" onclick="adjustCalibration(1)" data-lang-title="common.increase">▲</button>
                                    <span class="calibration-value-display" id="dimmer-status-calibration">3</span>
                                    <button class="btn-cal-down" onclick="adjustCalibration(-1)" data-lang-title="common.decrease">▼</button>
                                </div>
                            </div>
                            <div class="status-col status-col-action">
                                <div class="status-col-label" data-lang="dimmer.action">ACTION</div>
                                <div class="status-col-value">
                                    <button class="btn-status-connect" id="btn-connect" onclick="connectDimmer()" data-lang="common.connect">Connect</button>
                                    <button class="btn-status-disconnect" id="btn-disconnect" onclick="disconnectDimmer()" style="display: none;" data-lang="common.disconnect">Disconnect</button>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Connection Section -->
                        <div class="dimmer-config-section">
                            <h4 class="dimmer-section-title" data-lang="dimmer.device_connection">Device Connection</h4>
                            <div class="form-group">
                                <label data-lang="dimmer.manual_ip">Manual IP Entry</label>
                                <input type="text" id="dimmer-ip-input" placeholder="192.168.1.100" class="input-full">
                            </div>
                            <div class="form-actions-row">
                                <button class="btn-primary" onclick="connectDimmerManual()" data-lang="common.connect">Connect</button>
                                <button class="btn-secondary" onclick="startNetworkScan()" data-lang="dimmer.scan_network">Scan Network</button>
                                <button class="btn-secondary" onclick="stopNetworkScan()" style="display:none;" id="btn-stop-scan" data-lang="dimmer.stop_scan">Stop Scan</button>
                            </div>
                        </div>
                        
                        <!-- Saved Devices List -->
                        <div class="dimmer-config-section">
                            <h4 class="dimmer-section-title" data-lang="dimmer.saved_devices">Saved Devices</h4>
                            <div class="saved-devices-list" id="saved-devices-list">
                                <div class="saved-device-empty" data-lang="dimmer.no_devices">No saved devices yet. Scan network or connect manually.</div>
                            </div>
                        </div>
                        
                    </div>
                </div>
                
                <!-- Shutter Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="shutter.title">SHUTTER</span>
                        <span class="badge badge-not-configured" id="shutter-badge" data-lang="common.passive">Passive</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Status Info Bar (Top) - 4 Column Layout -->
                        <div class="shutter-status-bar">
                            <div class="status-col">
                                <div class="status-col-label" data-lang="shutter.ip_address">IP ADDRESS</div>
                                <div class="status-col-value" id="shutter-status-ip">-</div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label" data-lang="shutter.status">STATUS</div>
                                <div class="status-col-value">
                                    <span class="shutter-status-text" id="shutter-status-text" data-lang="shutter.not_connected">Not Connected</span>
                                </div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label" data-lang="shutter.position">POSITION</div>
                                <div class="status-col-value">
                                    <div class="position-display">
                                        <div class="position-bar-container">
                                            <div class="position-bar" id="shutter-position-bar" style="width: 0%;"></div>
                                        </div>
                                        <span class="position-percent" id="shutter-position-percent">0%</span>
                                    </div>
                                </div>
                            </div>
                            <div class="status-col status-col-action">
                                <div class="status-col-label" data-lang="shutter.encoder_step">ENCODER STEP</div>
                                <div class="calibration-controls">
                                    <button class="btn-cal-up" onclick="adjustShutterStep(1)" data-lang-title="common.increase">▲</button>
                                    <span class="calibration-value-display" id="shutter-encoder-step">3</span>
                                    <button class="btn-cal-down" onclick="adjustShutterStep(-1)" data-lang-title="common.decrease">▼</button>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Connection Section -->
                        <div class="shutter-config-section">
                            <h4 class="shutter-section-title" data-lang="shutter.device_connection">Device Connection</h4>
                            <div class="form-group">
                                <label data-lang="shutter.manual_ip">Manual IP Entry</label>
                                <input type="text" id="shutter-ip-input" placeholder="192.168.1.100" class="input-full">
                            </div>
                            <div class="form-actions-row">
                                <button class="btn-primary" onclick="connectShutterManual()" data-lang="common.connect">Connect</button>
                                <button class="btn-secondary" onclick="disconnectShutter()" id="btn-shutter-disconnect" style="display:none;" data-lang="common.disconnect">Disconnect</button>
                                <button class="btn-secondary" onclick="startShutterNetworkScan()" data-lang="shutter.scan_network">Scan Network</button>
                                <button class="btn-secondary" onclick="stopShutterNetworkScan()" style="display:none;" id="btn-stop-shutter-scan" data-lang="shutter.stop_scan">Stop Scan</button>
                            </div>
                        </div>
                        
                        <!-- Saved Shutter Devices List -->
                        <div class="shutter-config-section">
                            <h4 class="shutter-section-title" data-lang="shutter.saved_devices">Saved Devices</h4>
                            <div class="saved-devices-list" id="saved-shutter-devices-list">
                                <div class="saved-device-empty" data-lang="shutter.no_devices">No saved devices yet. Scan network or connect manually.</div>
                            </div>
                        </div>
                        
                        <div class="shutter-info-text warning">
                            <span class="warning-icon">&#9888;</span> <span data-lang="shutter.info_text">Shutter is controlled by encoder. You can only calibrate speed from your browser. Theoretically complete but not tested on real shutter.</span>
                        </div>
                        
                        <div class="shutter-info-text warning" id="shutter-calibration-warning" style="display: none;">
                            <span class="warning-icon">&#9888;</span> <span data-lang="shutter.calibration_warning">Your shutter device is not calibrated, encoder rotation is temporarily disabled. However, you can still perform full open, full close and stop operations with the encoder button.</span>
                        </div>
                        
                    </div>
                </div>
                
                <!-- Safe Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="safe.title">SAFE</span>
                        <span class="badge badge-not-configured" id="safe-badge" data-lang="common.passive">Passive</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Safe Mod Açıklama -->
                        <div class="mode-info-text" data-lang="safe.description">
                            Password entry is done with encoder movements. You can define an API endpoint for each password.
                            When the correct password is entered, ESP32C6 automatically triggers the defined API.
                        </div>
                        
                        <!-- Şifre Tab'ları -->
                        <div class="safe-tabs">
                            <button class="safe-tab active" onclick="openSafeTab(event, 0)" data-lang="safe.password_1">Password 1</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 1)" data-lang="safe.password_2">Password 2</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 2)" data-lang="safe.password_3">Password 3</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 3)" data-lang="safe.password_4">Password 4</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 4)" data-lang="safe.password_5">Password 5</button>
                        </div>
                        
                        <!-- Şifre 1 İçerik -->
                        <div class="safe-tab-content active" id="safe-tab-0">
                            
                            <!-- Enable Password Toggle -->
                            <div class="safe-toggle-row">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="safe-pwd-0-enabled">
                                    <span class="toggle-slider"></span>
                                </label>
                                <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label data-lang="safe.password_label">Password</label>
                                    <span class="form-hint-inline" data-lang="safe.password_format">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-0" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.api_url">API URL</label>
                                    <input type="text" id="safe-api-0-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.custom_header">Custom Header (optional)</label>
                                    <span class="form-hint-inline" data-lang="safe.header_format">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-0-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(0)" data-lang="safe.save_password">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(0)" data-lang="safe.test_api">Test API</button>
                            </div>
                            
                        </div>
                        
                        <!-- Şifre 2 İçerik -->
                        <div class="safe-tab-content" id="safe-tab-1">
                            
                            <!-- Enable Password Toggle -->
                            <div class="safe-toggle-row">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="safe-pwd-1-enabled">
                                    <span class="toggle-slider"></span>
                                </label>
                                <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label data-lang="safe.password_label">Password</label>
                                    <span class="form-hint-inline" data-lang="safe.password_format">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-1" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.api_url">API URL</label>
                                    <input type="text" id="safe-api-1-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.custom_header">Custom Header (optional)</label>
                                    <span class="form-hint-inline" data-lang="safe.header_format">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-1-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(1)" data-lang="safe.save_password">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(1)" data-lang="safe.test_api">Test API</button>
                            </div>
                            
                        </div>
                        
                        <!-- Şifre 3 İçerik -->
                        <div class="safe-tab-content" id="safe-tab-2">
                            
                            <!-- Enable Password Toggle -->
                            <div class="safe-toggle-row">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="safe-pwd-2-enabled">
                                    <span class="toggle-slider"></span>
                                </label>
                                <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label data-lang="safe.password_label">Password</label>
                                    <span class="form-hint-inline" data-lang="safe.password_format">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-2" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.api_url">API URL</label>
                                    <input type="text" id="safe-api-2-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.custom_header">Custom Header (optional)</label>
                                    <span class="form-hint-inline" data-lang="safe.header_format">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-2-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(2)" data-lang="safe.save_password">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(2)" data-lang="safe.test_api">Test API</button>
                            </div>
                            
                        </div>
                        
                        <!-- Şifre 4 İçerik -->
                        <div class="safe-tab-content" id="safe-tab-3">
                            
                            <!-- Enable Password Toggle -->
                            <div class="safe-toggle-row">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="safe-pwd-3-enabled">
                                    <span class="toggle-slider"></span>
                                </label>
                                <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label data-lang="safe.password_label">Password</label>
                                    <span class="form-hint-inline" data-lang="safe.password_format">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-3" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.api_url">API URL</label>
                                    <input type="text" id="safe-api-3-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.custom_header">Custom Header (optional)</label>
                                    <span class="form-hint-inline" data-lang="safe.header_format">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-3-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(3)" data-lang="safe.save_password">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(3)" data-lang="safe.test_api">Test API</button>
                            </div>
                            
                        </div>
                        
                        <!-- Şifre 5 İçerik -->
                        <div class="safe-tab-content" id="safe-tab-4">
                            
                            <!-- Enable Password Toggle -->
                            <div class="safe-toggle-row">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="safe-pwd-4-enabled">
                                    <span class="toggle-slider"></span>
                                </label>
                                <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label data-lang="safe.password_label">Password</label>
                                    <span class="form-hint-inline" data-lang="safe.password_format">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-4" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.api_url">API URL</label>
                                    <input type="text" id="safe-api-4-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label data-lang="safe.custom_header">Custom Header (optional)</label>
                                    <span class="form-hint-inline" data-lang="safe.header_format">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-4-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(4)" data-lang="safe.save_password">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(4)" data-lang="safe.test_api">Test API</button>
                            </div>
                            
                        </div>
                        
                    </div>
                </div>
                
                <!-- Alarm Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="alarm.title">ALARM</span>
                        <span class="badge badge-not-configured" id="alarm-badge" data-lang="common.passive">Passive</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Alarm Mod Açıklama -->
                        <div class="mode-info-text" data-lang="alarm.description">
                            Alarm mode is a security feature designed to instantly notify your loved ones or request help in emergencies. 
                            When you interact with the encoder, a signal is sent to the mobile app via API. 
                            This signal triggers the ringtone of phones with the app installed and displays your predetermined messages on screen.
                        </div>
                        
                        <div class="mode-info-text">
                            <strong data-lang="alarm.example_title">Example Usage Scenario:</strong><br><br>
                            <span data-lang="alarm.example_text">When you interact with the encoder while your SynDimm device is in Alarm mode, 
                            an emergency request is automatically sent to the mobile app on Android or iOS devices linked with your device ID. 
                            Phones receiving the notification start ringing loudly and your predetermined emergency message is displayed on screen. 
                            This provides a smart solution you can use as a panic button for your personal safety.</span>
                        </div>
                        
                        <div class="mode-info-text">
                            <strong data-lang="common.note">Note:</strong> <span data-lang="alarm.dev_note">The system is currently under development and will be available soon.</span>
                        </div>
                        
                    </div>
                </div>
                
            </div>
        </div>
        
        <div id="baglanti" class="tab-content">
            <div class="connection-content">
                
                <!-- Notification Box -->
                <div id="notification" class="notification" style="display: none;">
                    <div class="notification-content">
                        <span id="notification-icon" class="notification-icon"></span>
                        <span id="notification-message" class="notification-message"></span>
                    </div>
                    <button class="notification-close" onclick="closeNotification()">×</button>
                </div>
                
                <!-- Connection Status Card -->
                <div class="status-card">
                    <div class="status-row">
                        <div class="status-item">
                            <span class="status-label" data-lang="connection.mode">MODE</span>
                            <span class="status-value" id="status-mode">WiFi</span>
                        </div>
                        <div class="status-item">
                            <span class="status-label" data-lang="connection.ssid">SSID</span>
                            <span class="status-value" id="status-ssid">-</span>
                        </div>
                        <div class="status-item">
                            <span class="status-label" data-lang="connection.ip_address">IP ADDRESS</span>
                            <span class="status-value" id="status-ip">-</span>
                            <span class="status-mdns" id="status-mdns">-</span>
                        </div>
                    </div>
                </div>
                
                <!-- AP Mode Bilgilendirme -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="connection.ap_mode_title">AP MODE - EMERGENCY ACCESS</span>
                        <span class="badge badge-info" data-lang="common.automatic">Automatic</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        <div class="ap-info-text" data-lang="connection.ap_info">
                            If WiFi connection cannot be established, the device automatically opens its own access point. 
                            This way you can always access the device and configure its settings.
                        </div>
                        
                        <div class="ap-details">
                            <div class="ap-detail-row">
                                <span class="ap-label" data-lang="connection.network_name">Network Name (SSID)</span>
                                <span class="ap-value">SynDimm-SK[ChipID]</span>
                            </div>
                            <div class="ap-detail-row">
                                <span class="ap-label" data-lang="connection.password">Password</span>
                                <span class="ap-value" data-lang="connection.no_password">None (Open Network)</span>
                            </div>
                            <div class="ap-detail-row">
                                <span class="ap-label" data-lang="connection.ip_address">IP Address</span>
                                <span class="ap-value">192.168.4.1</span>
                            </div>
                            <div class="ap-detail-row">
                                <span class="ap-label">mDNS</span>
                                <span class="ap-value">dimm.local</span>
                            </div>
                        </div>
                        
                        <div class="ap-steps">
                            <div class="ap-step-title" data-lang="connection.connection_order">Connection Order</div>
                            <div class="ap-step-flow">
                                <span class="ap-step" data-lang="connection.step_primary">1. Primary WiFi</span>
                                <span class="ap-arrow">&gt;</span>
                                <span class="ap-step" data-lang="connection.step_backup">2. Backup WiFi</span>
                                <span class="ap-arrow">&gt;</span>
                                <span class="ap-step ap-step-active" data-lang="connection.step_ap">3. AP Mode</span>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Primary WiFi -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="connection.primary_wifi">PRIMARY WIFI</span>
                        <span class="badge badge-not-configured" id="primary-badge" data-lang="common.not_configured">Not Configured</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        <div class="form-group">
                            <label data-lang="connection.ssid">SSID</label>
                            <input type="text" id="primary-ssid" data-lang-placeholder="connection.enter_ssid">
                        </div>
                        <div class="form-group">
                            <label data-lang="connection.password_optional">Password (optional)</label>
                            <input type="password" id="primary-password" data-lang-placeholder="connection.leave_empty_open">
                        </div>
                        <div class="form-group">
                            <label data-lang="connection.static_ip">Static IP (optional)</label>
                            <input type="text" id="primary-static" data-lang-placeholder="connection.leave_empty_dhcp">
                        </div>
                        <div class="form-group">
                            <label data-lang="connection.local_domain">.local Domain Name (optional)</label>
                            <input type="text" id="primary-mdns" placeholder="e.g: dimm">
                            <small class="form-hint" data-lang="connection.mdns_hint">Will be accessible as [name].local on the network</small>
                        </div>
                    </div>
                </div>

                <!-- Backup WiFi -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text" data-lang="connection.backup_wifi">BACKUP WIFI</span>
                        <span class="badge badge-not-configured" id="backup-badge" data-lang="common.not_configured">Not Configured</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        <div class="form-group">
                            <label data-lang="connection.ssid">SSID</label>
                            <input type="text" id="backup-ssid" data-lang-placeholder="connection.enter_ssid">
                        </div>
                        <div class="form-group">
                            <label data-lang="connection.password_optional">Password (optional)</label>
                            <input type="password" id="backup-password" data-lang-placeholder="connection.leave_empty_open">
                        </div>
                        <div class="form-group">
                            <label data-lang="connection.static_ip">Static IP (optional)</label>
                            <input type="text" id="backup-static" data-lang-placeholder="connection.leave_empty_dhcp">
                        </div>
                        <div class="form-group">
                            <label data-lang="connection.local_domain">.local Domain Name (optional)</label>
                            <input type="text" id="backup-mdns" placeholder="e.g: mysyndimm">
                            <small class="form-hint" data-lang="connection.mdns_hint">Will be accessible as [name].local on the network</small>
                        </div>
                    </div>
                </div>

                <!-- Kaydet Butonu -->
                <div class="save-button-container">
                    <button class="save-button" onclick="saveNetworkSettings()" data-lang="connection.save_network">Save Network Configuration</button>
                </div>

            </div>
        </div>
        
        <div id="info" class="tab-content">
            <div class="info-content">
                
                <!-- OTA Update Section -->
                <div class="ota-section">
                    <h2 data-lang="info.version_update">Version Update</h2>
                    
                    <!-- Current Version Card -->
                    <div class="ota-version-card">
                        <div class="ota-version-label" data-lang="info.device_version">Device Version:</div>
                        <div class="ota-version-value" id="ota-current-version">v1.0.2</div>
                    </div>
                    
                    <!-- Update Available Card -->
                    <div class="ota-update-card" id="ota-update-card" style="display: none;">
                        <div class="ota-update-header">
                            <span class="ota-update-icon">⬆</span>
                            <span class="ota-update-title" data-lang="info.new_version_available">New Version Available!</span>
                        </div>
                        <div class="ota-update-version" id="ota-latest-version">-</div>
                        <div class="ota-update-date" id="ota-publish-date">-</div>
                        <div class="ota-release-notes" id="ota-release-notes">-</div>
                    </div>
                    
                    <!-- Auto Update Toggle -->
                    <div class="ota-toggle-row">
                        <label class="toggle-switch">
                            <input type="checkbox" id="ota-auto-update">
                            <span class="toggle-slider"></span>
                        </label>
                        <span class="toggle-label" data-lang="info.auto_update">Automatic Update</span>
                    </div>
                    
                    <div class="ota-auto-info" data-lang="info.auto_update_info">
                        When automatic update is enabled, the device checks for new versions and updates itself. When disabled, you only receive notifications.
                    </div>
                    
                    <!-- Action Buttons -->
                    <div class="ota-actions">
                        <button class="btn-ota-check" onclick="checkForUpdate()" data-lang="info.check_update">Check for Update</button>
                        <button class="btn-ota-update" id="btn-ota-update" onclick="performUpdate()" style="display: none;" data-lang="info.update_now">Update Now</button>
                    </div>
                    
                    <!-- Progress Bar -->
                    <div class="ota-progress-container" id="ota-progress-container" style="display: none;">
                        <div class="ota-progress-label" id="ota-progress-label" data-lang="info.downloading">Downloading...</div>
                        <div class="ota-progress-bar">
                            <div class="ota-progress-fill" id="ota-progress-fill"></div>
                        </div>
                        <div class="ota-progress-percent" id="ota-progress-percent">0%</div>
                    </div>
                    
                    <!-- Status Message -->
                    <div class="ota-status-message" id="ota-status-message" style="display: none;"></div>
                </div>
                
                <div class="info-divider"></div>
                
                <h2 data-lang="info.usage_guide">Usage Guide</h2>
                
                <div class="info-section">
                    <h3 data-lang="info.dimmer_mode">Dimmer Mode</h3>
                    <p data-lang="info.dimmer_desc">Works integrated with Shelly Dimmer devices. You can control lighting brightness with the encoder.</p>
                    <p><strong data-lang="info.encoder_rotation">Encoder Rotation:</strong> <span data-lang="info.dimmer_rotation_desc">Turning right increases brightness, turning left decreases.</span></p>
                    <p><strong data-lang="info.encoder_button">Encoder Button:</strong> <span data-lang="info.dimmer_button_desc">Single press toggles the device on/off.</span></p>
                    <p><strong data-lang="info.calibration">Calibration:</strong> <span data-lang="info.dimmer_calibration_desc">Calibration value between 1-5 can be set from web interface. This value determines the brightness percentage change for each encoder movement.</span></p>
                    <p><strong data-lang="info.device_connection">Device Connection:</strong> <span data-lang="info.dimmer_connection_desc">Find and connect to Shelly Dimmer devices on your local network using the Scan Network button.</span></p>
                </div>
                
                <div class="info-section">
                    <h3 data-lang="info.shutter_mode">Shutter Mode</h3>
                    <p data-lang="info.shutter_desc">Works with shutter/blind controller devices like Shelly 2.5 or Shelly Plus 2PM.</p>
                    <p><strong data-lang="info.encoder_rotation">Encoder Rotation:</strong> <span data-lang="info.shutter_rotation_desc">Turning right closes the shutter (down), turning left opens (up). Note: Encoder rotation is disabled if device is not calibrated.</span></p>
                    <p><strong data-lang="info.encoder_button">Encoder Button:</strong> <span data-lang="info.shutter_button_desc">Stops if moving. If stopped, performs full open or full close opposite to last movement.</span></p>
                    <p><strong data-lang="info.calibration">Calibration:</strong> <span data-lang="info.shutter_calibration_desc">Motor calibration should be done from Shelly's own app. Encoder sensitivity (1-5) can be adjusted from web interface.</span></p>
                </div>
                
                <div class="info-section">
                    <h3 data-lang="info.safe_mode">Safe Mode</h3>
                    <p data-lang="info.safe_desc">Password is entered with encoder movements and the defined API is called on correct password.</p>
                    <p><strong data-lang="info.password_format_title">Password Format:</strong> <span data-lang="info.safe_format_desc">Consists of movements like R5-L3-R2-B. R=right, L=left, number=rotation count, B=button.</span></p>
                    <p><strong data-lang="info.settings">Settings:</strong> <span data-lang="info.safe_settings_desc">5 different passwords can be defined from web interface. HTTP GET or POST API URL is entered for each password.</span></p>
                    <p><strong data-lang="info.usage">Usage:</strong> <span data-lang="info.safe_usage_desc">Suitable for smart lock opening, garage door control, custom automation scenarios.</span></p>
                </div>
                
                <div class="info-section">
                    <h3 data-lang="info.alarm_mode">Alarm Mode (Under Development)</h3>
                    <p><strong data-lang="info.emergency_notification">Emergency Notification:</strong> <span data-lang="info.alarm_notification_desc">When you touch or turn the encoder (direction/amount doesn't matter), sends signal to mobile app via API.</span></p>
                    <p><strong data-lang="info.function">Function:</strong> <span data-lang="info.alarm_function_desc">Triggers ringtone on target phone and delivers emergency message.</span></p>
                    <p class="info-note info-note-warning" data-lang="info.alarm_dev_note">Alarm mode is under development.</p>
                </div>
                
                <div class="info-section">
                    <h3 data-lang="info.general_features">General Features</h3>
                    <p><strong data-lang="info.mode_switch">Mode Switch:</strong> <span data-lang="info.mode_switch_desc">Hold encoder button for 3 seconds. Enters preview mode, select mode by turning encoder, confirm by holding button for 3 seconds again.</span></p>
                    <p><strong data-lang="info.wifi_connection">WiFi Connection:</strong> <span data-lang="info.wifi_desc">Connect to WiFi network by entering SSID and password in Connection tab. DHCP or static IP options available. With mDNS support, you can also access via dimm.local address from browser.</span></p>
                    <p><strong data-lang="info.quick_menu">Quick Menu:</strong> <span data-lang="info.quick_menu_desc">You can see active mode, WiFi status and IP address from status bar at top right. Clicking opens detailed system info panel.</span></p>
                    <p><strong data-lang="info.mode_memory">Mode Memory:</strong> <span data-lang="info.mode_memory_desc">Selected mode is saved to flash memory and device automatically runs with last active mode when restarted.</span></p>
                    <p><strong data-lang="info.ota_update">OTA Update:</strong> <span data-lang="info.ota_desc">New versions on GitHub can be checked from Info tab. If update available, wireless installation can be done with one click.</span></p>
                </div>
                
                <div class="info-section system-actions">
                    <h3 data-lang="info.system_operations">System Operations</h3>
                    <div class="system-buttons">
                        <button class="btn-restart" onclick="restartDevice()" data-lang="info.restart">Restart</button>
                        <button class="btn-factory-reset" onclick="showFactoryResetConfirm()" data-lang="info.factory_reset">Factory Reset</button>
                    </div>
                    <div id="factory-reset-confirm" class="factory-reset-confirm" style="display:none;">
                        <p data-lang="info.factory_reset_confirm">All settings will be deleted. Type <strong>Yes</strong> to confirm:</p>
                        <input type="text" id="factory-reset-input" placeholder="Yes" autocomplete="off">
                        <button class="btn-confirm-reset" onclick="confirmFactoryReset()" data-lang="info.confirm">Confirm</button>
                        <button class="btn-cancel-reset" onclick="hideFactoryResetConfirm()" data-lang="info.cancel">Cancel</button>
                    </div>
                </div>
                
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
