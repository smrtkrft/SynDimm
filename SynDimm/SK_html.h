/**
 * SK_html.h
 * SmartKraft SynDimm - HTML Structure
 * Version: v1.1.0
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

// HTML Part 1 - Before ChipID placeholder
const char SK_HTML_PART1[] PROGMEM = R"rawliteral(<!DOCTYPE html>
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

// HTML Part 2 - Between ChipID and Version
const char SK_HTML_PART2[] PROGMEM = R"rawliteral(</span>
                    <span class="info-separator">-</span>
                    <span class="info-label">Version:</span>
                    <span class="info-value">)rawliteral";

// HTML Part 3 - After Version (rest of HTML)
const char SK_HTML_PART3[] PROGMEM = R"rawliteral(</span>
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
                
                <!-- Encoder Mod Değiştirme Toggle -->
                <div class="mode-toggle-row">
                    <div class="toggle-info">
                        <span class="toggle-label" id="encoder-mode-label" data-lang="quick.encoder_mode_change">Encoder Mode Change</span>
                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.encoder_mode_change">When enabled, you can change modes by holding the encoder button and rotating. When disabled, mode changes are only possible via web interface.</span></span>
                    </div>
                    <label class="toggle-switch">
                        <input type="checkbox" id="encoder-mode-toggle" onchange="toggleEncoderModeChange(this.checked)">
                        <span class="toggle-slider"></span>
                    </label>
                </div>
                
                <!-- Mod Seçimi Butonları -->
                <div class="mode-buttons">
                    <button class="mode-btn" id="mode-btn-dimmer" onclick="selectMode('DIMMER')">
                        <span class="mode-btn-text">DIMMER</span>
                        <span class="info-tooltip-i mode-info">i<span class="tooltip-content" data-lang="tooltip.dimmer_mode_info">Control lighting brightness by rotating encoder, toggle on/off by pressing.</span></span>
                    </button>
                    <button class="mode-btn" id="mode-btn-shutter" onclick="selectMode('SHUTTER')">
                        <span class="mode-btn-text">SHUTTER</span>
                        <span class="info-tooltip-i mode-info">i<span class="tooltip-content" data-lang="tooltip.shutter_mode_info">Controls roller shutter position with encoder.</span></span>
                    </button>
                    <button class="mode-btn" id="mode-btn-safe" onclick="selectMode('SAFE')">
                        <span class="mode-btn-text">SAFE</span>
                        <span class="info-tooltip-i mode-info">i<span class="tooltip-content" data-lang="tooltip.safe_mode_info">Trigger API calls with password sequences.</span></span>
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
                            <!-- Hero Display -->
                            <div class="dimmer-hero" id="dimmer-hero">
                                <div class="dimmer-brightness-display" id="dimmer-brightness-display">--<span>%</span></div>
                                <div class="dimmer-status-row">
                                    <span class="dimmer-power-status" id="dimmer-power-status" data-lang="common.off">Off</span>
                                    <span class="dimmer-status-dot">•</span>
                                    <span class="dimmer-connection-status" id="dimmer-connection-status" data-lang="dimmer.not_connected">Not Connected</span>
                                </div>
                                <div class="dimmer-ip-display" id="dimmer-ip-display" data-lang="dimmer.no_device_connected">No device connected</div>
                                <div class="dimmer-calibration-controls">
                                    <button class="dimmer-cal-btn" id="cal-btn-minus" onclick="adjustCalibration(-1)" disabled>−</button>
                                    <div class="dimmer-cal-center">
                                        <div class="dimmer-cal-value" id="dimmer-cal-value">-</div>
                                        <div class="dimmer-cal-label">
                                            <span data-lang="dimmer.sensitivity">Sensitivity</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.sensitivity_info">Sets how much brightness changes per encoder tick. 1 = most precise, 5 = fastest.</span></span>
                                        </div>
                                    </div>
                                    <button class="dimmer-cal-btn" id="cal-btn-plus" onclick="adjustCalibration(1)" disabled>+</button>
                                </div>
                                
                                <!-- Default Brightness Section -->
                                <div class="default-brightness-section">
                                    <div class="default-brightness-toggle-row">
                                        <label class="toggle-label">
                                            <span data-lang="dimmer.default_brightness">Default Brightness</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.default_brightness">When enabled, the dimmer will start at this brightness level when turned on.</span></span>
                                        </label>
                                        <label class="toggle-switch">
                                            <input type="checkbox" id="default-brightness-toggle" onchange="toggleDefaultBrightness(this.checked)">
                                            <span class="toggle-slider"></span>
                                        </label>
                                    </div>
                                    <div class="default-brightness-slider-container" id="default-brightness-slider-container" style="display: none;">
                                        <input type="range" id="default-brightness-slider" class="default-brightness-slider" min="10" max="100" value="50" oninput="updateDefaultBrightnessPreview(this.value)" onchange="setDefaultBrightnessValue(this.value)">
                                        <span class="default-brightness-value" id="default-brightness-value">50%</span>
                                    </div>
                                </div>
                            </div>
                            
                            <!-- Connection Section -->
                            <div class="dimmer-compact-form">
                                <div class="section-title">
                                    <span data-lang="dimmer.device_connection">Device Connection</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.dimmer_device_connection">Search and connect to dimmer devices on your local network.</span></span>
                                </div>
                                <div class="dimmer-inline-form">
                                    <input type="text" id="quick-dimmer-ip-input" data-lang-placeholder="dimmer.ip_placeholder" placeholder="IP Address">
                                    <button class="btn btn-primary" onclick="connectDimmerFromQuick()" data-lang="common.connect">Connect</button>
                                    <button class="btn btn-secondary" onclick="startNetworkScan()">
                                        <span data-lang="dimmer.scan">Scan</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.scan_network">Scan local network for compatible smart devices.</span></span>
                                    </button>
                                </div>
                                <div class="saved-devices-list" id="quick-saved-devices-list">
                                    <div class="saved-device-empty" data-lang="dimmer.no_devices">No saved devices yet.</div>
                                </div>
                            </div>
                            
                            <!-- Kullanım Talimatı -->
                            <div class="mode-usage-guide">
                                <span class="usage-guide-text" data-lang="dimmer.usage_guide">Usage: Rotate encoder to adjust brightness. Press button to toggle light on/off.</span>
                                <a href="https://github.com/smrtkrft/SynDimm" target="_blank" class="usage-guide-link" data-lang="common.detailed_usage">Detailed usage: github.com/smrtkrft/SynDimm</a>
                            </div>
                        </div>
                        
                        <!-- SHUTTER Yapılandırması -->
                        <div class="mode-config-panel" id="config-panel-shutter" style="display: none;">
                            <!-- Hero Display (Dimmer ile aynı tasarım) -->
                            <div class="shutter-hero disconnected" id="shutter-hero">
                                <div class="shutter-position-display" id="shutter-position-display">--<span>%</span></div>
                                <div class="shutter-status-row">
                                    <span class="shutter-movement-status" id="shutter-movement-status" data-lang="shutter.stopped">Stopped</span>
                                    <span class="shutter-status-dot">•</span>
                                    <span class="shutter-connection-status" id="shutter-connection-status" data-lang="shutter.not_connected">Not Connected</span>
                                </div>
                                <div class="shutter-ip-display" id="shutter-ip-display" data-lang="shutter.no_device_connected">No device connected</div>
                                <div class="shutter-calibration-controls">
                                    <button class="shutter-cal-btn" id="shutter-cal-btn-minus" onclick="adjustShutterStep(-1)" disabled>−</button>
                                    <div class="shutter-cal-center">
                                        <div class="shutter-cal-value" id="shutter-cal-value">-</div>
                                        <div class="shutter-cal-label">
                                            <span data-lang="shutter.sensitivity">Sensitivity</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.sensitivity_info_shutter">Sets how much position changes per encoder tick. 1 = most precise, 5 = fastest.</span></span>
                                        </div>
                                    </div>
                                    <button class="shutter-cal-btn" id="shutter-cal-btn-plus" onclick="adjustShutterStep(1)" disabled>+</button>
                                </div>
                            </div>
                            
                            <!-- Connection Section -->
                            <div class="shutter-compact-form">
                                <div class="section-title">
                                    <span data-lang="shutter.device_connection">Device Connection</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.shutter_device_connection">Search and connect to shutter devices on your local network.</span></span>
                                </div>
                                <div class="shutter-inline-form">
                                    <input type="text" id="quick-shutter-ip-input" data-lang-placeholder="shutter.ip_placeholder" placeholder="IP Address">
                                    <button class="btn btn-primary" onclick="connectShutterFromQuick()" data-lang="common.connect">Connect</button>
                                    <button class="btn btn-secondary" onclick="startShutterNetworkScan()">
                                        <span data-lang="shutter.scan">Scan</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.scan_network">Scan local network for compatible smart devices.</span></span>
                                    </button>
                                </div>
                                <div class="saved-devices-list" id="shutter-saved-devices-list">
                                    <div class="saved-device-empty" data-lang="shutter.no_devices">No saved devices yet.</div>
                                </div>
                            </div>
                            
                            <div class="shutter-warning">
                                <span class="warning-icon">⚠</span> <span data-lang="shutter.info_text">Shutter is controlled by encoder.</span>
                            </div>
                            
                            <!-- Kullanım Talimatı -->
                            <div class="mode-usage-guide">
                                <span class="usage-guide-text" data-lang="shutter.usage_guide">Usage: Rotate encoder right to close shutter, left to open. Button stops movement or performs full open/close.</span>
                                <a href="https://github.com/smrtkrft/SynDimm" target="_blank" class="usage-guide-link" data-lang="common.detailed_usage">Detailed usage: github.com/smrtkrft/SynDimm</a>
                            </div>
                        </div>
                        
                        <!-- SAFE Yapılandırması -->
                        <div class="mode-config-panel" id="config-panel-safe" style="display: none;">
                            <!-- Safe Hero Display -->
                            <div class="safe-hero" id="safe-hero">
                                <div class="safe-hero-icon">
                                    <svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2">
                                        <rect x="3" y="11" width="18" height="11" rx="2" ry="2"></rect>
                                        <circle cx="12" cy="16" r="1"></circle>
                                        <path d="M7 11V7a5 5 0 0 1 10 0v4"></path>
                                    </svg>
                                </div>
                                <div class="safe-hero-title" data-lang="safe.title">SAFE</div>
                                <div class="safe-hero-subtitle" data-lang="tooltip.safe_hero_subtitle">Password protected API trigger</div>
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
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.enable_password_info">Temporarily disable password without deleting settings.</span></span>
                                    <span id="quick-safe-api-0-status" class="api-status-badge"></span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label>
                                            <span data-lang="safe.password_label">Password</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.password_format_info">Format: L3-R2-B (L=Left, R=Right, number=ticks, B=Button)</span></span>
                                        </label>
                                        <input type="text" id="quick-safe-pwd-0" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <details class="api-details">
                                        <summary>
                                            <span data-lang="safe.api_settings">API Settings</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.api_settings_info">See GitHub documentation for detailed API configuration guide.</span></span>
                                        </summary>
                                        <div class="form-group">
                                            <label data-lang="safe.api_url">URL</label>
                                            <input type="text" id="quick-safe-api-0-url" placeholder="http://192.168.1.100/api/action">
                                        </div>
                                        <div class="form-row">
                                            <div class="form-group half">
                                                <label data-lang="safe.api_method">Method</label>
                                                <select id="quick-safe-api-0-method">
                                                    <option value="GET">GET</option>
                                                    <option value="POST">POST</option>
                                                    <option value="PUT">PUT</option>
                                                    <option value="DELETE">DELETE</option>
                                                </select>
                                            </div>
                                            <div class="form-group half">
                                                <label data-lang="safe.content_type">Content-Type</label>
                                                <input type="text" id="quick-safe-api-0-contentType" value="application/json">
                                            </div>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.authorization">Authorization</label>
                                            <input type="text" id="quick-safe-api-0-auth" placeholder="Bearer token...">
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.custom_headers">Custom Headers</label>
                                            <textarea id="quick-safe-api-0-headers" rows="2" placeholder="X-Key: value"></textarea>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.request_body">Body (JSON)</label>
                                            <textarea id="quick-safe-api-0-body" rows="3" placeholder='{"action":"trigger"}'></textarea>
                                        </div>
                                    </details>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(0)" data-lang="safe.save_password">Save</button>
                                    <button class="btn-teach" onclick="startTeachPassword(0)">
                                        <span data-lang="safe.teach_password">Teach</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.teach_password_info">Create password by rotating encoder. Press button to add B step.</span></span>
                                    </button>
                                    <button class="btn-test" onclick="testSafeApi(0)">
                                        <span data-lang="safe.test_api">Test API</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.test_api_info">Test API call without entering password.</span></span>
                                    </button>
                                </div>
                                <div class="teaching-overlay" id="teaching-overlay-0" style="display:none;">
                                    <div class="teaching-content">
                                        <div class="teaching-icon"><svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
                                        <div class="teaching-title" data-lang="safe.teaching_title">Teaching Mode</div>
                                        <div class="teaching-pattern" id="teaching-pattern-0">-</div>
                                        <div class="teaching-hint" data-lang="safe.teaching_hint">Rotate encoder to create pattern</div>
                                        <div class="teaching-timer" id="teaching-timer-0">15s</div>
                                        <div class="teaching-buttons">
                                            <button class="btn-save-pattern" id="btn-save-pattern-0" onclick="completeTeachPassword(0)" data-lang="safe.save_password">Save</button>
                                            <button class="btn-cancel-teach" onclick="cancelTeachPassword(0)" data-lang="safe.cancel_teaching">Cancel</button>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-1" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-1-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.enable_password_info">Temporarily disable password without deleting settings.</span></span>
                                    <span id="quick-safe-api-1-status" class="api-status-badge"></span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label>
                                            <span data-lang="safe.password_label">Password</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.password_format_info">Format: L3-R2-B (L=Left, R=Right, number=ticks, B=Button)</span></span>
                                        </label>
                                        <input type="text" id="quick-safe-pwd-1" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <details class="api-details">
                                        <summary>
                                            <span data-lang="safe.api_settings">API Settings</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.api_settings_info">See GitHub documentation for detailed API configuration guide.</span></span>
                                        </summary>
                                        <div class="form-group">
                                            <label data-lang="safe.api_url">URL</label>
                                            <input type="text" id="quick-safe-api-1-url" placeholder="http://192.168.1.100/api/action">
                                        </div>
                                        <div class="form-row">
                                            <div class="form-group half">
                                                <label data-lang="safe.api_method">Method</label>
                                                <select id="quick-safe-api-1-method">
                                                    <option value="GET">GET</option>
                                                    <option value="POST">POST</option>
                                                    <option value="PUT">PUT</option>
                                                    <option value="DELETE">DELETE</option>
                                                </select>
                                            </div>
                                            <div class="form-group half">
                                                <label data-lang="safe.content_type">Content-Type</label>
                                                <input type="text" id="quick-safe-api-1-contentType" value="application/json">
                                            </div>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.authorization">Authorization</label>
                                            <input type="text" id="quick-safe-api-1-auth" placeholder="Bearer token...">
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.custom_headers">Custom Headers</label>
                                            <textarea id="quick-safe-api-1-headers" rows="2" placeholder="X-Key: value"></textarea>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.request_body">Body (JSON)</label>
                                            <textarea id="quick-safe-api-1-body" rows="3" placeholder='{"action":"trigger"}'></textarea>
                                        </div>
                                    </details>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(1)" data-lang="safe.save_password">Save</button>
                                    <button class="btn-teach" onclick="startTeachPassword(1)">
                                        <span data-lang="safe.teach_password">Teach</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.teach_password_info">Create password by rotating encoder. Press button to add B step.</span></span>
                                    </button>
                                    <button class="btn-test" onclick="testSafeApi(1)">
                                        <span data-lang="safe.test_api">Test API</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.test_api_info">Test API call without entering password.</span></span>
                                    </button>
                                </div>
                                <div class="teaching-overlay" id="teaching-overlay-1" style="display:none;">
                                    <div class="teaching-content">
                                        <div class="teaching-icon"><svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
                                        <div class="teaching-title" data-lang="safe.teaching_title">Teaching Mode</div>
                                        <div class="teaching-pattern" id="teaching-pattern-1">-</div>
                                        <div class="teaching-hint" data-lang="safe.teaching_hint">Rotate encoder to create pattern</div>
                                        <div class="teaching-timer" id="teaching-timer-1">15s</div>
                                        <div class="teaching-buttons">
                                            <button class="btn-save-pattern" id="btn-save-pattern-1" onclick="completeTeachPassword(1)" data-lang="safe.save_password">Save</button>
                                            <button class="btn-cancel-teach" onclick="cancelTeachPassword(1)" data-lang="safe.cancel_teaching">Cancel</button>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-2" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-2-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.enable_password_info">Temporarily disable password without deleting settings.</span></span>
                                    <span id="quick-safe-api-2-status" class="api-status-badge"></span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label>
                                            <span data-lang="safe.password_label">Password</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.password_format_info">Format: L3-R2-B (L=Left, R=Right, number=ticks, B=Button)</span></span>
                                        </label>
                                        <input type="text" id="quick-safe-pwd-2" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <details class="api-details">
                                        <summary>
                                            <span data-lang="safe.api_settings">API Settings</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.api_settings_info">See GitHub documentation for detailed API configuration guide.</span></span>
                                        </summary>
                                        <div class="form-group">
                                            <label data-lang="safe.api_url">URL</label>
                                            <input type="text" id="quick-safe-api-2-url" placeholder="http://192.168.1.100/api/action">
                                        </div>
                                        <div class="form-row">
                                            <div class="form-group half">
                                                <label data-lang="safe.api_method">Method</label>
                                                <select id="quick-safe-api-2-method">
                                                    <option value="GET">GET</option>
                                                    <option value="POST">POST</option>
                                                    <option value="PUT">PUT</option>
                                                    <option value="DELETE">DELETE</option>
                                                </select>
                                            </div>
                                            <div class="form-group half">
                                                <label data-lang="safe.content_type">Content-Type</label>
                                                <input type="text" id="quick-safe-api-2-contentType" value="application/json">
                                            </div>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.authorization">Authorization</label>
                                            <input type="text" id="quick-safe-api-2-auth" placeholder="Bearer token...">
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.custom_headers">Custom Headers</label>
                                            <textarea id="quick-safe-api-2-headers" rows="2" placeholder="X-Key: value"></textarea>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.request_body">Body (JSON)</label>
                                            <textarea id="quick-safe-api-2-body" rows="3" placeholder='{"action":"trigger"}'></textarea>
                                        </div>
                                    </details>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(2)" data-lang="safe.save_password">Save</button>
                                    <button class="btn-teach" onclick="startTeachPassword(2)">
                                        <span data-lang="safe.teach_password">Teach</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.teach_password_info">Create password by rotating encoder. Press button to add B step.</span></span>
                                    </button>
                                    <button class="btn-test" onclick="testSafeApi(2)">
                                        <span data-lang="safe.test_api">Test API</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.test_api_info">Test API call without entering password.</span></span>
                                    </button>
                                </div>
                                <div class="teaching-overlay" id="teaching-overlay-2" style="display:none;">
                                    <div class="teaching-content">
                                        <div class="teaching-icon"><svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
                                        <div class="teaching-title" data-lang="safe.teaching_title">Teaching Mode</div>
                                        <div class="teaching-pattern" id="teaching-pattern-2">-</div>
                                        <div class="teaching-hint" data-lang="safe.teaching_hint">Rotate encoder to create pattern</div>
                                        <div class="teaching-timer" id="teaching-timer-2">15s</div>
                                        <div class="teaching-buttons">
                                            <button class="btn-save-pattern" id="btn-save-pattern-2" onclick="completeTeachPassword(2)" data-lang="safe.save_password">Save</button>
                                            <button class="btn-cancel-teach" onclick="cancelTeachPassword(2)" data-lang="safe.cancel_teaching">Cancel</button>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-3" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-3-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.enable_password_info">Temporarily disable password without deleting settings.</span></span>
                                    <span id="quick-safe-api-3-status" class="api-status-badge"></span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label>
                                            <span data-lang="safe.password_label">Password</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.password_format_info">Format: L3-R2-B (L=Left, R=Right, number=ticks, B=Button)</span></span>
                                        </label>
                                        <input type="text" id="quick-safe-pwd-3" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <details class="api-details">
                                        <summary>
                                            <span data-lang="safe.api_settings">API Settings</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.api_settings_info">See GitHub documentation for detailed API configuration guide.</span></span>
                                        </summary>
                                        <div class="form-group">
                                            <label data-lang="safe.api_url">URL</label>
                                            <input type="text" id="quick-safe-api-3-url" placeholder="http://192.168.1.100/api/action">
                                        </div>
                                        <div class="form-row">
                                            <div class="form-group half">
                                                <label data-lang="safe.api_method">Method</label>
                                                <select id="quick-safe-api-3-method">
                                                    <option value="GET">GET</option>
                                                    <option value="POST">POST</option>
                                                    <option value="PUT">PUT</option>
                                                    <option value="DELETE">DELETE</option>
                                                </select>
                                            </div>
                                            <div class="form-group half">
                                                <label data-lang="safe.content_type">Content-Type</label>
                                                <input type="text" id="quick-safe-api-3-contentType" value="application/json">
                                            </div>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.authorization">Authorization</label>
                                            <input type="text" id="quick-safe-api-3-auth" placeholder="Bearer token...">
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.custom_headers">Custom Headers</label>
                                            <textarea id="quick-safe-api-3-headers" rows="2" placeholder="X-Key: value"></textarea>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.request_body">Body (JSON)</label>
                                            <textarea id="quick-safe-api-3-body" rows="3" placeholder='{"action":"trigger"}'></textarea>
                                        </div>
                                    </details>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(3)" data-lang="safe.save_password">Save</button>
                                    <button class="btn-teach" onclick="startTeachPassword(3)">
                                        <span data-lang="safe.teach_password">Teach</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.teach_password_info">Create password by rotating encoder. Press button to add B step.</span></span>
                                    </button>
                                    <button class="btn-test" onclick="testSafeApi(3)">
                                        <span data-lang="safe.test_api">Test API</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.test_api_info">Test API call without entering password.</span></span>
                                    </button>
                                </div>
                                <div class="teaching-overlay" id="teaching-overlay-3" style="display:none;">
                                    <div class="teaching-content">
                                        <div class="teaching-icon"><svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
                                        <div class="teaching-title" data-lang="safe.teaching_title">Teaching Mode</div>
                                        <div class="teaching-pattern" id="teaching-pattern-3">-</div>
                                        <div class="teaching-hint" data-lang="safe.teaching_hint">Rotate encoder to create pattern</div>
                                        <div class="teaching-timer" id="teaching-timer-3">15s</div>
                                        <div class="teaching-buttons">
                                            <button class="btn-save-pattern" id="btn-save-pattern-3" onclick="completeTeachPassword(3)" data-lang="safe.save_password">Save</button>
                                            <button class="btn-cancel-teach" onclick="cancelTeachPassword(3)" data-lang="safe.cancel_teaching">Cancel</button>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            
                            <div class="quick-safe-tab-content" id="quick-safe-tab-4" style="display: none;">
                                <div class="safe-toggle-row">
                                    <label class="toggle-switch">
                                        <input type="checkbox" id="quick-safe-pwd-4-enabled">
                                        <span class="toggle-slider"></span>
                                    </label>
                                    <span class="toggle-label" data-lang="safe.enable_password">Enable Password</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.enable_password_info">Temporarily disable password without deleting settings.</span></span>
                                    <span id="quick-safe-api-4-status" class="api-status-badge"></span>
                                </div>
                                <div class="safe-config-section">
                                    <div class="form-group">
                                        <label>
                                            <span data-lang="safe.password_label">Password</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.password_format_info">Format: L3-R2-B (L=Left, R=Right, number=ticks, B=Button)</span></span>
                                        </label>
                                        <input type="text" id="quick-safe-pwd-4" placeholder="L3-R2-L1-R3">
                                    </div>
                                    <details class="api-details">
                                        <summary>
                                            <span data-lang="safe.api_settings">API Settings</span>
                                            <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.api_settings_info">See GitHub documentation for detailed API configuration guide.</span></span>
                                        </summary>
                                        <div class="form-group">
                                            <label data-lang="safe.api_url">URL</label>
                                            <input type="text" id="quick-safe-api-4-url" placeholder="http://192.168.1.100/api/action">
                                        </div>
                                        <div class="form-row">
                                            <div class="form-group half">
                                                <label data-lang="safe.api_method">Method</label>
                                                <select id="quick-safe-api-4-method">
                                                    <option value="GET">GET</option>
                                                    <option value="POST">POST</option>
                                                    <option value="PUT">PUT</option>
                                                    <option value="DELETE">DELETE</option>
                                                </select>
                                            </div>
                                            <div class="form-group half">
                                                <label data-lang="safe.content_type">Content-Type</label>
                                                <input type="text" id="quick-safe-api-4-contentType" value="application/json">
                                            </div>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.authorization">Authorization</label>
                                            <input type="text" id="quick-safe-api-4-auth" placeholder="Bearer token...">
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.custom_headers">Custom Headers</label>
                                            <textarea id="quick-safe-api-4-headers" rows="2" placeholder="X-Key: value"></textarea>
                                        </div>
                                        <div class="form-group">
                                            <label data-lang="safe.request_body">Body (JSON)</label>
                                            <textarea id="quick-safe-api-4-body" rows="3" placeholder='{"action":"trigger"}'></textarea>
                                        </div>
                                    </details>
                                </div>
                                <div class="form-actions">
                                    <button class="btn-save" onclick="saveSafePassword(4)" data-lang="safe.save_password">Save</button>
                                    <button class="btn-teach" onclick="startTeachPassword(4)">
                                        <span data-lang="safe.teach_password">Teach</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.teach_password_info">Create password by rotating encoder. Press button to add B step.</span></span>
                                    </button>
                                    <button class="btn-test" onclick="testSafeApi(4)">
                                        <span data-lang="safe.test_api">Test API</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.test_api_info">Test API call without entering password.</span></span>
                                    </button>
                                </div>
                                <div class="teaching-overlay" id="teaching-overlay-4" style="display:none;">
                                    <div class="teaching-content">
                                        <div class="teaching-icon"><svg width="48" height="48" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><circle cx="12" cy="12" r="10"/><circle cx="12" cy="12" r="6"/><circle cx="12" cy="12" r="2"/></svg></div>
                                        <div class="teaching-title" data-lang="safe.teaching_title">Teaching Mode</div>
                                        <div class="teaching-pattern" id="teaching-pattern-4">-</div>
                                        <div class="teaching-hint" data-lang="safe.teaching_hint">Rotate encoder to create pattern</div>
                                        <div class="teaching-timer" id="teaching-timer-4">15s</div>
                                        <div class="teaching-buttons">
                                            <button class="btn-save-pattern" id="btn-save-pattern-4" onclick="completeTeachPassword(4)" data-lang="safe.save_password">Save</button>
                                            <button class="btn-cancel-teach" onclick="cancelTeachPassword(4)" data-lang="safe.cancel_teaching">Cancel</button>
                                        </div>
                                    </div>
                                </div>
                            </div>
                            
                            <!-- Kullanım Talimatı -->
                            <div class="mode-usage-guide">
                                <span class="usage-guide-text" data-lang="safe.usage_guide">Usage: Enter your password sequence with encoder. Example for L3-R2-B: rotate left 3x, right 2x, press button.</span>
                                <a href="https://github.com/smrtkrft/SynDimm" target="_blank" class="usage-guide-link" data-lang="common.detailed_usage">Detailed usage: github.com/smrtkrft/SynDimm</a>
                            </div>
                        </div>
                        
                    </div>
                </div>
                
                <!-- Ayırıcı Çizgi -->
                <div class="section-divider"></div>
                
                <!-- Cihaz Ayarları Başlığı -->
                <h2 class="device-settings-title">
                    <span data-lang="quick.device_settings">Device Settings</span>
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
                                <div class="form-label-row">
                                    <label data-lang="connection.ssid">SSID</label>
                                    <button type="button" class="btn-wifi-scan" onclick="openWifiScanModal('quick-primary-ssid')">
                                        <span data-lang="connection.scan">Scan</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.wifi_scan_info">Scan for available WiFi networks.</span></span>
                                    </button>
                                </div>
                                <input type="text" id="quick-primary-ssid" data-lang-placeholder="connection.enter_ssid">
                            </div>
                            <div class="form-group">
                                <label>
                                    <span data-lang="connection.password_optional">Password</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.wifi_password_info">Leave empty for open networks.</span></span>
                                </label>
                                <input type="password" id="quick-primary-password" autocomplete="current-password" data-lang-placeholder="connection.leave_empty_open">
                            </div>
                            <div class="form-group">
                                <label>
                                    <span data-lang="connection.static_ip">Static IP</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.static_ip_info">Fill in for static IP, leave empty for DHCP. Example: 192.168.1.100</span></span>
                                </label>
                                <input type="text" id="quick-primary-static" data-lang-placeholder="connection.leave_empty_dhcp">
                            </div>
                            <div class="form-group">
                                <label>
                                    <span data-lang="connection.mdns_domain">.local Domain</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.mdns_info">Access device as [name].local. Each device needs unique name.</span></span>
                                </label>
                                <input type="text" id="quick-primary-mdns" placeholder="e.g: dimm">
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
                                <div class="form-label-row">
                                    <label data-lang="connection.ssid">SSID</label>
                                    <button type="button" class="btn-wifi-scan" onclick="openWifiScanModal('quick-backup-ssid')">
                                        <span data-lang="connection.scan">Scan</span>
                                        <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.wifi_scan_info">Scan for available WiFi networks.</span></span>
                                    </button>
                                </div>
                                <input type="text" id="quick-backup-ssid" data-lang-placeholder="connection.enter_ssid">
                            </div>
                            <div class="form-group">
                                <label>
                                    <span data-lang="connection.password_optional">Password</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.wifi_password_info">Leave empty for open networks.</span></span>
                                </label>
                                <input type="password" id="quick-backup-password" autocomplete="current-password" data-lang-placeholder="connection.leave_empty_open">
                            </div>
                            <div class="form-group">
                                <label>
                                    <span data-lang="connection.static_ip">Static IP</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.static_ip_info">Fill in for static IP, leave empty for DHCP. Example: 192.168.1.100</span></span>
                                </label>
                                <input type="text" id="quick-backup-static" data-lang-placeholder="connection.leave_empty_dhcp">
                            </div>
                            <div class="form-group">
                                <label>
                                    <span data-lang="connection.mdns_domain">.local Domain</span>
                                    <span class="info-tooltip-i">i<span class="tooltip-content" data-lang="tooltip.mdns_info">Access device as [name].local. Each device needs unique name.</span></span>
                                </label>
                                <input type="text" id="quick-backup-mdns" placeholder="e.g: mysyndimm">
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
                        <div class="ota-update-icon">⬆</div>
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
    
    <!-- WiFi Scan Popup Modal -->
    <div id="wifi-scan-modal" class="modal-overlay" style="display: none;">
        <div class="modal-content wifi-scan-modal">
            <div class="modal-header">
                <h3 data-lang="connection.scan_wifi">Scan WiFi Networks</h3>
                <button class="modal-close" onclick="closeWifiScanModal()">&times;</button>
            </div>
            <div class="modal-body">
                <div id="wifi-scan-loading" class="wifi-scan-loading">
                    <div class="loading-spinner"></div>
                    <span data-lang="connection.scanning">Scanning...</span>
                </div>
                <div id="wifi-scan-results" class="wifi-scan-results" style="display: none;">
                    <div id="wifi-network-list" class="wifi-network-list"></div>
                </div>
                <div id="wifi-scan-empty" class="wifi-scan-empty" style="display: none;">
                    <span data-lang="connection.no_networks">No networks found</span>
                </div>
            </div>
            <div class="modal-footer">
                <button class="btn btn-secondary" onclick="scanWifiNetworks()" data-lang="connection.rescan">Rescan</button>
                <button class="btn btn-secondary" onclick="closeWifiScanModal()" data-lang="common.close">Close</button>
            </div>
        </div>
    </div>
    
    <script src="/script.js?v=3"></script>
</body>
</html>)rawliteral";

#endif // SK_HTML_H
