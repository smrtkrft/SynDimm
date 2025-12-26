/**
 * SK_js.h - SmartKraft SynDimm JavaScript Functions
 * Version: v1.2.0
 */
#ifndef SK_JS_H
#define SK_JS_H

const char SK_JS[] PROGMEM = R"rawliteral(
// === HELPER FUNCTIONS ===
// API call with retry logic and exponential backoff
let connectionFailCount = 0;
const MAX_FAIL_BEFORE_OFFLINE = 3;
let isDeviceOnline = true;

async function apiWithRetry(url, opts = {}, retries = 2) {
    let delay = 500;
    for (let i = 0; i <= retries; i++) {
        try {
            const r = await fetch(url, { headers: {'Content-Type': 'application/json'}, ...opts });
            if (r.ok) {
                connectionFailCount = 0;
                if (!isDeviceOnline) { isDeviceOnline = true; updateConnectionIndicator(); }
                return r.json();
            }
            if (r.status >= 500 && i < retries) {
                await new Promise(res => setTimeout(res, delay));
                delay *= 2;
                continue;
            }
            return Promise.reject('HTTP ' + r.status);
        } catch (e) {
            if (i < retries) {
                await new Promise(res => setTimeout(res, delay));
                delay *= 2;
            } else {
                connectionFailCount++;
                if (connectionFailCount >= MAX_FAIL_BEFORE_OFFLINE && isDeviceOnline) {
                    isDeviceOnline = false;
                    updateConnectionIndicator();
                }
                throw e;
            }
        }
    }
}

function updateConnectionIndicator() {
    const indicator = $('connection-indicator');
    if (indicator) {
        indicator.className = isDeviceOnline ? 'connection-online' : 'connection-offline';
        indicator.title = isDeviceOnline ? 'Online' : 'Offline - Trying to reconnect...';
    }
    if (!isDeviceOnline) {
        showNotification(t('notifications.device_offline') || 'Device offline - retrying...', 'warning');
    }
}

const api = (url, opts = {}) => apiWithRetry(url, opts, 2);
const post = (url, data) => api(url, { method: 'POST', body: JSON.stringify(data) });
const $ = id => document.getElementById(id);
const setText = (id, txt) => { const el = $(id); if(el) el.textContent = txt; };
const setDisplay = (id, show) => { const el = $(id); if(el) el.style.display = show ? 'block' : 'none'; };

// === LANGUAGE SYSTEM ===
let currentLang = 'en';
let langData = {};

function loadLanguage(lang) {
    return api('/getLang?lang=' + lang).then(data => {
        langData = data;
        currentLang = lang;
        applyLanguage();
        updateLangButtons();
        localStorage.setItem('lang', lang);
        return data;
    });
}

function applyLanguage() {
    // data-lang attribute - updates innerHTML/placeholder
    document.querySelectorAll('[data-lang]').forEach(el => {
        const key = el.getAttribute('data-lang');
        const text = getLangText(key);
        if (text) {
            if (el.tagName === 'INPUT' && el.type === 'text') {
                el.placeholder = text;
            } else {
                el.innerHTML = text;
            }
        }
    });
    // data-lang-title attribute - updates title attribute
    document.querySelectorAll('[data-lang-title]').forEach(el => {
        const key = el.getAttribute('data-lang-title');
        const text = getLangText(key);
        if (text) {
            el.title = text;
        }
    });
    // data-lang-placeholder attribute - updates placeholder
    document.querySelectorAll('[data-lang-placeholder]').forEach(el => {
        const key = el.getAttribute('data-lang-placeholder');
        const text = getLangText(key);
        if (text) {
            el.placeholder = text;
        }
    });
}

function getLangText(key) {
    const keys = key.split('.');
    let value = langData;
    for (const k of keys) {
        if (value && value[k] !== undefined) {
            value = value[k];
        } else {
            return null;
        }
    }
    return typeof value === 'string' ? value : null;
}

function t(key, replacements = {}) {
    let text = getLangText(key) || key;
    for (const [k, v] of Object.entries(replacements)) {
        text = text.replace('{' + k + '}', v);
    }
    return text;
}

function updateLangButtons() {
    document.querySelectorAll('.lang-btn').forEach(btn => {
        btn.classList.remove('active');
        if (btn.classList.contains('lang-btn-' + currentLang)) {
            btn.classList.add('active');
        }
    });
}

function setLanguage(lang) {
    if (lang === currentLang) return;
    loadLanguage(lang).then(() => {
        post('/setLang', { lang }).catch(() => {});
    }).catch(() => {});
}

function initLanguage() {
    return api('/getCurrentLang').then(d => {
        const savedLang = localStorage.getItem('lang') || d.lang || 'en';
        return loadLanguage(savedLang);
    }).catch(() => {
        return loadLanguage('en');
    });
}

// === THEME ===
function setTheme(theme) {
    const isLight = theme === 'light';
    document.body.classList.toggle('light-theme', isLight);
    document.querySelectorAll('input[name="theme"]').forEach(r => r.checked = r.value === theme);
    localStorage.setItem('theme', theme);
}
if(localStorage.getItem('theme') === 'light') document.addEventListener('DOMContentLoaded', () => setTheme('light'));

// === INIT ===
window.addEventListener('DOMContentLoaded', () => {
    // Load language first, then other data
    initLanguage().then(() => {
        loadSavedSettings(); loadConnectionStatus(); loadOTASettings();
        loadDimmerStatus(); loadShutterStatus(); loadSavedDevices(); loadSavedShutterDevices(); loadCurrentMode(); updateQuickSettings();
        loadSafePasswords();
        // Initialize mode config panel based on current mode
        api('/getCurrentMode').then(d => {
            const mode = d.activeModeStr || d.mode || 'DIMMER';
            updateModeConfigPanel(mode);
        }).catch(() => updateModeConfigPanel('DIMMER'));
        setInterval(loadConnectionStatus, 5000);
        setInterval(loadDimmerStatus, 2000);
        setInterval(loadShutterStatus, 2000);
        window.modePollingIntervalId = setInterval(loadCurrentMode, 3000);
        setInterval(updateQuickSettings, 5000);
    });
});

// === QUICK SETTINGS ===
function updateQuickSettings() {
    api('/getStatus').then(d => {
        setText('quick-mode', d.mode || '-');
        setText('quick-ssid', d.ssid || '-');
        setText('quick-ip', d.ip || '-');
    }).catch(() => {});
    api('/getCurrentMode').then(d => {
        setText('quick-mode-name', d.mode || 'Yüklenemedi');
        const icon = $('quick-mode-icon');
        if(icon) icon.style.color = d.mode === 'DIMMER' ? '#f59e0b' : d.mode === 'SHUTTER' ? '#3b82f6' : d.mode === 'SAFE' ? '#10b981' : 'var(--text-muted)';
    }).catch(() => {});
}

// === MODE MANAGEMENT ===
function loadCurrentMode() {
    api('/getCurrentMode').then(d => {
        const active = d.activeModeStr || d.mode;
        const modes = ['dimmer','shutter','safe','alarm'];
        modes.forEach(m => {
            const badge = $(m + '-badge'), btn = $('mode-btn-' + m);
            const isActive = active === m.toUpperCase();
            if(badge) { badge.textContent = isActive ? t('modes.active') : t('modes.passive'); badge.className = 'badge badge-' + (isActive ? 'connected' : 'not-configured'); }
            if(btn) { btn.classList.remove('active'); if(isActive) btn.classList.add('active'); }
        });
    }).catch(() => {
        ['dimmer','shutter','safe','alarm'].forEach(m => {
            const badge = $(m + '-badge');
            if(badge) { badge.textContent = t('modes.error'); badge.className = 'badge badge-not-configured'; }
        });
    });
}
function selectMode(mode) { 
    post('/setMode', { mode: mode }).then(() => {
        loadCurrentMode();
        updateModeConfigPanel(mode);
        // Auto-open mode config accordion
        const accordion = $('mode-config-accordion');
        if(accordion) {
            const header = accordion.querySelector('.accordion-header');
            if(header && !header.classList.contains('active')) {
                header.classList.add('active');
            }
        }
        showNotification(t('notifications.setting_saved'), 'success');
    }).catch(() => showNotification(t('notifications.setting_failed'), 'error')); 
}

// === MODE CONFIG PANEL ===
function updateModeConfigPanel(mode) {
    const panels = ['dimmer', 'shutter', 'safe'];
    panels.forEach(p => {
        const panel = $('config-panel-' + p);
        if(panel) panel.style.display = (p.toUpperCase() === mode) ? 'block' : 'none';
    });
    const badge = $('mode-config-badge');
    if(badge) badge.textContent = mode;
    // Update quick panel data
    if(mode === 'DIMMER') updateQuickDimmerPanel();
    else if(mode === 'SHUTTER') updateQuickShutterPanel();
    else if(mode === 'SAFE') updateQuickSafePanel();
}

function updateQuickDimmerPanel() {
    api('/getDimmerStatus').then(d => {
        const hero = $('dimmer-hero');
        const brightness = d.brightness || 0;
        const isOn = d.isOn;
        const connected = d.connected;
        
        // Update brightness display
        const brightnessEl = $('dimmer-brightness-display');
        if(brightnessEl) brightnessEl.innerHTML = connected ? brightness + '<span>%</span>' : '--<span>%</span>';
        
        // Update power status text
        setText('dimmer-power-status', isOn ? t('common.on') : t('common.off'));
        
        // Update connection status
        setText('dimmer-connection-status', connected ? t('dimmer.connected') : t('dimmer.not_connected'));
        
        // Update IP display with device type (displayName from backend)
        const deviceTypeName = d.deviceType || t('dimmer.unknown');
        const ipText = connected ? (d.ip || '-') + ' • ' + deviceTypeName : t('dimmer.no_device_connected');
        setText('dimmer-ip-display', ipText);
        
        // Update calibration value and buttons
        const ratio = d.ratio || 3;
        setText('dimmer-cal-value', ratio);
        const btnMinus = $('cal-btn-minus'), btnPlus = $('cal-btn-plus');
        if(btnMinus) btnMinus.disabled = !connected || ratio <= 1;
        if(btnPlus) btnPlus.disabled = !connected || ratio >= 5;
        
        // Update hero state class
        if(hero) {
            hero.classList.remove('connected-on', 'connected-off', 'disconnected');
            if(connected && isOn) hero.classList.add('connected-on');
            else if(connected && !isOn) hero.classList.add('connected-off');
            else hero.classList.add('disconnected');
        }
    }).catch(() => {});
    // Update saved devices in quick panel
    api('/getSavedDevices').then(d => displayQuickSavedDevices(d.devices || [])).catch(() => {});
}

function displayQuickSavedDevices(devs) {
    const c = $('quick-saved-devices-list');
    if(!c) return;
    if(!devs.length) { c.innerHTML = '<div class="saved-device-empty">' + t('dimmer.no_saved_devices') + '</div>'; return; }
    const types = { 0: t('dimmer.unknown'), 1: t('dimmer.shelly_dimmer'), 2: t('dimmer.shelly_dali'), 3: t('dimmer.tasmota') };
    c.innerHTML = devs.map(d => `<div class="saved-device-item"><div class="saved-device-info"><div class="saved-device-ip">${d.ip}</div><div class="saved-device-type">${types[d.type] || t('dimmer.unknown')}</div></div><div class="saved-device-actions"><button class="btn-device-connect" onclick="connectToSavedDevice('${d.ip}')">${t('common.connect')}</button></div></div>`).join('');
}

function connectDimmerFromQuick() {
    const ip = $('quick-dimmer-ip-input').value;
    if(!ip) { showNotification(t('dimmer.enter_ip'), 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification(t('dimmer.invalid_ip'), 'error'); return; }
    connectDimmer(ip);
}

function updateQuickShutterPanel() {
    api('/getShutterStatus').then(d => {
        setText('quick-shutter-ip', d.ip || '-');
        setText('quick-shutter-status', d.connected ? t('shutter.connected') : t('shutter.not_connected'));
        const bar = $('quick-shutter-position-bar'); if(bar) bar.style.width = (d.position || 0) + '%';
        setText('quick-shutter-position', (d.position || 0) + '%');
        setText('quick-shutter-step', d.encoderStep || 3);
    }).catch(() => {});
}

function connectShutterFromQuick() {
    const ip = $('quick-shutter-ip-input').value;
    if(!ip) { showNotification(t('dimmer.enter_ip'), 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification(t('dimmer.invalid_ip'), 'error'); return; }
    connectShutter(ip);
}

function updateQuickSafePanel() {
    fetch('/getSafeStatus').then(r => r.json()).then(d => {
        if(d.passwords) {
            d.passwords.forEach((p, i) => {
                const pwdEl = $('quick-safe-pwd-' + i);
                const enabledEl = $('quick-safe-pwd-' + i + '-enabled');
                const apiStatusEl = $('quick-safe-api-' + i + '-status');
                if(pwdEl) pwdEl.value = p.password || '';
                if(enabledEl) enabledEl.checked = p.active || false;
                // API config lazy-load olacak - sadece hasApiConfig göster
                if(apiStatusEl) apiStatusEl.textContent = p.hasApiConfig ? '✓ API' : '';
                // API config'i ayrı yükle
                if(p.hasApiConfig) loadSafeApiConfig(i);
            });
        }
    }).catch(() => {});
}

// Tek bir şifre için API config yükle (lazy-load)
function loadSafeApiConfig(idx) {
    fetch('/getSafeApiConfig?index=' + idx).then(r => r.json()).then(cfg => {
        const urlEl = $('quick-safe-api-' + idx + '-url');
        const methodEl = $('quick-safe-api-' + idx + '-method');
        const contentTypeEl = $('quick-safe-api-' + idx + '-contentType');
        const authEl = $('quick-safe-api-' + idx + '-auth');
        const headersEl = $('quick-safe-api-' + idx + '-headers');
        const bodyEl = $('quick-safe-api-' + idx + '-body');
        if(urlEl) urlEl.value = cfg.url || '';
        if(methodEl) methodEl.value = cfg.method || 0;
        if(contentTypeEl) contentTypeEl.value = cfg.contentType || 'application/json';
        if(authEl) authEl.value = cfg.authorization || '';
        if(headersEl) headersEl.value = cfg.customHeaders || '';
        if(bodyEl) bodyEl.value = cfg.body || '';
    }).catch(() => {});
}

function openQuickSafeTab(evt, tabIndex) {
    document.querySelectorAll('.quick-safe-tab-content').forEach(el => el.style.display = 'none');
    document.querySelectorAll('#config-panel-safe .safe-tab').forEach(el => el.classList.remove('active'));
    const tab = $('quick-safe-tab-' + tabIndex);
    if(tab) tab.style.display = 'block';
    evt.currentTarget.classList.add('active');
}

// === CONNECTION STATUS ===
function loadConnectionStatus() {
    api('/getStatus').then(d => {
        // Update connection tab
        setText('status-mode', d.mode || '-');
        setText('status-ssid', d.ssid || '-');
        setText('status-ip', d.ip || '-');
        setText('status-mdns', d.mdns ? d.mdns + '.local' : '');
        // Update global status bar
        setText('global-status-mode', d.mode || '-');
        setText('global-status-ssid', d.ssid || '-');
        setText('global-status-ip', d.ip || '-');
        setText('global-status-mdns', d.mdns ? d.mdns + '.local' : '');
        // Update header status info
        setText('header-status-mode', d.mode || '-');
        setText('header-status-ssid', d.ssid || '-');
        setText('header-status-ip', d.ip || '-');
        setText('header-status-mdns', d.mdns ? '(' + d.mdns + '.local)' : '');
        // Update AP Mode toggle and UI
        const apToggle = $('ap-mode-enabled');
        if(apToggle && d.apModeEnabled !== undefined) {
            apToggle.checked = d.apModeEnabled;
            updateAPModeUI(d.apModeEnabled);
        }
        const updateBadge = (id, connected, configured) => {
            const el = $(id);
            if(!el) return;
            el.textContent = connected ? t('connection.connected') : configured ? t('connection.configured') : t('common.not_configured');
            el.className = 'badge badge-' + (connected ? 'connected' : configured ? 'passive' : 'not-configured');
        };
        updateBadge('primary-badge', d.connectedTo === 'primary', d.primaryConfigured);
        updateBadge('backup-badge', d.connectedTo === 'backup', d.backupConfigured);
    }).catch(() => {});
}

function toggleAPMode(enabled) {
    updateAPModeUI(enabled);
    post('/setAPMode', { enabled: enabled }).then(d => {
        if(d.success) showNotification(t('notifications.setting_saved'), 'success');
        else showNotification(t('notifications.setting_failed'), 'error');
    }).catch(() => showNotification(t('notifications.connection_error'), 'error'));
}

function updateAPModeUI(enabled) {
    // Update warning visibility
    const warning = $('ap-mode-warning');
    if(warning) warning.style.display = enabled ? 'none' : 'flex';
    
    // Update status text
    const status = $('ap-mode-status');
    if(status) {
        status.textContent = enabled ? t('common.enabled') : t('common.disabled');
        status.className = 'ap-mode-status' + (enabled ? '' : ' disabled');
    }
    
    // Update card class
    const card = $('ap-mode-card');
    if(card) {
        if(enabled) card.classList.remove('disabled');
        else card.classList.add('disabled');
    }
}

// === SETTINGS ===
function loadSavedSettings() {
    api('/getSettings').then(d => {
        // Quick tab - use safe element access
        if(d.primary) {
            if(d.primary.ssid) { const el = $('quick-primary-ssid'); if(el) el.value = d.primary.ssid; }
            if(d.primary.staticIP) { const el = $('quick-primary-static'); if(el) el.value = d.primary.staticIP; }
            if(d.primary.mdns) { const el = $('quick-primary-mdns'); if(el) el.value = d.primary.mdns; }
        }
        if(d.backup) {
            if(d.backup.ssid) { const el = $('quick-backup-ssid'); if(el) el.value = d.backup.ssid; }
            if(d.backup.staticIP) { const el = $('quick-backup-static'); if(el) el.value = d.backup.staticIP; }
            if(d.backup.mdns) { const el = $('quick-backup-mdns'); if(el) el.value = d.backup.mdns; }
        }
        // Update badges
        updateQuickConnectionBadges(d);
    }).catch(() => {});
}

function updateQuickConnectionBadges(d) {
    const updateBadge = (id, configured) => {
        const el = $(id);
        if(!el) return;
        el.textContent = configured ? t('connection.configured') : t('common.not_configured');
        el.className = 'badge badge-' + (configured ? 'passive' : 'not-configured');
    };
    updateBadge('quick-primary-badge', d.primary && d.primary.ssid);
    updateBadge('quick-backup-badge', d.backup && d.backup.ssid);
}

// === TABS & ACCORDION ===
function openTab(evt, tabName) {
    document.querySelectorAll('.tab-content').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(el => el.classList.remove('active'));
    $(tabName).classList.add('active');
    evt.currentTarget.classList.add('active');
}
function toggleAccordion(header) {
    const isActive = header.classList.contains('active');
    document.querySelectorAll('.accordion-header').forEach(h => h.classList.remove('active'));
    if(!isActive) header.classList.add('active');
}
function toggleInfoBox(header) {
    header.classList.toggle('open');
    const content = header.nextElementSibling;
    if(content) content.classList.toggle('open');
}
function openSafeTab(evt, tabIndex) {
    document.querySelectorAll('.safe-tab-content').forEach(el => el.classList.remove('active'));
    document.querySelectorAll('.safe-tab').forEach(el => el.classList.remove('active'));
    $('safe-tab-' + tabIndex).classList.add('active');
    evt.currentTarget.classList.add('active');
}

// === NOTIFICATION ===
function showNotification(msg, type = 'info') {
    const n = $('notification'), m = $('notification-message');
    n.className = 'notification ' + type;
    m.textContent = msg;
    n.style.display = 'flex';
    if(type !== 'error') setTimeout(closeNotification, 5000);
}
function closeNotification() { $('notification').style.display = 'none'; }

// === SAFE PASSWORD ===
function loadSafePasswords() {
    fetch('/getSafeStatus')
        .then(r => r.json())
        .then(d => {
            if(d.passwords) {
                d.passwords.forEach((p, i) => {
                    const pwdEl = $('quick-safe-pwd-' + i);
                    const enabledEl = $('quick-safe-pwd-' + i + '-enabled');
                    const apiStatusEl = $('quick-safe-api-' + i + '-status');
                    if(pwdEl) pwdEl.value = p.password || '';
                    if(enabledEl) enabledEl.checked = p.active || false;
                    if(apiStatusEl) apiStatusEl.textContent = p.hasApiConfig ? '✓ API' : '';
                    // Lazy-load API config
                    if(p.hasApiConfig) loadSafeApiConfig(i);
                });
            }
        })
        .catch(() => {});
}

function saveSafePassword(idx) {
    const pwd = $('quick-safe-pwd-' + idx) ? $('quick-safe-pwd-' + idx).value : '';
    const enabled = $('quick-safe-pwd-' + idx + '-enabled') ? $('quick-safe-pwd-' + idx + '-enabled').checked : false;
    const url = $('quick-safe-api-' + idx + '-url') ? $('quick-safe-api-' + idx + '-url').value : '';
    const method = $('quick-safe-api-' + idx + '-method') ? $('quick-safe-api-' + idx + '-method').value : 'GET';
    const contentType = $('quick-safe-api-' + idx + '-contentType') ? $('quick-safe-api-' + idx + '-contentType').value : 'application/json';
    const authorization = $('quick-safe-api-' + idx + '-auth') ? $('quick-safe-api-' + idx + '-auth').value : '';
    const customHeaders = $('quick-safe-api-' + idx + '-headers') ? $('quick-safe-api-' + idx + '-headers').value : '';
    const body = $('quick-safe-api-' + idx + '-body') ? $('quick-safe-api-' + idx + '-body').value : '';
    
    if(enabled && !pwd) { showNotification(t('safe.password_empty'), 'error'); return; }
    if(enabled && !/^([LRB]\d*-)*[LRB]\d*$/.test(pwd)) { showNotification(t('safe.invalid_format'), 'error'); return; }
    
    showNotification(t('safe.saving'), 'info');
    post('/saveSafePassword', { 
        index: idx, 
        password: pwd, 
        pwdEnabled: enabled, 
        api: { 
            url, 
            method, 
            contentType, 
            authorization, 
            customHeaders, 
            body, 
            enabled: url.length > 0 
        } 
    })
        .then(r => showNotification(r.success ? t('safe.saved') : t('safe.save_failed') + ': ' + (r.message || ''), r.success ? 'success' : 'error'))
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function testSafeApi(idx) {
    const url = $('quick-safe-api-' + idx + '-url') ? $('quick-safe-api-' + idx + '-url').value : '';
    const method = $('quick-safe-api-' + idx + '-method') ? $('quick-safe-api-' + idx + '-method').value : 'GET';
    const contentType = $('quick-safe-api-' + idx + '-contentType') ? $('quick-safe-api-' + idx + '-contentType').value : 'application/json';
    const authorization = $('quick-safe-api-' + idx + '-auth') ? $('quick-safe-api-' + idx + '-auth').value : '';
    const customHeaders = $('quick-safe-api-' + idx + '-headers') ? $('quick-safe-api-' + idx + '-headers').value : '';
    const body = $('quick-safe-api-' + idx + '-body') ? $('quick-safe-api-' + idx + '-body').value : '';
    
    if(!url) { showNotification(t('safe.api_empty'), 'error'); return; }
    showNotification(t('safe.testing'), 'info');
    post('/testSafeApi', { url, method, contentType, authorization, customHeaders, body })
        .then(r => showNotification(r.success ? t('safe.test_success') : t('safe.test_failed') + ': ' + (r.message || ''), r.success ? 'success' : 'error'))
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}

// === TEACHING MODE (SAFE PASSWORD) ===
let teachingInterval = null;
let teachingStartTime = 0;
let teachingIndex = -1;
const TEACHING_TIMEOUT = 15000;

function startTeachPassword(idx) {
    showNotification(t('safe.teaching_starting'), 'info');
    post('/startSafeTeaching', { index: idx })
        .then(r => {
            if(r.success) {
                teachingIndex = idx;
                teachingStartTime = Date.now();
                showTeachingOverlay(idx);
                startTeachingPolling(idx);
                showNotification(t('safe.teaching_started'), 'success');
            } else {
                showNotification(t('safe.teaching_failed') + ': ' + (r.message || ''), 'error');
            }
        })
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}

function completeTeachPassword(idx) {
    post('/completeSafeTeaching', {})
        .then(r => {
            if(r.success) {
                const pwdInput = $('quick-safe-pwd-' + idx);
                if(pwdInput) pwdInput.value = r.pattern || '';
                hideTeachingOverlay(idx);
                stopTeachingPolling();
                showNotification(t('safe.password_saved') + ': ' + (r.pattern || ''), 'success');
            } else {
                showNotification(t('safe.save_failed') + ': ' + (r.message || ''), 'error');
            }
        })
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}

function cancelTeachPassword(idx) {
    post('/cancelSafeTeaching', {})
        .then(() => {
            hideTeachingOverlay(idx);
            stopTeachingPolling();
            showNotification(t('safe.teaching_cancelled'), 'info');
        })
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}

function showTeachingOverlay(idx) {
    const overlay = $('teaching-overlay-' + idx);
    if(overlay) {
        overlay.style.display = 'flex';
        overlay.classList.add('active');
    }
}

function hideTeachingOverlay(idx) {
    const overlay = $('teaching-overlay-' + idx);
    if(overlay) {
        overlay.style.display = 'none';
        overlay.classList.remove('active');
    }
    teachingIndex = -1;
}

function startTeachingPolling(idx) {
    stopTeachingPolling();
    teachingInterval = setInterval(() => {
        // Timer update
        const elapsed = Date.now() - teachingStartTime;
        const remaining = Math.max(0, Math.ceil((TEACHING_TIMEOUT - elapsed) / 1000));
        const timerEl = $('teaching-timer-' + idx);
        if(timerEl) timerEl.textContent = remaining + 's';
        
        // Timeout check
        if(remaining <= 0) {
            hideTeachingOverlay(idx);
            stopTeachingPolling();
            showNotification(t('safe.teaching_timeout'), 'warning');
            return;
        }
        
        // Poll status
        fetch('/getTeachingStatus')
            .then(r => r.json())
            .then(data => {
                const patternEl = $('teaching-pattern-' + idx);
                if(patternEl) patternEl.textContent = data.pattern || '-';
                
                // Teaching completed (server stopped teaching)
                if(!data.teaching && teachingIndex === idx) {
                    hideTeachingOverlay(idx);
                    stopTeachingPolling();
                    if(data.pattern && data.pattern.length > 0) {
                        // Put pattern in input field
                        const pwdInput = $('quick-safe-pwd-' + idx);
                        if(pwdInput) pwdInput.value = data.pattern;
                        showNotification(t('safe.teaching_complete') + ': ' + data.pattern, 'success');
                    }
                }
            })
            .catch(() => {});
    }, 300);
}

function stopTeachingPolling() {
    if(teachingInterval) {
        clearInterval(teachingInterval);
        teachingInterval = null;
    }
}

// === NETWORK VALIDATION & SAVE ===
function validateNetworkSettings(d) {
    const errs = [], ipRe = /^(\d{1,3}\.){3}\d{1,3}$/;
    const check = (pre, cfg) => {
        if(cfg.ssid && cfg.ssid.length > 32) errs.push(pre + ' ' + t('connection.validation_ssid_long'));
        if(cfg.password && (cfg.password.length < 8 || cfg.password.length > 63)) errs.push(pre + ' ' + t('connection.validation_password'));
        if(cfg.staticIP && !ipRe.test(cfg.staticIP)) errs.push(pre + ' ' + t('connection.validation_ip'));
        if(cfg.mdns && cfg.mdns.length > 63) errs.push(pre + ' ' + t('connection.validation_mdns'));
    };
    check('Primary', d.primary); check('Backup', d.backup);
    if(!d.primary.ssid && !d.backup.ssid) errs.push(t('connection.validation_wifi_required'));
    return { valid: errs.length === 0, errors: errs };
}

function saveDeviceSettings() {
    // Get element values safely
    const getVal = (id) => { const el = $(id); return el ? el.value : ''; };
    
    // Save network settings
    const networkData = {
        primary: { ssid: getVal('quick-primary-ssid'), password: getVal('quick-primary-password'), staticIP: getVal('quick-primary-static'), mdns: getVal('quick-primary-mdns') || 'dimm' },
        backup: { ssid: getVal('quick-backup-ssid'), password: getVal('quick-backup-password'), staticIP: getVal('quick-backup-static'), mdns: getVal('quick-backup-mdns') || 'dimm' }
    };
    
    console.log('Network data to save:', networkData);
    
    const v = validateNetworkSettings(networkData);
    if(!v.valid) { showNotification(t('common.error') + ': ' + v.errors.join(', '), 'error'); return; }
    
    // Save theme
    const theme = document.querySelector('input[name="theme"]:checked');
    if(theme) localStorage.setItem('theme', theme.value);
    
    // Save language
    const lang = currentLang || 'en';
    localStorage.setItem('lang', lang);
    
    showNotification(t('connection.saving'), 'info');
    post('/saveNetwork', networkData).then(r => {
        if(r.success) { showNotification(t('quick.settings_saved'), 'success'); setTimeout(() => location.reload(), 3000); }
        else showNotification(t('connection.save_failed') + ': ' + (r.message || ''), 'error');
    }).catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}

// === OTA ===
function loadOTASettings() {
    api('/getOTASettings').then(d => {
        setText('ota-current-version', d.currentVersion);
        setText('quick-ota-version', d.currentVersion);
        if($('ota-auto-update')) $('ota-auto-update').checked = d.autoUpdateEnabled;
        if(d.updateAvailable) showUpdateAvailable(d);
    }).catch(() => {});
}
function showUpdateAvailable(d) {
    setText('ota-latest-version', d.latestVersion);
    setText('ota-publish-date', t('info.release_date') + ' ' + formatDate(d.publishedAt));
    setText('ota-release-notes', d.releaseNotes || t('info.no_release_notes'));
    setDisplay('ota-update-card', true); setDisplay('btn-ota-update', true);
}
function formatDate(s) { if(!s) return '-'; const d = new Date(s); return d.getDate().toString().padStart(2,'0') + '.' + (d.getMonth()+1).toString().padStart(2,'0') + '.' + d.getFullYear(); }
function checkForUpdate() {
    showOTAStatus(t('info.checking'), 'info');
    setDisplay('btn-ota-update', false); setDisplay('ota-update-card', false);
    post('/checkOTAUpdate', {}).then(d => {
        if(d.success) { if(d.updateAvailable) { showUpdateAvailable(d); showOTAStatus(t('info.update_found'), 'success'); } else showOTAStatus(t('info.up_to_date'), 'success'); }
        else showOTAStatus(t('info.update_failed') + ': ' + (d.message || ''), 'error');
    }).catch(e => showOTAStatus(t('notifications.connection_error') + ': ' + e, 'error'));
}
function performUpdate() {
    if(!confirm(t('info.update_confirm'))) return;
    showOTAStatus(t('info.update_starting'), 'info');
    setDisplay('btn-ota-update', false); setDisplay('ota-progress-container', true);
    post('/performOTAUpdate', {}).then(d => { if(d.success) monitorUpdateProgress(); else { showOTAStatus(t('info.update_failed') + ': ' + (d.message || ''), 'error'); setDisplay('ota-progress-container', false); } })
        .catch(e => { showOTAStatus(t('notifications.connection_error') + ': ' + e, 'error'); setDisplay('ota-progress-container', false); });
}
function monitorUpdateProgress() {
    const iv = setInterval(() => {
        api('/getOTASettings').then(d => {
            updateProgressBar(d.progress || 0);
            if(d.status === 6) { clearInterval(iv); updateProgressBar(100); showOTAStatus(t('info.update_success'), 'success'); setTimeout(() => location.reload(), 3000); }
            else if(d.status === 7) { clearInterval(iv); setDisplay('ota-progress-container', false); showOTAStatus(t('info.update_failed') + ': ' + (d.errorMessage || ''), 'error'); }
        }).catch(() => {});
    }, 1000);
}
function updateProgressBar(p) {
    $('ota-progress-fill').style.width = p + '%';
    setText('ota-progress-percent', p + '%');
    setText('ota-progress-label', p < 30 ? t('info.downloading') : p < 100 ? t('info.installing') : t('info.completed'));
}
function showOTAStatus(msg, type) {
    const el = $('ota-status-message');
    el.textContent = msg; el.className = 'ota-status-message ' + type; el.style.display = 'block';
    if(type !== 'error') setTimeout(() => el.style.display = 'none', 5000);
}
function checkOTAUpdate() {
    showQuickOTAStatus(t('info.checking'), 'info');
    setDisplay('quick-ota-update-available', false);
    post('/checkOTAUpdate', {}).then(d => {
        if(d.success) {
            if(d.updateAvailable) {
                setText('quick-ota-new-version', d.latestVersion);
                setDisplay('quick-ota-update-available', true);
                showQuickOTAStatus(t('info.update_found'), 'success');
                showUpdateAvailable(d);
            } else showQuickOTAStatus(t('info.up_to_date'), 'success');
        } else showQuickOTAStatus(t('info.update_failed') + ': ' + (d.message || ''), 'error');
    }).catch(e => showQuickOTAStatus(t('notifications.connection_error') + ': ' + e, 'error'));
}
function startOTAUpdate() {
    if(!confirm(t('info.update_confirm'))) return;
    showQuickOTAStatus(t('info.update_starting'), 'info');
    setDisplay('quick-ota-update-available', false);
    setDisplay('quick-ota-progress', true);
    post('/performOTAUpdate', {}).then(d => {
        if(d.success) monitorQuickUpdateProgress();
        else { showQuickOTAStatus(t('info.update_failed') + ': ' + (d.message || ''), 'error'); setDisplay('quick-ota-progress', false); }
    }).catch(e => { showQuickOTAStatus(t('notifications.connection_error') + ': ' + e, 'error'); setDisplay('quick-ota-progress', false); });
}
function monitorQuickUpdateProgress() {
    const iv = setInterval(() => {
        api('/getOTASettings').then(d => {
            updateQuickProgressBar(d.progress || 0);
            if(d.status === 6) { clearInterval(iv); updateQuickProgressBar(100); showQuickOTAStatus(t('info.update_success'), 'success'); setTimeout(() => location.reload(), 3000); }
            else if(d.status === 7) { clearInterval(iv); setDisplay('quick-ota-progress', false); showQuickOTAStatus(t('info.update_failed') + ': ' + (d.errorMessage || ''), 'error'); }
        }).catch(() => {});
    }, 1000);
}
function updateQuickProgressBar(p) {
    const fill = $('quick-ota-progress-fill'), text = $('quick-ota-progress-text');
    if(fill) fill.style.width = p + '%';
    if(text) text.textContent = p + '%';
}
function showQuickOTAStatus(msg, type) {
    const el = $('quick-ota-status');
    if(el) { el.textContent = msg; el.className = 'ota-quick-status ' + type; el.style.display = 'block'; if(type !== 'error') setTimeout(() => el.style.display = 'none', 5000); }
}
document.addEventListener('DOMContentLoaded', () => {
    const toggle = $('ota-auto-update');
    if(toggle) toggle.addEventListener('change', function() {
        post('/saveOTASettings', { autoUpdate: this.checked })
            .then(d => showOTAStatus(d.success ? t('notifications.setting_saved') : t('notifications.setting_failed'), d.success ? 'success' : 'error'))
            .catch(() => {});
    });
});

// === DIMMER ===
function loadDimmerStatus() {
    api('/getDimmerStatus').then(d => updateDimmerUI(d)).catch(() => {});
}
function updateDimmerUI(d) {
    // Hero Display elements (new Design 3)
    const hero = $('dimmer-hero');
    const brightnessDisplay = $('dimmer-brightness-display');
    const powerStatus = $('dimmer-power-status');
    const connectionStatus = $('dimmer-connection-status');
    const ipDisplay = $('dimmer-ip-display');
    const calValue = $('dimmer-cal-value');
    const calBtnMinus = $('cal-btn-minus');
    const calBtnPlus = $('cal-btn-plus');
    
    if(d.connected) {
        // Update hero class for CSS styling
        if(hero) hero.className = 'dimmer-hero' + (d.isOn ? '' : ' connected-off');
        
        // Update brightness display
        if(brightnessDisplay) brightnessDisplay.innerHTML = (d.brightness || 0) + '<span>%</span>';
        
        // Update power status
        if(powerStatus) powerStatus.textContent = d.isOn ? t('common.on') : t('common.off');
        
        // Update connection status
        if(connectionStatus) connectionStatus.textContent = t('dimmer.connected');
        
        // Update IP + device type display
        if(ipDisplay) {
            const deviceInfo = d.deviceType ? (d.ip + ' - ' + d.deviceType) : d.ip;
            ipDisplay.textContent = deviceInfo;
        }
        
        // Update calibration value
        if(calValue) calValue.textContent = d.ratio || 3;
        
        // Enable calibration buttons
        if(calBtnMinus) calBtnMinus.disabled = (d.ratio || 3) <= 1;
        if(calBtnPlus) calBtnPlus.disabled = (d.ratio || 3) >= 5;
        
    } else {
        // Update hero class for disconnected state
        if(hero) hero.className = 'dimmer-hero disconnected';
        
        // Not connected state
        if(brightnessDisplay) brightnessDisplay.innerHTML = '--<span>%</span>';
        if(powerStatus) powerStatus.textContent = t('common.off');
        if(connectionStatus) connectionStatus.textContent = t('dimmer.not_connected');
        if(ipDisplay) ipDisplay.textContent = t('dimmer.no_device_connected');
        if(calValue) calValue.textContent = '-';
        
        // Disable calibration buttons
        if(calBtnMinus) calBtnMinus.disabled = true;
        if(calBtnPlus) calBtnPlus.disabled = true;
    }
}
function updateCalibrationValue(v) { setText('calibration-value', v); }
function adjustCalibration(delta) {
    const el = $('dimmer-cal-value'), cur = parseInt(el.textContent);
    if(isNaN(cur)) return;
    let nv = Math.max(1, Math.min(5, cur + delta));
    el.textContent = nv;
    // Update button states
    const btnMinus = $('cal-btn-minus'), btnPlus = $('cal-btn-plus');
    if(btnMinus) btnMinus.disabled = nv <= 1;
    if(btnPlus) btnPlus.disabled = nv >= 5;
    post('/setDimmerRatio', { ratio: nv }).then(d => { if(!d.success) { showNotification(t('dimmer.calibration_failed'), 'error'); el.textContent = cur; } })
        .catch(() => { showNotification(t('notifications.connection_error'), 'error'); el.textContent = cur; });
}
function connectDimmerManual() {
    const ip = $('quick-dimmer-ip-input').value;
    if(!ip) { showNotification(t('dimmer.enter_ip'), 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification(t('dimmer.invalid_ip'), 'error'); return; }
    connectDimmer(ip);
}
function connectDimmer(ip) {
    ip = ip || $('quick-dimmer-ip-input').value;
    showNotification(t('dimmer.connecting'), 'info');
    post('/connectDimmer', { ip }).then(d => {
        if(d.success) { showNotification(t('dimmer.connected'), 'success'); loadDimmerStatus(); loadSavedDevices(); }
        else showNotification(t('dimmer.connection_failed') + ': ' + (d.message || ''), 'error');
    }).catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function disconnectDimmer() {
    if(!confirm(t('notifications.confirm_disconnect'))) return;
    post('/disconnectDimmer', {}).then(d => { if(d.success) { showNotification(t('dimmer.disconnected'), 'success'); loadDimmerStatus(); } }).catch(() => {});
}
function loadSavedDevices() {
    api('/getSavedDevices').then(d => displaySavedDevices(d.devices || [])).catch(() => {});
}
function loadDiscoveredDevices() {
    api('/getScanResults').then(d => {
        if(d.devices && d.devices.length > 0) displayDiscoveredDevices(d.devices);
        else loadSavedDevices();
    }).catch(() => loadSavedDevices());
}
function displayDiscoveredDevices(devs) {
    const c = $('quick-saved-devices-list');
    if(!c) return;
    const cats = { 1: t('dimmer.shelly_dimmer'), 2: t('shutter.shelly_shutter'), 3: t('dimmer.switch') };
    c.innerHTML = '<div class="discovered-header">' + t('dimmer.found_devices') + ' (' + devs.length + ')</div>' + 
        devs.map(d => `<div class="saved-device-item discovered"><div class="saved-device-info"><div class="saved-device-ip">${d.ip}</div><div class="saved-device-type">${d.displayName || cats[d.category] || t('dimmer.unknown')}</div></div><div class="saved-device-actions"><button class="btn-device-connect" onclick="connectToDiscoveredDevice('${d.ip}', ${d.category})">${t('common.connect')}</button></div></div>`).join('');
}
function connectToDiscoveredDevice(ip, category) {
    if(category === 2) { $('quick-shutter-ip-input').value = ip; connectShutter(ip); }
    else { $('quick-dimmer-ip-input').value = ip; connectDimmerFromQuick(); }
}
function displaySavedDevices(devs) {
    const c = $('quick-saved-devices-list');
    if(!c) return;
    if(!devs.length) { c.innerHTML = '<div class="saved-device-empty">' + t('dimmer.no_saved_devices') + '</div>'; return; }
    const types = { 0: t('dimmer.unknown'), 1: t('dimmer.shelly_dimmer'), 2: t('dimmer.shelly_dali'), 3: t('dimmer.tasmota') };
    c.innerHTML = devs.map(d => `<div class="saved-device-item"><div class="saved-device-info"><div class="saved-device-ip">${d.ip}</div><div class="saved-device-type">${types[d.type] || t('dimmer.unknown')}</div></div><div class="saved-device-actions"><button class="btn-device-connect" onclick="connectToSavedDevice('${d.ip}')">${t('common.connect')}</button><button class="btn-device-remove" onclick="removeSavedDevice('${d.ip}')">${t('dimmer.remove')}</button></div></div>`).join('');
}
function connectToSavedDevice(ip) { $('quick-dimmer-ip-input').value = ip; connectDimmerFromQuick(); }
function removeSavedDevice(ip) {
    if(!confirm(t('notifications.confirm_remove'))) return;
    post('/removeSavedDevice', { ip }).then(d => { if(d.success) { showNotification(t('dimmer.device_removed'), 'success'); loadSavedDevices(); } else showNotification(t('dimmer.remove_failed'), 'error'); })
        .catch(e => showNotification(t('common.error') + ': ' + e, 'error'));
}

// === NETWORK SCAN ===
let scanProgressInterval = null;
function startNetworkScan() {
    showNotification(t('notifications.scanning'), 'info');
    setDisplay('btn-stop-scan', true);
    post('/scanNetwork', {}).then(d => { if(d.success) pollScanProgress(); else { showNotification(t('notifications.scan_failed'), 'error'); setDisplay('btn-stop-scan', false); } })
        .catch(e => { showNotification(t('common.error') + ': ' + e, 'error'); setDisplay('btn-stop-scan', false); });
}
function pollScanProgress() {
    if(scanProgressInterval) clearInterval(scanProgressInterval);
    scanProgressInterval = setInterval(() => {
        api('/getScanProgress').then(d => {
            if(d.scanning) showNotification(t('notifications.scan_progress', {percent: d.percentage, scanned: d.scannedIPs, total: d.totalIPs, count: d.dimmerCount}), 'info');
            else {
                clearInterval(scanProgressInterval); scanProgressInterval = null;
                setDisplay('btn-stop-scan', false);
                showNotification(d.dimmerCount > 0 ? t('notifications.scan_complete', {count: d.dimmerCount}) : t('notifications.scan_complete_none'), d.dimmerCount > 0 ? 'success' : 'info');
                loadDiscoveredDevices();
            }
        }).catch(() => { clearInterval(scanProgressInterval); scanProgressInterval = null; setDisplay('btn-stop-scan', false); });
    }, 1000);
}
function stopNetworkScan() {
    if(!confirm(t('notifications.confirm_stop_scan'))) return;
    post('/stopScan', {}).then(d => {
        if(d.success) { showNotification(t('notifications.scan_stopped'), 'info'); if(scanProgressInterval) { clearInterval(scanProgressInterval); scanProgressInterval = null; } setDisplay('btn-stop-scan', false); loadSavedDevices(); }
    }).catch(() => {});
}

// === SHUTTER ===
function connectShutterManual() {
    const ip = $('quick-shutter-ip-input').value;
    if(!ip) { showNotification(t('dimmer.enter_ip'), 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification(t('dimmer.invalid_ip'), 'error'); return; }
    connectShutter(ip);
}
function connectShutter(ip) {
    ip = ip || $('quick-shutter-ip-input').value;
    showNotification(t('shutter.connecting'), 'info');
    post('/connectShutter', { ip }).then(d => {
        if(d.success) { showNotification(t('shutter.shutter_connected'), 'success'); loadShutterStatus(); loadSavedShutterDevices(); }
        else showNotification(t('shutter.connection_failed') + ': ' + (d.message || ''), 'error');
    }).catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function disconnectShutter() {
    if(!confirm(t('notifications.confirm_disconnect'))) return;
    post('/disconnectShutter', {}).then(d => { if(d.success) { showNotification(t('shutter.disconnected'), 'success'); loadShutterStatus(); } }).catch(() => {});
}
function loadShutterStatus() {
    api('/getShutterStatus').then(d => updateShutterUI(d)).catch(() => {});
}
function updateShutterUI(d) {
    // Hero Display elements (same structure as Dimmer)
    const hero = $('shutter-hero');
    const positionDisplay = $('shutter-position-display');
    const movementStatus = $('shutter-movement-status');
    const connectionStatus = $('shutter-connection-status');
    const ipDisplay = $('shutter-ip-display');
    const calValue = $('shutter-cal-value');
    const calBtnMinus = $('shutter-cal-btn-minus');
    const calBtnPlus = $('shutter-cal-btn-plus');
    
    if(d.connected) {
        // Determine movement state for hero class
        let heroClass = 'shutter-hero';
        if(d.status === 'moving_up') heroClass += ' moving-up';
        else if(d.status === 'moving_down') heroClass += ' moving-down';
        else if(d.position === 0) heroClass += ' open';
        else if(d.position === 100) heroClass += ' closed';
        if(hero) hero.className = heroClass;
        
        // Update position display
        if(positionDisplay) positionDisplay.innerHTML = (d.position || 0) + '<span>%</span>';
        
        // Update movement status
        if(movementStatus) {
            let statusText = t('shutter.stopped');
            if(d.status === 'moving_up') statusText = '↑ ' + t('shutter.moving_up');
            else if(d.status === 'moving_down') statusText = '↓ ' + t('shutter.moving_down');
            else if(d.position === 0) statusText = t('shutter.open');
            else if(d.position === 100) statusText = t('shutter.closed');
            movementStatus.textContent = statusText;
        }
        
        // Update connection status
        if(connectionStatus) connectionStatus.textContent = t('shutter.connected');
        
        // Update IP + device type display
        if(ipDisplay) {
            const deviceInfo = d.model ? (d.ip + ' - ' + d.model) : d.ip;
            ipDisplay.textContent = deviceInfo;
        }
        
        // Update calibration value
        if(calValue) calValue.textContent = d.encoderStep || 3;
        
        // Enable calibration buttons
        if(calBtnMinus) calBtnMinus.disabled = (d.encoderStep || 3) <= 1;
        if(calBtnPlus) calBtnPlus.disabled = (d.encoderStep || 3) >= 5;
        
    } else {
        // Not connected state
        if(hero) hero.className = 'shutter-hero disconnected';
        if(positionDisplay) positionDisplay.innerHTML = '--<span>%</span>';
        if(movementStatus) movementStatus.textContent = t('shutter.not_connected');
        if(connectionStatus) connectionStatus.textContent = '';
        if(ipDisplay) ipDisplay.textContent = t('shutter.no_device_connected');
        if(calValue) calValue.textContent = '-';
        
        // Disable calibration buttons
        if(calBtnMinus) calBtnMinus.disabled = true;
        if(calBtnPlus) calBtnPlus.disabled = true;
    }
}
function adjustShutterStep(delta) {
    const el = $('shutter-cal-value'); if(!el) return;
    let cur = parseInt(el.textContent) || 3, nv = Math.max(1, Math.min(5, cur + delta));
    post('/adjustShutterStep', { step: nv }).then(d => { if(d.success) loadShutterStatus(); }).catch(() => {});
}
let shutterScanProgressInterval = null;
function startShutterNetworkScan() {
    showNotification(t('notifications.scanning'), 'info');
    post('/scanShutterNetwork', {}).then(d => {
        if(d.success) { showNotification(t('notifications.scan_started'), 'success'); setDisplay('btn-stop-shutter-scan', true); pollShutterScanProgress(); }
        else showNotification(t('notifications.scan_failed'), 'error');
    }).catch(() => showNotification(t('common.error'), 'error'));
}
function pollShutterScanProgress() {
    if(shutterScanProgressInterval) clearInterval(shutterScanProgressInterval);
    shutterScanProgressInterval = setInterval(() => {
        api('/getScanProgress').then(d => {
            if(d.scanning) showNotification(t('notifications.scan_progress', {percent: d.percentage, scanned: d.scannedIPs, total: d.totalIPs, count: d.shutterCount}), 'info');
            else {
                clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null;
                setDisplay('btn-stop-shutter-scan', false);
                showNotification(d.shutterCount > 0 ? t('notifications.scan_complete', {count: d.shutterCount}) : t('notifications.scan_complete_none'), d.shutterCount > 0 ? 'success' : 'info');
                loadSavedShutterDevices();
            }
        }).catch(() => { clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null; setDisplay('btn-stop-shutter-scan', false); });
    }, 1000);
}
function stopShutterNetworkScan() {
    if(!confirm(t('notifications.confirm_stop_scan'))) return;
    post('/stopScan', {}).then(d => {
        if(d.success) { showNotification(t('notifications.scan_stopped'), 'info'); if(shutterScanProgressInterval) { clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null; } setDisplay('btn-stop-shutter-scan', false); loadSavedShutterDevices(); }
    }).catch(() => {});
}

// === SHUTTER SAVED DEVICES ===
function loadSavedShutterDevices() {
    api('/getSavedShutterDevices').then(d => displaySavedShutterDevices(d.devices || [])).catch(() => {});
}
function displaySavedShutterDevices(devs) {
    const c = $('shutter-saved-devices-list');
    if(!c) return;
    if(!devs.length) { c.innerHTML = '<div class="saved-device-empty">' + t('shutter.no_saved_devices') + '</div>'; return; }
    const types = { 0: t('dimmer.unknown'), 1: t('shutter.shelly_25'), 2: t('shutter.shelly_plus_2pm') };
    c.innerHTML = devs.map(d => `<div class="saved-device-item"><div class="saved-device-info"><div class="saved-device-ip">${d.ip}</div><div class="saved-device-type">${types[d.type] || t('dimmer.unknown')}</div></div><div class="saved-device-actions"><button class="btn-device-connect" onclick="connectToSavedShutterDevice('${d.ip}')">${t('common.connect')}</button><button class="btn-device-remove" onclick="removeSavedShutterDevice('${d.ip}')">${t('shutter.remove')}</button></div></div>`).join('');
}
function connectToSavedShutterDevice(ip) { $('quick-shutter-ip-input').value = ip; connectShutter(ip); }
function removeSavedShutterDevice(ip) {
    if(!confirm(t('notifications.confirm_remove'))) return;
    post('/removeSavedShutterDevice', { ip }).then(d => { if(d.success) { showNotification(t('shutter.device_removed'), 'success'); loadSavedShutterDevices(); } else showNotification(t('shutter.remove_failed'), 'error'); })
        .catch(e => showNotification(t('common.error') + ': ' + e, 'error'));
}

// === SYSTEM ACTIONS ===
function restartDevice() {
    showNotification(t('info.restarting'), 'info');
    post('/restart', {}).then(() => {
        showNotification(t('info.restart_success'), 'success');
        setTimeout(() => location.reload(), 5000);
    }).catch(e => showNotification(t('common.error') + ': ' + e, 'error'));
}

function showFactoryResetConfirm() {
    $('factory-reset-confirm').style.display = 'block';
    $('factory-reset-input').value = '';
    $('factory-reset-input').focus();
}

function hideFactoryResetConfirm() {
    $('factory-reset-confirm').style.display = 'none';
    $('factory-reset-input').value = '';
}

function confirmFactoryReset() {
    const input = $('factory-reset-input').value.trim().toLowerCase();
    const confirmWord = currentLang === 'tr' ? 'evet' : currentLang === 'de' ? 'ja' : 'yes';
    if(input !== confirmWord) {
        showNotification(t('info.factory_reset_validation'), 'error');
        return;
    }
    showNotification(t('info.factory_resetting'), 'info');
    post('/factoryReset', {}).then(() => {
        showNotification(t('info.factory_reset_success'), 'success');
        setTimeout(() => location.reload(), 10000);
    }).catch(e => showNotification(t('common.error') + ': ' + e, 'error'));
}

// === WIFI SCAN MODAL ===
let wifiScanTargetInput = null;
function openWifiScanModal(targetInputId) {
    wifiScanTargetInput = targetInputId;
    $('wifi-scan-modal').style.display = 'flex';
    scanWifiNetworks();
}
function closeWifiScanModal() {
    $('wifi-scan-modal').style.display = 'none';
    wifiScanTargetInput = null;
}
function scanWifiNetworks() {
    setDisplay('wifi-scan-loading', true);
    setDisplay('wifi-scan-results', false);
    setDisplay('wifi-scan-empty', false);
    api('/scanWiFi').then(d => {
        setDisplay('wifi-scan-loading', false);
        if(d.count > 0) {
            setDisplay('wifi-scan-results', true);
            renderWifiNetworks(d.networks);
        } else {
            setDisplay('wifi-scan-empty', true);
        }
    }).catch(() => {
        setDisplay('wifi-scan-loading', false);
        setDisplay('wifi-scan-empty', true);
        showNotification(t('notifications.scan_failed'), 'error');
    });
}
function renderWifiNetworks(networks) {
    const list = $('wifi-network-list');
    list.innerHTML = '';
    networks.sort((a, b) => b.rssi - a.rssi).forEach(net => {
        const securityText = net.encryption === 'open' ? t('connection.wifi_open') : t('connection.wifi_secured');
        const item = document.createElement('div');
        item.className = 'wifi-network-item';
        item.onclick = () => selectWifiNetwork(net.ssid);
        item.innerHTML = '<span class="wifi-network-ssid">' + net.ssid + '</span><span class="wifi-network-security">(' + securityText + ')</span>';
        list.appendChild(item);
    });
}
function selectWifiNetwork(ssid) {
    if(wifiScanTargetInput) {
        $(wifiScanTargetInput).value = ssid;
    }
    closeWifiScanModal();
    showNotification(t('connection.wifi_selected', {ssid: ssid}), 'success');
}
)rawliteral";

#endif // SK_JS_H
