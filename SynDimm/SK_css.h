/**
 * SK_css.h
 * SmartKraft SynDimm - CSS Styles
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
}

body.light-theme {
    /* Light Theme */
    --bg-primary: #ffffff;
    --bg-secondary: #f5f5f5;
    --bg-card: rgba(0, 0, 0, 0.03);
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

* {
    margin: 0;
    padding: 0;
    box-sizing: border-box;
}

body {
    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, 'Helvetica Neue', Arial, sans-serif;
    background: var(--bg-primary);
    color: var(--text-primary);
    min-height: 100vh;
    padding: 0;
    display: flex;
    justify-content: center;
    align-items: flex-start;
    transition: background 0.3s ease, color 0.3s ease;
}

.container {
    max-width: 900px;
    width: 100%;
    background: var(--bg-primary);
    min-height: 100vh;
}

.header {
    background: var(--bg-primary);
    color: var(--text-primary);
    padding: 40px 40px 35px 40px;
    text-align: center;
    border-bottom: 1px solid var(--border-color);
    position: relative;
}

.header h1 {
    font-size: 2.2em;
    margin-bottom: 12px;
    font-weight: 400;
    letter-spacing: 0.5px;
    color: var(--text-primary);
}

.theme-toggle {
    position: absolute;
    top: 20px;
    right: 20px;
    display: flex;
    gap: 8px;
    background: var(--bg-card);
    padding: 6px;
    border-radius: 20px;
    border: 1px solid var(--border-color);
}

.theme-btn {
    padding: 8px 16px;
    background: transparent;
    border: none;
    color: var(--text-secondary);
    font-size: 0.85em;
    cursor: pointer;
    border-radius: 14px;
    transition: all 0.2s ease;
    font-weight: 500;
}

.theme-btn.active {
    background: var(--text-primary);
    color: var(--bg-primary);
}

.theme-btn:hover {
    color: var(--text-primary);
}

.info-box {
    display: flex;
    justify-content: center;
    gap: 80px;
    margin-top: 15px;
    flex-wrap: wrap;
}

.info-single {
    display: flex;
    align-items: baseline;
    gap: 8px;
    font-size: 0.9em;
    flex-wrap: nowrap;
    white-space: nowrap;
}

.info-single .info-label {
    color: var(--text-secondary);
    font-weight: 400;
    letter-spacing: 0.5px;
    font-size: 0.95em;
    line-height: 1;
}

.info-single .info-value {
    color: var(--text-primary);
    font-weight: 400;
    letter-spacing: 0.5px;
    font-size: 0.95em;
    line-height: 1;
}

.info-single .info-separator {
    color: rgba(255, 255, 255, 0.3);
    margin: 0 5px;
    font-size: 0.95em;
    line-height: 1;
}

.info-item {
    text-align: center;
}

.info-label {
    font-size: 0.75em;
    color: rgba(255, 255, 255, 0.5);
    text-transform: uppercase;
    letter-spacing: 1.5px;
    margin-bottom: 8px;
    font-weight: 500;
}

.info-value {
    font-size: 1.1em;
    font-weight: 400;
    color: #ffffff;
    letter-spacing: 0.5px;
}

.tabs {
    display: flex;
    background: var(--bg-primary);
    border-bottom: 1px solid var(--border-color);
    padding: 0 20px;
}

.tab {
    flex: 1;
    padding: 22px 20px;
    text-align: center;
    cursor: pointer;
    background: transparent;
    border: none;
    font-size: 0.95em;
    font-weight: 400;
    color: var(--text-secondary);
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
    letter-spacing: 0.3px;
    position: relative;
}

.tab::after {
    content: '';
    position: absolute;
    bottom: 0;
    left: 50%;
    transform: translateX(-50%) scaleX(0);
    width: 60%;
    height: 2px;
    background: var(--text-primary);
    transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.tab:hover {
    color: var(--text-secondary);
    opacity: 0.8;
}

.tab.active {
    color: var(--text-primary);
}

.tab.active::after {
    transform: translateX(-50%) scaleX(1);
}

.tab-content {
    display: none !important;
    padding: 50px 40px;
    min-height: 500px;
    animation: fadeInUp 0.4s cubic-bezier(0.4, 0, 0.2, 1);
}

.tab-content.active {
    display: block !important;
}

@keyframes fadeInUp {
    from {
        opacity: 0;
        transform: translateY(20px);
    }
    to {
        opacity: 1;
        transform: translateY(0);
    }
}

.placeholder {
    text-align: center;
    color: var(--text-muted);
    font-size: 0.95em;
    padding: 100px 30px;
    font-weight: 300;
    letter-spacing: 0.5px;
    opacity: 0.5;
}

.modes-content,
.connection-content,
.info-content {
    max-width: 750px;
    margin: 0 auto;
}

/* Info Card Styles */
.info-card {
    background: var(--bg-card);
    border: 1px solid var(--border-color);
    border-radius: 8px;
    padding: 25px;
    margin-bottom: 25px;
    transition: all 0.3s ease;
}

.info-card:hover {
    border-color: var(--border-light);
}

.info-card h3 {
    font-size: 1.1em;
    font-weight: 500;
    color: var(--text-primary);
    margin-bottom: 20px;
    letter-spacing: 0.3px;
}

.info-grid {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 20px;
}

.info-item {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.info-item-label {
    font-size: 0.75em;
    color: var(--text-muted);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 500;
}

.info-item-value {
    font-size: 1em;
    color: var(--text-primary);
    font-weight: 400;
}

/* Theme Selector */
.theme-selector {
    display: flex;
    gap: 15px;
}

.theme-option {
    flex: 1;
    cursor: pointer;
}

.theme-option input[type="radio"] {
    display: none;
}

.theme-option-label {
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 15px 20px;
    background: var(--bg-card);
    border: 2px solid var(--border-color);
    border-radius: 8px;
    transition: all 0.3s ease;
}

.theme-option input[type="radio"]:checked + .theme-option-label {
    background: var(--hover-bg);
    border-color: var(--text-primary);
}

.theme-option:hover .theme-option-label {
    border-color: var(--border-light);
}

.theme-name {
    font-size: 0.95em;
    color: var(--text-primary);
    font-weight: 500;
}

/* Mode Selection Buttons */
.mode-buttons {
    display: flex;
    gap: 15px;
    margin-bottom: 25px;
}

.mode-btn {
    flex: 1;
    display: flex;
    flex-direction: column;
    align-items: center;
    justify-content: center;
    padding: 25px 15px;
    background: var(--bg-card);
    border: 2px solid var(--border-color);
    border-radius: 12px;
    cursor: pointer;
    transition: all 0.3s ease;
    color: var(--text-primary);
    position: relative;
}

.mode-btn:hover {
    border-color: var(--border-light);
    background: var(--hover-bg);
    transform: translateY(-2px);
}

.mode-btn.active {
    background: var(--hover-bg);
    border-color: var(--text-primary);
}

.mode-btn.preview {
    background: rgba(255, 200, 0, 0.25);
    border-color: #ffc800;
    box-shadow: 0 0 25px rgba(255, 200, 0, 0.6);
    animation: preview-pulse 1s ease-in-out infinite;
}

@keyframes preview-pulse {
    0%, 100% {
        box-shadow: 0 0 25px rgba(255, 200, 0, 0.6);
        background: rgba(255, 200, 0, 0.25);
    }
    50% {
        box-shadow: 0 0 40px rgba(255, 200, 0, 0.9), 0 0 50px rgba(255, 200, 0, 0.5);
        background: rgba(255, 200, 0, 0.35);
    }
}

.mode-btn-text {
    font-size: 1.1em;
    font-weight: 600;
    letter-spacing: 0.5px;
}

/* Settings Row */
.settings-row {
    display: flex;
    gap: 20px;
}

.settings-group {
    flex: 1;
    background: var(--bg-card);
    border: 1px solid var(--border-color);
    border-radius: 12px;
    padding: 20px;
}

.settings-group h4 {
    font-size: 1em;
    font-weight: 500;
    color: var(--text-primary);
    margin-bottom: 15px;
    letter-spacing: 0.3px;
}

/* Language Selector */
.language-selector {
    display: flex;
    gap: 10px;
}

.lang-btn {
    flex: 1;
    padding: 12px 15px;
    background: var(--bg-card);
    border: 2px solid var(--border-color);
    border-radius: 8px;
    color: var(--text-primary);
    font-size: 0.95em;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
}

.lang-btn:hover {
    border-color: var(--border-light);
    background: var(--hover-bg);
}

.lang-btn.active {
    border-color: var(--text-primary);
    background: var(--hover-bg);
}

/* Current Mode Display */
.current-mode-display {
    text-align: center;
    padding: 20px;
}

.mode-indicator {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 12px;
    margin-bottom: 15px;
}

.mode-icon {
    font-size: 2em;
    color: var(--text-primary);
}

.mode-text {
    font-size: 1.3em;
    font-weight: 500;
    color: var(--text-primary);
    letter-spacing: 0.5px;
}

.mode-hint {
    font-size: 0.85em;
    color: var(--text-secondary);
    line-height: 1.6;
}

.info-content {
    max-width: 750px;
    margin: 0 auto;
    padding: 0;
}

.modes-content {
    max-width: 750px;
    margin: 0 auto;
}

.connection-content {
    max-width: 750px;
    margin: 0 auto;
}

.info-content h2 {
    font-size: 1.6em;
    font-weight: 400;
    text-align: center;
    color: #ffffff;
    margin-bottom: 50px;
    letter-spacing: 0.5px;
}

.info-section {
    margin-bottom: 45px;
    padding-bottom: 35px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.06);
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.info-section:last-of-type {
    border-bottom: none;
}

.info-section:hover {
    transform: translateX(5px);
}

.info-section h3 {
    font-size: 1.15em;
    font-weight: 500;
    color: #ffffff;
    margin-bottom: 18px;
    letter-spacing: 0.3px;
}

.info-section p {
    font-size: 0.92em;
    line-height: 1.9;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 14px;
    transition: color 0.3s ease;
    letter-spacing: 0.2px;
}

.info-section:hover p {
    color: rgba(255, 255, 255, 0.85);
}

.info-section p strong {
    color: #ffffff;
    font-weight: 500;
}

.info-note {
    font-style: italic;
    color: rgba(255, 255, 255, 0.5) !important;
    font-size: 0.88em !important;
}

.info-note-warning {
    background: rgba(255, 152, 0, 0.15) !important;
    border-left: 3px solid rgb(255, 152, 0) !important;
    color: rgb(255, 152, 0) !important;
    padding: 12px 15px !important;
    border-radius: 4px !important;
    font-style: normal !important;
    margin-top: 15px !important;
}

.info-footer {
    margin-top: 60px;
    padding-top: 40px;
    border-top: 1px solid rgba(255, 255, 255, 0.1);
    text-align: center;
}

.info-footer h3 {
    font-size: 1.1em;
    font-weight: 400;
    color: #ffffff;
    margin-bottom: 18px;
    letter-spacing: 0.3px;
}

.info-footer p {
    font-size: 0.88em;
    color: rgba(255, 255, 255, 0.6);
    margin-bottom: 30px;
    letter-spacing: 0.2px;
}

.button-group {
    display: flex;
    justify-content: center;
    gap: 20px;
    flex-wrap: wrap;
}

.info-button {
    display: inline-block;
    padding: 14px 45px;
    background: transparent;
    color: #ffffff;
    text-decoration: none;
    font-weight: 400;
    letter-spacing: 0.5px;
    font-size: 0.9em;
    border: 1px solid rgba(255, 255, 255, 0.3);
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.info-button:hover {
    background: #ffffff;
    color: #000000;
    border-color: #ffffff;
    transform: translateY(-3px);
    box-shadow: 0 10px 25px rgba(255, 255, 255, 0.15);
}

.connection-content {
    max-width: 750px;
    margin: 0 auto;
}

/* Status Card */
.status-card {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    padding: 20px 25px;
    margin-bottom: 25px;
}

.status-row {
    display: grid;
    grid-template-columns: repeat(3, 1fr);
    gap: 0;
}

.status-item {
    display: flex;
    flex-direction: column;
    gap: 8px;
    text-align: center;
    padding: 0 15px;
    border-right: 1px solid rgba(255, 255, 255, 0.15);
}

.status-item:last-child {
    border-right: none;
}

.status-label {
    font-size: 0.75em;
    color: rgba(255, 255, 255, 0.5);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 500;
}

.status-value {
    font-size: 1.1em;
    color: #ffffff;
    font-weight: 400;
}

.status-mdns {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    margin-top: 2px;
}

@media (max-width: 600px) {
    .status-row {
        grid-template-columns: 1fr;
        gap: 20px;
    }
    
    .status-item {
        border-right: none;
        border-bottom: 1px solid rgba(255, 255, 255, 0.15);
        padding-bottom: 15px;
    }
    
    .status-item:last-child {
        border-bottom: none;
        padding-bottom: 0;
    }
}

.notification {
    position: relative;
    padding: 18px 50px 18px 20px;
    margin-bottom: 25px;
    border-radius: 8px;
    display: flex;
    align-items: center;
    justify-content: space-between;
    animation: slideDown 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.3);
}

@keyframes slideDown {
    from {
        opacity: 0;
        transform: translateY(-20px);
    }
    to {
        opacity: 1;
        transform: translateY(0);
    }
}

.notification.success {
    background: rgba(16, 185, 129, 0.15);
    border: 1px solid rgba(16, 185, 129, 0.4);
}

.notification.error {
    background: rgba(239, 68, 68, 0.15);
    border: 1px solid rgba(239, 68, 68, 0.4);
}

.notification.info {
    background: rgba(59, 130, 246, 0.15);
    border: 1px solid rgba(59, 130, 246, 0.4);
}

.notification-content {
    display: flex;
    align-items: center;
    gap: 15px;
    flex: 1;
}

.notification-icon {
    font-size: 1.3em;
    font-weight: bold;
}

.notification.success .notification-icon {
    color: rgb(16, 185, 129);
}

.notification.success .notification-icon::before {
    content: "[OK]";
}

.notification.error .notification-icon {
    color: rgb(239, 68, 68);
}

.notification.error .notification-icon::before {
    content: "[!]";
}

.notification.info .notification-icon {
    color: rgb(59, 130, 246);
}

.notification.info .notification-icon::before {
    content: "[i]";
}

.notification-message {
    color: #ffffff;
    font-size: 0.95em;
    line-height: 1.5;
}

.notification-close {
    position: absolute;
    top: 8px;
    right: 12px;
    background: transparent;
    border: none;
    color: rgba(255, 255, 255, 0.6);
    font-size: 1.8em;
    cursor: pointer;
    padding: 0;
    width: 30px;
    height: 30px;
    display: flex;
    align-items: center;
    justify-content: center;
    transition: all 0.2s ease;
    line-height: 1;
}

.notification-close:hover {
    color: #ffffff;
    transform: scale(1.2);
}

.accordion {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    margin-bottom: 20px;
    overflow: hidden;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.accordion:hover {
    border-color: rgba(255, 255, 255, 0.15);
}

.accordion-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 20px 25px;
    cursor: pointer;
    user-select: none;
    transition: background 0.3s ease;
    gap: 15px;
}

.accordion-header:hover {
    background: rgba(255, 255, 255, 0.05);
}

.accordion-title-text {
    flex: 1;
    font-size: 1.05em;
    font-weight: 500;
    color: #ffffff;
    letter-spacing: 0.5px;
}

.accordion-title {
    display: flex;
    align-items: center;
    gap: 15px;
    font-size: 1.05em;
    font-weight: 500;
    color: #ffffff;
    letter-spacing: 0.5px;
}

.badge {
    padding: 5px 15px;
    border-radius: 4px;
    font-size: 0.75em;
    font-weight: 500;
    letter-spacing: 0.5px;
    white-space: nowrap;
}

.badge-passive {
    background: rgba(255, 255, 255, 0.1);
    color: rgba(255, 255, 255, 0.6);
}

.badge-info {
    background: rgba(59, 130, 246, 0.2);
    color: rgb(59, 130, 246);
}

.badge-connected {
    background: rgba(16, 185, 129, 0.2);
    color: rgb(16, 185, 129);
}

.badge-not-configured {
    background: rgba(255, 255, 255, 0.05);
    color: rgba(255, 255, 255, 0.4);
}

.accordion-icon {
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.5);
    transition: transform 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.accordion-header.active .accordion-icon {
    transform: rotate(180deg);
}

.accordion-content {
    max-height: 0;
    overflow: hidden;
    transition: max-height 0.4s cubic-bezier(0.4, 0, 0.2, 1);
    padding: 0 25px;
}

.accordion-header.active + .accordion-content {
    max-height: 5000px;
    padding: 0 25px 25px 25px;
    overflow-y: auto;
}

.form-group {
    margin-bottom: 12px;
}

.form-group label {
    display: block;
    margin-bottom: 8px;
    font-size: 0.9em;
    font-weight: 500;
    color: #ffffff;
    letter-spacing: 0.3px;
}

.form-group input[type="text"],
.form-group input[type="password"] {
    width: 100%;
    padding: 10px 12px;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
    color: #ffffff;
    font-size: 0.9em;
    transition: all 0.3s ease;
}

.form-group input:focus {
    outline: none;
    background: rgba(255, 255, 255, 0.08);
    border-color: rgba(255, 255, 255, 0.3);
}

.form-group input::placeholder {
    color: rgba(255, 255, 255, 0.3);
}

.input-readonly {
    background: rgba(255, 255, 255, 0.02) !important;
    color: rgba(255, 255, 255, 0.5) !important;
    cursor: not-allowed;
}

.form-hint {
    display: block;
    margin-top: 6px;
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.4);
    font-style: italic;
}

.form-note {
    margin-top: 20px;
    padding: 15px;
    background: rgba(255, 255, 255, 0.03);
    border-left: 3px solid rgba(255, 255, 255, 0.2);
    border-radius: 4px;
    font-size: 0.85em;
    line-height: 1.7;
    color: rgba(255, 255, 255, 0.7);
}

.form-note strong {
    color: #ffffff;
    display: block;
    margin-bottom: 8px;
}

.save-button-container {
    margin-top: 35px;
    text-align: center;
    padding-bottom: 20px;
}

.save-button {
    padding: 15px 50px;
    background: transparent;
    color: #ffffff;
    border: 2px solid rgba(255, 255, 255, 0.3);
    border-radius: 6px;
    font-size: 1em;
    font-weight: 500;
    letter-spacing: 0.5px;
    cursor: pointer;
    transition: all 0.3s cubic-bezier(0.4, 0, 0.2, 1);
}

.save-button:hover {
    background: #ffffff;
    color: #000000;
    border-color: #ffffff;
    transform: translateY(-3px);
    box-shadow: 0 10px 25px rgba(255, 255, 255, 0.2);
}

/* AP Mode Compact Styles */
.ap-info-text {
    font-size: 0.9em;
    line-height: 1.6;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 20px;
    padding-bottom: 20px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
}

.ap-details {
    margin-bottom: 20px;
}

.ap-detail-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 10px 0;
    border-bottom: 1px solid rgba(255, 255, 255, 0.05);
}

.ap-detail-row:last-child {
    border-bottom: none;
}

.ap-label {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.5);
    font-weight: 500;
}

.ap-value {
    font-size: 0.9em;
    color: #ffffff;
    font-family: monospace;
}

.ap-steps {
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 6px;
    padding: 15px;
}

.ap-step-title {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    margin-bottom: 12px;
    text-align: center;
}

.ap-step-flow {
    display: flex;
    align-items: center;
    justify-content: center;
    gap: 8px;
    flex-wrap: wrap;
}

.ap-step {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    padding: 6px 12px;
    background: rgba(255, 255, 255, 0.03);
    border-radius: 4px;
}

.ap-step-active {
    color: rgb(59, 130, 246);
    background: rgba(59, 130, 246, 0.1);
    border: 1px solid rgba(59, 130, 246, 0.3);
}

.ap-arrow {
    color: rgba(255, 255, 255, 0.3);
    font-size: 0.9em;
}

/* Mode Info Text */
.mode-info-text {
    font-size: 0.9em;
    line-height: 1.6;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 25px;
    padding: 15px;
    background: rgba(255, 255, 255, 0.02);
    border-left: 3px solid rgba(255, 255, 255, 0.2);
    border-radius: 4px;
}

/* Safe Tabs */
.safe-tabs {
    display: flex;
    gap: 10px;
    margin-bottom: 30px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.1);
}

.safe-tab {
    flex: 1;
    padding: 15px 20px;
    background: transparent;
    border: none;
    border-bottom: 3px solid transparent;
    color: rgba(255, 255, 255, 0.5);
    font-size: 0.95em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
}

.safe-tab:hover {
    color: rgba(255, 255, 255, 0.8);
    background: rgba(255, 255, 255, 0.03);
}

.safe-tab.active {
    color: #ffffff;
    border-bottom-color: #ffffff;
    background: rgba(255, 255, 255, 0.05);
}

.safe-tab-content {
    display: none;
}

.safe-tab-content.active {
    display: block;
}

/* Toggle Switch */
.safe-toggle-row {
    display: flex;
    align-items: center;
    gap: 15px;
    margin-bottom: 25px;
}

.toggle-switch {
    position: relative;
    width: 50px;
    height: 26px;
    display: inline-block;
}

.toggle-switch input {
    opacity: 0;
    width: 0;
    height: 0;
}

.toggle-slider {
    position: absolute;
    cursor: pointer;
    top: 0;
    left: 0;
    right: 0;
    bottom: 0;
    background-color: rgba(255, 255, 255, 0.1);
    transition: 0.3s;
    border-radius: 26px;
    border: 1px solid rgba(255, 255, 255, 0.2);
}

.toggle-slider:before {
    position: absolute;
    content: "";
    height: 18px;
    width: 18px;
    left: 4px;
    bottom: 3px;
    background-color: rgba(255, 255, 255, 0.6);
    transition: 0.3s;
    border-radius: 50%;
}

.toggle-switch input:checked + .toggle-slider {
    background-color: rgba(76, 175, 80, 0.3);
    border-color: rgb(76, 175, 80);
}

.toggle-switch input:checked + .toggle-slider:before {
    transform: translateX(24px);
    background-color: rgb(76, 175, 80);
}

.toggle-label {
    font-size: 0.95em;
    color: #ffffff;
    font-weight: 500;
}

/* Safe Config Section */
.safe-config-section {
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    padding: 20px;
    margin-bottom: 20px;
}

.form-hint-inline {
    display: block;
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.4);
    margin-bottom: 8px;
}

/* Form Actions - Save & Test Buttons */
.form-actions {
    display: flex;
    gap: 15px;
    margin-top: 30px;
    padding-top: 20px;
    border-top: 1px solid rgba(255, 255, 255, 0.08);
}

.btn-save, .btn-test {
    flex: 1;
    padding: 14px 25px;
    border-radius: 6px;
    font-size: 0.95em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
    border: none;
}

.btn-save {
    background: #ffffff;
    color: #000000;
}

.btn-save:hover {
    background: rgba(255, 255, 255, 0.9);
    transform: translateY(-2px);
}

.btn-test {
    background: rgba(255, 255, 255, 0.08);
    color: rgba(255, 255, 255, 0.8);
    border: 1px solid rgba(255, 255, 255, 0.2);
}

.btn-test:hover {
    background: rgba(255, 255, 255, 0.12);
    color: #ffffff;
    border-color: rgba(255, 255, 255, 0.4);
}

@media (max-width: 600px) {
    body {
        padding: 0;
        align-items: flex-start;
    }
    
    .header {
        padding: 40px 25px 30px 25px;
    }
    
    .header h1 {
        font-size: 1.6em;
        letter-spacing: 0.3px;
    }
    
    .info-box {
        gap: 50px;
        flex-direction: column;
        margin-top: 20px;
    }
    
    .tabs {
        flex-wrap: wrap;
        padding: 0;
    }
    
    .tab {
        flex: 1 1 50%;
        font-size: 0.85em;
        padding: 20px 12px;
        border-right: none;
        border-bottom: 1px solid rgba(255, 255, 255, 0.08);
    }
    
    .tab:nth-child(odd) {
        border-right: 1px solid rgba(255, 255, 255, 0.08);
    }
    
    .tab::after {
        width: 40%;
    }
    
    .tab-content {
        padding: 35px 25px;
    }
    
    .info-content {
        padding: 0;
    }
    
    .info-content h2 {
        font-size: 1.3em;
        margin-bottom: 35px;
    }
    
    .info-section h3 {
        font-size: 1.05em;
    }
    
    .info-section p {
        font-size: 0.9em;
    }
    
    .button-group {
        flex-direction: column;
        gap: 15px;
    }
    
    .info-button {
        width: 100%;
        text-align: center;
    }
    
    .ap-step-flow {
        flex-direction: column;
        align-items: stretch;
    }
    
    .ap-arrow {
        transform: rotate(90deg);
        text-align: center;
    }
}

/* OTA Update Styles */
.ota-section {
    margin-bottom: 50px;
}

.ota-section h2 {
    font-size: 1.6em;
    font-weight: 400;
    text-align: center;
    color: #ffffff;
    margin-bottom: 30px;
    letter-spacing: 0.5px;
}

.ota-version-card {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    padding: 20px;
    margin-bottom: 20px;
    text-align: center;
}

.ota-version-label {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.5);
    margin-bottom: 10px;
    text-transform: uppercase;
    letter-spacing: 1px;
}

.ota-version-value {
    font-size: 1.8em;
    color: #ffffff;
    font-weight: 500;
    letter-spacing: 1px;
}

.ota-update-card {
    background: rgba(59, 130, 246, 0.1);
    border: 1px solid rgba(59, 130, 246, 0.3);
    border-radius: 8px;
    padding: 20px;
    margin-bottom: 20px;
}

.ota-update-header {
    display: flex;
    align-items: center;
    gap: 10px;
    margin-bottom: 15px;
}

.ota-update-icon {
    font-size: 1.5em;
    color: rgb(59, 130, 246);
}

.ota-update-title {
    font-size: 1.1em;
    color: rgb(59, 130, 246);
    font-weight: 500;
}

.ota-update-version {
    font-size: 1.4em;
    color: #ffffff;
    font-weight: 500;
    margin-bottom: 8px;
}

.ota-update-date {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.5);
    margin-bottom: 15px;
}

.ota-release-notes {
    font-size: 0.9em;
    color: rgba(255, 255, 255, 0.7);
    line-height: 1.6;
    padding: 15px;
    background: rgba(0, 0, 0, 0.2);
    border-radius: 6px;
    max-height: 200px;
    overflow-y: auto;
    white-space: pre-wrap;
}

.ota-toggle-row {
    display: flex;
    align-items: center;
    gap: 15px;
    margin-bottom: 15px;
}

.ota-auto-info {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.5);
    line-height: 1.6;
    margin-bottom: 25px;
    padding: 12px 15px;
    background: rgba(255, 255, 255, 0.03);
    border-left: 3px solid rgba(255, 255, 255, 0.2);
    border-radius: 4px;
}

.ota-actions {
    display: flex;
    gap: 15px;
    margin-bottom: 20px;
}

.btn-ota-check, .btn-ota-update {
    flex: 1;
    padding: 14px 25px;
    border-radius: 6px;
    font-size: 0.95em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
    border: none;
}

.btn-ota-check {
    background: rgba(255, 255, 255, 0.08);
    color: rgba(255, 255, 255, 0.8);
    border: 1px solid rgba(255, 255, 255, 0.2);
}

.btn-ota-check:hover {
    background: rgba(255, 255, 255, 0.12);
    color: #ffffff;
    border-color: rgba(255, 255, 255, 0.4);
}

.btn-ota-update {
    background: rgb(59, 130, 246);
    color: #ffffff;
}

.btn-ota-update:hover {
    background: rgb(37, 99, 235);
    transform: translateY(-2px);
}

.ota-progress-container {
    margin-top: 20px;
    padding: 20px;
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
}

.ota-progress-label {
    font-size: 0.9em;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 10px;
    text-align: center;
}

.ota-progress-bar {
    width: 100%;
    height: 8px;
    background: rgba(255, 255, 255, 0.1);
    border-radius: 4px;
    overflow: hidden;
    margin-bottom: 10px;
}

.ota-progress-fill {
    height: 100%;
    background: linear-gradient(90deg, rgb(59, 130, 246), rgb(37, 99, 235));
    width: 0%;
    transition: width 0.3s ease;
    border-radius: 4px;
}

.ota-progress-percent {
    font-size: 1.2em;
    color: #ffffff;
    font-weight: 500;
    text-align: center;
}

.ota-status-message {
    margin-top: 20px;
    padding: 15px 20px;
    border-radius: 6px;
    font-size: 0.9em;
    text-align: center;
}

.ota-status-message.success {
    background: rgba(16, 185, 129, 0.15);
    border: 1px solid rgba(16, 185, 129, 0.4);
    color: rgb(16, 185, 129);
}

.ota-status-message.error {
    background: rgba(239, 68, 68, 0.15);
    border: 1px solid rgba(239, 68, 68, 0.4);
    color: rgb(239, 68, 68);
}

.ota-status-message.info {
    background: rgba(59, 130, 246, 0.15);
    border: 1px solid rgba(59, 130, 246, 0.4);
    color: rgb(59, 130, 246);
}

.info-divider {
    height: 1px;
    background: rgba(255, 255, 255, 0.1);
    margin: 50px 0;
}

/* Dimmer Styles */
.dimmer-status-card {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    padding: 20px;
    margin-bottom: 25px;
}

.dimmer-status-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 20px;
    padding-bottom: 15px;
    border-bottom: 1px solid rgba(255, 255, 255, 0.08);
}

.dimmer-status-label {
    font-size: 0.75em;
    color: rgba(255, 255, 255, 0.5);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 500;
}

.dimmer-status-badge {
    padding: 5px 15px;
    border-radius: 4px;
    font-size: 0.75em;
    font-weight: 500;
    letter-spacing: 0.5px;
    background: rgba(255, 255, 255, 0.05);
    color: rgba(255, 255, 255, 0.4);
}

.dimmer-status-badge.connected {
    background: rgba(16, 185, 129, 0.2);
    color: rgb(16, 185, 129);
}

.dimmer-status-badge.error {
    background: rgba(239, 68, 68, 0.2);
    color: rgb(239, 68, 68);
}

.dimmer-info-grid {
    display: grid;
    grid-template-columns: repeat(2, 1fr);
    gap: 20px;
}

.dimmer-info-item {
    display: flex;
    flex-direction: column;
    gap: 8px;
}

.dimmer-info-label {
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.5);
    text-transform: uppercase;
    letter-spacing: 0.5px;
}

.dimmer-info-value {
    font-size: 1.1em;
    color: #ffffff;
    font-weight: 400;
}

.dimmer-config-section {
    background: rgba(255, 255, 255, 0.02);
    border: 1px solid rgba(255, 255, 255, 0.08);
    border-radius: 8px;
    padding: 15px;
    margin-bottom: 12px;
}

.dimmer-section-title {
    font-size: 0.95em;
    font-weight: 500;
    color: #ffffff;
    margin-bottom: 12px;
    text-align: center;
    letter-spacing: 0.5px;
}

.dimmer-calibration-info {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    line-height: 1.6;
    margin-bottom: 20px;
    padding: 12px 15px;
    background: rgba(255, 255, 255, 0.03);
    border-left: 3px solid rgba(255, 255, 255, 0.2);
    border-radius: 4px;
}

.dimmer-ratio-group {
    margin-bottom: 20px;
}

.dimmer-ratio-group label {
    display: block;
    margin-bottom: 12px;
    font-size: 0.9em;
    font-weight: 500;
    color: #ffffff;
    text-align: center;
}

.dimmer-ratio-buttons {
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 10px;
    margin-bottom: 15px;
}

.ratio-btn {
    padding: 12px;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 6px;
    color: rgba(255, 255, 255, 0.6);
    font-size: 1.1em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
}

.ratio-btn:hover {
    background: rgba(255, 255, 255, 0.08);
    border-color: rgba(255, 255, 255, 0.3);
    color: rgba(255, 255, 255, 0.9);
}

.ratio-btn.active {
    background: rgb(59, 130, 246);
    color: #ffffff;
    border-color: rgb(59, 130, 246);
}

.dimmer-ratio-display {
    text-align: center;
    padding: 10px;
    background: rgba(255, 255, 255, 0.03);
    border-radius: 6px;
}

.ratio-label {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.5);
    margin-right: 10px;
}

.ratio-value {
    font-size: 1.3em;
    color: #ffffff;
    font-weight: 500;
}

/* Compact Calibration Styles */
.dimmer-compact-section {
    padding: 15px;
    margin-bottom: 15px;
}

.dimmer-calibration-info-compact {
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.6);
    line-height: 1.4;
    margin-bottom: 12px;
    padding: 8px 10px;
    background: rgba(255, 255, 255, 0.03);
    border-left: 2px solid rgba(255, 255, 255, 0.2);
    border-radius: 4px;
}

.dimmer-ratio-buttons-compact {
    display: grid;
    grid-template-columns: repeat(5, 1fr);
    gap: 6px;
    margin-bottom: 10px;
}

.ratio-btn-compact {
    padding: 8px 4px;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 6px;
    color: rgba(255, 255, 255, 0.6);
    font-size: 0.9em;
    cursor: pointer;
    transition: all 0.2s ease;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 2px;
}

.ratio-btn-compact .ratio-num {
    font-size: 1.2em;
    font-weight: 600;
}

.ratio-desc-compact {
    font-size: 0.7em;
    opacity: 0.7;
}

.ratio-btn-compact:hover {
    background: rgba(255, 255, 255, 0.08);
    border-color: rgba(255, 255, 255, 0.3);
    color: rgba(255, 255, 255, 0.9);
}

.ratio-btn-compact.active {
    background: rgb(59, 130, 246);
    color: #ffffff;
    border-color: rgb(59, 130, 246);
}

.dimmer-ratio-display-compact {
    text-align: center;
    padding: 6px;
    background: rgba(255, 255, 255, 0.03);
    border-radius: 4px;
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    margin-bottom: 10px;
}

.ratio-value-compact {
    font-size: 1.2em;
    color: #ffffff;
    font-weight: 600;
    margin-left: 6px;
}

.btn-compact {
    padding: 10px 20px;
    font-size: 0.9em;
}

.dimmer-scan-info {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    line-height: 1.6;
    margin-bottom: 20px;
    padding: 12px 15px;
    background: rgba(255, 255, 255, 0.03);
    border-left: 3px solid rgba(255, 255, 255, 0.2);
    border-radius: 4px;
}

.dimmer-scan-progress {
    margin-top: 20px;
    padding: 15px;
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
}

.scan-progress-text {
    font-size: 0.9em;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 10px;
    text-align: center;
}

.scan-progress-bar {
    width: 100%;
    height: 6px;
    background: rgba(255, 255, 255, 0.1);
    border-radius: 3px;
    overflow: hidden;
}

.scan-progress-fill {
    height: 100%;
    background: linear-gradient(90deg, rgb(59, 130, 246), rgb(37, 99, 235));
    width: 0%;
    animation: scanProgress 2s ease-in-out infinite;
    border-radius: 3px;
}

@keyframes scanProgress {
    0% { width: 0%; }
    50% { width: 100%; }
    100% { width: 0%; }
}

.dimmer-devices-list {
    margin-top: 20px;
}

.devices-list-title {
    font-size: 0.9em;
    color: rgba(255, 255, 255, 0.7);
    margin-bottom: 15px;
    text-align: center;
}

.scanned-device-item {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 6px;
    padding: 15px;
    margin-bottom: 10px;
    display: flex;
    justify-content: space-between;
    align-items: center;
    transition: all 0.3s ease;
}

.scanned-device-item:hover {
    background: rgba(255, 255, 255, 0.05);
    border-color: rgba(255, 255, 255, 0.2);
}

.scanned-device-info {
    flex: 1;
}

.scanned-device-ip {
    font-size: 1em;
    color: #ffffff;
    font-weight: 500;
    margin-bottom: 5px;
}

.scanned-device-type {
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.5);
}

.scanned-device-connect {
    padding: 8px 20px;
    background: rgb(59, 130, 246);
    color: #ffffff;
    border: none;
    border-radius: 4px;
    font-size: 0.85em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
}

.scanned-device-connect:hover {
    background: rgb(37, 99, 235);
    transform: translateY(-2px);
}

/* Dimmer Brightness Section - NEW */
.dimmer-brightness-section {
    margin-top: 20px;
    padding-top: 20px;
    border-top: 1px solid rgba(255, 255, 255, 0.08);
}

.brightness-header {
    display: flex;
    justify-content: space-between;
    align-items: center;
    margin-bottom: 12px;
}

.brightness-label {
    font-size: 0.75em;
    color: rgba(255, 255, 255, 0.5);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 500;
}

.brightness-value {
    font-size: 1.5em;
    color: #ffffff;
    font-weight: 600;
    font-family: 'Courier New', monospace;
}

.brightness-bar-container {
    margin-bottom: 12px;
}

.brightness-bar-bg {
    width: 100%;
    height: 24px;
    background: rgba(255, 255, 255, 0.1);
    border-radius: 12px;
    overflow: hidden;
    position: relative;
}

.brightness-bar-fill {
    height: 100%;
    background: linear-gradient(90deg, rgb(251, 191, 36), rgb(245, 158, 11));
    border-radius: 12px;
    transition: width 0.3s ease;
    box-shadow: 0 0 10px rgba(251, 191, 36, 0.5);
}

.brightness-status-row {
    display: flex;
    justify-content: space-between;
    align-items: center;
    font-size: 0.8em;
}

.power-status {
    color: rgba(255, 255, 255, 0.6);
    font-weight: 500;
}

.power-status.on {
    color: rgb(16, 185, 129);
}

.control-hint {
    color: rgba(255, 255, 255, 0.4);
    font-style: italic;
}

/* Connection Status Inline - NEW */
.connection-status-inline {
    display: flex;
    align-items: center;
    gap: 10px;
    padding: 12px 15px;
    background: rgba(255, 255, 255, 0.03);
    border-radius: 6px;
    margin-bottom: 15px;
}

.status-dot {
    width: 10px;
    height: 10px;
    border-radius: 50%;
    background: rgba(255, 255, 255, 0.3);
    animation: statusPulse 2s ease-in-out infinite;
}

.status-dot.connected {
    background: rgb(16, 185, 129);
}

.status-dot.connecting {
    background: rgb(251, 191, 36);
}

.status-dot.error {
    background: rgb(239, 68, 68);
}

@keyframes statusPulse {
    0%, 100% { opacity: 1; }
    50% { opacity: 0.5; }
}

.status-text {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.7);
}

/* Input with Button - NEW */
.input-with-button {
    display: flex;
    gap: 10px;
}

.input-with-button input {
    flex: 1;
}

.btn-inline-scan {
    padding: 12px 20px;
    background: rgba(255, 255, 255, 0.08);
    color: rgba(255, 255, 255, 0.7);
    border: 1px solid rgba(255, 255, 255, 0.15);
    border-radius: 4px;
    font-size: 1.2em;
    cursor: pointer;
    transition: all 0.3s ease;
}

.btn-inline-scan:hover {
    background: rgba(255, 255, 255, 0.12);
    border-color: rgba(255, 255, 255, 0.25);
    transform: scale(1.05);
}

/* Button Icons - NEW */
.btn-icon {
    margin-right: 8px;
    font-size: 1.1em;
}

/* Enhanced Ratio Buttons - NEW */
.ratio-btn {
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 5px;
}

.ratio-num {
    font-size: 1.2em;
    font-weight: 600;
}

.ratio-desc {
    font-size: 0.7em;
    opacity: 0.7;
}

/* Scan Button Enhancement - NEW */
.btn-scan {
    width: 100%;
    justify-content: center;
    font-size: 1em;
}

.scan-icon {
    display: inline-block;
    animation: rotate 2s linear infinite;
}

@keyframes rotate {
    from { transform: rotate(0deg); }
    to { transform: rotate(360deg); }
}

/* Devices Container - NEW */
.devices-container {
    max-height: 400px;
    overflow-y: auto;
    padding-right: 5px;
}

.devices-container::-webkit-scrollbar {
    width: 6px;
}

.devices-container::-webkit-scrollbar-track {
    background: rgba(255, 255, 255, 0.05);
    border-radius: 3px;
}

.devices-container::-webkit-scrollbar-thumb {
    background: rgba(255, 255, 255, 0.2);
    border-radius: 3px;
}

.devices-container::-webkit-scrollbar-thumb:hover {
    background: rgba(255, 255, 255, 0.3);
}

@media (max-width: 600px) {
    .dimmer-info-grid {
        grid-template-columns: 1fr;
        gap: 15px;
    }
    
    .dimmer-ratio-buttons {
        grid-template-columns: repeat(5, 1fr);
        gap: 8px;
    }
    
    .ratio-btn {
        padding: 10px;
        font-size: 1em;
    }
    
    .brightness-value {
        font-size: 1.2em;
    }
    
    .dimmer-status-bar {
        flex-direction: column;
        gap: 15px;
    }
    
    .status-bar-item {
        width: 100%;
        text-align: center;
    }
}

/* NEW DESIGN - Status Bar - 4 Column Layout (Reference: Connection Status Card) */
.dimmer-status-bar-new {
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    padding: 20px 25px;
    margin-bottom: 25px;
}

.dimmer-status-bar-new {
    display: grid;
    grid-template-columns: repeat(4, 1fr);
    gap: 0;
}

.status-col {
    display: flex;
    flex-direction: column;
    gap: 8px;
    text-align: center;
    padding: 0 15px;
    border-right: 1px solid rgba(255, 255, 255, 0.15);
}

.status-col:last-child {
    border-right: none;
}

.status-col-label {
    font-size: 0.75em;
    color: rgba(255, 255, 255, 0.5);
    text-transform: uppercase;
    letter-spacing: 1px;
    font-weight: 500;
}

.status-col-value {
    font-size: 1.1em;
    color: #ffffff;
    font-weight: 400;
    display: flex;
    flex-direction: column;
    align-items: center;
    gap: 3px;
}

.status-power-text {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.6);
    margin-top: 2px;
}

/* Calibration Controls - Up/Down Buttons */
.calibration-controls {
    display: flex;
    align-items: center;
    gap: 6px;
    justify-content: center;
}

.calibration-value-display {
    font-size: 1.4em;
    font-weight: 500;
    color: #ffffff;
    min-width: 22px;
    text-align: center;
}

.btn-cal-up,
.btn-cal-down {
    background: rgba(255, 255, 255, 0.08);
    border: 1px solid rgba(255, 255, 255, 0.15);
    color: rgba(255, 255, 255, 0.7);
    font-size: 0.75em;
    width: 20px;
    height: 20px;
    border-radius: 3px;
    cursor: pointer;
    transition: all 0.2s ease;
    display: flex;
    align-items: center;
    justify-content: center;
    padding: 0;
    line-height: 1;
}

.btn-cal-up:hover,
.btn-cal-down:hover {
    background: rgba(255, 255, 255, 0.15);
    border-color: rgba(255, 255, 255, 0.3);
    color: #ffffff;
}

.btn-cal-up:active,
.btn-cal-down:active {
    transform: scale(0.9);
}

.btn-cal-up:disabled,
.btn-cal-down:disabled {
    opacity: 0.3;
    cursor: not-allowed;
    transform: none;
}

.btn-status-connect,
.btn-status-disconnect {
    padding: 8px 16px;
    border: none;
    border-radius: 4px;
    font-size: 0.85em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.2s ease;
    white-space: nowrap;
}

.btn-status-connect {
    background: rgba(255, 255, 255, 0.1);
    color: #ffffff;
    border: 1px solid rgba(255, 255, 255, 0.2);
}

.btn-status-connect:hover {
    background: rgba(255, 255, 255, 0.15);
    border-color: rgba(255, 255, 255, 0.3);
}

.btn-status-disconnect {
    background: rgba(239, 68, 68, 0.2);
    color: rgb(239, 68, 68);
    border: 1px solid rgba(239, 68, 68, 0.3);
}

.btn-status-disconnect:hover {
    background: rgba(239, 68, 68, 0.3);
    border-color: rgba(239, 68, 68, 0.5);
}

@media (max-width: 768px) {
    .dimmer-status-bar-new {
        grid-template-columns: repeat(2, 1fr);
        gap: 20px 0;
    }
    
    .status-col {
        border-right: none;
        border-bottom: none;
        padding: 0 10px;
    }
    
    .status-col:nth-child(odd) {
        border-right: 1px solid rgba(255, 255, 255, 0.15);
    }
}

@media (max-width: 480px) {
    .dimmer-status-bar-new {
        grid-template-columns: 1fr;
        gap: 15px;
    }
    
    .status-col:nth-child(odd) {
        border-right: none;
    }
}

/* Calibration Slider */
.calibration-slider-container {
    padding: 20px 0 10px 0;
}

.slider-labels {
    display: flex;
    justify-content: space-between;
    margin-bottom: 8px;
    font-size: 0.75em;
    color: rgba(255, 255, 255, 0.35);
    letter-spacing: 0.3px;
    font-weight: 400;
}

.slider-label-left,
.slider-label-center,
.slider-label-right {
    flex: 1;
    transition: color 0.3s ease;
}

.slider-label-left {
    text-align: left;
}

.slider-label-center {
    text-align: center;
}

.slider-label-right {
    text-align: right;
}

.calibration-slider {
    width: 100%;
    height: 4px;
    -webkit-appearance: none;
    appearance: none;
    background: linear-gradient(to right, 
        rgba(255, 255, 255, 0.08), 
        rgba(255, 255, 255, 0.12), 
        rgba(255, 255, 255, 0.08));
    border-radius: 2px;
    outline: none;
    margin: 12px 0;
    cursor: pointer;
    position: relative;
    box-shadow: inset 0 1px 2px rgba(0, 0, 0, 0.3);
}

.calibration-slider::-webkit-slider-thumb {
    -webkit-appearance: none;
    appearance: none;
    width: 18px;
    height: 18px;
    background: #ffffff;
    border-radius: 50%;
    cursor: grab;
    transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4),
                0 0 0 1px rgba(255, 255, 255, 0.1);
}

.calibration-slider::-webkit-slider-thumb:hover {
    transform: scale(1.15);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.5),
                0 0 0 1px rgba(255, 255, 255, 0.2),
                0 0 0 4px rgba(255, 255, 255, 0.08);
}

.calibration-slider::-webkit-slider-thumb:active {
    cursor: grabbing;
    transform: scale(1.05);
}

.calibration-slider::-moz-range-thumb {
    width: 18px;
    height: 18px;
    background: #ffffff;
    border: none;
    border-radius: 50%;
    cursor: grab;
    transition: all 0.25s cubic-bezier(0.4, 0, 0.2, 1);
    box-shadow: 0 2px 8px rgba(0, 0, 0, 0.4),
                0 0 0 1px rgba(255, 255, 255, 0.1);
}

.calibration-slider::-moz-range-thumb:hover {
    transform: scale(1.15);
    box-shadow: 0 4px 12px rgba(0, 0, 0, 0.5),
                0 0 0 1px rgba(255, 255, 255, 0.2),
                0 0 0 4px rgba(255, 255, 255, 0.08);
}

.calibration-slider::-moz-range-thumb:active {
    cursor: grabbing;
    transform: scale(1.05);
}

.slider-value-display {
    text-align: center;
    padding: 12px 16px;
    background: rgba(255, 255, 255, 0.03);
    border: 1px solid rgba(255, 255, 255, 0.06);
    border-radius: 8px;
    margin-top: 16px;
    transition: all 0.3s ease;
}

.slider-value-label {
    font-size: 0.8em;
    color: rgba(255, 255, 255, 0.4);
    margin-right: 12px;
    letter-spacing: 0.5px;
    font-weight: 400;
}

.slider-value-number {
    font-size: 1.4em;
    color: #ffffff;
    font-weight: 500;
    letter-spacing: 0.5px;
}

/* Form Actions Row */
.form-actions-row {
    display: flex;
    gap: 10px;
}

.btn-primary,
.btn-secondary {
    flex: 1;
    padding: 10px 18px;
    border: none;
    border-radius: 6px;
    font-size: 0.9em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
}

.btn-primary {
    background: rgb(59, 130, 246);
    color: #ffffff;
}

.btn-primary:hover {
    background: rgb(37, 99, 235);
}

.btn-secondary {
    background: rgba(255, 255, 255, 0.1);
    color: #ffffff;
    border: 1px solid rgba(255, 255, 255, 0.2);
}

.btn-secondary:hover {
    background: rgba(255, 255, 255, 0.15);
}

/* Saved Devices List */
.saved-devices-list {
    display: flex;
    flex-direction: column;
    gap: 10px;
    max-height: 300px;
    overflow-y: auto;
}

.saved-device-empty {
    text-align: center;
    padding: 30px 20px;
    color: rgba(255, 255, 255, 0.4);
    font-size: 0.9em;
    font-style: italic;
}

.saved-device-item {
    display: flex;
    justify-content: space-between;
    align-items: center;
    padding: 12px;
    background: rgba(255, 255, 255, 0.05);
    border: 1px solid rgba(255, 255, 255, 0.1);
    border-radius: 8px;
    transition: all 0.3s ease;
}

.saved-device-item:hover {
    background: rgba(255, 255, 255, 0.08);
    border-color: rgba(255, 255, 255, 0.2);
}

.saved-device-info {
    display: flex;
    flex-direction: column;
    gap: 5px;
}

.saved-device-ip {
    font-size: 1em;
    color: #ffffff;
    font-weight: 500;
}

.saved-device-type {
    font-size: 0.85em;
    color: rgba(255, 255, 255, 0.5);
}

.saved-device-actions {
    display: flex;
    gap: 8px;
}

.btn-device-connect,
.btn-device-remove {
    padding: 8px 16px;
    border: none;
    border-radius: 5px;
    font-size: 0.85em;
    font-weight: 500;
    cursor: pointer;
    transition: all 0.3s ease;
}

.btn-device-connect {
    background: rgb(59, 130, 246);
    color: #ffffff;
}

.btn-device-connect:hover {
    background: rgb(37, 99, 235);
}

.btn-device-remove {
    background: rgba(239, 68, 68, 0.2);
    color: rgb(239, 68, 68);
    border: 1px solid rgba(239, 68, 68, 0.3);
}

.btn-device-remove:hover {
    background: rgba(239, 68, 68, 0.3);
}

/* Main Save Button */
.dimmer-save-container {
    text-align: center;
    margin-top: 30px;
    padding-top: 20px;
    border-top: 1px solid rgba(255, 255, 255, 0.1);
}

.btn-save-main {
    padding: 15px 50px;
    background: linear-gradient(135deg, rgb(59, 130, 246), rgb(16, 185, 129));
    color: #ffffff;
    border: none;
    border-radius: 8px;
    font-size: 1.1em;
    font-weight: 600;
    cursor: pointer;
    transition: all 0.3s ease;
    box-shadow: 0 4px 15px rgba(59, 130, 246, 0.3);
}

.btn-save-main:hover {
    transform: translateY(-2px);
    box-shadow: 0 6px 20px rgba(59, 130, 246, 0.4);
}

.btn-save-main:active {
    transform: translateY(0);
}

)rawliteral";

#endif // SK_CSS_H
