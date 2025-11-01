/*
 * SynDimm - Auto-generated PROGMEM file
 * Source: index.html
 */

#ifndef HTML_PAGE_H
#define HTML_PAGE_H

const char HTML_PAGE[] PROGMEM = R"=====(
<!DOCTYPE html>
<html lang="tr">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>SynDimm</title>
    <link rel="stylesheet" href="style.css">
</head>
<body data-theme="dark">
    <!-- Header -->
    <header class="header">
        <div class="container">
            <div class="header-content">
                <div class="brand">
                    <h1>SynDimm</h1>
                    <a href="https://smartkraft.ch" target="_blank" class="brand-link">SmartKraft</a>
                </div>
            </div>
        </div>
    </header>
    
    <!-- Navigation -->
    <nav class="nav">
        <div class="container">
            <div class="nav-links">
                <a href="#" class="nav-link active" data-tab="control">Hizli kontrol</a>
                <a href="#" class="nav-link" data-tab="settings">Modlar</a>
                <a href="#" class="nav-link" data-tab="network">Baglanti</a>
                <a href="#" class="nav-link" data-tab="info">Info</a>
            </div>
        </div>
    </nav>
    
    <!-- Main Content -->
    <main class="main">
        <div class="container">
            
            <!-- Tab: Control -->
            <section id="control" class="tab-section active">
                <div class="panel">
                    <div class="section-header">Aktif Modunuzu Secin</div>
                    <div class="mode-grid">
                        <label class="mode-card active">
                            <input type="radio" name="mode" value="dimmer" checked>
                            <div class="mode-content">
                                <div class="mode-name">Dimmer</div>
                                <div class="mode-desc">Control brightness levels</div>
                            </div>
                            <div class="mode-check"></div>
                        </label>
                        <label class="mode-card">
                            <input type="radio" name="mode" value="safe">
                            <div class="mode-content">
                                <div class="mode-name">Safe</div>
                                <div class="mode-desc">Counter display mode</div>
                            </div>
                            <div class="mode-check"></div>
                        </label>
                        <label class="mode-card">
                            <input type="radio" name="mode" value="panic">
                            <div class="mode-content">
                                <div class="mode-name">Panik</div>
                                <div class="mode-desc">Emergency mode</div>
                            </div>
                            <div class="mode-check"></div>
                        </label>
                    </div>
                    
                    <div class="section-divider"></div>
                    
                    <div class="preferences-row">
                        <div class="pref-section">
                            <label class="pref-label">Theme</label>
                            <div class="option-group compact">
                                <label class="option-card active">
                                    <input type="radio" name="theme" value="dark" checked>
                                    <span>Dark</span>
                                </label>
                                <label class="option-card">
                                    <input type="radio" name="theme" value="light">
                                    <span>Light</span>
                                </label>
                            </div>
                        </div>
                        <div class="pref-divider"></div>
                        <div class="pref-section">
                            <label class="pref-label">Language</label>
                            <div class="option-group compact">
                                <label class="option-card">
                                    <input type="radio" name="language" value="en">
                                    <span>EN</span>
                                </label>
                                <label class="option-card">
                                    <input type="radio" name="language" value="de">
                                    <span>DE</span>
                                </label>
                                <label class="option-card active">
                                    <input type="radio" name="language" value="tr" checked>
                                    <span>TR</span>
                                </label>
                            </div>
                        </div>
                    </div>
                </div>
            </section>
            
            <!-- Tab: Settings -->
            <section id="settings" class="tab-section">
                
                <!-- Dimmer Mode Accordion -->
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title">Dimmer</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="dimmer-status">
                            <div class="status-item">
                                <span class="status-label">Connection</span>
                                <span class="status-value" id="dimmer-connection">Connected</span>
                            </div>
                            <div class="status-item">
                                <span class="status-label">Device</span>
                                <span class="status-value" id="dimmer-device">Shelly (192.168.11.11)</span>
                            </div>
                            <div class="status-item status-toggle">
                                <span class="status-label">Power</span>
                                <label class="device-toggle">
                                    <input type="checkbox" id="device-power" checked onchange="toggleDevicePower(this.checked)">
                                    <span class="device-toggle-slider"></span>
                                </label>
                            </div>
                        </div>
                        <div class="slider-controls">
                            <div class="slider-group">
                                <span class="slider-value" id="brightness-value">50</span>
                                <label class="slider-label">Brightness</label>
                                <input type="range" class="range-slider" min="0" max="100" value="50" id="brightness" oninput="updateSliderValue(this)">
                            </div>
                            <div class="slider-group">
                                <span class="slider-value" id="ratio-value">3</span>
                                <label class="slider-label">Dimm Ratio</label>
                                <input type="range" class="range-slider" min="1" max="5" value="3" id="ratio" oninput="updateSliderValue(this)">
                            </div>
                        </div>
                        <div class="scan-section">
                            <button class="btn btn-scan" onclick="scanDevices()">Scan Network</button>
                        </div>
                        <div class="device-list">
                            <div class="empty-state">
                                <p>No devices found</p>
                                <span>Start scanning to discover devices</span>
                            </div>
                        </div>
                    </div>
                </div>
                
                <!-- Safe Mode Accordion -->
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title">Safe</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="safe-tabs">
                            <button class="safe-tab active" onclick="switchSafeTab(0)">Password 1</button>
                            <button class="safe-tab" onclick="switchSafeTab(1)">Password 2</button>
                            <button class="safe-tab" onclick="switchSafeTab(2)">Password 3</button>
                            <button class="safe-tab" onclick="switchSafeTab(3)">Password 4</button>
                            <button class="safe-tab" onclick="switchSafeTab(4)">Password 5</button>
                        </div>
                        
                        <!-- Password 1 -->
                        <div class="safe-tab-content active" data-password-index="0">
                            <div class="form-field" style="margin-bottom: 20px;">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="pwd0-enabled" onchange="togglePassword(0, this.checked)">
                                    <span class="toggle-slider"></span>
                                    <span class="toggle-label">Enable Password</span>
                                </label>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">Password Configuration</div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Password</span>
                                        <span class="field-hint">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd0-value" placeholder="e.g., L5-R3-L10-B">
                                    <div class="password-validation" id="pwd0-validation"></div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Old Password (for changes)</span>
                                        <span class="field-hint">Required when modifying existing password</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd0-old" placeholder="Enter old password">
                                </div>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">API Configuration</div>
                                <div class="form-field">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="pwd0-api-enabled" onchange="toggleApiConfig(0, this.checked)">
                                        <span class="toggle-slider"></span>
                                        <span class="toggle-label">Enable API</span>
                                    </label>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">API URL</label>
                                    <input type="text" class="text-input" id="pwd0-api-url" placeholder="https://example.com/api/unlock">
                                </div>
                                <div class="form-field">
                                    <label class="field-label">HTTP Method</label>
                                    <div class="option-group compact">
                                        <label class="option-card active">
                                            <input type="radio" name="pwd0-api-method" value="GET" checked>
                                            <span>GET</span>
                                        </label>
                                        <label class="option-card">
                                            <input type="radio" name="pwd0-api-method" value="POST">
                                            <span>POST</span>
                                        </label>
                                    </div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Custom Header (optional)</span>
                                        <span class="field-hint">Format: HeaderName: Value</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd0-api-header" placeholder="X-API-Key: your-api-key">
                                </div>
                                <div class="form-field" id="pwd0-body-field" style="display: none;">
                                    <label class="field-label">
                                        <span>Request Body (JSON)</span>
                                        <span class="field-hint">Only for POST requests</span>
                                    </label>
                                    <textarea class="text-area" id="pwd0-api-body" rows="3" placeholder='{"password_id": 1, "action": "unlock"}'></textarea>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Password 2 -->
                        <div class="safe-tab-content" data-password-index="1">
                            <div class="form-field" style="margin-bottom: 20px;">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="pwd1-enabled" onchange="togglePassword(1, this.checked)">
                                    <span class="toggle-slider"></span>
                                    <span class="toggle-label">Enable Password</span>
                                </label>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">Password Configuration</div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Password</span>
                                        <span class="field-hint">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd1-value" placeholder="e.g., L5-R3-L10-B">
                                    <div class="password-validation" id="pwd1-validation"></div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Old Password (for changes)</span>
                                        <span class="field-hint">Required when modifying existing password</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd1-old" placeholder="Enter old password">
                                </div>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">API Configuration</div>
                                <div class="form-field">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="pwd1-api-enabled" onchange="toggleApiConfig(1, this.checked)">
                                        <span class="toggle-slider"></span>
                                        <span class="toggle-label">Enable API</span>
                                    </label>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">API URL</label>
                                    <input type="text" class="text-input" id="pwd1-api-url" placeholder="https://example.com/api/unlock">
                                </div>
                                <div class="form-field">
                                    <label class="field-label">HTTP Method</label>
                                    <div class="option-group compact">
                                        <label class="option-card active">
                                            <input type="radio" name="pwd1-api-method" value="GET" checked>
                                            <span>GET</span>
                                        </label>
                                        <label class="option-card">
                                            <input type="radio" name="pwd1-api-method" value="POST">
                                            <span>POST</span>
                                        </label>
                                    </div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Custom Header (optional)</span>
                                        <span class="field-hint">Format: HeaderName: Value</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd1-api-header" placeholder="X-API-Key: your-api-key">
                                </div>
                                <div class="form-field" id="pwd1-body-field" style="display: none;">
                                    <label class="field-label">
                                        <span>Request Body (JSON)</span>
                                        <span class="field-hint">Only for POST requests</span>
                                    </label>
                                    <textarea class="text-area" id="pwd1-api-body" rows="3" placeholder='{"password_id": 2, "action": "unlock"}'></textarea>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Password 3 -->
                        <div class="safe-tab-content" data-password-index="2">
                            <div class="form-field" style="margin-bottom: 20px;">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="pwd2-enabled" onchange="togglePassword(2, this.checked)">
                                    <span class="toggle-slider"></span>
                                    <span class="toggle-label">Enable Password</span>
                                </label>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">Password Configuration</div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Password</span>
                                        <span class="field-hint">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd2-value" placeholder="e.g., L5-R3-L10-B">
                                    <div class="password-validation" id="pwd2-validation"></div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Old Password (for changes)</span>
                                        <span class="field-hint">Required when modifying existing password</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd2-old" placeholder="Enter old password">
                                </div>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">API Configuration</div>
                                <div class="form-field">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="pwd2-api-enabled" onchange="toggleApiConfig(2, this.checked)">
                                        <span class="toggle-slider"></span>
                                        <span class="toggle-label">Enable API</span>
                                    </label>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">API URL</label>
                                    <input type="text" class="text-input" id="pwd2-api-url" placeholder="https://example.com/api/unlock">
                                </div>
                                <div class="form-field">
                                    <label class="field-label">HTTP Method</label>
                                    <div class="option-group compact">
                                        <label class="option-card active">
                                            <input type="radio" name="pwd2-api-method" value="GET" checked>
                                            <span>GET</span>
                                        </label>
                                        <label class="option-card">
                                            <input type="radio" name="pwd2-api-method" value="POST">
                                            <span>POST</span>
                                        </label>
                                    </div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Custom Header (optional)</span>
                                        <span class="field-hint">Format: HeaderName: Value</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd2-api-header" placeholder="X-API-Key: your-api-key">
                                </div>
                                <div class="form-field" id="pwd2-body-field" style="display: none;">
                                    <label class="field-label">
                                        <span>Request Body (JSON)</span>
                                        <span class="field-hint">Only for POST requests</span>
                                    </label>
                                    <textarea class="text-area" id="pwd2-api-body" rows="3" placeholder='{"password_id": 3, "action": "unlock"}'></textarea>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Password 4 -->
                        <div class="safe-tab-content" data-password-index="3">
                            <div class="form-field" style="margin-bottom: 20px;">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="pwd3-enabled" onchange="togglePassword(3, this.checked)">
                                    <span class="toggle-slider"></span>
                                    <span class="toggle-label">Enable Password</span>
                                </label>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">Password Configuration</div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Password</span>
                                        <span class="field-hint">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd3-value" placeholder="e.g., L5-R3-L10-B">
                                    <div class="password-validation" id="pwd3-validation"></div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Old Password (for changes)</span>
                                        <span class="field-hint">Required when modifying existing password</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd3-old" placeholder="Enter old password">
                                </div>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">API Configuration</div>
                                <div class="form-field">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="pwd3-api-enabled" onchange="toggleApiConfig(3, this.checked)">
                                        <span class="toggle-slider"></span>
                                        <span class="toggle-label">Enable API</span>
                                    </label>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">API URL</label>
                                    <input type="text" class="text-input" id="pwd3-api-url" placeholder="https://example.com/api/unlock">
                                </div>
                                <div class="form-field">
                                    <label class="field-label">HTTP Method</label>
                                    <div class="option-group compact">
                                        <label class="option-card active">
                                            <input type="radio" name="pwd3-api-method" value="GET" checked>
                                            <span>GET</span>
                                        </label>
                                        <label class="option-card">
                                            <input type="radio" name="pwd3-api-method" value="POST">
                                            <span>POST</span>
                                        </label>
                                    </div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Custom Header (optional)</span>
                                        <span class="field-hint">Format: HeaderName: Value</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd3-api-header" placeholder="X-API-Key: your-api-key">
                                </div>
                                <div class="form-field" id="pwd3-body-field" style="display: none;">
                                    <label class="field-label">
                                        <span>Request Body (JSON)</span>
                                        <span class="field-hint">Only for POST requests</span>
                                    </label>
                                    <textarea class="text-area" id="pwd3-api-body" rows="3" placeholder='{"password_id": 4, "action": "unlock"}'></textarea>
                                </div>
                            </div>
                        </div>
                        
                        <!-- Password 5 -->
                        <div class="safe-tab-content" data-password-index="4">
                            <div class="form-field" style="margin-bottom: 20px;">
                                <label class="toggle-switch">
                                    <input type="checkbox" id="pwd4-enabled" onchange="togglePassword(4, this.checked)">
                                    <span class="toggle-slider"></span>
                                    <span class="toggle-label">Enable Password</span>
                                </label>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">Password Configuration</div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Password</span>
                                        <span class="field-hint">Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd4-value" placeholder="e.g., L5-R3-L10-B">
                                    <div class="password-validation" id="pwd4-validation"></div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Old Password (for changes)</span>
                                        <span class="field-hint">Required when modifying existing password</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd4-old" placeholder="Enter old password">
                                </div>
                            </div>
                            <div class="safe-section">
                                <div class="section-header">API Configuration</div>
                                <div class="form-field">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="pwd4-api-enabled" onchange="toggleApiConfig(4, this.checked)">
                                        <span class="toggle-slider"></span>
                                        <span class="toggle-label">Enable API</span>
                                    </label>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">API URL</label>
                                    <input type="text" class="text-input" id="pwd4-api-url" placeholder="https://example.com/api/unlock">
                                </div>
                                <div class="form-field">
                                    <label class="field-label">HTTP Method</label>
                                    <div class="option-group compact">
                                        <label class="option-card active">
                                            <input type="radio" name="pwd4-api-method" value="GET" checked>
                                            <span>GET</span>
                                        </label>
                                        <label class="option-card">
                                            <input type="radio" name="pwd4-api-method" value="POST">
                                            <span>POST</span>
                                        </label>
                                    </div>
                                </div>
                                <div class="form-field">
                                    <label class="field-label">
                                        <span>Custom Header (optional)</span>
                                        <span class="field-hint">Format: HeaderName: Value</span>
                                    </label>
                                    <input type="text" class="text-input" id="pwd4-api-header" placeholder="X-API-Key: your-api-key">
                                </div>
                                <div class="form-field" id="pwd4-body-field" style="display: none;">
                                    <label class="field-label">
                                        <span>Request Body (JSON)</span>
                                        <span class="field-hint">Only for POST requests</span>
                                    </label>
                                    <textarea class="text-area" id="pwd4-api-body" rows="3" placeholder='{"password_id": 5, "action": "unlock"}'></textarea>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
                
                <!-- Panic Mode Accordion -->
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title">Panic</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <p class="panel-text">Panik butonu acil durumlarda sevdiklerinize haber vermek veya yardım istemek için tasarlanmaktadır. Encoder'a dokunduğunuzda veya çevirdiğinizde (yön ve miktar fark etmeksizin), API üzerinden mobil uygulamaya sinyal gönderilerek telefonun zil sesi tetiklenir ve mesaj metni iletilir. Sistem şu anda geliştirilme aşamasındadır.</p>
                    </div>
                </div>
                
                <!-- Save Button -->
                <div class="network-save">
                    <button class="btn-save" onclick="saveModeConfig()">Save Mode Configuration</button>
                </div>
            </section>
            
            <!-- Tab: Network -->
            <section id="network" class="tab-section">
                <div class="network-status">
                    <div class="status-item">
                        <span class="status-label">Mode</span>
                        <span class="status-value">WiFi</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">SSID</span>
                        <span class="status-value">MyNetwork</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label">IP Address</span>
                        <span class="status-value">192.168.1.100</span>
                    </div>
                </div>
                
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title">AP MODU AYARLARI</span>
                        <span class="accordion-status inactive" id="ap-status">Inactive</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="info-row">
                            <span class="info-label">SSID</span>
                            <span class="info-value" id="ap-ssid-display">SynDimm-XXXXXX</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label">Password</span>
                            <span class="info-value">None (Open Network)</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label">IP Address</span>
                            <span class="info-value">192.168.4.1</span>
                        </div>
                        <div class="info-row" style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #333;">
                            <span class="info-label" style="font-size: 0.85rem; opacity: 0.7; line-height: 1.5;">
                                <strong>Otomatik Failover:</strong><br>
                                Primary WiFi → Backup WiFi → AP Mode<br>
                                <br>
                                AP Mode otomatik olarak devreye girer. Şifresiz açık ağ olarak çalışır.
                            </span>
                        </div>
                    </div>
                </div>
                
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title">PRIMARY WIFI</span>
                        <span class="accordion-status connected">Connected</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="form-group">
                            <label for="wifi1-ssid">SSID</label>
                            <input type="text" id="wifi1-ssid" placeholder="Enter WiFi SSID">
                        </div>
                        <div class="form-group">
                            <label for="wifi1-password">Password</label>
                            <input type="password" id="wifi1-password" placeholder="Enter WiFi password">
                        </div>
                        <div class="form-group">
                            <label for="wifi1-ip">Static IP (optional)</label>
                            <input type="text" id="wifi1-ip" placeholder="Leave empty for DHCP">
                        </div>
                    </div>
                </div>
                
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title">BACKUP WIFI</span>
                        <span class="accordion-status not-configured">Not Configured</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="form-group">
                            <label for="wifi2-ssid">SSID</label>
                            <input type="text" id="wifi2-ssid" placeholder="Enter WiFi SSID">
                        </div>
                        <div class="form-group">
                            <label for="wifi2-password">Password</label>
                            <input type="password" id="wifi2-password" placeholder="Enter WiFi password">
                        </div>
                        <div class="form-group">
                            <label for="wifi2-ip">Static IP (optional)</label>
                            <input type="text" id="wifi2-ip" placeholder="Leave empty for DHCP">
                        </div>
                    </div>
                </div>
                
                <div style="text-align: center; margin-top: 30px;">
                    <button class="btn-save" onclick="saveNetworkConfig()">Save Network Configuration</button>
                </div>
            </section>
            
            <!-- Tab: Info -->
            <section id="info" class="tab-section">
                <div class="info-guide">
                    <h3>Kullanım Kılavuzu</h3>
                    
                    <h4>Dimmer Modu</h4>
                    <p><strong>Encoder ile Kontrol:</strong> Saat yönünde çevirerek parlaklığı artırın, ters yönde azaltın. Encoder butonuna basarak bağlı cihazı açıp/kapatabilirsiniz.</p>
                    <p><strong>Cihaz Bağlantısı:</strong> Settings → Dimmer bölümünden Scan Network ile ağınızdaki akıllı dimmer cihazlarını bulun ve Connect butonuyla senkronize olun.</p>
                    <p><strong>Web Kontrolü:</strong> Control sekmesinden Brightness (parlaklık) ve Dimm Ratio (her encoder hareketi için değişim miktarı 1-5 arası) ayarlarını yapabilirsiniz.</p>
                    
                    <h4>Safe Lock Modu</h4>
                    <p><strong>Şifre Sistemi:</strong> Encoder hareketleriyle şifre girilir. Örnek: R5-L3-R2-B (sağa 5, sola 3, sağa 2, buton bas).</p>
                    <p><strong>Ayarlama:</strong> Settings → Safe sekmesinden 5 farklı şifre kaydedebilirsiniz. Her şifre için API URL tanımlayın.</p>
                    <p><strong>Tetikleme:</strong> Doğru şifre girildiğinde, o şifreye kayıtlı API otomatik olarak çağrılır (HTTP GET/POST).</p>
                    <p><strong>Kullanım Alanları:</strong> Akıllı kilit açma, garaj kapısı kontrolü, özel otomasyon senaryoları.</p>
                    
                    <h4>Panik Modu</h4>
                    <p><strong>Acil Bildirim:</strong> Encoder'a dokunduğunuzda veya çevirdiğinizde (yön/miktar önemsiz) API üzerinden mobil uygulamaya sinyal gönderilir.</p>
                    <p><strong>İşlev:</strong> Hedef telefonda zil sesi tetiklenir ve acil durum mesajı iletilir.</p>
                    <p class="warning-text">Panik modu geliştirme aşamasındadır.</p>
                    
                    <h4>Ağ Ayarları</h4>
                    <p><strong>WiFi Bağlantısı:</strong> Network sekmesinden SSID ve şifre girerek WiFi ağına bağlanın.</p>
                    <p><strong>IP Yapılandırması:</strong> DHCP (otomatik) veya Statik IP seçeneklerini kullanabilirsiniz.</p>
                </div>
                <div class="info-documentation">
                    <h3 class="doc-title">Destek ve Dokümantasyon</h3>
                    <p class="doc-description">Detaylı kullanım kılavuzu, örnek senaryolar ve güncellemeler için:</p>
                    <a href="https://smartkraft.ch/syndimm" target="_blank" class="doc-button">SmartKraft.ch/SynDimm</a>
                </div>
            </section>
            
        </div>
    </main>
    
    <script src="script.js"></script>
</body>
</html>
)=====";

#endif
