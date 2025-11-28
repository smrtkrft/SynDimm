/**
 * SK_js.h - SmartKraft SynDimm JavaScript Functions
 * Version: v1.0.1
 */
#ifndef SK_JS_H
#define SK_JS_H

const char SK_JS[] PROGMEM = R"rawliteral(
// === HELPER FUNCTIONS ===
const api = (url, opts = {}) => fetch(url, { headers: {'Content-Type': 'application/json'}, ...opts }).then(r => r.ok ? r.json() : Promise.reject('HTTP ' + r.status));
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
        loadDimmerStatus(); loadShutterStatus(); loadSavedDevices(); loadCurrentMode(); updateQuickSettings();
        loadSafePasswords();
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
let lastSelectionModeState = false, lastActiveMode = null;
function loadCurrentMode() {
    api('/getCurrentMode').then(d => {
        const active = d.activeModeStr || d.mode, preview = d.previewModeStr, inSelection = d.inSelectionMode || false;
        const modes = ['dimmer','shutter','safe','alarm'];
        modes.forEach(m => {
            const badge = $(m + '-badge'), btn = $('mode-btn-' + m);
            const isActive = active === m.toUpperCase();
            if(badge) { badge.textContent = isActive ? t('modes.active') : t('modes.passive'); badge.className = 'badge badge-' + (isActive ? 'connected' : 'not-configured'); }
            if(btn) { btn.classList.remove('active','preview'); }
        });
        lastActiveMode = active;
        const activeBtn = $('mode-btn-' + active.toLowerCase());
        if(inSelection && preview) {
            const previewBtn = $('mode-btn-' + preview.toLowerCase());
            if(previewBtn) previewBtn.classList.add('preview');
            if(!lastSelectionModeState) { clearInterval(window.modePollingIntervalId); window.modePollingIntervalId = setInterval(loadCurrentMode, 500); lastSelectionModeState = true; }
        } else {
            if(activeBtn) activeBtn.classList.add('active');
            if(lastSelectionModeState) { clearInterval(window.modePollingIntervalId); window.modePollingIntervalId = setInterval(loadCurrentMode, 3000); lastSelectionModeState = false; }
        }
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
        showNotification(t('notifications.setting_saved'), 'success');
    }).catch(() => showNotification(t('notifications.setting_failed'), 'error')); 
}

// === CONNECTION STATUS ===
function loadConnectionStatus() {
    api('/getStatus').then(d => {
        setText('status-mode', d.mode || '-');
        setText('status-ssid', d.ssid || '-');
        setText('status-ip', d.ip || '-');
        setText('status-mdns', d.mdns ? d.mdns + '.local' : '');
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

// === SETTINGS ===
function loadSavedSettings() {
    api('/getSettings').then(d => {
        if(d.primary.ssid) $('primary-ssid').value = d.primary.ssid;
        if(d.primary.staticIP) $('primary-static').value = d.primary.staticIP;
        if(d.primary.mdns) $('primary-mdns').value = d.primary.mdns;
        if(d.backup.ssid) $('backup-ssid').value = d.backup.ssid;
        if(d.backup.staticIP) $('backup-static').value = d.backup.staticIP;
        if(d.backup.mdns) $('backup-mdns').value = d.backup.mdns;
    }).catch(() => {});
}

// === TABS & ACCORDION ===
function openTab(evt, tabName) {
    document.querySelectorAll('.tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.tab').forEach(t => t.classList.remove('active'));
    $(tabName).classList.add('active');
    evt.currentTarget.classList.add('active');
}
function toggleAccordion(header) {
    const isActive = header.classList.contains('active');
    document.querySelectorAll('.accordion-header').forEach(h => h.classList.remove('active'));
    if(!isActive) header.classList.add('active');
}
function openSafeTab(evt, tabIndex) {
    document.querySelectorAll('.safe-tab-content').forEach(t => t.classList.remove('active'));
    document.querySelectorAll('.safe-tab').forEach(t => t.classList.remove('active'));
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
                    const pwdEl = $('safe-pwd-' + i);
                    const enabledEl = $('safe-pwd-' + i + '-enabled');
                    const urlEl = $('safe-api-' + i + '-url');
                    const headerEl = $('safe-api-' + i + '-header');
                    if(pwdEl) pwdEl.value = p.password || '';
                    if(enabledEl) enabledEl.checked = p.active || false;
                    if(urlEl) urlEl.value = p.apiUrl || '';
                    if(headerEl) headerEl.value = p.apiHeader || '';
                });
            }
        })
        .catch(() => {});
}

function saveSafePassword(idx) {
    const pwd = $('safe-pwd-' + idx).value, enabled = $('safe-pwd-' + idx + '-enabled').checked;
    const url = $('safe-api-' + idx + '-url').value, header = $('safe-api-' + idx + '-header').value;
    if(enabled && !pwd) { showNotification(t('safe.password_empty'), 'error'); return; }
    if(enabled && !/^([LRB]\d*-)*[LRB]\d*$/.test(pwd)) { showNotification(t('safe.invalid_format'), 'error'); return; }
    if(enabled && !url) { showNotification(t('safe.api_empty'), 'error'); return; }
    showNotification(t('safe.saving'), 'info');
    post('/saveSafePassword', { index: idx, password: pwd, pwdEnabled: enabled, api: { url, method: 'GET', header, enabled } })
        .then(r => showNotification(r.success ? t('safe.saved') : t('safe.save_failed') + ': ' + (r.message || ''), r.success ? 'success' : 'error'))
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function testSafeApi(idx) {
    const url = $('safe-api-' + idx + '-url').value, header = $('safe-api-' + idx + '-header').value;
    if(!url) { showNotification(t('safe.api_empty'), 'error'); return; }
    showNotification(t('safe.testing'), 'info');
    post('/testSafeApi', { url, method: 'GET', header })
        .then(r => showNotification(r.success ? t('safe.test_success') + ' ' + r.httpCode : t('safe.test_failed') + ': ' + (r.message || ''), r.success ? 'success' : 'error'))
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
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
function saveNetworkSettings() {
    const data = {
        primary: { ssid: $('primary-ssid').value, password: $('primary-password').value, staticIP: $('primary-static').value, mdns: $('primary-mdns').value || 'dimm' },
        backup: { ssid: $('backup-ssid').value, password: $('backup-password').value, staticIP: $('backup-static').value, mdns: $('backup-mdns').value || 'dimm' }
    };
    const v = validateNetworkSettings(data);
    if(!v.valid) { showNotification(t('common.error') + ': ' + v.errors.join(', '), 'error'); return; }
    showNotification(t('connection.saving'), 'info');
    post('/saveNetwork', data).then(r => {
        if(r.success) { showNotification(t('connection.saved'), 'success'); setTimeout(() => location.reload(), 3000); }
        else showNotification(t('connection.save_failed') + ': ' + (r.message || ''), 'error');
    }).catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}

// === OTA ===
function loadOTASettings() {
    api('/getOTASettings').then(d => {
        setText('ota-current-version', d.currentVersion);
        $('ota-auto-update').checked = d.autoUpdateEnabled;
        if(d.updateAvailable) showUpdateAvailable(d);
    }).catch(() => {});
}
function showUpdateAvailable(d) {
    setText('ota-latest-version', d.latestVersion);
    setText('ota-publish-date', t('info.release_date') + ' ' + formatDate(d.publishedAt));
    setText('ota-release-notes', d.releaseNotes || t('info.no_release_notes'));
    setDisplay('ota-update-card', true); setDisplay('btn-ota-update', true);
}
function formatDate(s) { return s ? new Date(s).toLocaleDateString('tr-TR', { year:'numeric', month:'long', day:'numeric' }) : '-'; }
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
    post('/doUpdate', {}).then(d => { if(d.success) monitorUpdateProgress(); else { showOTAStatus(t('info.update_failed') + ': ' + (d.message || ''), 'error'); setDisplay('ota-progress-container', false); } })
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
document.addEventListener('DOMContentLoaded', () => {
    const toggle = $('ota-auto-update');
    if(toggle) toggle.addEventListener('change', function() {
        post('/saveOTASettings', { autoUpdate: this.checked })
            .then(d => showOTAStatus(d.success ? t('notifications.setting_saved') : t('notifications.setting_failed'), d.success ? 'success' : 'error'))
            .catch(() => {});
    });
});

// === DIMMER ===
let selectedRatio = 3;
function loadDimmerStatus() {
    api('/getDimmerStatus').then(d => updateDimmerUI(d)).catch(() => {});
}
function updateDimmerUI(d) {
    const ip = $('dimmer-status-ip'), br = $('dimmer-status-brightness'), pw = $('dimmer-status-power'), cal = $('dimmer-status-calibration');
    const btnC = $('btn-connect'), btnD = $('btn-disconnect');
    if(d.connected) {
        setText('dimmer-status-ip', d.ip || '-');
        setText('dimmer-status-brightness', (d.brightness || 0) + '%');
        setText('dimmer-status-power', d.isOn ? ' ' + t('common.on') : ' ' + t('common.off'));
        setText('dimmer-status-calibration', d.ratio || 3);
        if(btnC) btnC.style.display = 'none'; if(btnD) btnD.style.display = 'block';
    } else {
        setText('dimmer-status-ip', '-'); setText('dimmer-status-brightness', '0%');
        setText('dimmer-status-power', ' ' + t('common.off')); setText('dimmer-status-calibration', '1-5');
        if(btnC) btnC.style.display = 'block'; if(btnD) btnD.style.display = 'none';
    }
    if(d.ratio && cal) cal.textContent = d.ratio;
}
function updateCalibrationValue(v) { setText('calibration-value', v); }
function adjustCalibration(delta) {
    const el = $('dimmer-status-calibration'), cur = parseInt(el.textContent);
    let nv = Math.max(1, Math.min(5, cur + delta));
    el.textContent = nv;
    post('/setDimmerRatio', { ratio: nv }).then(d => { if(!d.success) { showNotification(t('dimmer.calibration_failed'), 'error'); el.textContent = cur; } })
        .catch(() => { showNotification(t('notifications.connection_error'), 'error'); el.textContent = cur; });
}
function connectDimmerManual() {
    const ip = $('dimmer-ip-input').value;
    if(!ip) { showNotification(t('dimmer.enter_ip'), 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification(t('dimmer.invalid_ip'), 'error'); return; }
    connectDimmer(ip);
}
function connectDimmer(ip) {
    ip = ip || $('dimmer-ip-input').value;
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
function selectRatio(r) { selectedRatio = r; updateRatioButtons(r); setText('dimmer-ratio-value', r); }
function updateRatioButtons(active) {
    document.querySelectorAll('.ratio-btn, .ratio-btn-compact').forEach(b => {
        b.classList.toggle('active', parseInt(b.getAttribute('data-ratio')) === active);
    });
}
function saveDimmerRatio() {
    showNotification(t('dimmer.calibration_saved').replace('!', '...'), 'info');
    post('/setDimmerRatio', { ratio: selectedRatio })
        .then(d => showNotification(d.success ? t('dimmer.calibration_saved') : t('dimmer.calibration_failed'), d.success ? 'success' : 'error'))
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function saveDimmerSettings() {
    const v = parseInt($('calibration-slider').value);
    showNotification(t('dimmer.calibration_saved').replace('!', '...'), 'info');
    post('/setDimmerRatio', { ratio: v })
        .then(d => showNotification(d.success ? t('dimmer.calibration_saved') : t('dimmer.calibration_failed'), d.success ? 'success' : 'error'))
        .catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function loadSavedDevices() {
    api('/getSavedDevices').then(d => displaySavedDevices(d.devices || [])).catch(() => {});
}
function displaySavedDevices(devs) {
    const c = $('saved-devices-list');
    if(!devs.length) { c.innerHTML = '<div class="saved-device-empty">' + t('dimmer.no_saved_devices') + '</div>'; return; }
    const types = { 0: t('dimmer.unknown'), 1: t('dimmer.shelly_dimmer'), 2: t('dimmer.shelly_dali'), 3: t('dimmer.tasmota') };
    c.innerHTML = devs.map(d => `<div class="saved-device-item"><div class="saved-device-info"><div class="saved-device-ip">${d.ip}</div><div class="saved-device-type">${types[d.type] || t('dimmer.unknown')}</div></div><div class="saved-device-actions"><button class="btn-device-connect" onclick="connectToSavedDevice('${d.ip}')">${t('common.connect')}</button><button class="btn-device-remove" onclick="removeSavedDevice('${d.ip}')">${t('dimmer.remove')}</button></div></div>`).join('');
}
function connectToSavedDevice(ip) { $('dimmer-ip-input').value = ip; connectDimmer(ip); }
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
                loadSavedDevices();
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
    const ip = $('shutter-ip-input').value;
    if(!ip) { showNotification(t('dimmer.enter_ip'), 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification(t('dimmer.invalid_ip'), 'error'); return; }
    connectShutter(ip);
}
function connectShutter(ip) {
    ip = ip || $('shutter-ip-input').value;
    showNotification(t('shutter.connecting'), 'info');
    post('/connectShutter', { ip }).then(d => {
        if(d.success) { showNotification(t('shutter.shutter_connected'), 'success'); loadShutterStatus(); }
        else showNotification(t('shutter.connection_failed') + ': ' + (d.message || ''), 'error');
    }).catch(e => showNotification(t('notifications.connection_error') + ': ' + e, 'error'));
}
function disconnectShutter() {
    if(!confirm(t('notifications.confirm_disconnect'))) return;
    post('/disconnectShutter', {}).then(d => { if(d.success) { showNotification(t('shutter.disconnected'), 'success'); loadShutterStatus(); } }).catch(() => {});
}
function loadShutterStatus() {
    api('/getShutterStatus').then(d => {
        setText('shutter-status-ip', d.ip || '-');
        setText('shutter-status-text', d.connected ? t('shutter.connected') : t('shutter.not_connected'));
        const bar = $('shutter-position-bar'); if(bar) bar.style.width = (d.position || 0) + '%';
        setText('shutter-position-percent', (d.position || 0) + '%');
        setText('shutter-encoder-step', d.encoderStep || 3);
        const warn = $('shutter-calibration-warning');
        if(warn) warn.style.display = (d.connected && !d.isCalibrated) ? 'block' : 'none';
    }).catch(() => {});
}
function adjustShutterStep(delta) {
    const el = $('shutter-encoder-step'); if(!el) return;
    let cur = parseInt(el.textContent) || 3, nv = Math.max(1, Math.min(5, cur + delta));
    post('/adjustShutterStep', { step: nv }).then(d => { if(d.success) loadShutterStatus(); }).catch(() => {});
}
let shutterScanProgressInterval = null;
function startShutterNetworkScan() {
    showNotification(t('notifications.scanning'), 'info');
    post('/startShutterScan', {}).then(d => {
        if(d.success) { showNotification(t('notifications.scan_started'), 'success'); setDisplay('btn-stop-shutter-scan', true); pollShutterScanProgress(); }
        else showNotification(t('notifications.scan_failed'), 'error');
    }).catch(() => showNotification(t('common.error'), 'error'));
}
function pollShutterScanProgress() {
    if(shutterScanProgressInterval) clearInterval(shutterScanProgressInterval);
    shutterScanProgressInterval = setInterval(() => {
        api('/getShutterScanProgress').then(d => {
            if(d.scanning) showNotification(t('notifications.scan_progress', {percent: d.percentage, scanned: d.scannedIPs, total: d.totalIPs, count: d.shutterCount}), 'info');
            else {
                clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null;
                setDisplay('btn-stop-shutter-scan', false);
                showNotification(d.shutterCount > 0 ? t('notifications.scan_complete', {count: d.shutterCount}) : t('notifications.scan_complete_none'), d.shutterCount > 0 ? 'success' : 'info');
            }
        }).catch(() => { clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null; setDisplay('btn-stop-shutter-scan', false); });
    }, 1000);
}
function stopShutterNetworkScan() {
    if(!confirm(t('notifications.confirm_stop_scan'))) return;
    post('/stopShutterScan', {}).then(d => {
        if(d.success) { showNotification(t('notifications.scan_stopped'), 'info'); if(shutterScanProgressInterval) { clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null; } setDisplay('btn-stop-shutter-scan', false); }
    }).catch(() => {});
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
)rawliteral";

#endif // SK_JS_H
