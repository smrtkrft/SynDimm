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
                    <h1 data-i18n="header.title">SmartKraft SynDimm</h1>
                    <div class="brand-info">
                        <span class="chip-id"><span data-i18n="header.chipId">Chip ID</span>: <span id="chip-id-value">Loading...</span></span>
                        <span class="version-info"><span data-i18n="header.version">Version</span>: v1.0.0</span>
                    </div>
                </div>
            </div>
        </div>
    </header>
    
    <!-- Navigation -->
    <nav class="nav">
        <div class="container">
            <div class="nav-links">
                <a href="#" class="nav-link active" data-tab="control" data-i18n="nav.control">Hizli kontrol</a>
                <a href="#" class="nav-link" data-tab="settings" data-i18n="nav.settings">Modlar</a>
                <a href="#" class="nav-link" data-tab="network" data-i18n="nav.network">Baglanti</a>
                <a href="#" class="nav-link" data-tab="info" data-i18n="nav.info">Info</a>
            </div>
        </div>
    </nav>
    
    <!-- Main Content -->
    <main class="main">
        <div class="container">
            
            <!-- Tab: Control -->
            <section id="control" class="tab-section active">
                <div class="panel">
                    <div class="section-header" data-i18n="control.selectMode">Aktif Modunuzu Secin</div>
                    <div class="mode-grid">
                        <label class="mode-card active">
                            <input type="radio" name="mode" value="dimmer" checked>
                            <div class="mode-content">
                                <div class="mode-name" data-i18n="control.mode.dimmer.name">Dimmer</div>
                                <div class="mode-desc" data-i18n="control.mode.dimmer.desc">Control brightness levels</div>
                            </div>
                            <div class="mode-check"></div>
                        </label>
                        <label class="mode-card">
                            <input type="radio" name="mode" value="shutter">
                            <div class="mode-content">
                                <div class="mode-name" data-i18n="control.mode.shutter.name">Shutter</div>
                                <div class="mode-desc" data-i18n="control.mode.shutter.desc">Control shutter position</div>
                            </div>
                            <div class="mode-check"></div>
                        </label>
                        <label class="mode-card">
                            <input type="radio" name="mode" value="safe">
                            <div class="mode-content">
                                <div class="mode-name" data-i18n="control.mode.safe.name">Safe</div>
                                <div class="mode-desc" data-i18n="control.mode.safe.desc">Counter display mode</div>
                            </div>
                            <div class="mode-check"></div>
                        </label>
                    </div>
                    
                    <div class="section-divider"></div>
                    
                    <div class="preferences-row">
                        <div class="pref-section">
                            <label class="pref-label" data-i18n="control.theme.label">Theme</label>
                            <div class="option-group compact">
                                <label class="option-card active">
                                    <input type="radio" name="theme" value="dark" checked>
                                    <span data-i18n="control.theme.dark">Dark</span>
                                </label>
                                <label class="option-card">
                                    <input type="radio" name="theme" value="light">
                                    <span data-i18n="control.theme.light">Light</span>
                                </label>
                            </div>
                        </div>
                        <div class="pref-divider"></div>
                        <div class="pref-section">
                            <label class="pref-label" data-i18n="control.language.label">Language</label>
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
                        <span class="accordion-title" data-i18n="settings.dimmer.title">Dimmer</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="dimmer-status">
                            <div class="status-item">
                                <span class="status-label" data-i18n="settings.dimmer.connection">Connection</span>
                                <span class="status-value" id="dimmer-connection" data-i18n="settings.dimmer.connected">Connected</span>
                            </div>
                            <div class="status-item">
                                <span class="status-label" data-i18n="settings.dimmer.device">Device</span>
                                <span class="status-value" id="dimmer-device">Shelly (192.168.11.11)</span>
                            </div>
                        </div>
                        <div class="slider-controls">
                            <div class="slider-group">
                                <span class="slider-value" id="brightness-value">50</span>
                                <label class="slider-label" data-i18n="settings.dimmer.brightness">Brightness</label>
                                <input type="range" class="range-slider" min="0" max="100" value="50" id="brightness" disabled oninput="updateSliderValue(this)">
                            </div>
                            <div class="slider-group">
                                <span class="slider-value" id="ratio-value">3</span>
                                <label class="slider-label" data-i18n="settings.dimmer.dimmRatio">Dimm Ratio</label>
                                <input type="range" class="range-slider" min="1" max="5" value="3" id="ratio" oninput="updateSliderValue(this)">
                            </div>
                        </div>
                        <div class="manual-connect">
                            <button class="btn btn-manual" onclick="showManualIPDialog()" data-i18n="settings.dimmer.manualIP">Manual IP</button>
                        </div>
                    </div>
                </div>
                
                <!-- Shutter Mode Accordion -->
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title" data-i18n="settings.shutter.title">Shutter</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="shutter-status">
                            <div class="status-item">
                                <span class="status-label" data-i18n="settings.shutter.position">Position</span>
                                <span class="status-value" id="shutter-position">50%</span>
                            </div>
                            <div class="status-item">
                                <span class="status-label" data-i18n="settings.shutter.status">Status</span>
                                <span class="status-value" id="shutter-status" data-i18n="settings.shutter.stopped">Stopped</span>
                            </div>
                        </div>
                        <div class="slider-controls">
                            <div class="slider-group">
                                <span class="slider-value" id="shutter-position-value">50</span>
                                <label class="slider-label" data-i18n="settings.shutter.setPosition">Set Position</label>
                                <input type="range" class="range-slider" min="0" max="100" value="50" id="shutter-slider" oninput="updateShutterSlider(this)">
                            </div>
                        </div>
                        <div class="button-group">
                            <button class="btn btn-primary" onclick="shutterOpen()" data-i18n="settings.shutter.open">Open</button>
                            <button class="btn btn-secondary" onclick="shutterStop()" data-i18n="settings.shutter.stop">Stop</button>
                            <button class="btn btn-primary" onclick="shutterClose()" data-i18n="settings.shutter.close">Close</button>
                        </div>
                        <div class="info-text">
                            <p data-i18n="settings.shutter.info">Use encoder to control shutter position. Rotate left to close, right to open.</p>
                        </div>
                    </div>
                </div>
                
                <!-- Safe Mode Accordion -->
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title" data-i18n="settings.safe.title">Safe</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="safe-tabs">
                            <button class="safe-tab active" onclick="switchSafeTab(0)"><span data-i18n="settings.safe.password">Password</span> 1</button>
                            <button class="safe-tab" onclick="switchSafeTab(1)"><span data-i18n="settings.safe.password">Password</span> 2</button>
                            <button class="safe-tab" onclick="switchSafeTab(2)"><span data-i18n="settings.safe.password">Password</span> 3</button>
                            <button class="safe-tab" onclick="switchSafeTab(3)"><span data-i18n="settings.safe.password">Password</span> 4</button>
                            <button class="safe-tab" onclick="switchSafeTab(4)"><span data-i18n="settings.safe.password">Password</span> 5</button>
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
                
                <!-- Alarm Mode Accordion -->
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title" data-i18n="settings.alarm.title">Alarm</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <p class="panel-text" data-i18n="settings.alarm.description">Alarm butonu acil durumlarda sevdiklerinize haber vermek veya yardım istemek için tasarlanmaktadır. Encoder'a dokunduğunuzda veya çevirdiğinizde (yön ve miktar fark etmeksizin), API üzerinden mobil uygulamaya sinyal gönderilerek telefonun zil sesi tetiklenir ve mesaj metni iletilir. Sistem şu anda geliştirilme aşamasındadır.</p>
                    </div>
                </div>
                
                <!-- Save Button -->
                <div class="network-save">
                    <button class="btn-save" onclick="saveModeConfig()" data-i18n="settings.saveConfig">Save Mode Configuration</button>
                </div>
            </section>
            
            <!-- Tab: Network -->
            <section id="network" class="tab-section">
                <div class="network-status">
                    <div class="status-item">
                        <span class="status-label" data-i18n="network.status.mode">MOD</span>
                        <span class="status-value" data-i18n="network.status.wifi">WiFi</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label" data-i18n="network.status.ssid">SSID</span>
                        <span class="status-value">MyNetwork</span>
                    </div>
                    <div class="status-item">
                        <span class="status-label" data-i18n="network.status.ip">IP Adresi</span>
                        <div class="status-value-stacked">
                            <span class="ip-value">192.168.1.100</span>
                            <span class="domain-value" id="status-mdns">syndimm-xxxxxx.local</span>
                        </div>
                    </div>
                </div>
                
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title" data-i18n="network.ap.title">AP MODU AYARLARI</span>
                        <span class="accordion-status inactive" id="ap-status" data-i18n="network.ap.inactive">Inactive</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="info-row">
                            <span class="info-label" data-i18n="network.ap.ssid">SSID</span>
                            <span class="info-value" id="ap-ssid-display">SynDimm-XXXXXX</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label" data-i18n="network.ap.password">Password</span>
                            <span class="info-value" data-i18n="network.ap.noPassword">None (Open Network)</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label" data-i18n="network.ap.ip">IP Address</span>
                            <span class="info-value">192.168.4.1</span>
                        </div>
                        <div class="info-row">
                            <span class="info-label" data-i18n="network.ap.mdns">mDNS Address</span>
                            <span class="info-value" id="ap-mdns-display">syndimm-xxxxxx.local</span>
                        </div>
                        <div class="info-row" style="margin-top: 15px; padding-top: 15px; border-top: 1px solid #333;">
                            <span class="info-label" style="font-size: 0.85rem; opacity: 0.7; line-height: 1.5;">
                                <strong data-i18n="network.ap.autoFailover">Otomatik Failover:</strong><br>
                                <span data-i18n="network.ap.failoverDesc">Primary WiFi → Backup WiFi → AP Mode<br><br>AP Mode otomatik olarak devreye girer. Şifresiz açık ağ olarak çalışır.</span>
                            </span>
                        </div>
                    </div>
                </div>
                
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title" data-i18n="network.wifi1.title">PRIMARY WIFI</span>
                        <span class="accordion-status connected" data-i18n="network.wifi1.connected">Connected</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="form-group">
                            <label for="wifi1-ssid" data-i18n="network.wifi1.ssid">SSID</label>
                            <input type="text" id="wifi1-ssid" placeholder="Enter WiFi SSID" data-i18n-placeholder="network.wifi1.ssidPlaceholder">
                        </div>
                        <div class="form-group">
                            <label for="wifi1-password" data-i18n="network.wifi1.password">Password</label>
                            <input type="password" id="wifi1-password" placeholder="Enter WiFi password" data-i18n-placeholder="network.wifi1.passwordPlaceholder">
                        </div>
                        <div class="form-group">
                            <label for="wifi1-ip" data-i18n="network.wifi1.staticIp">Static IP (optional)</label>
                            <input type="text" id="wifi1-ip" placeholder="Leave empty for DHCP" data-i18n-placeholder="network.wifi1.staticIpPlaceholder">
                        </div>
                        <div class="form-group">
                            <label for="wifi1-local" data-i18n="network.wifi1.localDomain">.local Domain (optional)</label>
                            <input type="text" id="wifi1-local" placeholder="e.g., mysyndimm" data-i18n-placeholder="network.wifi1.localDomainPlaceholder">
                            <small class="form-hint">Will be accessible as [name].local on the network</small>
                        </div>
                    </div>
                </div>
                
                <div class="accordion-panel">
                    <div class="accordion-header" onclick="toggleAccordion(this)">
                        <span class="accordion-title" data-i18n="network.wifi2.title">BACKUP WIFI</span>
                        <span class="accordion-status not-configured" data-i18n="network.wifi2.notConfigured">Not Configured</span>
                        <span class="accordion-arrow">▼</span>
                    </div>
                    <div class="accordion-content collapsed">
                        <div class="form-group">
                            <label for="wifi2-ssid" data-i18n="network.wifi2.ssid">SSID</label>
                            <input type="text" id="wifi2-ssid" placeholder="Enter WiFi SSID" data-i18n-placeholder="network.wifi2.ssidPlaceholder">
                        </div>
                        <div class="form-group">
                            <label for="wifi2-password" data-i18n="network.wifi2.password">Password</label>
                            <input type="password" id="wifi2-password" placeholder="Enter WiFi password" data-i18n-placeholder="network.wifi2.passwordPlaceholder">
                        </div>
                        <div class="form-group">
                            <label for="wifi2-ip" data-i18n="network.wifi2.staticIp">Static IP (optional)</label>
                            <input type="text" id="wifi2-ip" placeholder="Leave empty for DHCP" data-i18n-placeholder="network.wifi2.staticIpPlaceholder">
                        </div>
                        <div class="form-group">
                            <label for="wifi2-local" data-i18n="network.wifi2.localDomain">.local Domain (optional)</label>
                            <input type="text" id="wifi2-local" placeholder="e.g., mysyndimm" data-i18n-placeholder="network.wifi2.localDomainPlaceholder">
                            <small class="form-hint">Will be accessible as [name].local on the network</small>
                        </div>
                    </div>
                </div>
                
                <div style="text-align: center; margin-top: 30px;">
                    <button class="btn-save" onclick="saveNetworkConfig()" data-i18n="network.saveConfig">Save Network Configuration</button>
                </div>
            </section>
            
            <!-- Tab: Info -->
            <section id="info" class="tab-section">
                <div class="info-guide">
                    <h3 data-i18n="info.userGuide">Kullanım Kılavuzu</h3>
                    
                    <h4 data-i18n="info.dimmerMode.title">Dimmer Modu</h4>
                    <p><strong data-i18n="info.dimmerMode.encoderControl.title">Encoder ile Kontrol:</strong> <span data-i18n="info.dimmerMode.encoderControl.text">Saat yönünde çevirerek parlaklığı artırın, ters yönde azaltın. Encoder butonuna basarak bağlı cihazı açıp/kapatabilirsiniz.</span></p>
                    <p><strong data-i18n="info.dimmerMode.deviceConnection.title">Cihaz Bağlantısı:</strong> <span data-i18n="info.dimmerMode.deviceConnection.text">Settings → Dimmer bölümünden Scan Network ile ağınızdaki akıllı dimmer cihazlarını bulun ve Connect butonuyla senkronize olun.</span></p>
                    <p><strong data-i18n="info.dimmerMode.webControl.title">Web Kontrolü:</strong> <span data-i18n="info.dimmerMode.webControl.text">Control sekmesinden Brightness (parlaklık) ve Dimm Ratio (her encoder hareketi için değişim miktarı 1-5 arası) ayarlarını yapabilirsiniz.</span></p>
                    
                    <h4 data-i18n="info.safeMode.title">Safe Lock Modu</h4>
                    <p><strong data-i18n="info.safeMode.passwordSystem.title">Şifre Sistemi:</strong> <span data-i18n="info.safeMode.passwordSystem.text">Encoder hareketleriyle şifre girilir. Örnek: R5-L3-R2-B (sağa 5, sola 3, sağa 2, buton bas).</span></p>
                    <p><strong data-i18n="info.safeMode.configuration.title">Ayarlama:</strong> <span data-i18n="info.safeMode.configuration.text">Settings → Safe sekmesinden 5 farklı şifre kaydedebilirsiniz. Her şifre için API URL tanımlayın.</span></p>
                    <p><strong data-i18n="info.safeMode.triggering.title">Tetikleme:</strong> <span data-i18n="info.safeMode.triggering.text">Doğru şifre girildiğinde, o şifreye kayıtlı API otomatik olarak çağrılır (HTTP GET/POST).</span></p>
                    <p><strong data-i18n="info.safeMode.useCases.title">Kullanım Alanları:</strong> <span data-i18n="info.safeMode.useCases.text">Akıllı kilit açma, garaj kapısı kontrolü, özel otomasyon senaryoları.</span></p>
                    
                    <h4 data-i18n="info.alarmMode.title">Alarm Modu</h4>
                    <p><strong data-i18n="info.alarmMode.emergencyNotification.title">Acil Bildirim:</strong> <span data-i18n="info.alarmMode.emergencyNotification.text">Encoder'a dokunduğunuzda veya çevirdiğinizde (yön/miktar önemsiz) API üzerinden mobil uygulamaya sinyal gönderilir.</span></p>
                    <p><strong data-i18n="info.alarmMode.function.title">İşlev:</strong> <span data-i18n="info.alarmMode.function.text">Hedef telefonda zil sesi tetiklenir ve acil durum mesajı iletilir.</span></p>
                    <p class="warning-text" data-i18n="info.alarmMode.warning">Alarm modu geliştirme aşamasındadır.</p>
                    
                    <h4 data-i18n="info.networkSettings.title">Ağ Ayarları</h4>
                    <p><strong data-i18n="info.networkSettings.wifiConnection.title">WiFi Bağlantısı:</strong> <span data-i18n="info.networkSettings.wifiConnection.text">Network sekmesinden SSID ve şifre girerek WiFi ağına bağlanın.</span></p>
                    <p><strong data-i18n="info.networkSettings.ipConfiguration.title">IP Yapılandırması:</strong> <span data-i18n="info.networkSettings.ipConfiguration.text">DHCP (otomatik) veya Statik IP seçeneklerini kullanabilirsiniz.</span></p>
                    
                    <h4 data-i18n="info.otaSettings.title">Versiyon Güncelleme</h4>
                    <div class="ota-info-section">
                        <div class="version-display">
                            <p><strong data-i18n="info.otaSettings.currentVersion">Cihaz Sürümü:</strong> <span id="ota-current-version">-</span></p>
                            <p id="ota-latest-container" style="display:none;"><strong data-i18n="info.otaSettings.latestVersion">Yeni Sürüm:</strong> <span id="ota-latest-version">-</span></p>
                        </div>
                        <div class="ota-controls">
                            <label class="toggle-switch">
                                <input type="checkbox" id="ota-auto-update">
                                <span class="toggle-slider"></span>
                                <span class="toggle-label" data-i18n="info.otaSettings.autoUpdate">Otomatik Güncelleme</span>
                            </label>
                            <button id="ota-update-button" class="ota-button" style="display:none;" data-i18n="info.otaSettings.updateNow">Şimdi Güncelle</button>
                        </div>
                        <p class="ota-description" data-i18n="info.otaSettings.description">Otomatik güncelleme açık olduğunda cihaz yeni sürümleri kontrol eder ve kendini günceller. Kapalı olduğunda sadece bildirim alırsınız.</p>
                    </div>
                </div>
                <div class="info-documentation">
                    <h3 class="doc-title" data-i18n="info.documentation.title">Destek ve Dokümantasyon</h3>
                    <p class="doc-description" data-i18n="info.documentation.description">Detaylı kullanım kılavuzu, örnek senaryolar ve güncellemeler için:</p>
                    <div class="doc-buttons">
                        <a href="https://smartkraft.ch/syndimm" target="_blank" class="doc-button">SmartKraft.ch</a>
                        <a href="https://github.com/smrtkrft/SynDimm" target="_blank" class="doc-button">GitHub</a>
                    </div>
                </div>
            </section>
            
        </div>
    </main>
    
    <!-- Manual IP Modal -->
    <div id="manualIPModal" class="modal" onclick="modalBackdropClick(event)">
        <div class="modal-content">
            <div class="modal-header">
                <h3 data-i18n="settings.dimmer.manualIPTitle">Manuel IP</h3>
                <span class="modal-close" onclick="closeManualIPModal()">&times;</span>
            </div>
            <div class="modal-body">
                <label for="manualIP" data-i18n="settings.dimmer.enterIP">IP Adresi:</label>
                <input type="text" id="manualIP" class="modal-input" placeholder="192.168.1.100" pattern="\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}\\.\\d{1,3}" autocomplete="off">
                <div class="modal-error" id="manualIPError"></div>
            </div>
            <div class="modal-footer">
                <button class="btn btn-cancel" onclick="closeManualIPModal()" data-i18n="common.cancel">Iptal</button>
                <button class="btn btn-connect" onclick="connectManualIP()" data-i18n="common.connect">Baglan</button>
            </div>
        </div>
    </div>
    
    <script src="script.js"></script>
</body>
</html>
)=====";

#endif
