/**
 * SK_js.h
 * SmartKraft SynDimm - JavaScript Functions
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

#ifndef SK_JS_H
#define SK_JS_H

const char SK_JS[] PROGMEM = R"rawliteral(
// Theme Management
function setTheme(theme) {
    const body = document.body;
    const radioButtons = document.querySelectorAll('input[name="theme"]');
    
    if (theme === 'light') {
        body.classList.add('light-theme');
        radioButtons.forEach(radio => {
            radio.checked = (radio.value === 'light');
        });
        localStorage.setItem('theme', 'light');
    } else {
        body.classList.remove('light-theme');
        radioButtons.forEach(radio => {
            radio.checked = (radio.value === 'dark');
        });
        localStorage.setItem('theme', 'dark');
    }
}

// Load saved theme on page load
const savedTheme = localStorage.getItem('theme');
if (savedTheme === 'light') {
    document.addEventListener('DOMContentLoaded', function() {
        setTheme('light');
    });
}

// Load saved settings and status on page load
window.addEventListener('DOMContentLoaded', function() {
    loadSavedSettings();
    loadConnectionStatus();
    loadOTASettings();
    loadDimmerStatus();
    loadSavedDevices(); // Load saved dimmer devices
    loadCurrentMode(); // Load active mode
    updateQuickSettings(); // Update quick settings panel
    // Refresh status every 5 seconds
    setInterval(loadConnectionStatus, 5000);
    // Refresh dimmer status every 2 seconds
    setInterval(loadDimmerStatus, 2000);
    // Refresh mode every 3 seconds (will be dynamically adjusted during selection mode)
    window.modePollingIntervalId = setInterval(loadCurrentMode, 3000);
    // Refresh quick settings every 5 seconds
    setInterval(updateQuickSettings, 5000);
});

// Update Quick Settings Panel
function updateQuickSettings() {
    // Update connection info
    fetch('/getStatus')
        .then(response => response.json())
        .then(data => {
            const quickMode = document.getElementById('quick-mode');
            const quickSsid = document.getElementById('quick-ssid');
            const quickIp = document.getElementById('quick-ip');
            
            if (quickMode) quickMode.textContent = data.mode || '-';
            if (quickSsid) quickSsid.textContent = data.ssid || '-';
            if (quickIp) quickIp.textContent = data.ip || '-';
        })
        .catch(error => console.error('Quick settings update failed:', error));
    
    // Update active mode
    fetch('/getCurrentMode')
        .then(response => response.json())
        .then(data => {
            const modeIcon = document.getElementById('quick-mode-icon');
            const modeName = document.getElementById('quick-mode-name');
            
            if (modeIcon && modeName) {
                modeName.textContent = data.mode || 'Yüklenemedi';
                
                // Change icon color based on mode
                if (data.mode === 'DIMMER') {
                    modeIcon.style.color = '#f59e0b';
                } else if (data.mode === 'SHUTTER') {
                    modeIcon.style.color = '#3b82f6';
                } else if (data.mode === 'SAFE') {
                    modeIcon.style.color = '#10b981';
                } else {
                    modeIcon.style.color = 'var(--text-muted)';
                }
            }
        })
        .catch(error => console.error('Mode update failed:', error));
}

// Load current active mode
let modePollingInterval = null;

function loadCurrentMode() {
    fetch('/getCurrentMode')
        .then(response => response.json())
        .then(data => {
            // Handle legacy response format (just mode string) vs new format (full status)
            const activeMode = data.activeModeStr || data.mode;
            const previewMode = data.previewModeStr;
            const inSelectionMode = data.inSelectionMode || false;
            
            // Update mode badges based on active mode
            const dimmerBadge = document.getElementById('dimmer-badge');
            const shutterBadge = document.getElementById('shutter-badge');
            const safeBadge = document.getElementById('safe-badge');
            
            if (dimmerBadge) {
                if (activeMode === 'DIMMER') {
                    dimmerBadge.textContent = 'Aktif';
                    dimmerBadge.className = 'badge badge-connected';
                } else {
                    dimmerBadge.textContent = 'Pasif';
                    dimmerBadge.className = 'badge badge-not-configured';
                }
            }
            
            if (shutterBadge) {
                if (activeMode === 'SHUTTER') {
                    shutterBadge.textContent = 'Aktif';
                    shutterBadge.className = 'badge badge-connected';
                } else {
                    shutterBadge.textContent = 'Pasif';
                    shutterBadge.className = 'badge badge-not-configured';
                }
            }
            
            if (safeBadge) {
                if (activeMode === 'SAFE') {
                    safeBadge.textContent = 'Aktif';
                    safeBadge.className = 'badge badge-connected';
                } else {
                    safeBadge.textContent = 'Pasif';
                    safeBadge.className = 'badge badge-not-configured';
                }
            }
            
            // Update mode buttons highlighting
            const dimmerBtn = document.getElementById('mode-btn-dimmer');
            const shutterBtn = document.getElementById('mode-btn-shutter');
            const safeBtn = document.getElementById('mode-btn-safe');
            
            // Remove all classes first
            if (dimmerBtn) {
                dimmerBtn.classList.remove('active', 'preview');
            }
            if (shutterBtn) {
                shutterBtn.classList.remove('active', 'preview');
            }
            if (safeBtn) {
                safeBtn.classList.remove('active', 'preview');
            }
            
            // If in selection mode, show preview with yellow pulsing
            if (inSelectionMode && previewMode) {
                if (previewMode === 'DIMMER' && dimmerBtn) {
                    dimmerBtn.classList.add('preview');
                } else if (previewMode === 'SHUTTER' && shutterBtn) {
                    shutterBtn.classList.add('preview');
                } else if (previewMode === 'SAFE' && safeBtn) {
                    safeBtn.classList.add('preview');
                }
                
                // Increase polling rate during selection mode
                if (!modePollingInterval) {
                    clearInterval(window.modePollingIntervalId);
                    window.modePollingIntervalId = setInterval(loadCurrentMode, 500);
                    modePollingInterval = true;
                }
            } else {
                // Normal mode - show active mode
                if (activeMode === 'DIMMER' && dimmerBtn) {
                    dimmerBtn.classList.add('active');
                } else if (activeMode === 'SHUTTER' && shutterBtn) {
                    shutterBtn.classList.add('active');
                } else if (activeMode === 'SAFE' && safeBtn) {
                    safeBtn.classList.add('active');
                }
                
                // Reset to normal polling rate
                if (modePollingInterval) {
                    clearInterval(window.modePollingIntervalId);
                    window.modePollingIntervalId = setInterval(loadCurrentMode, 3000);
                    modePollingInterval = false;
                }
            }
        })
        .catch(error => {
            console.error('Failed to load current mode:', error);
        });
}

// Select mode function
function selectMode(mode) {
    fetch('/setMode', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/x-www-form-urlencoded',
        },
        body: 'mode=' + mode
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            console.log('Mode changed to: ' + mode);
            // Reload mode display
            loadCurrentMode();
        } else {
            console.error('Failed to change mode');
        }
    })
    .catch(error => {
        console.error('Error changing mode:', error);
    });
}

// Language selector placeholder function
function setLanguage(lang) {
    console.log('Language selector clicked: ' + lang);
    // No functionality yet - placeholder only
}

function loadConnectionStatus() {
    fetch('/getStatus')
        .then(response => response.json())
        .then(data => {
            // Update status card
            document.getElementById('status-mode').textContent = data.mode || '-';
            document.getElementById('status-ssid').textContent = data.ssid || '-';
            document.getElementById('status-ip').textContent = data.ip || '-';
            document.getElementById('status-mdns').textContent = data.mdns ? data.mdns + '.local' : '';
            
            // Update badges
            const primaryBadge = document.getElementById('primary-badge');
            const backupBadge = document.getElementById('backup-badge');
            
            if (data.connectedTo === 'primary') {
                primaryBadge.textContent = 'Connected';
                primaryBadge.className = 'badge badge-connected';
            } else if (data.primaryConfigured) {
                primaryBadge.textContent = 'Configured';
                primaryBadge.className = 'badge badge-passive';
            } else {
                primaryBadge.textContent = 'Not Configured';
                primaryBadge.className = 'badge badge-not-configured';
            }
            
            if (data.connectedTo === 'backup') {
                backupBadge.textContent = 'Connected';
                backupBadge.className = 'badge badge-connected';
            } else if (data.backupConfigured) {
                backupBadge.textContent = 'Configured';
                backupBadge.className = 'badge badge-passive';
            } else {
                backupBadge.textContent = 'Not Configured';
                backupBadge.className = 'badge badge-not-configured';
            }
        })
        .catch(error => {
            console.error('Failed to load status:', error);
        });
}

function loadSavedSettings() {
    fetch('/getSettings')
        .then(response => response.json())
        .then(data => {
            // Fill Primary WiFi
            if (data.primary.ssid) {
                document.getElementById('primary-ssid').value = data.primary.ssid;
            }
            if (data.primary.staticIP) {
                document.getElementById('primary-static').value = data.primary.staticIP;
            }
            if (data.primary.mdns) {
                document.getElementById('primary-mdns').value = data.primary.mdns;
            }
            
            // Fill Backup WiFi
            if (data.backup.ssid) {
                document.getElementById('backup-ssid').value = data.backup.ssid;
            }
            if (data.backup.staticIP) {
                document.getElementById('backup-static').value = data.backup.staticIP;
            }
            if (data.backup.mdns) {
                document.getElementById('backup-mdns').value = data.backup.mdns;
            }
        })
        .catch(error => {
            console.error('Failed to load settings:', error);
        });
}

function openTab(evt, tabName) {
    var i, tabcontent, tablinks;
    
    // Hide all tab contents
    tabcontent = document.getElementsByClassName("tab-content");
    for (i = 0; i < tabcontent.length; i++) {
        tabcontent[i].classList.remove("active");
    }
    
    // Remove active class from all tabs
    tablinks = document.getElementsByClassName("tab");
    for (i = 0; i < tablinks.length; i++) {
        tablinks[i].classList.remove("active");
    }
    
    // Show selected tab and mark button as active
    document.getElementById(tabName).classList.add("active");
    evt.currentTarget.classList.add("active");
}

function toggleAccordion(header) {
    const isCurrentlyActive = header.classList.contains('active');
    
    // Tüm akordionları kapat
    const allHeaders = document.querySelectorAll('.accordion-header');
    allHeaders.forEach(h => h.classList.remove('active'));
    
    // Eğer tıklanan başlık kapalıysa, sadece onu aç
    if (!isCurrentlyActive) {
        header.classList.add('active');
    }
}

function openSafeTab(evt, tabIndex) {
    // Hide all tab contents
    const tabContents = document.getElementsByClassName('safe-tab-content');
    for (let i = 0; i < tabContents.length; i++) {
        tabContents[i].classList.remove('active');
    }
    
    // Remove active class from all tabs
    const tabs = document.getElementsByClassName('safe-tab');
    for (let i = 0; i < tabs.length; i++) {
        tabs[i].classList.remove('active');
    }
    
    // Show selected tab and mark button as active
    document.getElementById('safe-tab-' + tabIndex).classList.add('active');
    evt.currentTarget.classList.add('active');
}

function saveSafePassword(index) {
    const password = document.getElementById('safe-pwd-' + index).value;
    const pwdEnabled = document.getElementById('safe-pwd-' + index + '-enabled').checked;
    const apiUrl = document.getElementById('safe-api-' + index + '-url').value;
    const apiHeader = document.getElementById('safe-api-' + index + '-header').value;
    
    // Validation
    if (pwdEnabled && (!password || password.length === 0)) {
        showNotification('Şifre boş olamaz!', 'error');
        return;
    }
    
    // Password format kontrolü (L3-R12-L11-R3-B)
    const passwordPattern = /^([LRB]\d*-)*[LRB]\d*$/;
    if (pwdEnabled && !passwordPattern.test(password)) {
        showNotification('Geçersiz şifre formatı! Örnek: L3-R12-L11-R3-B', 'error');
        return;
    }
    
    if (pwdEnabled && (!apiUrl || apiUrl.length === 0)) {
        showNotification('API URL boş olamaz!', 'error');
        return;
    }
    
    const data = {
        index: index,
        password: password,
        pwdEnabled: pwdEnabled,
        api: {
            url: apiUrl,
            method: 'GET',
            header: apiHeader,
            enabled: pwdEnabled
        }
    };
    
    showNotification('Şifre kaydediliyor...', 'info');
    
    fetch('/saveSafePassword', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => {
        console.log('Response status:', response.status);
        if (!response.ok) {
            throw new Error('HTTP ' + response.status);
        }
        return response.json();
    })
    .then(result => {
        console.log('Server response:', result);
        if (result.success) {
            showNotification('Şifre başarıyla kaydedildi!', 'success');
        } else {
            showNotification('Hata: ' + (result.message || 'Şifre kaydedilemedi'), 'error');
        }
    })
    .catch(error => {
        console.error('Fetch error:', error);
        showNotification('Bağlantı hatası: ' + error.message, 'error');
    });
}

function testSafeApi(index) {
    const apiUrl = document.getElementById('safe-api-' + index + '-url').value;
    const apiHeader = document.getElementById('safe-api-' + index + '-header').value;
    
    if (!apiUrl || apiUrl.length === 0) {
        showNotification('API URL boş olamaz!', 'error');
        return;
    }
    
    const data = {
        url: apiUrl,
        method: 'GET',
        header: apiHeader
    };
    
    showNotification('API test ediliyor...', 'info');
    
    fetch('/testSafeApi', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => {
        console.log('Response status:', response.status);
        if (!response.ok) {
            throw new Error('HTTP ' + response.status);
        }
        return response.json();
    })
    .then(result => {
        console.log('Server response:', result);
        if (result.success) {
            showNotification('API test başarılı! HTTP ' + result.httpCode, 'success');
        } else {
            showNotification('API test başarısız: ' + (result.message || 'Bilinmeyen hata'), 'error');
        }
    })
    .catch(error => {
        console.error('Fetch error:', error);
        showNotification('Bağlantı hatası: ' + error.message, 'error');
    });
}

function showNotification(message, type = 'info') {
    const notification = document.getElementById('notification');
    const messageEl = document.getElementById('notification-message');
    
    // Remove existing type classes
    notification.classList.remove('success', 'error', 'info');
    
    // Add new type class
    notification.classList.add(type);
    
    // Set message
    messageEl.textContent = message;
    
    // Show notification
    notification.style.display = 'flex';
    
    // Auto-hide after 5 seconds for success/info
    if (type !== 'error') {
        setTimeout(() => {
            closeNotification();
        }, 5000);
    }
}

function closeNotification() {
    const notification = document.getElementById('notification');
    notification.style.display = 'none';
}

function validateNetworkSettings(data) {
    const errors = [];
    
    // Primary WiFi validation
    if (data.primary.ssid && data.primary.ssid.length > 0) {
        if (data.primary.ssid.length > 32) {
            errors.push('Primary SSID maksimum 32 karakter olmalıdır');
        }
        if (data.primary.password && (data.primary.password.length < 8 || data.primary.password.length > 63)) {
            errors.push('Primary WiFi şifresi 8-63 karakter arasında olmalıdır (boş bırakılabilir)');
        }
        // Static IP validation (optional)
        if (data.primary.staticIP && data.primary.staticIP.length > 0) {
            const ipPattern = /^(\d{1,3}\.){3}\d{1,3}$/;
            if (!ipPattern.test(data.primary.staticIP)) {
                errors.push('Primary Static IP geçerli bir IP adresi değil');
            }
        }
        // mDNS validation
        if (data.primary.mdns && data.primary.mdns.length > 63) {
            errors.push('Primary mDNS 63 karakterden uzun olamaz');
        }
    }
    
    // Backup WiFi validation
    if (data.backup.ssid && data.backup.ssid.length > 0) {
        if (data.backup.ssid.length > 32) {
            errors.push('Backup SSID maksimum 32 karakter olmalıdır');
        }
        if (data.backup.password && (data.backup.password.length < 8 || data.backup.password.length > 63)) {
            errors.push('Backup WiFi şifresi 8-63 karakter arasında olmalıdır (boş bırakılabilir)');
        }
        // Static IP validation (optional)
        if (data.backup.staticIP && data.backup.staticIP.length > 0) {
            const ipPattern = /^(\d{1,3}\.){3}\d{1,3}$/;
            if (!ipPattern.test(data.backup.staticIP)) {
                errors.push('Backup Static IP geçerli bir IP adresi değil');
            }
        }
        // mDNS validation
        if (data.backup.mdns && data.backup.mdns.length > 63) {
            errors.push('Backup mDNS 63 karakterden uzun olamaz');
        }
    }
    
    // At least one network should be configured
    if (!data.primary.ssid && !data.backup.ssid) {
        errors.push('En az bir WiFi ağı yapılandırılmalıdır');
    }
    
    return {
        valid: errors.length === 0,
        errors: errors
    };
}

function saveNetworkSettings() {
    const primarySSID = document.getElementById('primary-ssid').value;
    const primaryPassword = document.getElementById('primary-password').value;
    const primaryStaticIP = document.getElementById('primary-static').value;
    const primaryMDNS = document.getElementById('primary-mdns').value || 'dimm';
    
    const backupSSID = document.getElementById('backup-ssid').value;
    const backupPassword = document.getElementById('backup-password').value;
    const backupStaticIP = document.getElementById('backup-static').value;
    const backupMDNS = document.getElementById('backup-mdns').value || 'dimm';
    
    const data = {
        primary: {
            ssid: primarySSID,
            password: primaryPassword,
            staticIP: primaryStaticIP,
            mdns: primaryMDNS
        },
        backup: {
            ssid: backupSSID,
            password: backupPassword,
            staticIP: backupStaticIP,
            mdns: backupMDNS
        }
    };
    
    // Validate settings
    const validation = validateNetworkSettings(data);
    if (!validation.valid) {
        showNotification('Hata: ' + validation.errors.join(', '), 'error');
        return;
    }
    
    // Show saving notification
    showNotification('Ayarlar kaydediliyor...', 'info');
    
    fetch('/saveNetwork', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify(data)
    })
    .then(response => {
        console.log('Response status:', response.status);
        if (!response.ok) {
            throw new Error('HTTP ' + response.status);
        }
        return response.json();
    })
    .then(result => {
        console.log('Server response:', result);
        if (result.success) {
            showNotification('Ayarlar basariyla kaydedildi! Cihaz yeniden baslatiliyor...', 'success');
            setTimeout(() => {
                location.reload();
            }, 3000);
        } else {
            showNotification('Hata: ' + (result.message || 'Ayarlar kaydedilemedi'), 'error');
        }
    })
    .catch(error => {
        console.error('Fetch error:', error);
        showNotification('Bağlantı hatası: ' + error.message, 'error');
    });
}

// OTA Functions
function loadOTASettings() {
    fetch('/getOTASettings')
        .then(response => response.json())
        .then(data => {
            document.getElementById('ota-current-version').textContent = data.currentVersion;
            document.getElementById('ota-auto-update').checked = data.autoUpdateEnabled;
            
            if (data.updateAvailable) {
                showUpdateAvailable(data);
            }
        })
        .catch(error => {
            console.error('Failed to load OTA settings:', error);
        });
}

function showUpdateAvailable(data) {
    document.getElementById('ota-latest-version').textContent = data.latestVersion;
    document.getElementById('ota-publish-date').textContent = 'Yayın Tarihi: ' + formatDate(data.publishedAt);
    document.getElementById('ota-release-notes').textContent = data.releaseNotes || 'Sürüm notları mevcut değil';
    document.getElementById('ota-update-card').style.display = 'block';
    document.getElementById('btn-ota-update').style.display = 'block';
}

function formatDate(dateString) {
    if (!dateString) return '-';
    const date = new Date(dateString);
    return date.toLocaleDateString('tr-TR', { year: 'numeric', month: 'long', day: 'numeric' });
}

function checkForUpdate() {
    showOTAStatus('Güncelleme kontrol ediliyor...', 'info');
    document.getElementById('btn-ota-update').style.display = 'none';
    document.getElementById('ota-update-card').style.display = 'none';
    
    fetch('/checkUpdate')
        .then(response => response.json())
        .then(data => {
            if (data.success) {
                if (data.updateAvailable) {
                    showUpdateAvailable(data);
                    showOTAStatus('Yeni sürüm bulundu!', 'success');
                } else {
                    showOTAStatus('Cihazınız güncel!', 'success');
                }
            } else {
                showOTAStatus('Hata: ' + (data.message || 'Kontrol edilemedi'), 'error');
            }
        })
        .catch(error => {
            console.error('Check update error:', error);
            showOTAStatus('Bağlantı hatası: ' + error.message, 'error');
        });
}

function performUpdate() {
    if (!confirm('Güncelleme sırasında cihaz yeniden başlatılacak. Devam etmek istiyor musunuz?')) {
        return;
    }
    
    showOTAStatus('Güncelleme başlatılıyor...', 'info');
    document.getElementById('btn-ota-update').style.display = 'none';
    document.getElementById('ota-progress-container').style.display = 'block';
    
    fetch('/doUpdate', {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            // Start progress monitoring
            monitorUpdateProgress();
        } else {
            showOTAStatus('Hata: ' + (data.message || 'Güncelleme başlatılamadı'), 'error');
            document.getElementById('ota-progress-container').style.display = 'none';
        }
    })
    .catch(error => {
        console.error('Update error:', error);
        showOTAStatus('Bağlantı hatası: ' + error.message, 'error');
        document.getElementById('ota-progress-container').style.display = 'none';
    });
}

function monitorUpdateProgress() {
    const progressInterval = setInterval(() => {
        fetch('/getOTASettings')
            .then(response => response.json())
            .then(data => {
                const progress = data.progress || 0;
                updateProgressBar(progress);
                
                if (data.status === 6) { // OTA_SUCCESS
                    clearInterval(progressInterval);
                    updateProgressBar(100);
                    showOTAStatus('Güncelleme başarılı! Cihaz yeniden başlatılıyor...', 'success');
                    setTimeout(() => {
                        location.reload();
                    }, 3000);
                } else if (data.status === 7) { // OTA_ERROR
                    clearInterval(progressInterval);
                    document.getElementById('ota-progress-container').style.display = 'none';
                    showOTAStatus('Hata: ' + (data.errorMessage || 'Güncelleme başarısız'), 'error');
                }
            })
            .catch(error => {
                console.error('Progress check error:', error);
            });
    }, 1000);
}

function updateProgressBar(percent) {
    document.getElementById('ota-progress-fill').style.width = percent + '%';
    document.getElementById('ota-progress-percent').textContent = percent + '%';
    
    if (percent < 30) {
        document.getElementById('ota-progress-label').textContent = 'İndiriliyor...';
    } else if (percent < 100) {
        document.getElementById('ota-progress-label').textContent = 'Yükleniyor...';
    } else {
        document.getElementById('ota-progress-label').textContent = 'Tamamlandı!';
    }
}

function showOTAStatus(message, type) {
    const statusEl = document.getElementById('ota-status-message');
    statusEl.textContent = message;
    statusEl.className = 'ota-status-message ' + type;
    statusEl.style.display = 'block';
    
    if (type !== 'error') {
        setTimeout(() => {
            statusEl.style.display = 'none';
        }, 5000);
    }
}

// Save auto-update setting
document.addEventListener('DOMContentLoaded', function() {
    const autoUpdateToggle = document.getElementById('ota-auto-update');
    if (autoUpdateToggle) {
        autoUpdateToggle.addEventListener('change', function() {
            fetch('/saveOTASettings', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({
                    autoUpdate: this.checked
                })
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showOTAStatus('Ayar kaydedildi', 'success');
                } else {
                    showOTAStatus('Ayar kaydedilemedi', 'error');
                }
            })
            .catch(error => {
                console.error('Save OTA settings error:', error);
            });
        });
    }
});

// ========================================
// DIMMER FUNCTIONS
// ========================================
// KRITIK: Web tarayıcısı dimmer cihazlara ASLA müdahale edemez!
// Tüm kontroller ESP32-C6 üzerinden yapılır.
// JavaScript sadece ESP32-C6'ya komut gönderir ve durum bilgisi alır.
// ========================================

let selectedRatio = 3; // Default

// Load Dimmer Status from ESP32-C6
function loadDimmerStatus() {
    fetch('/getDimmerStatus')
        .then(response => response.json())
        .then(data => {
            updateDimmerUI(data);
        })
        .catch(error => {
            console.error('Failed to load dimmer status:', error);
        });
}

// Update Dimmer UI with status from ESP32-C6
function updateDimmerUI(data) {
    // Update badge
    const badge = document.getElementById('dimmer-badge');
    
    // Update Status Bar - New 4 Column Design
    const statusIP = document.getElementById('dimmer-status-ip');
    const statusBrightness = document.getElementById('dimmer-status-brightness');
    const statusPower = document.getElementById('dimmer-status-power');
    const statusCalibration = document.getElementById('dimmer-status-calibration');
    const btnConnect = document.getElementById('btn-connect');
    const btnDisconnect = document.getElementById('btn-disconnect');
    
    if (data.connected) {
        badge.textContent = 'Aktif';
        badge.className = 'badge badge-connected';
        
        // Update status columns
        statusIP.textContent = data.ip || '-';
        const brightnessValue = data.brightness || 0;
        statusBrightness.textContent = brightnessValue + '%';
        
        if (data.isOn) {
            statusPower.textContent = ' Açık';
        } else {
            statusPower.textContent = ' Kapalı';
        }
        
        // Update calibration display
        const ratioValue = data.ratio || 3;
        statusCalibration.textContent = ratioValue;
        
        // Toggle buttons
        btnConnect.style.display = 'none';
        btnDisconnect.style.display = 'block';
    } else {
        badge.textContent = 'Pasif';
        badge.className = 'badge badge-not-configured';
        
        // Reset status columns
        statusIP.textContent = '-';
        statusBrightness.textContent = '0%';
        statusPower.textContent = ' Kapalı';
        statusCalibration.textContent = '1-5';
        
        // Toggle buttons
        btnConnect.style.display = 'block';
        btnDisconnect.style.display = 'none';
    }
    
    // Update calibration slider
    if (data.ratio) {
        const ratioValue = data.ratio || 3;
        statusCalibration.textContent = ratioValue;
    }
}

// Update Calibration Value from Slider
function updateCalibrationValue(value) {
    document.getElementById('calibration-value').textContent = value;
}

// Adjust Calibration with Up/Down Buttons
function adjustCalibration(delta) {
    const currentValue = parseInt(document.getElementById('dimmer-status-calibration').textContent);
    let newValue = currentValue + delta;
    
    // Clamp between 1-5
    if (newValue < 1) newValue = 1;
    if (newValue > 5) newValue = 5;
    
    // Update UI
    document.getElementById('dimmer-status-calibration').textContent = newValue;
    
    // Save to ESP32-C6 immediately
    fetch('/setDimmerRatio', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ ratio: newValue })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            // Success - no notification needed for smooth UX
        } else {
            showNotification('Kalibrasyon kaydedilemedi', 'error');
            // Revert on error
            document.getElementById('dimmer-status-calibration').textContent = currentValue;
        }
    })
    .catch(error => {
        console.error('Calibration error:', error);
        showNotification('Bağlantı hatası', 'error');
        // Revert on error
        document.getElementById('dimmer-status-calibration').textContent = currentValue;
    });
}

// Connect to Dimmer (Manual)
function connectDimmerManual() {
    const ip = document.getElementById('dimmer-ip-input').value;
    
    if (!ip || ip.length === 0) {
        showNotification('Lütfen bir IP adresi girin!', 'error');
        return;
    }
    
    // Validate IP format
    const ipPattern = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipPattern.test(ip)) {
        showNotification('Geçersiz IP adresi formatı!', 'error');
        return;
    }
    
    connectDimmer(ip);
}

// Connect to Dimmer (Core Function)
function connectDimmer(ip = null) {
    if (!ip) {
        ip = document.getElementById('dimmer-ip-input').value;
    }
    
    showNotification('Bağlanılıyor...', 'info');
    
    // Send connection request to ESP32-C6
    fetch('/connectDimmer', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ ip: ip })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification('Dimmer cihazına bağlandı!', 'success');
            loadDimmerStatus(); // Refresh status
            loadSavedDevices(); // Refresh device list
        } else {
            showNotification('Bağlantı başarısız: ' + (data.message || 'Bilinmeyen hata'), 'error');
        }
    })
    .catch(error => {
        console.error('Connect error:', error);
        showNotification('Bağlantı hatası: ' + error.message, 'error');
    });
}

// Disconnect from Dimmer
function disconnectDimmer() {
    if (!confirm('Dimmer bağlantısını kesmek istediğinize emin misiniz?')) {
        return;
    }
    
    fetch('/disconnectDimmer', {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification('Bağlantı kesildi', 'success');
            loadDimmerStatus();
        }
    })
    .catch(error => {
        console.error('Disconnect error:', error);
    });
}

// Select Ratio Button
function selectRatio(ratio) {
    selectedRatio = ratio;
    updateRatioButtons(ratio);
    document.getElementById('dimmer-ratio-value').textContent = ratio;
}

// Update Ratio Buttons Visual State
function updateRatioButtons(activeRatio) {
    const buttons = document.querySelectorAll('.ratio-btn, .ratio-btn-compact');
    buttons.forEach(btn => {
        const btnRatio = parseInt(btn.getAttribute('data-ratio'));
        if (btnRatio === activeRatio) {
            btn.classList.add('active');
        } else {
            btn.classList.remove('active');
        }
    });
}

// Save Dimmer Ratio to ESP32-C6
function saveDimmerRatio() {
    showNotification('Kalibrasyon kaydediliyor...', 'info');
    
    // Send ratio to ESP32-C6
    fetch('/setDimmerRatio', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ ratio: selectedRatio })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification('Kalibrasyon kaydedildi!', 'success');
        } else {
            showNotification('Kayıt başarısız: ' + (data.message || 'Bilinmeyen hata'), 'error');
        }
    })
    .catch(error => {
        console.error('Save ratio error:', error);
        showNotification('Bağlantı hatası: ' + error.message, 'error');
    });
}

// Save Dimmer Settings (Calibration)
function saveDimmerSettings() {
    const sliderValue = document.getElementById('calibration-slider').value;
    
    showNotification('Kalibrasyon kaydediliyor...', 'info');
    
    fetch('/setDimmerRatio', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ ratio: parseInt(sliderValue) })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification('Kalibrasyon kaydedildi!', 'success');
        } else {
            showNotification('Kayıt başarısız: ' + (data.message || 'Bilinmeyen hata'), 'error');
        }
    })
    .catch(error => {
        console.error('Save settings error:', error);
        showNotification('Bağlantı hatası: ' + error.message, 'error');
    });
}

// Load Saved Devices List
function loadSavedDevices() {
    fetch('/getSavedDevices')
        .then(response => response.json())
        .then(data => {
            displaySavedDevices(data.devices || []);
        })
        .catch(error => {
            console.error('Load saved devices error:', error);
        });
}

// Display Saved Devices
function displaySavedDevices(devices) {
    const container = document.getElementById('saved-devices-list');
    
    if (!devices || devices.length === 0) {
        container.innerHTML = '<div class="saved-device-empty">Henüz kayıtlı cihaz yok. Ağ taraması yapın veya manuel bağlanın.</div>';
        return;
    }
    
    const typeNames = {
        0: 'Bilinmeyen',
        1: 'Shelly Dimmer',
        2: 'Shelly DALI',
        3: 'Tasmota'
    };
    
    container.innerHTML = '';
    devices.forEach(device => {
        const deviceElement = document.createElement('div');
        deviceElement.className = 'saved-device-item';
        deviceElement.innerHTML = `
            <div class="saved-device-info">
                <div class="saved-device-ip">${device.ip}</div>
                <div class="saved-device-type">${typeNames[device.type] || 'Bilinmeyen'}</div>
            </div>
            <div class="saved-device-actions">
                <button class="btn-device-connect" onclick="connectToSavedDevice('${device.ip}')">Bağlan</button>
                <button class="btn-device-remove" onclick="removeSavedDevice('${device.ip}')">Kaldır</button>
            </div>
        `;
        container.appendChild(deviceElement);
    });
}

// Connect to Saved Device
function connectToSavedDevice(ip) {
    document.getElementById('dimmer-ip-input').value = ip;
    connectDimmer(ip);
}

// Remove Saved Device
function removeSavedDevice(ip) {
    if (!confirm('Bu cihazı listeden kaldırmak istediğinize emin misiniz?')) {
        return;
    }
    
    fetch('/removeSavedDevice', {
        method: 'POST',
        headers: {
            'Content-Type': 'application/json'
        },
        body: JSON.stringify({ ip: ip })
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification('Cihaz listeden kaldırıldı', 'success');
            loadSavedDevices();
        } else {
            showNotification('Kaldırma başarısız', 'error');
        }
    })
    .catch(error => {
        console.error('Remove device error:', error);
        showNotification('Hata: ' + error.message, 'error');
    });
}

// Start Network Scan
function startNetworkScan() {
    showNotification('Ağ taranıyor... Bulunan cihazlar otomatik eklenecek', 'info');
    
    // Show stop button
    const btnStop = document.getElementById('btn-stop-scan');
    if (btnStop) btnStop.style.display = 'block';
    
    fetch('/scanNetwork', {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            // Start polling for progress
            pollScanProgress();
        } else {
            showNotification('Tarama başlatılamadı', 'error');
            if (btnStop) btnStop.style.display = 'none';
        }
    })
    .catch(error => {
        console.error('Scan error:', error);
        showNotification('Tarama hatası: ' + error.message, 'error');
        if (btnStop) btnStop.style.display = 'none';
    });
}

// Poll Scan Progress (called every 1 second)
let scanProgressInterval = null;

function pollScanProgress() {
    // Clear any existing interval
    if (scanProgressInterval) {
        clearInterval(scanProgressInterval);
    }
    
    scanProgressInterval = setInterval(() => {
        fetch('/getScanProgress')
            .then(response => response.json())
            .then(data => {
                if (data.scanning) {
                    // Update progress display
                    const message = `Taranıyor: ${data.percentage}% (${data.scannedIPs}/${data.totalIPs}) - ${data.dimmerCount} dimmer bulundu`;
                    showNotification(message, 'info');
                } else {
                    // Scan complete
                    clearInterval(scanProgressInterval);
                    scanProgressInterval = null;
                    
                    // Hide stop button
                    const btnStop = document.getElementById('btn-stop-scan');
                    if (btnStop) btnStop.style.display = 'none';
                    
                    if (data.dimmerCount > 0) {
                        showNotification(`Tarama tamamlandı! ${data.dimmerCount} dimmer bulundu`, 'success');
                    } else {
                        showNotification('Tarama tamamlandı - dimmer bulunamadı', 'info');
                    }
                    
                    // Refresh device list
                    loadSavedDevices();
                }
            })
            .catch(error => {
                console.error('Progress poll error:', error);
                clearInterval(scanProgressInterval);
                scanProgressInterval = null;
                const btnStop = document.getElementById('btn-stop-scan');
                if (btnStop) btnStop.style.display = 'none';
            });
    }, 1000); // Poll every 1 second
}

// Stop Network Scan
function stopNetworkScan() {
    if (!confirm('Taramayı durdurmak istediğinize emin misiniz?')) {
        return;
    }
    
    fetch('/stopScan', {
        method: 'POST'
    })
    .then(response => response.json())
    .then(data => {
        if (data.success) {
            showNotification('Tarama durduruldu', 'info');
            if (scanProgressInterval) {
                clearInterval(scanProgressInterval);
                scanProgressInterval = null;
            }
            // Hide stop button
            const btnStop = document.getElementById('btn-stop-scan');
            if (btnStop) btnStop.style.display = 'none';
            // Refresh device list with found devices
            loadSavedDevices();
        }
    })
    .catch(error => {
        console.error('Stop scan error:', error);
    });
}

)rawliteral";

#endif // SK_JS_H
