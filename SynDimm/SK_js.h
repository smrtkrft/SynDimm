/**
 * SK_js.h - SmartKraft SynDimm JavaScript Functions
 * Version: v0.9.1 (Optimized)
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
    loadSavedSettings(); loadConnectionStatus(); loadOTASettings();
    loadDimmerStatus(); loadShutterStatus(); loadSavedDevices(); loadCurrentMode(); updateQuickSettings();
    loadSafePasswords(); // Safe şifrelerini yükle
    setInterval(loadConnectionStatus, 5000);
    setInterval(loadDimmerStatus, 2000);
    setInterval(loadShutterStatus, 2000);
    window.modePollingIntervalId = setInterval(loadCurrentMode, 3000);
    setInterval(updateQuickSettings, 5000);
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
            if(badge) { badge.textContent = isActive ? 'Aktif' : 'Pasif'; badge.className = 'badge badge-' + (isActive ? 'connected' : 'not-configured'); }
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
            if(badge) { badge.textContent = 'Hata'; badge.className = 'badge badge-not-configured'; }
        });
    });
}
function selectMode(mode) { post('/setMode', {}).then(() => loadCurrentMode()).catch(() => {}); }
function setLanguage(lang) { }

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
            el.textContent = connected ? 'Connected' : configured ? 'Configured' : 'Not Configured';
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
    if(enabled && !pwd) { showNotification('Şifre boş olamaz!', 'error'); return; }
    if(enabled && !/^([LRB]\d*-)*[LRB]\d*$/.test(pwd)) { showNotification('Geçersiz şifre formatı! Örnek: L3-R12-L11-R3-B', 'error'); return; }
    if(enabled && !url) { showNotification('API URL boş olamaz!', 'error'); return; }
    showNotification('Şifre kaydediliyor...', 'info');
    post('/saveSafePassword', { index: idx, password: pwd, pwdEnabled: enabled, api: { url, method: 'GET', header, enabled } })
        .then(r => showNotification(r.success ? 'Şifre başarıyla kaydedildi!' : 'Hata: ' + (r.message || 'Kaydedilemedi'), r.success ? 'success' : 'error'))
        .catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
}
function testSafeApi(idx) {
    const url = $('safe-api-' + idx + '-url').value, header = $('safe-api-' + idx + '-header').value;
    if(!url) { showNotification('API URL boş olamaz!', 'error'); return; }
    showNotification('API test ediliyor...', 'info');
    post('/testSafeApi', { url, method: 'GET', header })
        .then(r => showNotification(r.success ? 'API test başarılı! HTTP ' + r.httpCode : 'API test başarısız: ' + (r.message || 'Hata'), r.success ? 'success' : 'error'))
        .catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
}

// === NETWORK VALIDATION & SAVE ===
function validateNetworkSettings(d) {
    const errs = [], ipRe = /^(\d{1,3}\.){3}\d{1,3}$/;
    const check = (pre, cfg) => {
        if(cfg.ssid && cfg.ssid.length > 32) errs.push(pre + ' SSID max 32 karakter');
        if(cfg.password && (cfg.password.length < 8 || cfg.password.length > 63)) errs.push(pre + ' şifre 8-63 karakter');
        if(cfg.staticIP && !ipRe.test(cfg.staticIP)) errs.push(pre + ' Static IP geçersiz');
        if(cfg.mdns && cfg.mdns.length > 63) errs.push(pre + ' mDNS max 63 karakter');
    };
    check('Primary', d.primary); check('Backup', d.backup);
    if(!d.primary.ssid && !d.backup.ssid) errs.push('En az bir WiFi ağı gerekli');
    return { valid: errs.length === 0, errors: errs };
}
function saveNetworkSettings() {
    const data = {
        primary: { ssid: $('primary-ssid').value, password: $('primary-password').value, staticIP: $('primary-static').value, mdns: $('primary-mdns').value || 'dimm' },
        backup: { ssid: $('backup-ssid').value, password: $('backup-password').value, staticIP: $('backup-static').value, mdns: $('backup-mdns').value || 'dimm' }
    };
    const v = validateNetworkSettings(data);
    if(!v.valid) { showNotification('Hata: ' + v.errors.join(', '), 'error'); return; }
    showNotification('Ayarlar kaydediliyor...', 'info');
    post('/saveNetwork', data).then(r => {
        if(r.success) { showNotification('Ayarlar kaydedildi! Yeniden başlatılıyor...', 'success'); setTimeout(() => location.reload(), 3000); }
        else showNotification('Hata: ' + (r.message || 'Kaydedilemedi'), 'error');
    }).catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
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
    setText('ota-publish-date', 'Yayın Tarihi: ' + formatDate(d.publishedAt));
    setText('ota-release-notes', d.releaseNotes || 'Sürüm notları yok');
    setDisplay('ota-update-card', true); setDisplay('btn-ota-update', true);
}
function formatDate(s) { return s ? new Date(s).toLocaleDateString('tr-TR', { year:'numeric', month:'long', day:'numeric' }) : '-'; }
function checkForUpdate() {
    showOTAStatus('Güncelleme kontrol ediliyor...', 'info');
    setDisplay('btn-ota-update', false); setDisplay('ota-update-card', false);
    post('/checkOTAUpdate', {}).then(d => {
        if(d.success) { if(d.updateAvailable) { showUpdateAvailable(d); showOTAStatus('Yeni sürüm bulundu!', 'success'); } else showOTAStatus('Cihazınız güncel!', 'success'); }
        else showOTAStatus('Hata: ' + (d.message || 'Kontrol edilemedi'), 'error');
    }).catch(e => showOTAStatus('Bağlantı hatası: ' + e, 'error'));
}
function performUpdate() {
    if(!confirm('Güncelleme sırasında cihaz yeniden başlatılacak. Devam?')) return;
    showOTAStatus('Güncelleme başlatılıyor...', 'info');
    setDisplay('btn-ota-update', false); setDisplay('ota-progress-container', true);
    post('/doUpdate', {}).then(d => { if(d.success) monitorUpdateProgress(); else { showOTAStatus('Hata: ' + (d.message || 'Başlatılamadı'), 'error'); setDisplay('ota-progress-container', false); } })
        .catch(e => { showOTAStatus('Bağlantı hatası: ' + e, 'error'); setDisplay('ota-progress-container', false); });
}
function monitorUpdateProgress() {
    const iv = setInterval(() => {
        api('/getOTASettings').then(d => {
            updateProgressBar(d.progress || 0);
            if(d.status === 6) { clearInterval(iv); updateProgressBar(100); showOTAStatus('Güncelleme başarılı! Yeniden başlatılıyor...', 'success'); setTimeout(() => location.reload(), 3000); }
            else if(d.status === 7) { clearInterval(iv); setDisplay('ota-progress-container', false); showOTAStatus('Hata: ' + (d.errorMessage || 'Başarısız'), 'error'); }
        }).catch(() => {});
    }, 1000);
}
function updateProgressBar(p) {
    $('ota-progress-fill').style.width = p + '%';
    setText('ota-progress-percent', p + '%');
    setText('ota-progress-label', p < 30 ? 'İndiriliyor...' : p < 100 ? 'Yükleniyor...' : 'Tamamlandı!');
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
            .then(d => showOTAStatus(d.success ? 'Ayar kaydedildi' : 'Kaydedilemedi', d.success ? 'success' : 'error'))
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
        setText('dimmer-status-power', d.isOn ? ' Açık' : ' Kapalı');
        setText('dimmer-status-calibration', d.ratio || 3);
        if(btnC) btnC.style.display = 'none'; if(btnD) btnD.style.display = 'block';
    } else {
        setText('dimmer-status-ip', '-'); setText('dimmer-status-brightness', '0%');
        setText('dimmer-status-power', ' Kapalı'); setText('dimmer-status-calibration', '1-5');
        if(btnC) btnC.style.display = 'block'; if(btnD) btnD.style.display = 'none';
    }
    if(d.ratio && cal) cal.textContent = d.ratio;
}
function updateCalibrationValue(v) { setText('calibration-value', v); }
function adjustCalibration(delta) {
    const el = $('dimmer-status-calibration'), cur = parseInt(el.textContent);
    let nv = Math.max(1, Math.min(5, cur + delta));
    el.textContent = nv;
    post('/setDimmerRatio', { ratio: nv }).then(d => { if(!d.success) { showNotification('Kalibrasyon kaydedilemedi', 'error'); el.textContent = cur; } })
        .catch(() => { showNotification('Bağlantı hatası', 'error'); el.textContent = cur; });
}
function connectDimmerManual() {
    const ip = $('dimmer-ip-input').value;
    if(!ip) { showNotification('Lütfen IP girin!', 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification('Geçersiz IP!', 'error'); return; }
    connectDimmer(ip);
}
function connectDimmer(ip) {
    ip = ip || $('dimmer-ip-input').value;
    showNotification('Bağlanılıyor...', 'info');
    post('/connectDimmer', { ip }).then(d => {
        if(d.success) { showNotification('Dimmer bağlandı!', 'success'); loadDimmerStatus(); loadSavedDevices(); }
        else showNotification('Bağlantı başarısız: ' + (d.message || 'Hata'), 'error');
    }).catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
}
function disconnectDimmer() {
    if(!confirm('Bağlantıyı kesmek istediğinize emin misiniz?')) return;
    post('/disconnectDimmer', {}).then(d => { if(d.success) { showNotification('Bağlantı kesildi', 'success'); loadDimmerStatus(); } }).catch(() => {});
}
function selectRatio(r) { selectedRatio = r; updateRatioButtons(r); setText('dimmer-ratio-value', r); }
function updateRatioButtons(active) {
    document.querySelectorAll('.ratio-btn, .ratio-btn-compact').forEach(b => {
        b.classList.toggle('active', parseInt(b.getAttribute('data-ratio')) === active);
    });
}
function saveDimmerRatio() {
    showNotification('Kalibrasyon kaydediliyor...', 'info');
    post('/setDimmerRatio', { ratio: selectedRatio })
        .then(d => showNotification(d.success ? 'Kalibrasyon kaydedildi!' : 'Kayıt başarısız', d.success ? 'success' : 'error'))
        .catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
}
function saveDimmerSettings() {
    const v = parseInt($('calibration-slider').value);
    showNotification('Kalibrasyon kaydediliyor...', 'info');
    post('/setDimmerRatio', { ratio: v })
        .then(d => showNotification(d.success ? 'Kalibrasyon kaydedildi!' : 'Kayıt başarısız', d.success ? 'success' : 'error'))
        .catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
}
function loadSavedDevices() {
    api('/getSavedDevices').then(d => displaySavedDevices(d.devices || [])).catch(() => {});
}
function displaySavedDevices(devs) {
    const c = $('saved-devices-list');
    if(!devs.length) { c.innerHTML = '<div class="saved-device-empty">Henüz kayıtlı cihaz yok.</div>'; return; }
    const types = { 0:'Bilinmeyen', 1:'Shelly Dimmer', 2:'Shelly DALI', 3:'Tasmota' };
    c.innerHTML = devs.map(d => `<div class="saved-device-item"><div class="saved-device-info"><div class="saved-device-ip">${d.ip}</div><div class="saved-device-type">${types[d.type] || 'Bilinmeyen'}</div></div><div class="saved-device-actions"><button class="btn-device-connect" onclick="connectToSavedDevice('${d.ip}')">Bağlan</button><button class="btn-device-remove" onclick="removeSavedDevice('${d.ip}')">Kaldır</button></div></div>`).join('');
}
function connectToSavedDevice(ip) { $('dimmer-ip-input').value = ip; connectDimmer(ip); }
function removeSavedDevice(ip) {
    if(!confirm('Bu cihazı kaldırmak istediğinize emin misiniz?')) return;
    post('/removeSavedDevice', { ip }).then(d => { if(d.success) { showNotification('Cihaz kaldırıldı', 'success'); loadSavedDevices(); } else showNotification('Kaldırma başarısız', 'error'); })
        .catch(e => showNotification('Hata: ' + e, 'error'));
}

// === NETWORK SCAN ===
let scanProgressInterval = null;
function startNetworkScan() {
    showNotification('Ağ taranıyor...', 'info');
    setDisplay('btn-stop-scan', true);
    post('/scanNetwork', {}).then(d => { if(d.success) pollScanProgress(); else { showNotification('Tarama başlatılamadı', 'error'); setDisplay('btn-stop-scan', false); } })
        .catch(e => { showNotification('Tarama hatası: ' + e, 'error'); setDisplay('btn-stop-scan', false); });
}
function pollScanProgress() {
    if(scanProgressInterval) clearInterval(scanProgressInterval);
    scanProgressInterval = setInterval(() => {
        api('/getScanProgress').then(d => {
            if(d.scanning) showNotification(`Taranıyor: ${d.percentage}% (${d.scannedIPs}/${d.totalIPs}) - ${d.dimmerCount} dimmer`, 'info');
            else {
                clearInterval(scanProgressInterval); scanProgressInterval = null;
                setDisplay('btn-stop-scan', false);
                showNotification(d.dimmerCount > 0 ? `Tamamlandı! ${d.dimmerCount} dimmer bulundu` : 'Tamamlandı - dimmer bulunamadı', d.dimmerCount > 0 ? 'success' : 'info');
                loadSavedDevices();
            }
        }).catch(() => { clearInterval(scanProgressInterval); scanProgressInterval = null; setDisplay('btn-stop-scan', false); });
    }, 1000);
}
function stopNetworkScan() {
    if(!confirm('Taramayı durdurmak istiyor musunuz?')) return;
    post('/stopScan', {}).then(d => {
        if(d.success) { showNotification('Tarama durduruldu', 'info'); if(scanProgressInterval) { clearInterval(scanProgressInterval); scanProgressInterval = null; } setDisplay('btn-stop-scan', false); loadSavedDevices(); }
    }).catch(() => {});
}

// === SHUTTER ===
function connectShutterManual() {
    const ip = $('shutter-ip-input').value;
    if(!ip) { showNotification('Lütfen IP girin!', 'error'); return; }
    if(!/^(\d{1,3}\.){3}\d{1,3}$/.test(ip)) { showNotification('Geçersiz IP!', 'error'); return; }
    connectShutter(ip);
}
function connectShutter(ip) {
    ip = ip || $('shutter-ip-input').value;
    showNotification('Bağlanılıyor...', 'info');
    post('/connectShutter', { ip }).then(d => {
        if(d.success) { showNotification('Shutter bağlandı!', 'success'); loadShutterStatus(); }
        else showNotification('Bağlantı başarısız: ' + (d.message || 'Hata'), 'error');
    }).catch(e => showNotification('Bağlantı hatası: ' + e, 'error'));
}
function disconnectShutter() {
    if(!confirm('Bağlantıyı kesmek istediğinize emin misiniz?')) return;
    post('/disconnectShutter', {}).then(d => { if(d.success) { showNotification('Bağlantı kesildi', 'success'); loadShutterStatus(); } }).catch(() => {});
}
function loadShutterStatus() {
    api('/getShutterStatus').then(d => {
        setText('shutter-status-ip', d.ip || '-');
        setText('shutter-status-text', d.connected ? 'Bağlı' : 'Bağlı Değil');
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
    showNotification('Ağ taraması başlatılıyor...', 'info');
    post('/startShutterScan', {}).then(d => {
        if(d.success) { showNotification('Tarama başladı', 'success'); setDisplay('btn-stop-shutter-scan', true); pollShutterScanProgress(); }
        else showNotification('Tarama başlatılamadı', 'error');
    }).catch(() => showNotification('Tarama hatası', 'error'));
}
function pollShutterScanProgress() {
    if(shutterScanProgressInterval) clearInterval(shutterScanProgressInterval);
    shutterScanProgressInterval = setInterval(() => {
        api('/getShutterScanProgress').then(d => {
            if(d.scanning) showNotification(`Taranıyor: ${d.percentage}% (${d.scannedIPs}/${d.totalIPs}) - ${d.shutterCount} shutter`, 'info');
            else {
                clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null;
                setDisplay('btn-stop-shutter-scan', false);
                showNotification(d.shutterCount > 0 ? `Tamamlandı! ${d.shutterCount} shutter bulundu` : 'Tamamlandı - shutter bulunamadı', d.shutterCount > 0 ? 'success' : 'info');
            }
        }).catch(() => { clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null; setDisplay('btn-stop-shutter-scan', false); });
    }, 1000);
}
function stopShutterNetworkScan() {
    if(!confirm('Taramayı durdurmak istiyor musunuz?')) return;
    post('/stopShutterScan', {}).then(d => {
        if(d.success) { showNotification('Tarama durduruldu', 'info'); if(shutterScanProgressInterval) { clearInterval(shutterScanProgressInterval); shutterScanProgressInterval = null; } setDisplay('btn-stop-shutter-scan', false); }
    }).catch(() => {});
}

// === SYSTEM ACTIONS ===
function restartDevice() {
    showNotification('Cihaz yeniden başlatılıyor...', 'info');
    post('/restart', {}).then(() => {
        showNotification('Cihaz yeniden başlatıldı. Sayfa 5 saniye sonra yenilenecek.', 'success');
        setTimeout(() => location.reload(), 5000);
    }).catch(e => showNotification('Restart hatası: ' + e, 'error'));
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
    if(input !== 'evet') {
        showNotification('Onay için "Evet" yazmanız gerekiyor!', 'error');
        return;
    }
    showNotification('Factory reset yapılıyor...', 'info');
    post('/factoryReset', {}).then(() => {
        showNotification('Factory reset tamamlandı. Cihaz yeniden başlatılıyor...', 'success');
        setTimeout(() => location.reload(), 10000);
    }).catch(e => showNotification('Factory reset hatası: ' + e, 'error'));
}
)rawliteral";

#endif // SK_JS_H
