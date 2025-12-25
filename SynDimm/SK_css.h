/**
 * SK_css.h
 * SmartKraft SynDimm - CSS Styles
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

#ifndef SK_CSS_H
#define SK_CSS_H

const char SK_CSS[] PROGMEM = R"rawliteral(
:root {
    /* Dark Theme (Default) */
    --bg-primary: #000000;
    --bg-secondary: #0a0a0a;
    --bg-card: rgba(255, 255, 255, 0.03);
    --text-primary: #ffffff;
    --text-secondary: rgba(255, 255, 255, 0.6);
    --text-muted: rgba(255, 255, 255, 0.5);
    --border-color: rgba(255, 255, 255, 0.1);
    --border-light: rgba(255, 255, 255, 0.2);
    --hover-bg: rgba(255, 255, 255, 0.1);
    --card-hover: rgba(255, 255, 255, 0.05);
    --input-bg: rgba(255, 255, 255, 0.05);
    --input-bg-focus: rgba(255, 255, 255, 0.08);
    --shadow-hover: rgba(255, 255, 255, 0.15);
    /* Status Colors */
    --color-success: rgb(16, 185, 129);
    --color-success-bg: rgba(16, 185, 129, 0.15);
    --color-success-bg-light: rgba(16, 185, 129, 0.2);
    --color-success-border: rgba(16, 185, 129, 0.4);
    --color-error: rgb(239, 68, 68);
    --color-error-bg: rgba(239, 68, 68, 0.15);
    --color-error-bg-light: rgba(239, 68, 68, 0.2);
    --color-error-border: rgba(239, 68, 68, 0.4);
    --color-info: rgb(59, 130, 246);
    --color-info-dark: rgb(37, 99, 235);
    --color-info-bg: rgba(59, 130, 246, 0.15);
    --color-info-bg-light: rgba(59, 130, 246, 0.2);
    --color-info-border: rgba(59, 130, 246, 0.3);
    --color-warning: rgb(251, 191, 36);
    --color-warning-dark: rgb(245, 158, 11);
    /* Transitions */
    --transition-fast: all 0.2s ease;
    --transition-normal: all 0.3s ease;
    --transition-smooth: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    /* Border Radius */
    --radius-sm: 4px;
    --radius-md: 6px;
    --radius-lg: 8px;
}

body.light-theme {
    /* Light Theme */
    --bg-primary: #ffffff;
    --bg-secondary: #f5f5f5;
    --bg-card: rgba(0, 0, 0, 0.03);
    --bg-tertiary: #f0f0f0;
    --text-primary: #000000;
    --text-secondary: rgba(0, 0, 0, 0.6);
    --text-muted: rgba(0, 0, 0, 0.5);
    --border-color: rgba(0, 0, 0, 0.1);
    --border-light: rgba(0, 0, 0, 0.2);
    --hover-bg: rgba(0, 0, 0, 0.05);
    --card-hover: rgba(0, 0, 0, 0.03);
    --input-bg: rgba(0, 0, 0, 0.05);
    --input-bg-focus: rgba(0, 0, 0, 0.08);
    --shadow-hover: rgba(0, 0, 0, 0.15);
}

/* Light Theme Specific Overrides */
body.light-theme .info-section { border-bottom-color: rgba(0, 0, 0, 0.06); }
body.light-theme .info-section:hover p { color: rgba(0, 0, 0, 0.85); }
body.light-theme .info-button { border-color: rgba(0, 0, 0, 0.15); }
body.light-theme .info-button:hover { border-color: rgba(0, 0, 0, 0.3); }
body.light-theme .info-single .info-separator { color: rgba(0, 0, 0, 0.3); }

/* Light Theme - OTA Section */
body.light-theme .ota-section h2 { color: var(--text-primary); }
body.light-theme .ota-version-card { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .ota-version-label { color: var(--text-muted); }
body.light-theme .ota-version-value { color: var(--text-primary); }
body.light-theme .ota-update-version { color: var(--text-primary); }
body.light-theme .ota-update-date { color: var(--text-muted); }
body.light-theme .ota-release-notes { color: var(--text-secondary); background: rgba(0, 0, 0, 0.05); }
body.light-theme .ota-auto-info { color: var(--text-muted); background: var(--bg-card); border-left-color: var(--border-light); }
body.light-theme .ota-progress-container { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .ota-progress-label { color: var(--text-secondary); }
body.light-theme .ota-progress-bar { background: var(--border-color); }
body.light-theme .ota-progress-percent { color: var(--text-primary); }
body.light-theme .info-divider { background: var(--border-color); }
body.light-theme .section-divider { background: var(--border-color); }
body.light-theme .ota-quick-section { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .ota-quick-label { color: var(--text-muted); }
body.light-theme .ota-quick-version { color: var(--text-primary); }
body.light-theme .ota-progress-bar { background: var(--border-color); }
body.light-theme .ota-update-available { background: rgba(76, 175, 80, 0.08); border-color: rgba(76, 175, 80, 0.2); }
body.light-theme .wifi-connection-info { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .wifi-info-text { color: var(--text-primary); }
body.light-theme .ap-mode-card { background: linear-gradient(135deg, rgba(59, 130, 246, 0.06) 0%, rgba(139, 92, 246, 0.03) 100%); border-color: rgba(59, 130, 246, 0.15); }
body.light-theme .ap-mode-card:hover { border-color: rgba(59, 130, 246, 0.3); }
body.light-theme .ap-mode-card.disabled { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .ap-mode-header { background: rgba(0, 0, 0, 0.02); }
body.light-theme .ap-mode-title-text h4 { color: var(--text-primary); }
body.light-theme .ap-mode-subtitle { color: var(--text-muted); }
body.light-theme .ap-mode-status { color: var(--color-success); }
body.light-theme .ap-mode-status.disabled { color: var(--text-muted); }
body.light-theme .ap-credential-label { color: var(--text-muted); }
body.light-theme .ap-credential-value { color: var(--text-primary); }
body.light-theme .ap-mode-warning { background: rgba(251, 191, 36, 0.08); border-top-color: rgba(251, 191, 36, 0.2); }
body.light-theme #mode-config-accordion { border-color: rgba(0, 0, 0, 0.2); box-shadow: 0 0 15px rgba(0, 0, 0, 0.08); }
body.light-theme #mode-config-accordion:hover { border-color: rgba(0, 0, 0, 0.3); }

/* Light Theme - Info Tooltips */
body.light-theme .info-tooltip-i { background: rgba(0, 0, 0, 0.05); border-color: var(--border-color); color: var(--text-muted); }
body.light-theme .info-tooltip-i:hover { background: var(--primary); border-color: var(--primary); color: #fff; }
body.light-theme .info-tooltip-i .tooltip-content { background: rgba(255, 255, 255, 0.95); border-color: var(--border-color); color: var(--text-secondary); box-shadow: 0 4px 20px rgba(0, 0, 0, 0.25); }
body.light-theme .info-tooltip-i .tooltip-content::after { border-top-color: rgba(255, 255, 255, 0.95); }

/* Light Theme - Dimmer Section */
body.light-theme .dimmer-status-card { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .dimmer-status-header { border-bottom-color: var(--border-color); }
body.light-theme .dimmer-info-label { color: var(--text-muted); }
body.light-theme .dimmer-info-value { color: var(--text-primary); }
body.light-theme .dimmer-config-section { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .dimmer-section-title { color: var(--text-primary); }
body.light-theme .dimmer-calibration-info { color: var(--text-secondary); background: var(--bg-card); border-left-color: var(--border-light); }
body.light-theme .dimmer-ratio-group label { color: var(--text-primary); }
body.light-theme .dimmer-ratio-display { background: var(--bg-card); }
body.light-theme .ratio-label { color: var(--text-muted); }
body.light-theme .ratio-value { color: var(--text-primary); }
body.light-theme .dimmer-calibration-info-compact { color: var(--text-secondary); background: var(--bg-card); border-left-color: var(--border-light); }
body.light-theme .dimmer-ratio-display-compact { background: var(--bg-card); color: var(--text-muted); }
body.light-theme .ratio-value-compact { color: var(--text-primary); }
body.light-theme .dimmer-scan-info { color: var(--text-secondary); background: var(--bg-card); border-left-color: var(--border-light); }
body.light-theme .dimmer-scan-progress { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .scan-progress-text { color: var(--text-secondary); }
body.light-theme .scan-progress-bar { background: var(--border-color); }
body.light-theme .devices-list-title { color: var(--text-secondary); }

/* Light Theme - Dimmer Hero */
body.light-theme .dimmer-hero { background: radial-gradient(ellipse at center, rgba(59, 130, 246, 0.1) 0%, transparent 70%); }
body.light-theme .dimmer-hero.connected-on { background: radial-gradient(ellipse at center, rgba(16, 185, 129, 0.1) 0%, transparent 70%); }
body.light-theme .dimmer-hero.connected-off { background: radial-gradient(ellipse at center, rgba(59, 130, 246, 0.1) 0%, transparent 70%); }
body.light-theme .dimmer-hero.disconnected { background: radial-gradient(ellipse at center, rgba(239, 68, 68, 0.1) 0%, transparent 70%); }
body.light-theme .dimmer-brightness-display { color: var(--text-primary); }
body.light-theme .dimmer-brightness-display span { color: var(--text-muted); }
body.light-theme .dimmer-hero.connected-off .dimmer-brightness-display { color: var(--text-muted); }
body.light-theme .dimmer-hero.disconnected .dimmer-brightness-display { color: var(--text-muted); }
body.light-theme .dimmer-power-status { color: rgb(16, 185, 129); }
body.light-theme .dimmer-hero.connected-off .dimmer-power-status { color: var(--text-muted); }
body.light-theme .dimmer-hero.disconnected .dimmer-power-status { color: var(--text-muted); }
body.light-theme .dimmer-status-dot { color: var(--text-muted); }
body.light-theme .dimmer-connection-status { color: rgb(16, 185, 129); }
body.light-theme .dimmer-hero.disconnected .dimmer-connection-status { color: rgb(239, 68, 68); }
body.light-theme .dimmer-ip-display { color: var(--text-muted); }
body.light-theme .dimmer-cal-btn { background: var(--bg-card); border-color: var(--border-light); color: var(--text-primary); }
body.light-theme .dimmer-cal-btn:hover:not(:disabled) { border-color: rgb(59, 130, 246); background: rgba(59, 130, 246, 0.1); }
body.light-theme .dimmer-cal-value { color: rgb(59, 130, 246); }
body.light-theme .dimmer-cal-label { color: var(--text-muted); }
body.light-theme .dimmer-compact-form { background: rgba(0, 0, 0, 0.02); border-color: var(--border-color); }
body.light-theme .dimmer-section-title { color: var(--text-secondary); }
body.light-theme .dimmer-inline-form input { background: var(--bg-card); border-color: var(--border-light); color: var(--text-primary); }
body.light-theme .dimmer-inline-form input::placeholder { color: var(--text-muted); }
body.light-theme .dimmer-inline-form input:focus { border-color: rgba(59, 130, 246, 0.4); }
body.light-theme .saved-device-empty { color: var(--text-muted); }

/* Light Theme - Shutter Hero */
body.light-theme .shutter-hero { background: radial-gradient(ellipse at center, rgba(6, 182, 212, 0.1) 0%, transparent 70%); }
body.light-theme .shutter-hero.disconnected { background: radial-gradient(ellipse at center, rgba(239, 68, 68, 0.1) 0%, transparent 70%); }
body.light-theme .shutter-hero.moving-up { background: radial-gradient(ellipse at center, rgba(34, 197, 94, 0.12) 0%, transparent 70%); }
body.light-theme .shutter-hero.moving-down { background: radial-gradient(ellipse at center, rgba(251, 191, 36, 0.12) 0%, transparent 70%); }
body.light-theme .shutter-position-display { color: var(--text-primary); }
body.light-theme .shutter-position-display span { color: var(--text-muted); }
body.light-theme .shutter-hero.disconnected .shutter-position-display { color: var(--text-muted); }
body.light-theme .shutter-movement-status { color: rgb(6, 182, 212); }
body.light-theme .shutter-hero.disconnected .shutter-movement-status { color: rgb(239, 68, 68); }
body.light-theme .shutter-hero.moving-up .shutter-movement-status { color: rgb(34, 197, 94); }
body.light-theme .shutter-hero.moving-down .shutter-movement-status { color: rgb(217, 119, 6); }
body.light-theme .shutter-status-dot { color: var(--text-muted); }
body.light-theme .shutter-connection-status { color: rgb(16, 185, 129); }
body.light-theme .shutter-ip-display { color: var(--text-muted); }
body.light-theme .shutter-cal-btn { background: var(--bg-card); border-color: var(--border-light); color: var(--text-primary); }
body.light-theme .shutter-cal-btn:hover:not(:disabled) { border-color: rgb(6, 182, 212); background: rgba(6, 182, 212, 0.1); }
body.light-theme .shutter-cal-value { color: rgb(6, 182, 212); }
body.light-theme .shutter-hero.disconnected .shutter-cal-value { color: var(--text-muted); }
body.light-theme .shutter-cal-label { color: var(--text-muted); }
body.light-theme .shutter-compact-form { background: rgba(0, 0, 0, 0.02); border-color: var(--border-color); }
body.light-theme .shutter-inline-form input { background: var(--bg-card); border-color: var(--border-light); color: var(--text-primary); }
body.light-theme .shutter-inline-form input::placeholder { color: var(--text-muted); }
body.light-theme .shutter-inline-form input:focus { border-color: rgba(6, 182, 212, 0.4); }
body.light-theme .shutter-warning { background: rgba(251, 191, 36, 0.1); border-color: rgba(217, 119, 6, 0.4); color: rgb(180, 83, 9); }
body.light-theme .saved-device-item { background: var(--bg-card); border-color: var(--border-light); }
body.light-theme .saved-device-ip { color: var(--text-primary); }
body.light-theme .saved-device-type { color: var(--text-muted); }

/* Light Theme - Status Bar (shutter) */
body.light-theme .shutter-status-bar { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .status-col { border-right-color: var(--border-light); }
body.light-theme .status-col-label { color: var(--text-muted); }
body.light-theme .status-col-value { color: var(--text-primary); }
body.light-theme .status-power-text { color: var(--text-secondary); }

/* Light Theme - Calibration */
body.light-theme .calibration-value-display { color: var(--text-primary); }
body.light-theme .btn-cal-up, body.light-theme .btn-cal-down { background: var(--bg-card); border-color: var(--border-light); color: var(--text-secondary); }
body.light-theme .btn-cal-up:hover, body.light-theme .btn-cal-down:hover { background: var(--hover-bg); border-color: var(--border-light); color: var(--text-primary); }

/* Light Theme - Brightness */
body.light-theme .brightness-label { color: var(--text-muted); }
body.light-theme .brightness-value { color: var(--text-primary); }
body.light-theme .brightness-bar-bg { background: var(--border-color); }
body.light-theme .power-status { color: var(--text-secondary); }
body.light-theme .control-hint { color: var(--text-muted); }

/* Light Theme - Connection Status */
body.light-theme .connection-status-inline { background: var(--bg-card); }

/* Light Theme - Input Button */
body.light-theme .btn-inline-scan { background: var(--bg-card); color: var(--text-secondary); border-color: var(--border-light); }
body.light-theme .btn-inline-scan:hover { background: var(--hover-bg); border-color: var(--border-light); }

/* Light Theme - Scrollbar */
body.light-theme .devices-container::-webkit-scrollbar-track { background: var(--bg-card); }
body.light-theme .devices-container::-webkit-scrollbar-thumb { background: var(--border-light); }
body.light-theme .devices-container::-webkit-scrollbar-thumb:hover { background: var(--text-muted); }

/* Light Theme - Slider */
body.light-theme .calibration-slider { background: linear-gradient(to right, rgba(0, 0, 0, 0.08), rgba(0, 0, 0, 0.12), rgba(0, 0, 0, 0.08)); }
body.light-theme .calibration-slider::-webkit-slider-thumb { background: #333333; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2), 0 0 0 1px rgba(0, 0, 0, 0.1); }
body.light-theme .calibration-slider::-webkit-slider-thumb:hover { box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3), 0 0 0 1px rgba(0, 0, 0, 0.2), 0 0 0 4px rgba(0, 0, 0, 0.08); }
body.light-theme .calibration-slider::-moz-range-thumb { background: #333333; box-shadow: 0 2px 8px rgba(0, 0, 0, 0.2), 0 0 0 1px rgba(0, 0, 0, 0.1); }
body.light-theme .slider-labels { color: rgba(0, 0, 0, 0.35); }
body.light-theme .slider-value-display { background: var(--bg-card); border-color: var(--border-color); }
body.light-theme .slider-value-label { color: var(--text-muted); }
body.light-theme .slider-value-number { color: var(--text-primary); }

/* Light Theme - Mobile Tabs */
body.light-theme .tab:nth-child(odd) { border-right-color: var(--border-color); }
body.light-theme .tab { border-bottom-color: var(--border-color); }

/* Light Theme - System Actions */
body.light-theme .btn-restart { background: #333333; color: #ffffff; }
body.light-theme .btn-restart:hover { background: #444444; }
body.light-theme .factory-reset-confirm input { background: var(--bg-secondary); }

/* Light Theme - Buttons with colored backgrounds (must have white text) */
body.light-theme .btn-ota-update { color: #ffffff; }
body.light-theme .btn-primary { color: #ffffff; }
body.light-theme .btn-device-connect { color: #ffffff; }
body.light-theme .scanned-device-connect { color: #ffffff; }
body.light-theme .btn-save-main { color: #ffffff; }
body.light-theme .btn-save { color: #ffffff; background: #333333; }
body.light-theme .btn-save:hover { background: #444444; }
body.light-theme .save-button { color: #ffffff; background: #333333; border-color: #333333; }
body.light-theme .save-button:hover { background: #444444; border-color: #444444; }
body.light-theme .ratio-btn.active { color: #ffffff; }
body.light-theme .ratio-btn-compact.active { color: #ffffff; }
body.light-theme .btn-status-connect { color: var(--text-primary); }
body.light-theme .theme-btn.active { color: #ffffff; background: #333333; }

* { margin: 0; padding: 0; box-sizing: border-box; }
body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif; font-size: 19.2px; background: var(--bg-primary); color: var(--text-primary); min-height: 100vh; padding: 0; display: flex; justify-content: center; align-items: flex-start; transition: background 0.3s ease, color 0.3s ease; }
.container { max-width: 900px; width: 100%; background: var(--bg-primary); min-height: 100vh; }
.header { background: var(--bg-primary); color: var(--text-primary); padding: 40px 40px 15px 40px; text-align: center; border-bottom: none; position: relative; }
.header h1 { font-size: 2.2em; margin-bottom: 12px; font-weight: 400; letter-spacing: 0.5px; color: var(--text-primary); }
.theme-toggle { position: absolute; top: 20px; right: 20px; display: flex; gap: 8px; background: var(--bg-card); padding: 6px; border-radius: 20px; border: 1px solid var(--border-color); }
.theme-btn { padding: 8px 16px; background: transparent; border: none; color: var(--text-secondary); font-size: 0.85em; cursor: pointer; border-radius: 14px; transition: var(--transition-fast); font-weight: 500; }
.theme-btn.active { background: var(--text-primary); color: var(--bg-primary); }
.theme-btn:hover { color: var(--text-primary); }

.info-box { display: flex; flex-direction: column; align-items: center; gap: 8px; margin-top: 15px; }
.info-single { display: flex; align-items: baseline; justify-content: center; gap: 8px; font-size: 0.9em; flex-wrap: nowrap; white-space: nowrap; }
.info-single .info-label { color: var(--text-secondary); font-weight: 400; letter-spacing: 0.5px; font-size: 0.95em; line-height: 1; }
.info-single .info-value { color: var(--text-primary); font-weight: 400; letter-spacing: 0.5px; font-size: 0.95em; line-height: 1; }
.info-single .info-separator { color: rgba(255, 255, 255, 0.3); margin: 0 5px; font-size: 0.95em; line-height: 1; }
.info-single .info-mdns { color: var(--text-secondary); font-size: 0.85em; margin-left: 5px; }
.info-item { text-align: center; }
.info-label { font-size: 0.75em; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1.5px; margin-bottom: 8px; font-weight: 500; }
.info-value { font-size: 1.1em; font-weight: 400; color: var(--text-primary); letter-spacing: 0.5px; }

/* Global Status Bar */
.global-status-bar { background: var(--bg-card); border: none; border-radius: var(--radius-lg); padding: 16px 20px; margin: 15px 40px 20px 40px; }
.global-status-bar .status-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 0; }
.global-status-bar .status-item { display: flex; flex-direction: column; gap: 6px; text-align: center; padding: 0 12px; border-right: 1px solid var(--border-light); }
.global-status-bar .status-item:last-child { border-right: none; }
@media (max-width: 600px) {
    .global-status-bar { margin: 20px 20px 12px 20px; padding: 12px 16px; }
    .global-status-bar .status-row { grid-template-columns: 1fr; gap: 12px; }
    .global-status-bar .status-item { border-right: none; border-bottom: 1px solid var(--border-light); padding-bottom: 10px; }
    .global-status-bar .status-item:last-child { border-bottom: none; padding-bottom: 0; }
}

.tabs { display: flex; background: var(--bg-primary); border-bottom: 1px solid var(--border-color); padding: 0 20px; }
.tab { flex: 1; padding: 22px 20px; text-align: center; cursor: pointer; background: transparent; border: none; font-size: 0.95em; font-weight: 400; color: var(--text-secondary); transition: var(--transition-smooth); letter-spacing: 0.3px; position: relative; }
.tab::after { content: ''; position: absolute; bottom: 0; left: 50%; transform: translateX(-50%) scaleX(0); width: 60%; height: 2px; background: var(--text-primary); transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1); }
.tab:hover { color: var(--text-secondary); opacity: 0.8; }
.tab.active { color: var(--text-primary); }
.tab.active::after { transform: translateX(-50%) scaleX(1); }
.tab-content { display: none !important; padding: 50px 40px; min-height: 500px; animation: fadeInUp 0.4s cubic-bezier(0.4, 0, 0.2, 1); }
.tab-content.active { display: block !important; }
@keyframes fadeInUp { from { opacity: 0; transform: translateY(20px); } to { opacity: 1; transform: translateY(0); } }
.placeholder { text-align: center; color: var(--text-muted); font-size: 0.95em; padding: 100px 30px; font-weight: 300; letter-spacing: 0.5px; opacity: 0.5; }
.modes-content, .connection-content, .info-content { max-width: 750px; margin: 0 auto; }

/* Info Card Styles */
.info-card { background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-lg); padding: 25px; margin-bottom: 25px; transition: var(--transition-normal); }
.info-card:hover { border-color: var(--border-light); }
.info-card h3 { font-size: 1.1em; font-weight: 500; color: var(--text-primary); margin-bottom: 20px; letter-spacing: 0.3px; }
.info-grid { display: grid; grid-template-columns: repeat(3, 1fr); gap: 20px; }
.info-item { display: flex; flex-direction: column; gap: 8px; }
.info-item-label { font-size: 0.75em; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; font-weight: 500; }
.info-item-value { font-size: 1em; color: var(--text-primary); font-weight: 400; }

/* Theme Selector */
.theme-selector { display: flex; gap: 15px; }
.theme-option { flex: 1; cursor: pointer; }
.theme-option input[type="radio"] { display: none; }
.theme-option-label { display: flex; align-items: center; justify-content: center; padding: 15px 20px; background: var(--bg-card); border: 2px solid var(--border-color); border-radius: var(--radius-lg); transition: var(--transition-normal); }
.theme-option input[type="radio"]:checked + .theme-option-label { background: var(--hover-bg); border-color: var(--text-primary); }
.theme-option:hover .theme-option-label { border-color: var(--border-light); }
.theme-name { font-size: 0.95em; color: var(--text-primary); font-weight: 500; }

/* Mode Selection Buttons */
.mode-buttons { display: flex; gap: 15px; margin-bottom: 25px; }
.mode-btn { flex: 1; display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 25px 15px; background: var(--bg-card); border: 2px solid var(--border-color); border-radius: 12px; cursor: pointer; transition: var(--transition-normal); color: var(--text-primary); position: relative; }
.mode-btn:hover { border-color: var(--border-light); background: var(--hover-bg); transform: translateY(-2px); }
.mode-btn.active { background: var(--hover-bg); border-color: var(--text-primary); }
.mode-btn.preview { background: rgba(255, 200, 0, 0.25); border-color: #ffc800; box-shadow: 0 0 25px rgba(255, 200, 0, 0.6); animation: preview-pulse 1s ease-in-out infinite; }
@keyframes preview-pulse { 0%, 100% { box-shadow: 0 0 25px rgba(255, 200, 0, 0.6); background: rgba(255, 200, 0, 0.25); } 50% { box-shadow: 0 0 40px rgba(255, 200, 0, 0.9), 0 0 50px rgba(255, 200, 0, 0.5); background: rgba(255, 200, 0, 0.35); } }
.mode-btn-text { font-size: 1.1em; font-weight: 600; letter-spacing: 0.5px; }

/* Info Tooltip Styles - Test Variants */
/* Variant 1: Circle with "i" (italik) */
.info-tooltip-i { position: relative; display: inline-flex; align-items: center; justify-content: center; width: 18px; height: 18px; background: rgba(255, 255, 255, 0.1); border: 1px solid var(--border-light); border-radius: 50%; font-size: 12px; font-style: italic; font-weight: 600; color: var(--text-muted); cursor: help; margin-left: 8px; transition: var(--transition-fast); }
.info-tooltip-i:hover { background: var(--primary); border-color: var(--primary); color: #fff; }
.info-tooltip-i .tooltip-content { position: absolute; bottom: calc(100% + 10px); left: 50%; transform: translateX(-50%); min-width: 220px; max-width: 320px; padding: 14px 18px; background: rgba(30, 30, 35, 0.95); border: 1px solid var(--border-light); border-radius: 8px; font-size: 1.1em; font-style: normal; font-weight: 400; color: var(--text-secondary); line-height: 1.6; visibility: hidden; opacity: 0; transition: var(--transition-fast); z-index: 99999; box-shadow: 0 4px 20px rgba(0, 0, 0, 0.5); text-align: left; }
.info-tooltip-i .tooltip-content::after { content: ''; position: absolute; top: 100%; left: 50%; transform: translateX(-50%); border: 6px solid transparent; border-top-color: rgba(30, 30, 35, 0.95); }
.info-tooltip-i:hover .tooltip-content { visibility: visible; opacity: 1; }

/* Variant 2: Circle with "?" */
.info-tooltip-q { position: relative; display: inline-flex; align-items: center; justify-content: center; width: 18px; height: 18px; background: rgba(59, 130, 246, 0.15); border: 1px solid var(--color-info-border); border-radius: 50%; font-size: 12px; font-weight: 700; color: var(--color-info); cursor: help; margin-left: 8px; transition: var(--transition-fast); }
.info-tooltip-q:hover { background: var(--color-info); color: #fff; }
.info-tooltip-q .tooltip-content { position: absolute; bottom: calc(100% + 10px); left: 50%; transform: translateX(-50%); min-width: 220px; max-width: 320px; padding: 14px 18px; background: var(--bg-card); border: 1px solid var(--color-info-border); border-radius: 8px; font-size: 1.1em; font-weight: 400; color: var(--text-secondary); line-height: 1.6; visibility: hidden; opacity: 0; transition: var(--transition-fast); z-index: 1000; box-shadow: 0 4px 20px rgba(59, 130, 246, 0.2); text-align: left; }
.info-tooltip-q .tooltip-content::after { content: ''; position: absolute; top: 100%; left: 50%; transform: translateX(-50%); border: 6px solid transparent; border-top-color: var(--color-info-border); }
.info-tooltip-q:hover .tooltip-content { visibility: visible; opacity: 1; }

/* Variant 3: Asterisk (*) */
.info-tooltip-ast { position: relative; display: inline-flex; align-items: center; justify-content: center; width: 16px; height: 16px; font-size: 14px; font-weight: 700; color: var(--color-warning); cursor: help; margin-left: 6px; transition: var(--transition-fast); }
.info-tooltip-ast:hover { color: var(--color-warning-dark); transform: scale(1.2); }
.info-tooltip-ast .tooltip-content { position: absolute; bottom: calc(100% + 10px); left: 50%; transform: translateX(-50%); min-width: 220px; max-width: 320px; padding: 14px 18px; background: var(--bg-card); border: 1px solid var(--color-warning); border-radius: 8px; font-size: 1.1em; font-weight: 400; color: var(--text-secondary); line-height: 1.6; visibility: hidden; opacity: 0; transition: var(--transition-fast); z-index: 1000; box-shadow: 0 4px 20px rgba(251, 191, 36, 0.2); text-align: left; }
.info-tooltip-ast .tooltip-content::after { content: ''; position: absolute; top: 100%; left: 50%; transform: translateX(-50%); border: 6px solid transparent; border-top-color: var(--color-warning); }
.info-tooltip-ast:hover .tooltip-content { visibility: visible; opacity: 1; }

/* Variant 4: Inline hint text (for theme/language) */
.info-hint-inline { display: block; font-size: 1.1em; color: var(--text-muted); margin-top: 8px; font-style: italic; }

/* Variant 5: Collapsible info box */
.info-box-collapsible { margin-top: 12px; background: rgba(59, 130, 246, 0.08); border: 1px solid var(--color-info-border); border-radius: 6px; overflow: hidden; }
.info-box-header { display: flex; align-items: center; gap: 8px; padding: 10px 12px; cursor: pointer; transition: var(--transition-fast); }
.info-box-header:hover { background: rgba(59, 130, 246, 0.12); }
.info-box-icon { width: 18px; height: 18px; display: flex; align-items: center; justify-content: center; background: var(--color-info); color: #fff; border-radius: 50%; font-size: 11px; font-style: italic; font-weight: 600; font-family: Georgia, serif; }
.info-box-title { font-size: 1.1em; color: var(--color-info); font-weight: 500; }
.info-box-arrow { margin-left: auto; color: var(--color-info); font-size: 10px; transition: transform 0.2s; }
.info-box-header.open .info-box-arrow { transform: rotate(180deg); }
.info-box-content { padding: 0 12px 12px 12px; font-size: 1.1em; color: var(--text-secondary); line-height: 1.6; display: none; }
.info-box-content.open { display: block; }

/* Settings Row */
.settings-row { display: flex; gap: 20px; }
.settings-group { flex: 1; background: var(--bg-card); border: 1px solid var(--border-color); border-radius: 12px; padding: 20px; }
.settings-group h4 { font-size: 1em; font-weight: 500; color: var(--text-primary); margin-bottom: 15px; letter-spacing: 0.3px; }

/* Language Selector */
.language-selector { display: flex; gap: 10px; }
.lang-btn { flex: 1; padding: 12px 15px; background: var(--bg-card); border: 2px solid var(--border-color); border-radius: var(--radius-lg); color: var(--text-primary); font-size: 0.95em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); }
.lang-btn:hover { border-color: var(--border-light); background: var(--hover-bg); }
.lang-btn.active { border-color: var(--text-primary); background: var(--hover-bg); }

/* Current Mode Display */
.current-mode-display { text-align: center; padding: 20px; }
.mode-indicator { display: flex; align-items: center; justify-content: center; gap: 12px; margin-bottom: 15px; }
.mode-icon { font-size: 2em; color: var(--text-primary); }
.mode-text { font-size: 1.3em; font-weight: 500; color: var(--text-primary); letter-spacing: 0.5px; }
.mode-hint { font-size: 0.85em; color: var(--text-secondary); line-height: 1.6; }

/* Info Section */
.info-content h2 { font-size: 1.6em; font-weight: 400; text-align: center; color: var(--text-primary); margin-bottom: 50px; letter-spacing: 0.5px; }
.info-section { margin-bottom: 45px; padding-bottom: 35px; border-bottom: 1px solid rgba(255, 255, 255, 0.06); transition: var(--transition-smooth); }
.info-section:last-of-type { border-bottom: none; }
.info-section:hover { transform: translateX(5px); }
.info-section h3 { font-size: 1.15em; font-weight: 500; color: var(--text-primary); margin-bottom: 18px; letter-spacing: 0.3px; }
.info-section p { font-size: 0.92em; line-height: 1.9; color: var(--text-secondary); margin-bottom: 14px; transition: color 0.3s ease; letter-spacing: 0.2px; }
.info-section:hover p { color: rgba(255, 255, 255, 0.85); }
.info-section p strong { color: var(--text-primary); font-weight: 500; }
.info-note { font-style: italic; color: var(--text-muted) !important; font-size: 1.1em !important; }
.info-note-warning { background: rgba(255, 152, 0, 0.15) !important; border-left: 3px solid rgb(255, 152, 0) !important; color: rgb(255, 152, 0) !important; padding: 14px 18px !important; border-radius: var(--radius-sm) !important; font-style: normal !important; margin-top: 15px !important; font-size: 1.1em !important; }

/* Info Footer */
.info-footer { margin-top: 60px; padding-top: 40px; border-top: 1px solid var(--border-color); text-align: center; }
.info-footer h3 { font-size: 1.1em; font-weight: 400; color: var(--text-primary); margin-bottom: 18px; letter-spacing: 0.3px; }
.info-footer p { font-size: 0.88em; color: var(--text-secondary); margin-bottom: 30px; letter-spacing: 0.2px; }
.info-footer .button-group { display: flex; justify-content: center; gap: 20px; flex-wrap: wrap; }
.info-button { padding: 14px 45px; background: transparent; color: var(--text-muted); text-decoration: none; font-weight: 400; letter-spacing: 0.5px; font-size: 0.9em; border: 1px solid rgba(255,255,255,0.15); border-radius: 4px; transition: var(--transition-smooth); }
.info-button:hover { color: var(--text-primary); border-color: rgba(255,255,255,0.3); }

/* Status Card */
.status-card { background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-lg); padding: 20px 25px; margin-bottom: 25px; }
.status-row { display: grid; grid-template-columns: repeat(3, 1fr); gap: 0; }
.status-item { display: flex; flex-direction: column; gap: 8px; text-align: center; padding: 0 15px; border-right: 1px solid var(--border-light); }
.status-item:last-child { border-right: none; }
.status-label { font-size: 0.6em; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.8px; font-weight: 500; }
.status-value { font-size: 0.88em; color: var(--text-primary); font-weight: 400; }
.status-mdns { font-size: 0.68em; color: var(--text-secondary); margin-top: 2px; }

@media (max-width: 600px) {
    .status-row { grid-template-columns: 1fr; gap: 20px; }
    .status-item { border-right: none; border-bottom: 1px solid var(--border-light); padding-bottom: 15px; }
    .status-item:last-child { border-bottom: none; padding-bottom: 0; }
}

/* Notification */
.notification { position: relative; padding: 18px 50px 18px 20px; margin-bottom: 25px; border-radius: var(--radius-lg); display: flex; align-items: center; justify-content: space-between; animation: slideDown 0.4s cubic-bezier(0.4, 0, 0.2, 1); box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3); }
@keyframes slideDown { from { opacity: 0; transform: translateY(-20px); } to { opacity: 1; transform: translateY(0); } }
.notification.success { background: var(--color-success-bg); border: 1px solid var(--color-success-border); }
.notification.error { background: var(--color-error-bg); border: 1px solid var(--color-error-border); }
.notification.info { background: var(--color-info-bg); border: 1px solid var(--color-info-border); }
.notification-content { display: flex; align-items: center; gap: 15px; flex: 1; }
.notification-icon { font-size: 1.3em; font-weight: bold; }
.notification.success .notification-icon { color: var(--color-success); }
.notification.success .notification-icon::before { content: "[OK]"; }
.notification.error .notification-icon { color: var(--color-error); }
.notification.error .notification-icon::before { content: "[!]"; }
.notification.info .notification-icon { color: var(--color-info); }
.notification.info .notification-icon::before { content: "[i]"; }
.notification-message { color: var(--text-primary); font-size: 0.95em; line-height: 1.5; }
.notification-close { position: absolute; top: 8px; right: 12px; background: transparent; border: none; color: var(--text-secondary); font-size: 1.8em; cursor: pointer; padding: 0; width: 30px; height: 30px; display: flex; align-items: center; justify-content: center; transition: var(--transition-fast); line-height: 1; }
.notification-close:hover { color: var(--text-primary); transform: scale(1.2); }

/* Accordion */
.accordion { background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-lg); margin-bottom: 20px; overflow: hidden; transition: var(--transition-smooth); }
.accordion:hover { border-color: var(--border-light); }
.accordion-header { display: flex; justify-content: space-between; align-items: center; padding: 20px 25px; cursor: pointer; user-select: none; transition: background 0.3s ease; gap: 15px; }
.accordion-header:hover { background: var(--card-hover); }
.accordion-title-text { flex: 1; font-size: 1.05em; font-weight: 500; color: var(--text-primary); letter-spacing: 0.5px; }
.accordion-title { display: flex; align-items: center; gap: 15px; font-size: 1.05em; font-weight: 500; color: var(--text-primary); letter-spacing: 0.5px; }
.badge { padding: 5px 15px; border-radius: var(--radius-sm); font-size: 0.75em; font-weight: 500; letter-spacing: 0.5px; white-space: nowrap; }
.badge-passive { background: var(--hover-bg); color: var(--text-secondary); }
.badge-info { background: var(--color-info-bg-light); color: var(--color-info); }
.badge-connected { background: var(--color-success-bg-light); color: var(--color-success); }
.badge-not-configured { background: var(--card-hover); color: var(--text-muted); }
.accordion-icon { font-size: 0.8em; color: var(--text-muted); transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1); }
.accordion-header.active .accordion-icon { transform: rotate(180deg); }
.accordion-content { max-height: 0; overflow: hidden; transition: max-height 0.4s cubic-bezier(0.4, 0, 0.2, 1); padding: 0 25px; }
.accordion-header.active + .accordion-content { max-height: 5000px; padding: 15px 25px 25px 25px; overflow-y: auto; }

/* Form Group */
.form-group { margin-bottom: 12px; }
.form-group label { display: block; margin-bottom: 8px; font-size: 0.9em; font-weight: 500; color: var(--text-primary); letter-spacing: 0.3px; }
.form-group input[type="text"], .form-group input[type="password"] { width: 100%; padding: 10px 12px; background: var(--input-bg); border: 1px solid var(--border-color); border-radius: var(--radius-md); color: var(--text-primary); font-size: 0.9em; transition: var(--transition-normal); }
.form-group input:focus { outline: none; background: var(--input-bg-focus); border-color: var(--border-light); }
.form-group input::placeholder { color: var(--text-muted); }
.input-readonly { background: var(--bg-card) !important; color: var(--text-muted) !important; cursor: not-allowed; }
.form-hint { display: block; margin-top: 6px; font-size: 0.8em; color: var(--text-muted); font-style: italic; }
.form-note { margin-top: 20px; padding: 15px; background: var(--bg-card); border-left: 3px solid var(--border-light); border-radius: var(--radius-sm); font-size: 0.85em; line-height: 1.7; color: var(--text-secondary); }
.form-note strong { color: var(--text-primary); display: block; margin-bottom: 8px; }
.save-button-container { margin-top: 35px; text-align: center; padding-bottom: 20px; }
.save-button { padding: 15px 50px; background: transparent; color: var(--text-primary); border: 2px solid var(--border-light); border-radius: var(--radius-md); font-size: 1em; font-weight: 500; letter-spacing: 0.5px; cursor: pointer; transition: var(--transition-smooth); }
.save-button:hover { background: var(--text-primary); color: var(--bg-primary); border-color: var(--text-primary); transform: translateY(-3px); box-shadow: 0 10px 25px var(--shadow-hover); }

/* AP Mode Compact Styles */
.ap-info-text { font-size: 0.9em; line-height: 1.6; color: var(--text-secondary); margin-bottom: 20px; padding-bottom: 20px; border-bottom: 1px solid var(--border-color); }
.ap-details { margin-bottom: 20px; }
.ap-detail-row { display: flex; justify-content: space-between; align-items: center; padding: 10px 0; border-bottom: 1px solid var(--card-hover); }
.ap-detail-row:last-child { border-bottom: none; }
.ap-label { font-size: 0.85em; color: var(--text-muted); font-weight: 500; }
.ap-value { font-size: 0.9em; color: var(--text-primary); font-family: monospace; }
.ap-steps { background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-md); padding: 15px; }
.ap-step-title { font-size: 0.85em; color: var(--text-secondary); margin-bottom: 12px; text-align: center; }
.ap-step-flow { display: flex; align-items: center; justify-content: center; gap: 8px; flex-wrap: wrap; }
.ap-step { font-size: 0.85em; color: var(--text-secondary); padding: 6px 12px; background: var(--bg-card); border-radius: var(--radius-sm); }
.ap-step-active { color: var(--color-info); background: var(--color-info-bg); border: 1px solid var(--color-info-border); }
.ap-arrow { color: var(--text-muted); font-size: 0.9em; }

/* Mode Info Text */
.mode-info-text { font-size: 0.9em; line-height: 1.6; color: var(--text-secondary); margin-bottom: 25px; padding: 15px; background: var(--bg-card); border-left: 3px solid var(--border-light); border-radius: var(--radius-sm); }

/* Safe Tabs - İyileştirilmiş */
.safe-tabs { display: flex; gap: 6px; margin-bottom: 20px; padding: 6px; background: var(--bg-card); border-radius: var(--radius-lg); border: 1px solid var(--border-color); }
.safe-tab { flex: 1; padding: 10px 8px; background: transparent; border: none; border-radius: var(--radius-md); color: var(--text-muted); font-size: 0.85em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); text-align: center; }
.safe-tab:hover { color: var(--text-secondary); background: var(--hover-bg); }
.safe-tab.active { color: #fff; background: linear-gradient(135deg, #10b981 0%, #059669 100%); box-shadow: 0 4px 12px rgba(16, 185, 129, 0.3); }
.safe-tab-content, .quick-safe-tab-content { display: none; animation: fadeIn 0.3s ease; }
.safe-tab-content.active, .quick-safe-tab-content.active { display: block; }
@keyframes fadeIn { from { opacity: 0; transform: translateY(5px); } to { opacity: 1; transform: translateY(0); } }

/* Toggle Switch - İyileştirilmiş */
.safe-toggle-row { display: flex; align-items: center; gap: 15px; margin-bottom: 20px; padding: 15px; background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-md); }
.toggle-switch { position: relative; width: 50px; height: 26px; display: inline-block; flex-shrink: 0; }
.toggle-switch input { opacity: 0; width: 0; height: 0; }
.toggle-slider { position: absolute; cursor: pointer; top: 0; left: 0; right: 0; bottom: 0; background-color: var(--hover-bg); transition: 0.3s; border-radius: 26px; border: 1px solid var(--border-light); }
.toggle-slider:before { position: absolute; content: ""; height: 18px; width: 18px; left: 4px; bottom: 3px; background-color: var(--text-secondary); transition: 0.3s; border-radius: 50%; }
.toggle-switch input:checked + .toggle-slider { background-color: rgba(16, 185, 129, 0.3); border-color: #10b981; }
.toggle-switch input:checked + .toggle-slider:before { transform: translateX(24px); background-color: #10b981; }
.toggle-label { font-size: 0.95em; color: var(--text-primary); font-weight: 500; }

/* Safe Config Section - İyileştirilmiş */
.safe-config-section { background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-lg); padding: 20px; margin-bottom: 0; }
.safe-config-section .form-group { margin-bottom: 15px; }
.safe-config-section .form-group:last-child { margin-bottom: 0; }
.safe-config-section .form-group label { font-size: 0.85em; color: var(--text-secondary); margin-bottom: 8px; display: block; font-weight: 500; }
.safe-config-section .form-group input,
.safe-config-section .form-group select { padding: 12px 14px; background: var(--input-bg); border: 1px solid var(--border-color); border-radius: var(--radius-md); color: var(--text-primary); font-size: 0.9em; width: 100%; transition: var(--transition-normal); }
.safe-config-section .form-group input:focus,
.safe-config-section .form-group select:focus { outline: none; border-color: #10b981; background: var(--input-bg-focus); }
.safe-config-section .form-group input::placeholder { color: var(--text-muted); }
.form-hint-inline { display: block; font-size: 0.8em; color: var(--text-muted); margin-bottom: 8px; }

/* API Details Accordion - İyileştirilmiş */
.api-details { margin-top: 15px; border: 1px solid var(--border-color); border-radius: var(--radius-md); overflow: hidden; background: var(--bg-secondary); }
.api-details summary { padding: 14px 16px; cursor: pointer; font-weight: 600; font-size: 0.9em; color: var(--text-primary); background: linear-gradient(135deg, var(--bg-card) 0%, var(--bg-secondary) 100%); display: flex; align-items: center; gap: 10px; list-style: none; transition: var(--transition-normal); }
.api-details summary::-webkit-details-marker { display: none; }
.api-details summary::before { content: "▶"; font-size: 0.7em; transition: transform 0.3s ease; color: var(--text-muted); }
.api-details[open] summary::before { transform: rotate(90deg); color: #10b981; }
.api-details summary:hover { background: var(--hover-bg); }
.api-details[open] summary { border-bottom: 1px solid var(--border-color); background: var(--bg-card); }
.api-details .form-group { padding: 12px 16px 0 16px; margin: 0; }
.api-details .form-row { padding: 12px 16px 0 16px; display: flex; gap: 12px; margin: 0; }
.api-details .form-group.half { flex: 1; padding: 0; min-width: 0; }
.api-details .form-group:last-child { padding-bottom: 16px; }
.api-details .form-group label { font-size: 0.8em; color: var(--text-secondary); margin-bottom: 6px; display: block; font-weight: 500; }
.api-details .form-group input,
.api-details .form-group select,
.api-details .form-group textarea { padding: 10px 12px; background: var(--input-bg); border: 1px solid var(--border-color); border-radius: var(--radius-sm); color: var(--text-primary); font-size: 0.85em; width: 100%; transition: var(--transition-normal); }
.api-details .form-group input:focus,
.api-details .form-group select:focus,
.api-details .form-group textarea:focus { outline: none; border-color: #10b981; background: var(--input-bg-focus); }
.api-details textarea { min-height: 60px; resize: vertical; font-family: 'SF Mono', Monaco, 'Cascadia Code', monospace; font-size: 0.85em; line-height: 1.5; }
.api-status-badge { font-size: 0.75em; color: #10b981; font-weight: 600; margin-left: auto; padding: 4px 10px; background: rgba(16, 185, 129, 0.15); border-radius: 20px; }

/* Form Actions - İyileştirilmiş */
.form-actions { display: flex; gap: 12px; margin-top: 20px; padding-top: 20px; border-top: 1px solid var(--border-color); flex-wrap: wrap; }
.btn-save, .btn-test, .btn-teach { flex: 1; padding: 14px 20px; border-radius: var(--radius-md); font-size: 0.9em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); border: none; display: flex; align-items: center; justify-content: center; gap: 8px; min-width: 80px; }
.btn-save { background: linear-gradient(135deg, #fff 0%, #f0f0f0 100%); color: #0a0a0f; box-shadow: 0 4px 12px rgba(255, 255, 255, 0.15); }
.btn-save:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(255, 255, 255, 0.25); }
.btn-save:active { transform: translateY(0); }
.btn-teach { background: linear-gradient(135deg, #f59e0b 0%, #d97706 100%); color: #fff; box-shadow: 0 4px 12px rgba(245, 158, 11, 0.3); }
.btn-teach:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(245, 158, 11, 0.4); }
.btn-teach:active { transform: translateY(0); }
.btn-test { background: var(--input-bg-focus); color: var(--text-secondary); border: 1px solid var(--border-light); }
.btn-test:hover { background: var(--hover-bg); color: var(--text-primary); border-color: #10b981; }
.btn-test:active { transform: scale(0.98); }

/* Teaching Mode Overlay */
.teaching-overlay { position: absolute; top: 0; left: 0; right: 0; bottom: 0; background: rgba(0, 0, 0, 0.9); display: flex; align-items: center; justify-content: center; border-radius: var(--radius-lg); z-index: 100; animation: fadeIn 0.3s ease; }
.teaching-content { text-align: center; padding: 30px; }
.teaching-icon { margin-bottom: 15px; animation: pulse 1.5s infinite; color: #f59e0b; }
.teaching-icon svg { width: 48px; height: 48px; }
@keyframes pulse { 0%, 100% { transform: scale(1); opacity: 1; } 50% { transform: scale(1.1); opacity: 0.8; } }
.teaching-title { font-size: 1.3em; font-weight: 700; color: #f59e0b; margin-bottom: 15px; }
.teaching-pattern { font-size: 1.6em; font-weight: 700; color: #10b981; font-family: 'SF Mono', Monaco, 'Cascadia Code', monospace; padding: 15px 25px; background: rgba(16, 185, 129, 0.15); border: 2px solid rgba(16, 185, 129, 0.3); border-radius: var(--radius-md); margin-bottom: 15px; min-width: 150px; min-height: 30px; display: inline-block; }
.teaching-hint { font-size: 0.9em; color: var(--text-muted); margin-bottom: 15px; }
.teaching-timer { font-size: 2em; font-weight: 700; color: #f59e0b; margin-bottom: 20px; }
.btn-cancel-teach { padding: 12px 30px; background: rgba(239, 68, 68, 0.2); color: #ef4444; border: 1px solid rgba(239, 68, 68, 0.4); border-radius: var(--radius-md); font-size: 0.9em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); }
.btn-cancel-teach:hover { background: rgba(239, 68, 68, 0.3); border-color: #ef4444; }

/* Quick Safe Tab Content - Relative for overlay */
.quick-safe-tab-content { position: relative; }

/* Mobile Responsive */
@media (max-width: 600px) {
    body { padding: 0; align-items: flex-start; }
    .header { padding: 30px 20px 25px 20px; }
    .header h1 { font-size: 1.5em; letter-spacing: 0.3px; }
    .theme-toggle { top: 15px; right: 15px; padding: 4px; }
    .theme-btn { padding: 6px 12px; font-size: 0.8em; }
    .info-box { gap: 20px; flex-direction: column; align-items: center; margin-top: 15px; }
    .info-single { font-size: 0.85em; justify-content: center; }
    .tabs { flex-wrap: wrap; padding: 0; }
    .tab { flex: 1 1 50%; font-size: 0.8em; padding: 16px 10px; border-right: none; border-bottom: 1px solid var(--border-color); }
    .tab:nth-child(odd) { border-right: 1px solid var(--border-color); }
    .tab::after { width: 40%; }
    .tab-content { padding: 25px 20px; min-height: auto; }
    .info-content { padding: 0; }
    .info-content h2 { font-size: 1.2em; margin-bottom: 30px; }
    .info-section { margin-bottom: 35px; padding-bottom: 25px; }
    .info-section h3 { font-size: 1em; }
    .info-section p { font-size: 0.88em; line-height: 1.7; }
    .button-group { flex-direction: column; gap: 12px; }
    .info-button { width: 100%; text-align: center; padding: 12px 30px; }
    .info-footer { margin-top: 40px; padding-top: 30px; }
    .info-footer h3 { font-size: 1em; }
    .ap-step-flow { flex-direction: column; align-items: stretch; }
    .ap-arrow { transform: rotate(90deg); text-align: center; }
    /* Mode buttons mobile */
    .mode-buttons { flex-direction: column; gap: 12px; }
    .mode-btn { padding: 20px 15px; }
    .mode-btn-text { font-size: 1em; }
    /* Settings row mobile */
    .settings-row { flex-direction: column; gap: 15px; }
    .settings-group { padding: 15px; }
    /* Form groups mobile */
    .form-group label { font-size: 0.85em; }
    .form-group input { padding: 12px; font-size: 0.9em; }
    .form-actions { flex-direction: column; gap: 10px; }
    .btn-save, .btn-test, .btn-teach { padding: 14px 20px; }
    /* Teaching overlay mobile */
    .teaching-content { padding: 20px; }
    .teaching-icon { font-size: 2.5em; }
    .teaching-title { font-size: 1.1em; }
    .teaching-pattern { font-size: 1.3em; padding: 12px 20px; }
    .teaching-timer { font-size: 1.6em; }
    /* Safe Mode mobile */
    .safe-tabs { gap: 4px; padding: 5px; flex-wrap: wrap; }
    .safe-tab { flex: 1 1 30%; min-width: 0; padding: 8px 4px; font-size: 0.75em; }
    .safe-toggle-row { padding: 12px; gap: 10px; flex-wrap: wrap; }
    .safe-config-section { padding: 15px; }
    .api-details .form-row { flex-direction: column; gap: 0; }
    .api-details .form-group.half { padding: 12px 16px 0 16px; }
    .api-details .form-group.half:last-child { padding-bottom: 0; }
    /* Accordion mobile */
    .accordion-header { padding: 15px 18px; }
    .accordion-title-text { font-size: 0.95em; }
    .accordion-content { padding: 0 18px; }
    .accordion-header.active + .accordion-content { padding: 15px 18px 18px 18px; }
    /* OTA mobile */
    .ota-section h2 { font-size: 1.3em; margin-bottom: 20px; }
    .ota-version-card { padding: 15px; }
    .ota-version-value { font-size: 1.5em; }
    .ota-actions { flex-direction: column; gap: 10px; }
    .btn-ota-check, .btn-ota-update { padding: 14px 20px; }
    /* Safe tabs mobile */
    .safe-tabs { gap: 5px; }
    .safe-tab { padding: 12px 10px; font-size: 0.85em; }
    .safe-config-section { padding: 15px; }
    .safe-toggle-row { gap: 10px; }
    .toggle-label { font-size: 0.9em; }
    /* System buttons mobile */
    .system-buttons { flex-direction: column; gap: 12px; }
    .btn-restart, .btn-factory-reset { width: 100%; padding: 14px 20px; }
    .factory-reset-confirm { padding: 12px; }
    .factory-reset-confirm input { width: 100%; margin-bottom: 10px; margin-right: 0; }
    .btn-confirm-reset, .btn-cancel-reset { width: 100%; margin-right: 0; margin-bottom: 8px; }
    /* Notification mobile */
    .notification { padding: 15px 40px 15px 15px; }
    .notification-message { font-size: 0.9em; }
    .notification-close { top: 5px; right: 8px; font-size: 1.5em; }
}

/* OTA Update Styles */
.ota-section { margin-bottom: 50px; }
.ota-section h2 { font-size: 1.6em; font-weight: 400; text-align: center; color: #ffffff; margin-bottom: 30px; letter-spacing: 0.5px; }
.ota-version-card { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 20px; margin-bottom: 20px; text-align: center; }
.ota-version-label { font-size: 0.85em; color: rgba(255, 255, 255, 0.5); margin-bottom: 10px; text-transform: uppercase; letter-spacing: 1px; }
.ota-version-value { font-size: 1.8em; color: #ffffff; font-weight: 500; letter-spacing: 1px; }
.ota-update-card { background: rgba(59, 130, 246, 0.1); border: 1px solid rgba(59, 130, 246, 0.3); border-radius: 8px; padding: 20px; margin-bottom: 20px; }
.ota-update-header { display: flex; align-items: center; gap: 10px; margin-bottom: 15px; }
.ota-update-icon { font-size: 1.5em; color: rgb(59, 130, 246); }
.ota-update-title { font-size: 1.1em; color: rgb(59, 130, 246); font-weight: 500; }
.ota-update-version { font-size: 1.4em; color: #ffffff; font-weight: 500; margin-bottom: 8px; }
.ota-update-date { font-size: 0.85em; color: rgba(255, 255, 255, 0.5); margin-bottom: 15px; }
.ota-release-notes { font-size: 1.1em; color: rgba(255, 255, 255, 0.7); line-height: 1.6; padding: 18px; background: rgba(0, 0, 0, 0.2); border-radius: 6px; max-height: 200px; overflow-y: auto; white-space: pre-wrap; }
.ota-toggle-row { display: flex; align-items: center; gap: 15px; margin-bottom: 15px; }
.ota-auto-info { font-size: 1.1em; color: rgba(255, 255, 255, 0.5); line-height: 1.6; margin-bottom: 25px; padding: 14px 18px; background: rgba(255, 255, 255, 0.03); border-left: 3px solid rgba(255, 255, 255, 0.2); border-radius: 4px; }
.ota-actions { display: flex; gap: 15px; margin-bottom: 20px; }
.btn-ota-check, .btn-ota-update { flex: 1; padding: 14px 25px; border-radius: var(--radius-md); font-size: 0.95em; font-weight: 500; cursor: pointer; transition: var(--transition-normal); border: none; }
.btn-ota-check { background: var(--input-bg-focus); color: var(--text-secondary); border: 1px solid var(--border-light); }
.btn-ota-check:hover { background: var(--hover-bg); color: var(--text-primary); border-color: var(--border-light); }
.btn-ota-update { background: var(--color-info); color: var(--text-primary); }
.btn-ota-update:hover { background: var(--color-info-dark); transform: translateY(-2px); }
.ota-progress-container { margin-top: 20px; padding: 20px; background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; }
.ota-progress-label { font-size: 0.9em; color: rgba(255, 255, 255, 0.7); margin-bottom: 10px; text-align: center; }
.ota-progress-bar { width: 100%; height: 8px; background: rgba(255, 255, 255, 0.1); border-radius: 4px; overflow: hidden; margin-bottom: 10px; }
.ota-progress-fill { height: 100%; background: linear-gradient(90deg, rgb(59, 130, 246), rgb(37, 99, 235)); width: 0%; transition: width 0.3s ease; border-radius: 4px; }
.ota-progress-percent { font-size: 1.2em; color: #ffffff; font-weight: 500; text-align: center; }
.ota-status-message { margin-top: 20px; padding: 15px 20px; border-radius: var(--radius-md); font-size: 0.9em; text-align: center; }
.ota-status-message.success { background: var(--color-success-bg); border: 1px solid var(--color-success-border); color: var(--color-success); }
.ota-status-message.error { background: var(--color-error-bg); border: 1px solid var(--color-error-border); color: var(--color-error); }
.ota-status-message.info { background: var(--color-info-bg); border: 1px solid var(--color-info-border); color: var(--color-info); }
.info-divider { height: 1px; background: rgba(255, 255, 255, 0.1); margin: 50px 0; }
.section-divider { height: 1px; background: var(--border-light); margin: 45px 0; }

/* OTA Quick Section */
.ota-quick-section { padding: 20px; background: rgba(255, 255, 255, 0.03); border: 1px solid var(--border-light); border-radius: 8px; }
.ota-quick-header { display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 15px; }
.ota-quick-info { display: flex; flex-direction: column; gap: 4px; }
.ota-quick-label { font-size: 0.75em; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px; display: flex; align-items: center; gap: 6px; }
.ota-quick-version { font-size: 1.1em; color: var(--text-primary); font-weight: 600; font-family: monospace; }
.btn-check-update { padding: 10px 20px; background: var(--primary); color: #fff; border: none; border-radius: 6px; cursor: pointer; font-size: 0.9em; font-weight: 500; transition: all 0.2s; }
.btn-check-update:hover { background: var(--primary-hover); transform: translateY(-1px); }
.ota-quick-status { margin-top: 15px; padding: 10px 15px; border-radius: 6px; font-size: 0.9em; }
.ota-quick-status.success { background: var(--color-success-bg); color: var(--color-success); border: 1px solid var(--color-success-border); }
.ota-quick-status.error { background: var(--color-error-bg); color: var(--color-error); border: 1px solid var(--color-error-border); }
.ota-quick-status.info { background: var(--color-info-bg); color: var(--color-info); border: 1px solid var(--color-info-border); }
.ota-update-available { margin-top: 15px; padding: 15px; background: rgba(76, 175, 80, 0.1); border: 1px solid rgba(76, 175, 80, 0.3); border-radius: 6px; display: flex; justify-content: space-between; align-items: center; flex-wrap: wrap; gap: 15px; }
.ota-new-version { display: flex; align-items: center; gap: 10px; }
.ota-new-version span:first-child { font-size: 0.9em; color: var(--text-secondary); }
.version-badge { background: var(--color-success); color: #fff; padding: 4px 12px; border-radius: 4px; font-size: 0.9em; font-weight: 600; font-family: monospace; }
.btn-ota-install { padding: 10px 25px; background: var(--color-success); color: #fff; border: none; border-radius: 6px; cursor: pointer; font-size: 0.9em; font-weight: 600; transition: all 0.2s; }
.btn-ota-install:hover { background: #43a047; transform: translateY(-1px); }
.ota-progress-container { margin-top: 15px; }
.ota-progress-bar { height: 8px; background: rgba(255, 255, 255, 0.1); border-radius: 4px; overflow: hidden; }
.ota-progress-fill { height: 100%; background: var(--primary); transition: width 0.3s; }
.ota-progress-text { display: block; text-align: center; margin-top: 8px; font-size: 0.85em; color: var(--text-muted); }

/* WiFi Connection Info */
.wifi-connection-info { margin: 30px 0; padding: 20px; background: rgba(255, 255, 255, 0.03); border: 1px solid var(--border-light); border-radius: 8px; overflow: visible; position: relative; }
.wifi-info-text { color: var(--text-primary); font-size: 1.1em; font-weight: 600; line-height: 1.6; margin: 0 0 15px 0; text-align: center; display: flex; align-items: center; justify-content: center; gap: 8px; }

/* AP Mode Card - Compact Design */
.ap-mode-card { background: linear-gradient(135deg, rgba(59, 130, 246, 0.08) 0%, rgba(139, 92, 246, 0.05) 100%); border: 1px solid rgba(59, 130, 246, 0.2); border-radius: 12px; padding: 0; overflow: visible; transition: all 0.3s ease; }
.ap-mode-card:hover { border-color: rgba(59, 130, 246, 0.35); }
.ap-mode-card.disabled { background: rgba(255, 255, 255, 0.02); border-color: rgba(255, 255, 255, 0.1); opacity: 0.7; }
.ap-mode-header { display: flex; justify-content: space-between; align-items: center; padding: 12px 16px; background: rgba(0, 0, 0, 0.15); border-radius: 12px 12px 0 0; position: relative; overflow: visible; }
.ap-mode-title-section { display: flex; align-items: center; gap: 10px; position: relative; overflow: visible; }
.ap-mode-title-text { display: flex; align-items: center; gap: 8px; position: relative; overflow: visible; }
.ap-mode-title-text h4 { margin: 0; font-size: 0.95em; font-weight: 600; color: var(--text-primary); }
.ap-mode-toggle-section { display: flex; align-items: center; gap: 8px; }
.ap-mode-status { font-size: 0.75em; font-weight: 500; color: var(--color-success); text-transform: uppercase; letter-spacing: 0.5px; }
.ap-mode-status.disabled { color: var(--text-muted); }
.ap-mode-credentials { padding: 10px 16px; }
.ap-credential-row { display: flex; justify-content: space-between; gap: 8px; flex-wrap: wrap; }
.ap-credential-item-compact { display: flex; flex-direction: column; gap: 1px; min-width: 80px; }
.ap-credential-label { font-size: 0.65em; color: var(--text-muted); text-transform: uppercase; letter-spacing: 0.5px; }
.ap-credential-value { font-size: 0.82em; color: var(--text-primary); font-family: 'SF Mono', 'Consolas', monospace; }
.ap-mode-warning { padding: 12px 18px; background: rgba(251, 191, 36, 0.1); border-top: 1px solid rgba(251, 191, 36, 0.2); font-size: 1.1em; color: var(--color-warning); display: flex; align-items: center; justify-content: center; gap: 8px; text-align: center; }
.ap-mode-warning .warning-icon { font-size: 1em; }
.wifi-accordion-first { margin-top: 15px; }
@media (max-width: 480px) { 
    .ap-credential-row { flex-direction: column; gap: 6px; }
    .ap-mode-header { flex-direction: column; gap: 10px; align-items: flex-start; }
    .ap-mode-toggle-section { width: 100%; justify-content: space-between; }
}

/* Dimmer Styles */
.dimmer-status-card { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 20px; margin-bottom: 25px; }
.dimmer-status-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 20px; padding-bottom: 15px; border-bottom: 1px solid rgba(255, 255, 255, 0.08); }
.dimmer-status-label { font-size: 0.75em; color: var(--text-muted); text-transform: uppercase; letter-spacing: 1px; font-weight: 500; }
.dimmer-status-badge { padding: 5px 15px; border-radius: var(--radius-sm); font-size: 0.75em; font-weight: 500; letter-spacing: 0.5px; background: var(--card-hover); color: var(--text-muted); }
.dimmer-status-badge.connected { background: var(--color-success-bg-light); color: var(--color-success); }
.dimmer-status-badge.error { background: var(--color-error-bg-light); color: var(--color-error); }
.dimmer-info-grid { display: grid; grid-template-columns: repeat(2, 1fr); gap: 20px; }
.dimmer-info-item { display: flex; flex-direction: column; gap: 8px; }
.dimmer-info-label { font-size: 0.8em; color: rgba(255, 255, 255, 0.5); text-transform: uppercase; letter-spacing: 0.5px; }
.dimmer-info-value { font-size: 1.1em; color: #ffffff; font-weight: 400; }
.dimmer-config-section { background: rgba(255, 255, 255, 0.02); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; padding: 15px; margin-bottom: 12px; }
.dimmer-section-title { font-size: 0.95em; font-weight: 500; color: #ffffff; margin-bottom: 12px; text-align: center; letter-spacing: 0.5px; }
.dimmer-calibration-info { font-size: 1.1em; color: rgba(255, 255, 255, 0.6); line-height: 1.6; margin-bottom: 20px; padding: 14px 18px; background: rgba(255, 255, 255, 0.03); border-left: 3px solid rgba(255, 255, 255, 0.2); border-radius: 4px; }
.dimmer-ratio-group { margin-bottom: 20px; }
.dimmer-ratio-group label { display: block; margin-bottom: 12px; font-size: 0.9em; font-weight: 500; color: #ffffff; text-align: center; }
.dimmer-ratio-buttons { display: grid; grid-template-columns: repeat(5, 1fr); gap: 10px; margin-bottom: 15px; }
.ratio-btn { padding: 12px; background: var(--card-hover); border: 1px solid var(--border-light); border-radius: var(--radius-md); color: var(--text-secondary); font-size: 1.1em; font-weight: 500; cursor: pointer; transition: var(--transition-normal); display: flex; flex-direction: column; align-items: center; gap: 5px; }
.ratio-btn:hover { background: var(--input-bg-focus); border-color: var(--border-light); color: var(--text-primary); }
.ratio-btn.active { background: var(--color-info); color: var(--text-primary); border-color: var(--color-info); }
.dimmer-ratio-display { text-align: center; padding: 10px; background: rgba(255, 255, 255, 0.03); border-radius: 6px; }
.ratio-label { font-size: 0.85em; color: rgba(255, 255, 255, 0.5); margin-right: 10px; }
.ratio-value { font-size: 1.3em; color: #ffffff; font-weight: 500; }

/* Compact Calibration Styles */
.dimmer-compact-section { padding: 15px; margin-bottom: 15px; }
.dimmer-calibration-info-compact { font-size: 1.1em; color: rgba(255, 255, 255, 0.6); line-height: 1.5; margin-bottom: 12px; padding: 12px 14px; background: rgba(255, 255, 255, 0.03); border-left: 2px solid rgba(255, 255, 255, 0.2); border-radius: 4px; }
.dimmer-ratio-buttons-compact { display: grid; grid-template-columns: repeat(5, 1fr); gap: 6px; margin-bottom: 10px; }
.ratio-btn-compact { padding: 8px 4px; background: var(--card-hover); border: 1px solid var(--border-light); border-radius: var(--radius-md); color: var(--text-secondary); font-size: 0.9em; cursor: pointer; transition: var(--transition-fast); display: flex; flex-direction: column; align-items: center; gap: 2px; }
.ratio-btn-compact .ratio-num { font-size: 1.2em; font-weight: 600; }
.ratio-desc-compact { font-size: 0.7em; opacity: 0.7; }
.ratio-btn-compact:hover { background: var(--input-bg-focus); border-color: var(--border-light); color: var(--text-primary); }
.ratio-btn-compact.active { background: var(--color-info); color: var(--text-primary); border-color: var(--color-info); }
.dimmer-ratio-display-compact { text-align: center; padding: 6px; background: rgba(255, 255, 255, 0.03); border-radius: 4px; font-size: 0.85em; color: rgba(255, 255, 255, 0.6); margin-bottom: 10px; }
.ratio-value-compact { font-size: 1.2em; color: #ffffff; font-weight: 600; margin-left: 6px; }
.btn-compact { padding: 10px 20px; font-size: 0.9em; }
.dimmer-scan-info { font-size: 1.1em; color: rgba(255, 255, 255, 0.6); line-height: 1.6; margin-bottom: 20px; padding: 14px 18px; background: rgba(255, 255, 255, 0.03); border-left: 3px solid rgba(255, 255, 255, 0.2); border-radius: 4px; }
.dimmer-scan-progress { margin-top: 20px; padding: 15px; background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 6px; }
.scan-progress-text { font-size: 0.9em; color: rgba(255, 255, 255, 0.7); margin-bottom: 10px; text-align: center; }
.scan-progress-bar { width: 100%; height: 6px; background: rgba(255, 255, 255, 0.1); border-radius: 3px; overflow: hidden; }
.scan-progress-fill { height: 100%; background: linear-gradient(90deg, rgb(59, 130, 246), rgb(37, 99, 235)); width: 0%; animation: scanProgress 2s ease-in-out infinite; border-radius: 3px; }
@keyframes scanProgress { 0% { width: 0%; } 50% { width: 100%; } 100% { width: 0%; } }
.dimmer-devices-list { margin-top: 20px; }
.devices-list-title { font-size: 0.9em; color: rgba(255, 255, 255, 0.7); margin-bottom: 15px; text-align: center; }
.scanned-device-item { background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-md); padding: 15px; margin-bottom: 10px; display: flex; justify-content: space-between; align-items: center; transition: var(--transition-normal); }
.scanned-device-item:hover { background: var(--card-hover); border-color: var(--border-light); }
.scanned-device-info { flex: 1; }
.scanned-device-ip { font-size: 1em; color: var(--text-primary); font-weight: 500; margin-bottom: 5px; }
.scanned-device-type { font-size: 0.8em; color: var(--text-muted); }
.scanned-device-connect { padding: 8px 20px; background: var(--color-info); color: var(--text-primary); border: none; border-radius: var(--radius-sm); font-size: 0.85em; font-weight: 500; cursor: pointer; transition: var(--transition-normal); }
.scanned-device-connect:hover { background: var(--color-info-dark); transform: translateY(-2px); }

/* Dimmer Brightness Section */
.dimmer-brightness-section { margin-top: 20px; padding-top: 20px; border-top: 1px solid rgba(255, 255, 255, 0.08); }
.brightness-header { display: flex; justify-content: space-between; align-items: center; margin-bottom: 12px; }
.brightness-label { font-size: 0.75em; color: rgba(255, 255, 255, 0.5); text-transform: uppercase; letter-spacing: 1px; font-weight: 500; }
.brightness-value { font-size: 1.5em; color: #ffffff; font-weight: 600; font-family: 'Courier New', monospace; }
.brightness-bar-container { margin-bottom: 12px; }
.brightness-bar-bg { width: 100%; height: 24px; background: rgba(255, 255, 255, 0.1); border-radius: 12px; overflow: hidden; position: relative; }
.brightness-bar-fill { height: 100%; background: linear-gradient(90deg, rgb(251, 191, 36), rgb(245, 158, 11)); border-radius: 12px; transition: width 0.3s ease; box-shadow: 0 0 10px rgba(251, 191, 36, 0.5); }
.brightness-status-row { display: flex; justify-content: space-between; align-items: center; font-size: 0.8em; }
.power-status { color: rgba(255, 255, 255, 0.6); font-weight: 500; }
.power-status.on { color: rgb(16, 185, 129); }
.control-hint { color: rgba(255, 255, 255, 0.4); font-style: italic; }

/* Connection Status Inline */
.connection-status-inline { display: flex; align-items: center; gap: 10px; padding: 12px 15px; background: rgba(255, 255, 255, 0.03); border-radius: 6px; margin-bottom: 15px; }
.status-dot { width: 10px; height: 10px; border-radius: 50%; background: var(--border-light); animation: statusPulse 2s ease-in-out infinite; }
.status-dot.connected { background: var(--color-success); }
.status-dot.connecting { background: var(--color-warning); }
.status-dot.error { background: var(--color-error); }
@keyframes statusPulse { 0%, 100% { opacity: 1; } 50% { opacity: 0.5; } }
.status-text { font-size: 0.85em; color: var(--text-secondary); }

/* Input with Button */
.input-with-button { display: flex; gap: 10px; }
.input-with-button input { flex: 1; }
.btn-inline-scan { padding: 12px 20px; background: rgba(255, 255, 255, 0.08); color: rgba(255, 255, 255, 0.7); border: 1px solid rgba(255, 255, 255, 0.15); border-radius: 4px; font-size: 1.2em; cursor: pointer; transition: all 0.3s ease; }
.btn-inline-scan:hover { background: rgba(255, 255, 255, 0.12); border-color: rgba(255, 255, 255, 0.25); transform: scale(1.05); }

/* Button Icons */
.btn-icon { margin-right: 8px; font-size: 1.1em; }
.ratio-num { font-size: 1.2em; font-weight: 600; }
.ratio-desc { font-size: 0.7em; opacity: 0.7; }

/* Scan Button */
.btn-scan { width: 100%; justify-content: center; font-size: 1em; }
.scan-icon { display: inline-block; animation: rotate 2s linear infinite; }
@keyframes rotate { from { transform: rotate(0deg); } to { transform: rotate(360deg); } }

/* Devices Container */
.devices-container { max-height: 400px; overflow-y: auto; padding-right: 5px; }
.devices-container::-webkit-scrollbar { width: 6px; }
.devices-container::-webkit-scrollbar-track { background: rgba(255, 255, 255, 0.05); border-radius: 3px; }
.devices-container::-webkit-scrollbar-thumb { background: rgba(255, 255, 255, 0.2); border-radius: 3px; }
.devices-container::-webkit-scrollbar-thumb:hover { background: rgba(255, 255, 255, 0.3); }

/* Dimmer Responsive */
@media (max-width: 600px) {
    .dimmer-info-grid { grid-template-columns: 1fr; gap: 15px; }
    .dimmer-ratio-buttons { grid-template-columns: repeat(5, 1fr); gap: 8px; }
    .ratio-btn { padding: 10px; font-size: 1em; }
    .brightness-value { font-size: 1.2em; }
    .dimmer-status-bar { flex-direction: column; gap: 15px; }
    .status-bar-item { width: 100%; text-align: center; }
}

/* Dimmer Hero Display */
.dimmer-hero { text-align: center; padding: 30px 20px; background: radial-gradient(ellipse at center, rgba(59, 130, 246, 0.15) 0%, transparent 70%); border-radius: 8px; margin-bottom: 20px; }
.dimmer-hero.connected-on { background: radial-gradient(ellipse at center, rgba(16, 185, 129, 0.15) 0%, transparent 70%); }
.dimmer-hero.connected-off { background: radial-gradient(ellipse at center, rgba(59, 130, 246, 0.15) 0%, transparent 70%); }
.dimmer-hero.disconnected { background: radial-gradient(ellipse at center, rgba(239, 68, 68, 0.15) 0%, transparent 70%); }
.dimmer-brightness-display { font-size: 4em; font-weight: 200; color: #fff; line-height: 1; }
.dimmer-brightness-display span { font-size: 0.4em; color: rgba(255, 255, 255, 0.5); }
.dimmer-hero.connected-off .dimmer-brightness-display { color: rgba(255, 255, 255, 0.5); }
.dimmer-hero.disconnected .dimmer-brightness-display { color: rgba(255, 255, 255, 0.3); }
.dimmer-status-row { display: flex; justify-content: center; align-items: center; gap: 8px; margin-top: 10px; font-size: 0.9em; }
.dimmer-power-status { color: rgb(16, 185, 129); font-weight: 500; }
.dimmer-hero.connected-off .dimmer-power-status { color: rgba(255, 255, 255, 0.5); }
.dimmer-hero.disconnected .dimmer-power-status { color: rgba(255, 255, 255, 0.4); }
.dimmer-status-dot { color: rgba(255, 255, 255, 0.5); }
.dimmer-connection-status { color: rgb(16, 185, 129); }
.dimmer-hero.disconnected .dimmer-connection-status { color: rgb(239, 68, 68); }
.dimmer-ip-display { margin-top: 5px; color: rgba(255, 255, 255, 0.5); font-size: 0.85em; }
.dimmer-calibration-controls { display: flex; justify-content: center; align-items: center; gap: 15px; margin-top: 20px; }
.dimmer-cal-btn { width: 50px; height: 50px; border-radius: 50%; border: 2px solid rgba(255, 255, 255, 0.2); background: rgba(255, 255, 255, 0.03); color: #fff; font-size: 1.5em; cursor: pointer; transition: all 0.2s ease; display: flex; align-items: center; justify-content: center; }
.dimmer-cal-btn:hover:not(:disabled) { border-color: rgb(59, 130, 246); background: rgba(59, 130, 246, 0.15); }
.dimmer-cal-btn:disabled { opacity: 0.3; cursor: not-allowed; }
.dimmer-cal-center { text-align: center; }
.dimmer-cal-value { font-size: 2em; font-weight: 600; min-width: 50px; text-align: center; color: rgb(59, 130, 246); line-height: 1; }
.dimmer-cal-label { font-size: 0.75em; color: rgba(255, 255, 255, 0.5); text-transform: uppercase; letter-spacing: 0.5px; margin-top: 8px; }

/* Dimmer Compact Form */
.dimmer-compact-form { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 20px; }
.dimmer-compact-form .section-title { font-size: 0.85em; font-weight: 600; color: rgba(255, 255, 255, 0.6); margin: 0 0 12px 0; padding: 0; border: none; text-transform: uppercase; letter-spacing: 0.5px; }
.dimmer-section-title { font-size: 0.85em; font-weight: 600; color: rgba(255, 255, 255, 0.6); margin-bottom: 12px; text-transform: uppercase; letter-spacing: 0.5px; }
.dimmer-inline-form { display: flex; gap: 10px; margin-bottom: 15px; }
.dimmer-inline-form input { flex: 1; padding: 12px 15px; background: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 6px; color: #fff; font-size: 1em; }
.dimmer-inline-form input:focus { outline: none; border-color: rgba(59, 130, 246, 0.3); background: rgba(255, 255, 255, 0.08); }
.dimmer-inline-form input::placeholder { color: rgba(255, 255, 255, 0.3); }
.dimmer-inline-form button { padding: 10px 20px; border: none; border-radius: 6px; font-size: 0.9em; font-weight: 500; cursor: pointer; transition: all 0.2s ease; }
.dimmer-inline-form .btn-primary { background: rgb(59, 130, 246); color: #fff; }
.dimmer-inline-form .btn-primary:hover { background: rgb(37, 99, 235); }
.dimmer-inline-form .btn-secondary { background: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.1); color: #fff; }
.dimmer-inline-form .btn-secondary:hover { background: rgba(255, 255, 255, 0.1); }

/* ========================================
   SHUTTER HERO DISPLAY (Dimmer ile ayni yapi)
   ======================================== */
.shutter-hero { text-align: center; padding: 30px 20px; background: radial-gradient(ellipse at center, rgba(6, 182, 212, 0.15) 0%, transparent 70%); border-radius: 8px; margin-bottom: 20px; }
.shutter-hero.disconnected { background: radial-gradient(ellipse at center, rgba(239, 68, 68, 0.1) 0%, transparent 70%); }
.shutter-hero.moving-up { background: radial-gradient(ellipse at center, rgba(34, 197, 94, 0.18) 0%, transparent 70%); }
.shutter-hero.moving-down { background: radial-gradient(ellipse at center, rgba(251, 191, 36, 0.18) 0%, transparent 70%); }
.shutter-hero.open { background: radial-gradient(ellipse at center, rgba(16, 185, 129, 0.15) 0%, transparent 70%); }
.shutter-hero.closed { background: radial-gradient(ellipse at center, rgba(107, 114, 128, 0.15) 0%, transparent 70%); }
.shutter-position-display { font-size: 4em; font-weight: 200; color: #fff; line-height: 1; }
.shutter-position-display span { font-size: 0.4em; color: rgba(255, 255, 255, 0.5); }
.shutter-hero.disconnected .shutter-position-display { color: rgba(255, 255, 255, 0.3); }
.shutter-status-row { display: flex; justify-content: center; align-items: center; gap: 8px; margin-top: 10px; font-size: 0.9em; }
.shutter-movement-status { color: rgb(6, 182, 212); font-weight: 500; }
.shutter-hero.disconnected .shutter-movement-status { color: rgb(239, 68, 68); }
.shutter-hero.moving-up .shutter-movement-status { color: rgb(34, 197, 94); }
.shutter-hero.moving-down .shutter-movement-status { color: rgb(251, 191, 36); }
.shutter-hero.open .shutter-movement-status { color: rgb(16, 185, 129); }
.shutter-hero.closed .shutter-movement-status { color: rgb(156, 163, 175); }
.shutter-status-dot { color: rgba(255, 255, 255, 0.5); }
.shutter-hero.disconnected .shutter-status-dot { display: none; }
.shutter-connection-status { color: rgb(16, 185, 129); }
.shutter-ip-display { margin-top: 5px; color: rgba(255, 255, 255, 0.5); font-size: 0.85em; }
.shutter-hero.disconnected .shutter-ip-display { color: rgba(255, 255, 255, 0.3); }
.shutter-calibration-controls { display: flex; justify-content: center; align-items: center; gap: 15px; margin-top: 20px; }
.shutter-cal-btn { width: 50px; height: 50px; border-radius: 50%; border: 2px solid rgba(255, 255, 255, 0.2); background: rgba(255, 255, 255, 0.03); color: #fff; font-size: 1.5em; cursor: pointer; transition: all 0.2s ease; display: flex; align-items: center; justify-content: center; }
.shutter-cal-btn:hover:not(:disabled) { border-color: rgb(6, 182, 212); background: rgba(6, 182, 212, 0.15); }
.shutter-cal-btn:disabled { opacity: 0.3; cursor: not-allowed; }
.shutter-cal-center { text-align: center; }
.shutter-cal-value { font-size: 2em; font-weight: 600; min-width: 50px; text-align: center; color: rgb(6, 182, 212); line-height: 1; }
.shutter-hero.disconnected .shutter-cal-value { color: rgba(255, 255, 255, 0.3); }
.shutter-cal-label { font-size: 0.75em; color: rgba(255, 255, 255, 0.5); text-transform: uppercase; letter-spacing: 0.5px; margin-top: 8px; }
.shutter-compact-form { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 20px; }
.shutter-compact-form .section-title { font-size: 0.85em; font-weight: 600; color: rgba(255, 255, 255, 0.6); margin: 0 0 12px 0; padding: 0; border: none; text-transform: uppercase; letter-spacing: 0.5px; }
.shutter-inline-form { display: flex; gap: 10px; margin-bottom: 15px; }
.shutter-inline-form input { flex: 1; padding: 12px 15px; background: rgba(255, 255, 255, 0.05); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 6px; color: #fff; font-size: 1em; }
.shutter-inline-form input:focus { outline: none; border-color: rgba(6, 182, 212, 0.3); background: rgba(255, 255, 255, 0.08); }
.shutter-inline-form input::placeholder { color: rgba(255, 255, 255, 0.3); }
.shutter-warning { padding: 12px 16px; border-radius: 6px; font-size: 0.85em; line-height: 1.5; margin-top: 15px; background: rgba(251, 191, 36, 0.15); border: 1px solid rgba(251, 191, 36, 0.4); color: rgb(251, 191, 36); }
.shutter-warning .warning-icon { margin-right: 8px; }

/* Shutter Status Bar (legacy - can be removed later) */
.shutter-status-bar { background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.1); border-radius: 8px; padding: 20px 25px; margin-bottom: 25px; display: grid; grid-template-columns: repeat(4, 1fr); gap: 0; }
.shutter-info-text { padding: 12px 16px; border-radius: 6px; font-size: 0.85em; line-height: 1.5; margin-top: 15px; }
.shutter-info-text.warning { background: rgba(255, 193, 7, 0.15); border: 1px solid rgba(255, 193, 7, 0.4); color: #ffc107; }
.shutter-info-text .warning-icon { font-size: 1.1em; margin-right: 8px; }
.status-col { display: flex; flex-direction: column; gap: 8px; text-align: center; padding: 0 15px; border-right: 1px solid rgba(255, 255, 255, 0.15); }
.status-col:last-child { border-right: none; }
.status-col-label { font-size: 0.75em; color: rgba(255, 255, 255, 0.5); text-transform: uppercase; letter-spacing: 1px; font-weight: 500; }
.status-col-value { font-size: 1.1em; color: #ffffff; font-weight: 400; display: flex; flex-direction: column; align-items: center; gap: 3px; }
.status-power-text { font-size: 0.85em; color: rgba(255, 255, 255, 0.6); margin-top: 2px; }

/* Saved Devices List */
.saved-devices-list { display: flex; flex-direction: column; gap: 8px; }
.saved-device-empty { color: rgba(255, 255, 255, 0.4); font-size: 0.9em; text-align: center; padding: 15px; }
.saved-device-item { display: flex; align-items: center; justify-content: space-between; padding: 10px 15px; background: rgba(0, 0, 0, 0.2); border-radius: 8px; border: 1px solid rgba(255, 255, 255, 0.06); }
.saved-device-info { display: flex; flex-direction: column; gap: 2px; }
.saved-device-ip { font-family: monospace; color: #fff; font-size: 0.95em; }
.saved-device-type { font-size: 0.75em; color: rgba(255, 255, 255, 0.4); }
.saved-device-actions { display: flex; gap: 8px; }
.saved-device-btn { padding: 6px 12px; border-radius: 6px; font-size: 0.8em; cursor: pointer; transition: all 0.2s ease; }
.saved-device-btn.connect { background: rgba(34, 197, 94, 0.15); border: 1px solid rgba(34, 197, 94, 0.3); color: #22c55e; }
.saved-device-btn.connect:hover { background: rgba(34, 197, 94, 0.25); }
.saved-device-btn.delete { background: rgba(239, 68, 68, 0.15); border: 1px solid rgba(239, 68, 68, 0.3); color: #ef4444; }
.saved-device-btn.delete:hover { background: rgba(239, 68, 68, 0.25); }

/* Status Bar Responsive (for shutter) */
@media (max-width: 768px) {
    .shutter-status-bar { grid-template-columns: repeat(2, 1fr); gap: 20px 0; padding: 15px 20px; }
    .status-col { border-right: none; border-bottom: none; padding: 0 10px; }
    .status-col:nth-child(odd) { border-right: 1px solid var(--border-light); }
    .dimmer-inline-form { flex-direction: column; }
    .dimmer-inline-form button { width: 100%; }
    .dimmer-brightness-display { font-size: 3.5em; }
}
@media (max-width: 480px) {
    .shutter-status-bar { grid-template-columns: 1fr; gap: 15px; padding: 15px; }
    .status-col { padding: 10px 0; border-bottom: 1px solid var(--border-color); }
    .status-col:nth-child(odd) { border-right: none; }
    .status-col:last-child { border-bottom: none; }
    .status-col-label { font-size: 0.7em; }
    .status-col-value { font-size: 1em; }
    .dimmer-hero { padding: 30px 20px 25px; }
    .dimmer-brightness-display { font-size: 3em; }
    .dimmer-calibration-controls { gap: 15px; max-width: 200px; }
    .dimmer-cal-btn { width: 40px; height: 40px; font-size: 1.2em; }
}

/* Calibration Slider */
.calibration-slider-container { padding: 20px 0 10px 0; }
.slider-labels { display: flex; justify-content: space-between; margin-bottom: 8px; font-size: 0.75em; color: rgba(255, 255, 255, 0.35); letter-spacing: 0.3px; font-weight: 400; }
.slider-label-left, .slider-label-center, .slider-label-right { flex: 1; transition: color 0.3s ease; }
.slider-label-left { text-align: left; }
.slider-label-center { text-align: center; }
.slider-label-right { text-align: right; }
.calibration-slider { width: 100%; height: 4px; -webkit-appearance: none; appearance: none; background: linear-gradient(to right, rgba(255, 255, 255, 0.08), rgba(255, 255, 255, 0.12), rgba(255, 255, 255, 0.08)); border-radius: 2px; outline: none; margin: 12px 0; cursor: pointer; position: relative; box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.3); }
.calibration-slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 18px; height: 18px; background: #ffffff; border-radius: 50%; cursor: grab; transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4), 0 0 0 1px rgba(255, 255, 255, 0.1); }
.calibration-slider::-webkit-slider-thumb:hover { transform: scale(1.15); box-shadow: 0 4px 12px rgba(0, 0, 0, 0.5), 0 0 0 1px rgba(255, 255, 255, 0.2), 0 0 0 4px rgba(255, 255, 255, 0.08); }
.calibration-slider::-webkit-slider-thumb:active { cursor: grabbing; transform: scale(1.05); }
.calibration-slider::-moz-range-thumb { width: 18px; height: 18px; background: #ffffff; border: none; border-radius: 50%; cursor: grab; transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1); box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4), 0 0 0 1px rgba(255, 255, 255, 0.1); }
.calibration-slider::-moz-range-thumb:hover { transform: scale(1.15); box-shadow: 0 4px 12px rgba(0, 0, 0, 0.5), 0 0 0 1px rgba(255, 255, 255, 0.2), 0 0 0 4px rgba(255, 255, 255, 0.08); }
.calibration-slider::-moz-range-thumb:active { cursor: grabbing; transform: scale(1.05); }
.slider-value-display { text-align: center; padding: 12px 16px; background: rgba(255, 255, 255, 0.03); border: 1px solid rgba(255, 255, 255, 0.06); border-radius: 8px; margin-top: 16px; transition: all 0.3s ease; }
.slider-value-label { font-size: 0.8em; color: rgba(255, 255, 255, 0.4); margin-right: 12px; letter-spacing: 0.5px; font-weight: 400; }
.slider-value-number { font-size: 1.4em; color: #ffffff; font-weight: 500; letter-spacing: 0.5px; }

/* Form Actions Row */
.form-actions-row { display: flex; gap: 10px; }
.btn-primary, .btn-secondary { flex: 1; padding: 10px 18px; border: none; border-radius: var(--radius-md); font-size: 0.9em; font-weight: 500; cursor: pointer; transition: var(--transition-normal); }
.btn-primary { background: var(--color-info); color: var(--text-primary); }
.btn-primary:hover { background: var(--color-info-dark); }
.btn-secondary { background: var(--hover-bg); color: var(--text-primary); border: 1px solid var(--border-light); }
.btn-secondary:hover { background: var(--shadow-hover); }

/* Saved Devices List */
.saved-devices-list { display: flex; flex-direction: column; gap: 10px; max-height: 300px; overflow-y: auto; }
.saved-device-empty { text-align: center; padding: 30px 20px; color: var(--text-muted); font-size: 0.9em; font-style: italic; }
.saved-device-item { display: flex; justify-content: space-between; align-items: center; padding: 12px; background: var(--card-hover); border: 1px solid var(--border-color); border-radius: var(--radius-lg); transition: var(--transition-normal); }
.saved-device-item:hover { background: var(--input-bg-focus); border-color: var(--border-light); }
.saved-device-item.discovered { border-left: 3px solid var(--color-success); }
.discovered-header { font-size: 0.9em; font-weight: 500; color: var(--color-success); padding: 8px 0; border-bottom: 1px solid var(--border-color); margin-bottom: 8px; }
.saved-device-info { display: flex; flex-direction: column; gap: 5px; }
.saved-device-ip { font-size: 1em; color: var(--text-primary); font-weight: 500; }
.saved-device-type { font-size: 0.85em; color: var(--text-muted); }
.saved-device-actions { display: flex; gap: 8px; }
.btn-device-connect, .btn-device-remove { padding: 8px 16px; border: none; border-radius: 5px; font-size: 0.85em; font-weight: 500; cursor: pointer; transition: var(--transition-normal); }
.btn-device-connect { background: var(--color-info); color: var(--text-primary); }
.btn-device-connect:hover { background: var(--color-info-dark); }
.btn-device-remove { background: var(--color-error-bg-light); color: var(--color-error); border: 1px solid var(--color-error-border); }
.btn-device-remove:hover { background: rgba(239, 68, 68, 0.3); }

/* Main Save Button */
.dimmer-save-container { text-align: center; margin-top: 30px; padding-top: 20px; border-top: 1px solid var(--border-color); }
.btn-save-main { padding: 15px 50px; background: linear-gradient(135deg, var(--color-info), var(--color-success)); color: var(--text-primary); border: none; border-radius: var(--radius-lg); font-size: 1.1em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); box-shadow: 0 4px 15px rgba(59, 130, 246, 0.3); }
.btn-save-main:hover { transform: translateY(-2px); box-shadow: 0 6px 20px rgba(59, 130, 246, 0.4); }
.btn-save-main:active { transform: translateY(0); }

/* System Actions (Restart & Factory Reset) */
.system-actions { text-align: center; }
.system-buttons { display: flex; justify-content: center; gap: 20px; margin-top: 15px; }
.btn-restart { padding: 12px 30px; background: #ffffff; color: #000000; border: none; border-radius: var(--radius-md); font-size: 1em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); }
.btn-restart:hover { background: #e0e0e0; transform: translateY(-2px); }
.btn-factory-reset { padding: 12px 30px; background: #ef4444; color: #ffffff; border: none; border-radius: var(--radius-md); font-size: 1em; font-weight: 600; cursor: pointer; transition: var(--transition-normal); display: inline-flex; align-items: center; gap: 8px; position: relative; overflow: visible; }
.btn-factory-reset:hover { background: #dc2626; transform: translateY(-2px); }
.btn-factory-reset .info-tooltip-i { background: rgba(255, 255, 255, 0.2); border-color: rgba(255, 255, 255, 0.4); color: #fff; margin-left: 0; }
.btn-factory-reset .info-tooltip-i:hover { background: rgba(255, 255, 255, 0.3); }
.factory-reset-confirm { margin-top: 20px; padding: 15px; background: rgba(239, 68, 68, 0.1); border: 1px solid rgba(239, 68, 68, 0.3); border-radius: var(--radius-md); }
.factory-reset-confirm p { margin-bottom: 10px; color: var(--text-primary); }
.factory-reset-confirm input { padding: 10px 15px; border: 1px solid var(--border-color); border-radius: var(--radius-sm); background: var(--bg-tertiary); color: var(--text-primary); margin-right: 10px; width: 120px; }
.btn-confirm-reset { padding: 10px 20px; background: #ef4444; color: #ffffff; border: none; border-radius: var(--radius-sm); font-weight: 600; cursor: pointer; margin-right: 10px; }
.btn-confirm-reset:hover { background: #dc2626; }
.btn-cancel-reset { padding: 10px 20px; background: var(--bg-tertiary); color: var(--text-primary); border: 1px solid var(--border-color); border-radius: var(--radius-sm); cursor: pointer; }
.btn-cancel-reset:hover { background: var(--bg-secondary); }

/* Mode Config Panel */
.mode-config-panel { animation: fadeIn 0.3s ease; }
@keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
#mode-config-accordion { margin-top: 25px; border: 2px solid rgba(255, 255, 255, 0.25); box-shadow: 0 0 15px rgba(255, 255, 255, 0.05); }
#mode-config-accordion:hover { border-color: rgba(255, 255, 255, 0.35); }
.quick-safe-tab-content { padding-top: 15px; }
.shutter-config-section { background: rgba(255, 255, 255, 0.02); border: 1px solid rgba(255, 255, 255, 0.08); border-radius: 8px; padding: 15px; margin-bottom: 12px; }
.shutter-section-title { font-size: 0.95em; font-weight: 500; color: #ffffff; margin-bottom: 12px; text-align: center; letter-spacing: 0.5px; }
.position-display { display: flex; align-items: center; gap: 10px; }
.position-bar-container { flex: 1; height: 8px; background: rgba(255, 255, 255, 0.1); border-radius: 4px; overflow: hidden; }
.position-bar { height: 100%; background: linear-gradient(90deg, rgb(59, 130, 246), rgb(37, 99, 235)); border-radius: 4px; transition: width 0.3s ease; }
.position-percent { font-size: 0.9em; color: var(--text-primary); font-weight: 500; min-width: 35px; text-align: right; }

/* Section Title */
.section-title { font-size: 1.1em; font-weight: 500; color: var(--text-primary); margin: 35px 0 20px 0; padding-bottom: 12px; border-bottom: 1px solid var(--border-color); letter-spacing: 0.3px; }

/* Mode Selection Title */
.mode-selection-title { font-size: 1.3em; font-weight: 500; color: var(--text-primary); text-align: center; margin: 0 0 20px 0; letter-spacing: 0.5px; display: flex; align-items: center; justify-content: center; gap: 8px; }

/* Device Settings Title */
.device-settings-title { font-size: 1.5em; font-weight: 600; color: var(--text-primary); text-align: center; margin: 40px 0 25px 0; padding-bottom: 15px; border-bottom: 2px solid var(--accent-color); letter-spacing: 0.5px; display: flex; align-items: center; justify-content: center; gap: 8px; }

/* Factory Reset Wrapper */
.factory-reset-wrapper { display: inline-flex; align-items: center; gap: 6px; }

/* System Operations */
.system-operations { margin: 40px 0; padding: 25px; background: var(--card-bg); border-radius: 12px; text-align: center; }
.system-operations h3 { margin: 0 0 20px 0; font-size: 1.2em; font-weight: 500; color: var(--text-primary); }
.system-buttons { display: flex; justify-content: center; gap: 15px; flex-wrap: wrap; }
.btn-restart { padding: 12px 30px; font-size: 1em; font-weight: 500; background: var(--card-bg); color: var(--text-primary); border: 2px solid var(--border-color); border-radius: 8px; cursor: pointer; transition: all 0.3s ease; }
.btn-restart:hover { background: var(--accent-color); color: white; border-color: var(--accent-color); }
.btn-factory-reset { padding: 12px 30px; font-size: 1em; font-weight: 500; background: #e74c3c; color: white; border: none; border-radius: 8px; cursor: pointer; transition: all 0.3s ease; }
.btn-factory-reset:hover { background: #c0392b; }
.factory-reset-confirm { margin-top: 20px; padding: 15px; background: rgba(231, 76, 60, 0.1); border-radius: 8px; }
.factory-reset-confirm p { margin: 0 0 10px 0; color: #e74c3c; font-weight: 500; }
.factory-reset-confirm input { padding: 10px; border: 1px solid var(--border-color); border-radius: 6px; background: var(--bg-primary); color: var(--text-primary); margin-right: 10px; }
.btn-confirm-reset { padding: 10px 20px; background: #e74c3c; color: white; border: none; border-radius: 6px; cursor: pointer; margin-right: 5px; }
.btn-cancel-reset { padding: 10px 20px; background: var(--card-bg); color: var(--text-primary); border: 1px solid var(--border-color); border-radius: 6px; cursor: pointer; }

/* Info Footer */
.info-footer { margin: 40px 0 20px 0; padding: 30px; text-align: center; }
.info-footer h3 { margin: 0 0 10px 0; font-size: 1.2em; font-weight: 500; color: var(--text-primary); }
.info-footer p { margin: 0 0 20px 0; color: var(--text-secondary); font-size: 0.95em; }
.info-footer .button-group { display: flex; justify-content: center; gap: 15px; flex-wrap: wrap; }
.info-button { padding: 12px 30px; background: var(--card-bg); color: var(--text-primary); border: 2px solid var(--border-color); border-radius: 8px; text-decoration: none; font-weight: 500; transition: all 0.3s ease; }
.info-button:hover { background: var(--accent-color); color: white; border-color: var(--accent-color); }

/* Form Label Row - SSID with Scan Button */
.form-label-row { display: flex; justify-content: space-between; align-items: center; margin-bottom: 8px; }
.btn-wifi-scan { padding: 4px 12px; font-size: 0.8em; background: var(--color-info-bg); color: var(--color-info); border: 1px solid var(--color-info-border); border-radius: var(--radius-sm); cursor: pointer; transition: var(--transition-fast); font-weight: 500; }
.btn-wifi-scan:hover { background: var(--color-info); color: #fff; }

/* WiFi Scan Modal */
.modal-overlay { position: fixed; top: 0; left: 0; width: 100%; height: 100%; background: rgba(0, 0, 0, 0.7); display: flex; justify-content: center; align-items: center; z-index: 10000; backdrop-filter: blur(4px); }
.modal-content { background: var(--bg-secondary); border: 1px solid var(--border-color); border-radius: 12px; width: 90%; max-width: 400px; max-height: 80vh; display: flex; flex-direction: column; box-shadow: 0 10px 40px rgba(0, 0, 0, 0.5); }
.wifi-scan-modal { overflow: hidden; }
.modal-header { display: flex; justify-content: space-between; align-items: center; padding: 18px 20px; border-bottom: 1px solid var(--border-color); }
.modal-header h3 { margin: 0; font-size: 1.1em; font-weight: 500; color: var(--text-primary); }
.modal-close { background: none; border: none; font-size: 1.8em; color: var(--text-muted); cursor: pointer; padding: 0; line-height: 1; transition: var(--transition-fast); }
.modal-close:hover { color: var(--text-primary); transform: scale(1.1); }
.modal-body { padding: 15px 20px; overflow-y: auto; flex: 1; max-height: 350px; }
.modal-footer { display: flex; gap: 10px; padding: 15px 20px; border-top: 1px solid var(--border-color); }
.modal-footer .btn { flex: 1; }

/* WiFi Scan Loading */
.wifi-scan-loading { display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 40px 20px; gap: 15px; }
.loading-spinner { width: 40px; height: 40px; border: 3px solid var(--border-color); border-top-color: var(--color-info); border-radius: 50%; animation: spin 1s linear infinite; }
@keyframes spin { to { transform: rotate(360deg); } }

/* WiFi Network List */
.wifi-network-list { display: flex; flex-direction: column; gap: 8px; }
.wifi-network-item { display: flex; align-items: center; justify-content: space-between; padding: 12px 15px; background: var(--bg-card); border: 1px solid var(--border-color); border-radius: var(--radius-md); cursor: pointer; transition: var(--transition-fast); }
.wifi-network-item:hover { background: var(--hover-bg); border-color: var(--color-info-border); }
.wifi-network-ssid { font-size: 1em; font-weight: 500; color: var(--text-primary); }
.wifi-network-security { font-size: 0.8em; color: var(--text-muted); margin-left: 8px; }

/* WiFi Scan Empty */
.wifi-scan-empty { display: flex; flex-direction: column; align-items: center; justify-content: center; padding: 40px 20px; color: var(--text-muted); }

)rawliteral";

#endif // SK_CSS_H
