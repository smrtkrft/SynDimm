/**
 * SK_html.h
 * SmartKraft SynDimm - HTML Structure
 * Version: v0.9.1
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
            <button class="tab active" onclick="openTab(event, 'hizli-ayarlar')">Hızlı Kontrol</button>
            <button class="tab" onclick="openTab(event, 'modlar')">Modlar</button>
            <button class="tab" onclick="openTab(event, 'baglanti')">Bağlantı</button>
            <button class="tab" onclick="openTab(event, 'info')">Info</button>
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
                        <h4>Tema</h4>
                        <div class="theme-selector">
                            <label class="theme-option">
                                <input type="radio" name="theme" value="dark" checked onclick="setTheme('dark')">
                                <span class="theme-option-label">
                                    <span class="theme-name">Koyu</span>
                                </span>
                            </label>
                            <label class="theme-option">
                                <input type="radio" name="theme" value="light" onclick="setTheme('light')">
                                <span class="theme-option-label">
                                    <span class="theme-name">Açık</span>
                                </span>
                            </label>
                        </div>
                    </div>
                    
                    <!-- Sağ: Dil -->
                    <div class="settings-group">
                        <h4>Dil</h4>
                        <div class="language-selector">
                            <button class="lang-btn lang-btn-tr active" onclick="setLanguage('tr')">TR</button>
                            <button class="lang-btn lang-btn-en" onclick="setLanguage('en')">EN</button>
                            <button class="lang-btn lang-btn-de" onclick="setLanguage('de')">DE</button>
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
                        <span class="accordion-title-text">DIMMER</span>
                        <span class="badge badge-not-configured" id="dimmer-badge">Pasif</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Status Info Bar (Top) - 4 Column Layout -->
                        <div class="dimmer-status-bar-new">
                            <div class="status-col">
                                <div class="status-col-label">IP ADRESI</div>
                                <div class="status-col-value" id="dimmer-status-ip">-</div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label">DURUM</div>
                                <div class="status-col-value">
                                    <span id="dimmer-status-brightness">0%</span>
                                    <span class="status-power-text" id="dimmer-status-power">Kapalı</span>
                                </div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label">KALIBRASYON</div>
                                <div class="calibration-controls">
                                    <button class="btn-cal-up" onclick="adjustCalibration(1)" title="Artır">▲</button>
                                    <span class="calibration-value-display" id="dimmer-status-calibration">3</span>
                                    <button class="btn-cal-down" onclick="adjustCalibration(-1)" title="Azalt">▼</button>
                                </div>
                            </div>
                            <div class="status-col status-col-action">
                                <div class="status-col-label">AKSIYON</div>
                                <div class="status-col-value">
                                    <button class="btn-status-connect" id="btn-connect" onclick="connectDimmer()">Bağlan</button>
                                    <button class="btn-status-disconnect" id="btn-disconnect" onclick="disconnectDimmer()" style="display: none;">Bağlantıyı Kes</button>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Connection Section -->
                        <div class="dimmer-config-section">
                            <h4 class="dimmer-section-title">Cihaz Bağlantısı</h4>
                            <div class="form-group">
                                <label>Manuel IP Girişi</label>
                                <input type="text" id="dimmer-ip-input" placeholder="192.168.1.100" class="input-full">
                            </div>
                            <div class="form-actions-row">
                                <button class="btn-primary" onclick="connectDimmerManual()">Bağlan</button>
                                <button class="btn-secondary" onclick="startNetworkScan()">Ağı Tara</button>
                                <button class="btn-secondary" onclick="stopNetworkScan()" style="display:none;" id="btn-stop-scan">Taramayı Durdur</button>
                            </div>
                        </div>
                        
                        <!-- Saved Devices List -->
                        <div class="dimmer-config-section">
                            <h4 class="dimmer-section-title">Kayıtlı Cihazlar</h4>
                            <div class="saved-devices-list" id="saved-devices-list">
                                <div class="saved-device-empty">Henüz kayıtlı cihaz yok. Ağ taraması yapın veya manuel bağlanın.</div>
                            </div>
                        </div>
                        
                    </div>
                </div>
                
                <!-- Shutter Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text">SHUTTER</span>
                        <span class="badge badge-not-configured" id="shutter-badge">Pasif</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Status Info Bar (Top) - 4 Column Layout -->
                        <div class="shutter-status-bar">
                            <div class="status-col">
                                <div class="status-col-label">IP ADRESI</div>
                                <div class="status-col-value" id="shutter-status-ip">-</div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label">DURUM</div>
                                <div class="status-col-value">
                                    <span class="shutter-status-text" id="shutter-status-text">Bağlı Değil</span>
                                </div>
                            </div>
                            <div class="status-col">
                                <div class="status-col-label">POZISYON</div>
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
                                <div class="status-col-label">ENCODER STEP</div>
                                <div class="calibration-controls">
                                    <button class="btn-cal-up" onclick="adjustShutterStep(1)" title="Artır">▲</button>
                                    <span class="calibration-value-display" id="shutter-encoder-step">3</span>
                                    <button class="btn-cal-down" onclick="adjustShutterStep(-1)" title="Azalt">▼</button>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Connection Section -->
                        <div class="shutter-config-section">
                            <h4 class="shutter-section-title">Cihaz Bağlantısı</h4>
                            <div class="form-group">
                                <label>Manuel IP Girişi</label>
                                <input type="text" id="shutter-ip-input" placeholder="192.168.1.100" class="input-full">
                            </div>
                            <div class="form-actions-row">
                                <button class="btn-primary" onclick="connectShutterManual()">Baglan</button>
                                <button class="btn-secondary" onclick="disconnectShutter()" id="btn-shutter-disconnect" style="display:none;">Baglatiyi Kes</button>
                                <button class="btn-secondary" onclick="startShutterNetworkScan()">Agi Tara</button>
                                <button class="btn-secondary" onclick="stopShutterNetworkScan()" style="display:none;" id="btn-stop-shutter-scan">Taramayi Durdur</button>
                            </div>
                        </div>
                        
                        <!-- Saved Shutter Devices List -->
                        <div class="shutter-config-section">
                            <h4 class="shutter-section-title">Kayıtlı Cihazlar</h4>
                            <div class="saved-devices-list" id="saved-shutter-devices-list">
                                <div class="saved-device-empty">Henüz kayıtlı cihaz yok. Ağ taraması yapın veya manuel bağlanın.</div>
                            </div>
                        </div>
                        
                        <div class="shutter-info-text warning">
                            <span class="warning-icon">&#9888;</span> Shutter encoder ile kontrol edilir. Tarayicinizdan sadece hiz kalibrasyonu yapabilirsiniz. Teorik olarak tamamlanmis ancak gercek dunyada panjur uzerinde test islemi gerceklestirilmemistir.
                        </div>
                        
                        <div class="shutter-info-text warning" id="shutter-calibration-warning" style="display: none;">
                            <span class="warning-icon">&#9888;</span> Shutter aygitiiniz kalibre edilmedigi icin encoder donus hareketleri gecici olarak devre disi. Ancak yine de encoder butonu ile tam acma, tam kapatma ve durdurma islemlerini gerceklestirmeye devam edebilirsiniz.
                        </div>
                        
                    </div>
                </div>
                
                <!-- Safe Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text">SAFE</span>
                        <span class="badge badge-not-configured" id="safe-badge">Pasif</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Safe Mod Açıklama -->
                        <div class="mode-info-text">
                            Encoder hareketleriyle şifre girişi yapılır. Her şifre için API endpoint tanımlayabilirsiniz.
                            Doğru şifre girildiğinde ESP32C6 otomatik olarak tanımlı API'yi tetikler.
                        </div>
                        
                        <!-- Şifre Tab'ları -->
                        <div class="safe-tabs">
                            <button class="safe-tab active" onclick="openSafeTab(event, 0)">Sifre 1</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 1)">Sifre 2</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 2)">Sifre 3</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 3)">Sifre 4</button>
                            <button class="safe-tab" onclick="openSafeTab(event, 4)">Sifre 5</button>
                        </div>
                        
                        <!-- Şifre 1 İçerik -->
                        <div class="safe-tab-content active" id="safe-tab-0">
                            
                            <!-- Enable Password Toggle -->
                            <div class="safe-toggle-row">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="safe-pwd-0-enabled">
                                    <span class="toggle-slider"></span>
                                </label>
                                <span class="toggle-label">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label>Password</label>
                                    <span class="form-hint-inline">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-0" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label>API URL</label>
                                    <input type="text" id="safe-api-0-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label>Custom Header (optional)</label>
                                    <span class="form-hint-inline">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-0-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(0)">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(0)">Test API</button>
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
                                <span class="toggle-label">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label>Password</label>
                                    <span class="form-hint-inline">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-1" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label>API URL</label>
                                    <input type="text" id="safe-api-1-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label>Custom Header (optional)</label>
                                    <span class="form-hint-inline">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-1-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(1)">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(1)">Test API</button>
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
                                <span class="toggle-label">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label>Password</label>
                                    <span class="form-hint-inline">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-2" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label>API URL</label>
                                    <input type="text" id="safe-api-2-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label>Custom Header (optional)</label>
                                    <span class="form-hint-inline">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-2-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(2)">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(2)">Test API</button>
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
                                <span class="toggle-label">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label>Password</label>
                                    <span class="form-hint-inline">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-3" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label>API URL</label>
                                    <input type="text" id="safe-api-3-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label>Custom Header (optional)</label>
                                    <span class="form-hint-inline">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-3-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(3)">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(3)">Test API</button>
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
                                <span class="toggle-label">Enable Password</span>
                            </div>
                            
                            <!-- Configuration Section -->
                            <div class="safe-config-section">
                                
                                <div class="form-group">
                                    <label>Password</label>
                                    <span class="form-hint-inline">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    <input type="text" id="safe-pwd-4" placeholder="L3-R2-L1-R3">
                                </div>
                                
                                <div class="form-group">
                                    <label>API URL</label>
                                    <input type="text" id="safe-api-4-url" placeholder="http://192.168.1.100/rpc/switch.toggle?id=0">
                                </div>
                                
                                <div class="form-group">
                                    <label>Custom Header (optional)</label>
                                    <span class="form-hint-inline">Format: HeaderName: Value</span>
                                    <input type="text" id="safe-api-4-header" placeholder="X-API-Key: your-api-key">
                                </div>
                            </div>
                            
                            <!-- Action Buttons -->
                            <div class="form-actions">
                                <button class="btn-save" onclick="saveSafePassword(4)">Save Password</button>
                                <button class="btn-test" onclick="testSafeApi(4)">Test API</button>
                            </div>
                            
                        </div>
                        
                    </div>
                </div>
                
                <!-- Alarm Modu -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text">ALARM</span>
                        <span class="badge badge-not-configured" id="alarm-badge">Pasif</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        
                        <!-- Alarm Mod Açıklama -->
                        <div class="mode-info-text">
                            Alarm modu, acil durumlarda sevdiklerinize anında haber vermek veya yardım istemek için tasarlanmış bir güvenlik özelliğidir. 
                            Encoder'la etkileşime girdiğinizde API üzerinden mobil uygulamaya sinyal gönderilir. 
                            Bu sinyal uygulamanın yüklü olduğu telefonların zil sesini tetikleyerek önceden belirlediğiniz mesajları ekranda gösterir.
                        </div>
                        
                        <div class="mode-info-text">
                            <strong>Örnek Kullanım Senaryosu:</strong><br><br>
                            SynDimm cihazınız Alarm modundayken encoder ile herhangi bir etkileşime girdiğinizde, cihaz ID numaranızla bağlantı kurulmuş Android veya iOS cihazlardaki 
                            mobil uygulamasına otomatik olarak acil durum isteği gönderilir. Bildirim alan telefonlar yüksek sesle çalmaya başlar ve önceden belirlediğiniz 
                            acil durum mesajınız ekranda görüntülenir. Bu sayede, kişisel güvenliğiniz için bir panik butonu olarak kullanabileceğiniz akıllı bir çözüm sunar.
                        </div>
                        
                        <div class="mode-info-text">
                            <strong>Not:</strong> Sistem şu anda geliştirme aşamasındadır ve yakında kullanıma sunulacaktır.
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
                            <span class="status-label">MOD</span>
                            <span class="status-value" id="status-mode">WiFi</span>
                        </div>
                        <div class="status-item">
                            <span class="status-label">SSID</span>
                            <span class="status-value" id="status-ssid">-</span>
                        </div>
                        <div class="status-item">
                            <span class="status-label">IP ADRESI</span>
                            <span class="status-value" id="status-ip">-</span>
                            <span class="status-mdns" id="status-mdns">-</span>
                        </div>
                    </div>
                </div>
                
                <!-- AP Mode Bilgilendirme -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text">AP MODE - ACIL ERISIM</span>
                        <span class="badge badge-info">Otomatik</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        <div class="ap-info-text">
                            WiFi baglantisi kurulamazsa cihaz otomatik olarak kendi erisim noktasini acar. 
                            Bu sayede her zaman cihaza erisebilir ve ayarlarini yapabilirsiniz.
                        </div>
                        
                        <div class="ap-details">
                            <div class="ap-detail-row">
                                <span class="ap-label">Ag Adi (SSID)</span>
                                <span class="ap-value">SynDimm-SK[ChipID]</span>
                            </div>
                            <div class="ap-detail-row">
                                <span class="ap-label">Sifre</span>
                                <span class="ap-value">Yok (Acik Ag)</span>
                            </div>
                            <div class="ap-detail-row">
                                <span class="ap-label">IP Adresi</span>
                                <span class="ap-value">192.168.4.1</span>
                            </div>
                            <div class="ap-detail-row">
                                <span class="ap-label">mDNS</span>
                                <span class="ap-value">dimm.local</span>
                            </div>
                        </div>
                        
                        <div class="ap-steps">
                            <div class="ap-step-title">Baglanti Sirasi</div>
                            <div class="ap-step-flow">
                                <span class="ap-step">1. Primary WiFi</span>
                                <span class="ap-arrow">&gt;</span>
                                <span class="ap-step">2. Backup WiFi</span>
                                <span class="ap-arrow">&gt;</span>
                                <span class="ap-step ap-step-active">3. AP Mode</span>
                            </div>
                        </div>
                    </div>
                </div>

                <!-- Primary WiFi -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text">PRIMARY WIFI</span>
                        <span class="badge badge-not-configured" id="primary-badge">Not Configured</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        <div class="form-group">
                            <label>SSID</label>
                            <input type="text" id="primary-ssid" placeholder="WiFi SSID girin">
                        </div>
                        <div class="form-group">
                            <label>Şifre (opsiyonel)</label>
                            <input type="password" id="primary-password" placeholder="Açık ağ için boş bırakın">
                        </div>
                        <div class="form-group">
                            <label>Statik IP (opsiyonel)</label>
                            <input type="text" id="primary-static" placeholder="DHCP için boş bırakın">
                        </div>
                        <div class="form-group">
                            <label>.local Alan Adı (opsiyonel)</label>
                            <input type="text" id="primary-mdns" placeholder="örn: dimm">
                            <small class="form-hint">Will be accessible as [name].local on the network</small>
                        </div>
                    </div>
                </div>

                <!-- Backup WiFi -->
                <div class="accordion">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title-text">BACKUP WIFI</span>
                        <span class="badge badge-not-configured" id="backup-badge">Not Configured</span>
                        <span class="accordion-icon">▼</span>
                    </div>
                    <div class="accordion-content">
                        <div class="form-group">
                            <label>SSID</label>
                            <input type="text" id="backup-ssid" placeholder="WiFi SSID girin">
                        </div>
                        <div class="form-group">
                            <label>Şifre (opsiyonel)</label>
                            <input type="password" id="backup-password" placeholder="Açık ağ için boş bırakın">
                        </div>
                        <div class="form-group">
                            <label>Statik IP (opsiyonel)</label>
                            <input type="text" id="backup-static" placeholder="DHCP için boş bırakın">
                        </div>
                        <div class="form-group">
                            <label>.local Alan Adı (opsiyonel)</label>
                            <input type="text" id="backup-mdns" placeholder="örn: benimsyndimm">
                            <small class="form-hint">Will be accessible as [name].local on the network</small>
                        </div>
                    </div>
                </div>

                <!-- Kaydet Butonu -->
                <div class="save-button-container">
                    <button class="save-button" onclick="saveNetworkSettings()">Ağ Yapılandırmasını Kaydet</button>
                </div>

            </div>
        </div>
        
        <div id="info" class="tab-content">
            <div class="info-content">
                
                <!-- OTA Update Section -->
                <div class="ota-section">
                    <h2>Versiyon Güncelleme</h2>
                    
                    <!-- Current Version Card -->
                    <div class="ota-version-card">
                        <div class="ota-version-label">Cihaz Sürümü:</div>
                        <div class="ota-version-value" id="ota-current-version">v0.9.1</div>
                    </div>
                    
                    <!-- Update Available Card -->
                    <div class="ota-update-card" id="ota-update-card" style="display: none;">
                        <div class="ota-update-header">
                            <span class="ota-update-icon">⬆</span>
                            <span class="ota-update-title">Yeni Sürüm Mevcut!</span>
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
                        <span class="toggle-label">Otomatik Güncelleme</span>
                    </div>
                    
                    <div class="ota-auto-info">
                        Otomatik güncelleme açık olduğunda cihaz yeni sürümleri kontrol eder ve kendini günceller. Kapalı olduğunda sadece bildirim alırsınız.
                    </div>
                    
                    <!-- Action Buttons -->
                    <div class="ota-actions">
                        <button class="btn-ota-check" onclick="checkForUpdate()">Güncelleme Kontrol Et</button>
                        <button class="btn-ota-update" id="btn-ota-update" onclick="performUpdate()" style="display: none;">Şimdi Güncelle</button>
                    </div>
                    
                    <!-- Progress Bar -->
                    <div class="ota-progress-container" id="ota-progress-container" style="display: none;">
                        <div class="ota-progress-label" id="ota-progress-label">İndiriliyor...</div>
                        <div class="ota-progress-bar">
                            <div class="ota-progress-fill" id="ota-progress-fill"></div>
                        </div>
                        <div class="ota-progress-percent" id="ota-progress-percent">0%</div>
                    </div>
                    
                    <!-- Status Message -->
                    <div class="ota-status-message" id="ota-status-message" style="display: none;"></div>
                </div>
                
                <div class="info-divider"></div>
                
                <h2>Kullanim Kilavuzu</h2>
                
                <div class="info-section">
                    <h3>Dimmer Modu</h3>
                    <p>Shelly Dimmer cihazlari ile entegre calisir. Encoder ile aydinlatma parcakligini kontrol edebilirsiniz.</p>
                    <p><strong>Encoder Donusu:</strong> Saga cevirme parlaklik arttirir, sola cevirme azaltir.</p>
                    <p><strong>Encoder Butonu:</strong> Tek basma ile cihazi acar/kapatir (toggle).</p>
                    <p><strong>Kalibrasyon:</strong> Web arayuzunden 1-5 arasi kalibrasyon degeri ayarlanabilir. Bu deger her encoder hareketi icin degisecek parlaklik yuzdesini belirler.</p>
                    <p><strong>Cihaz Baglantisi:</strong> Ag Tara butonu ile yerel agdaki Shelly Dimmer cihazlarini bulun ve baglanin.</p>
                </div>
                
                <div class="info-section">
                    <h3>Shutter Modu</h3>
                    <p>Shelly 2.5 veya Shelly Plus 2PM gibi panjur/kepenk kontrolcu cihazlarla calisir.</p>
                    <p><strong>Encoder Donusu:</strong> Saga cevirme panjuru kapatir (asagi), sola cevirme acar (yukari). Not: Cihaz kalibre edilmemisse encoder donusu devre disi kalir.</p>
                    <p><strong>Encoder Butonu:</strong> Hareket halindeyse durdurur. Duruyorsa son hareketin tersine tam acma veya tam kapatma yapar.</p>
                    <p><strong>Kalibrasyon:</strong> Shelly cihazinin kendi uygulamasindan motor kalibrasyonu yapilmalidir. Web arayuzunden encoder hassasiyeti (1-5) ayarlanabilir.</p>
                </div>
                
                <div class="info-section">
                    <h3>Safe Modu</h3>
                    <p>Encoder hareketleriyle sifre girilir ve dogru sifrede belirlenen API cagirilir.</p>
                    <p><strong>Sifre Formati:</strong> R5-L3-R2-B gibi hareketlerden olusur. R=saga, L=sola, rakam=tur sayisi, B=buton.</p>
                    <p><strong>Ayarlama:</strong> Web arayuzunden 5 farkli sifre tanimlanabilir. Her sifre icin HTTP GET veya POST API URL girilir.</p>
                    <p><strong>Kullanim:</strong> Akilli kilit acma, garaj kapisi kontrolu, ozel otomasyon senaryolari icin uygundur.</p>
                </div>
                
                <div class="info-section">
                    <h3>Alarm Modu (Gelistirme Asamasinda)</h3>
                    <p><strong>Acil Bildirim:</strong> Encoder'a dokundugunuzda veya cevirdiginizde (yon/miktar onemsiz) API uzerinden mobil uygulamaya sinyal gonderir.</p>
                    <p><strong>Islev:</strong> Hedef telefonda zil sesi tetiklenir ve acil durum mesaji iletilir.</p>
                    <p class="info-note info-note-warning">Alarm modu gelistirme asamasindadir.</p>
                </div>
                
                <div class="info-section">
                    <h3>Genel Ozellikler</h3>
                    <p><strong>Mod Degistirme:</strong> Encoder butonuna 3 saniye basili tutun. Onizleme moduna gecer, encoder cevirilerek mod secilir, tekrar 3 saniye butona basili tutularak secim onaylanir.</p>
                    <p><strong>WiFi Baglantisi:</strong> Baglanti sekmesinden SSID ve sifre girerek WiFi agina baglanin. DHCP veya statik IP secenekleri mevcuttur. mDNS destegi ile tarayicidan dimm.local adresiyle de erisebilirsiniz.</p>
                    <p><strong>Hizli Menu:</strong> Sag ust kosedeki durum cubugundan aktif mod, WiFi durumu ve IP adresini gorebilirsiniz. Tiklandiginda detayli sistem bilgisi paneli acilir.</p>
                    <p><strong>Mod Hafizasi:</strong> Secilen mod flash bellegine kaydedilir ve cihaz yeniden baslatildiginda otomatik olarak son aktif mod ile calisir.</p>
                    <p><strong>OTA Guncelleme:</strong> Info sekmesinden GitHub uzerindeki yeni surumler kontrol edilebilir. Guncelleme mevcut ise tek tikla kablosuz yukleme yapilabilir.</p>
                </div>
                
                <div class="info-section system-actions">
                    <h3>Sistem Islemleri</h3>
                    <div class="system-buttons">
                        <button class="btn-restart" onclick="restartDevice()">Restart</button>
                        <button class="btn-factory-reset" onclick="showFactoryResetConfirm()">Factory Reset</button>
                    </div>
                    <div id="factory-reset-confirm" class="factory-reset-confirm" style="display:none;">
                        <p>Tum ayarlar silinecek. Onaylamak icin <strong>Evet</strong> yazin:</p>
                        <input type="text" id="factory-reset-input" placeholder="Evet" autocomplete="off">
                        <button class="btn-confirm-reset" onclick="confirmFactoryReset()">Onayla</button>
                        <button class="btn-cancel-reset" onclick="hideFactoryResetConfirm()">Iptal</button>
                    </div>
                </div>
                
                <div class="info-footer">
                    <h3>Destek</h3>
                    <p>Dokümantasyon ve güncellemeler</p>
                    <div class="button-group">
                        <a href="https://smartkraft.ch/syndimm" target="_blank" class="info-button">smartkraft.ch</a>
                        <a href="https://github.com/smrtkrft/SynDimm" target="_blank" class="info-button">github</a>
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
