/*
 * SynDimm - Auto-generated PROGMEM file
 * Source: script.js
 */

#ifndef JS_SCRIPT_H
#define JS_SCRIPT_H

const char JS_SCRIPT[] PROGMEM = R"=====(
// ===================================
// SynDimm - Minimal Control Panel JS
// ===================================

// ========== Translation System ==========
let translations = {};
let currentLang = localStorage.getItem('syndimm-language') || 'tr';

// Load translations
async function loadTranslations() {
    try {
        const response = await fetch('/translations.json');
        translations = await response.json();
        console.log('Translations loaded');
        applyTranslations(currentLang);
    } catch (err) {
        console.error('Failed to load translations:', err);
    }
}

// Get translated text by key path
function t(keyPath, lang = currentLang) {
    const keys = keyPath.split('.');
    let value = translations;
    for (const key of keys) {
        if (value && value[key]) {
            value = value[key];
        } else {
            return keyPath; // Return key if translation not found
        }
    }
    return value[lang] || value['en'] || keyPath;
}

// Apply translations to all elements with data-i18n
function applyTranslations(lang) {
    currentLang = lang;
    
    // Translate text content
    document.querySelectorAll('[data-i18n]').forEach(el => {
        const key = el.getAttribute('data-i18n');
        el.textContent = t(key, lang);
    });
    
    // Translate placeholders
    document.querySelectorAll('[data-i18n-placeholder]').forEach(el => {
        const key = el.getAttribute('data-i18n-placeholder');
        el.placeholder = t(key, lang);
    });
    
    // Update HTML attributes like innerHTML (for complex HTML)
    document.querySelectorAll('[data-i18n-html]').forEach(el => {
        const key = el.getAttribute('data-i18n-html');
        el.innerHTML = t(key, lang);
    });
    
    console.log('Language set to:', lang);
}

// Switch language
function switchLanguage(lang) {
    localStorage.setItem('syndimm-language', lang);
    applyTranslations(lang);
}

// ========== End Translation System ==========

// Tab Navigation
document.querySelectorAll('.nav-link').forEach(link => {
    link.addEventListener('click', (e) => {
        e.preventDefault();
        const tabId = link.dataset.tab;
        // Update nav
        document.querySelectorAll('.nav-link').forEach(l => l.classList.remove('active'));
        link.classList.add('active');
        // Update sections
        document.querySelectorAll('.tab-section').forEach(section => section.classList.remove('active'));
        document.getElementById(tabId).classList.add('active');
    });
});
// Accordion Toggle
function toggleAccordion(header) {
    const content = header.nextElementSibling;
    const isCollapsed = content.classList.contains('collapsed');
    
    // Check if this accordion is in the "settings" tab (Modes section)
    const settingsTab = document.getElementById('settings');
    const clickedAccordionInSettings = settingsTab && settingsTab.contains(header);
    
    if (clickedAccordionInSettings) {
        // Close all other accordions in the settings tab
        settingsTab.querySelectorAll('.accordion-header').forEach(otherHeader => {
            if (otherHeader !== header) {
                const otherContent = otherHeader.nextElementSibling;
                otherHeader.classList.remove('active');
                otherContent.classList.add('collapsed');
            }
        });
    }
    
    // Toggle active state
    header.classList.toggle('active');
    content.classList.toggle('collapsed');
    console.log('Accordion toggled:', isCollapsed ? 'opened' : 'closed');
}
// Mode Selection
document.querySelectorAll('.mode-card input[type="radio"]').forEach(radio => {
    radio.addEventListener('change', (e) => {
        document.querySelectorAll('.mode-card').forEach(card => card.classList.remove('active'));
        e.target.closest('.mode-card').classList.add('active');
        const mode = e.target.value;
        console.log('Mode:', mode);
        // API call
        fetch(`/api/mode/set?mode=${mode}`)
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    console.log('Mode changed successfully');
                    // Switch to Settings tab
                    const settingsTab = document.querySelector('[data-tab="settings"]');
                    if (settingsTab) {
                        settingsTab.click();
                    }
                    // Open corresponding accordion
                    setTimeout(() => {
                        const accordions = document.querySelectorAll('.accordion-header');
                        accordions.forEach(header => {
                            const title = header.querySelector('.accordion-title').textContent;
                            const content = header.nextElementSibling;
                            // Match mode to accordion title
                            if ((mode === 'dimmer' && title === 'Dimmer') ||
                                (mode === 'safe' && title === 'Safe') ||
                                (mode === 'panic' && title === 'Panic')) {
                                // Open this accordion
                                if (content.classList.contains('collapsed')) {
                                    header.click();
                                }
                            }
                        });
                    }, 100);
                } else {
                    alert('Failed to change mode');
                }
            })
            .catch(err => console.error('Mode change error:', err));
    });
});
// Slider Value Update
function updateSliderValue(slider) {
    const valueDisplay = document.getElementById(slider.id + '-value');
    if (valueDisplay) {
        valueDisplay.textContent = slider.value;
    }
}

// Shutter Slider Update
function updateShutterSlider(slider) {
    updateSliderValue(slider);
    // Send to device
    fetch(`/api/shutter/position?value=${slider.value}`)
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('Shutter position set to:', slider.value);
            }
        })
        .catch(err => console.error('Shutter position error:', err));
}

// Shutter Control Functions
function shutterOpen() {
    fetch('/api/shutter/open')
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('Shutter opening...');
                document.getElementById('shutter-status').textContent = 'Opening';
            }
        })
        .catch(err => console.error('Shutter open error:', err));
}

function shutterClose() {
    fetch('/api/shutter/close')
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('Shutter closing...');
                document.getElementById('shutter-status').textContent = 'Closing';
            }
        })
        .catch(err => console.error('Shutter close error:', err));
}

function shutterStop() {
    fetch('/api/shutter/stop')
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('Shutter stopped');
                document.getElementById('shutter-status').textContent = 'Stopped';
            }
        })
        .catch(err => console.error('Shutter stop error:', err));
}

// Device Power Toggle
function toggleDevicePower(isOn) {
    console.log('Device power:', isOn ? 'ON' : 'OFF');
    fetch(`/api/device/power?state=${isOn ? 1 : 0}`)
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('Device power toggled successfully');
            } else {
                alert('Failed to toggle device power');
            }
        })
        .catch(err => console.error('Device power error:', err));
}
// Brightness Slider
const brightnessSlider = document.getElementById('brightness');
if (brightnessSlider) {
    brightnessSlider.addEventListener('input', (e) => {
        e.target.setAttribute('value', e.target.value);
        updateSliderValue(e.target);
    });
    brightnessSlider.addEventListener('change', (e) => {
        const level = e.target.value;
        console.log('Brightness:', level);
        // API call
        fetch(`/api/dimmer/level?value=${level}`)
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    console.log('Brightness set to', level);
                }
            })
            .catch(err => console.error('Brightness error:', err));
    });
    // Initialize
    brightnessSlider.setAttribute('value', brightnessSlider.value);
    updateSliderValue(brightnessSlider);
}
// Ratio Slider
const ratioSlider = document.getElementById('ratio');
if (ratioSlider) {
    ratioSlider.addEventListener('input', (e) => {
        e.target.setAttribute('value', e.target.value);
        updateSliderValue(e.target);
    });
    ratioSlider.addEventListener('change', (e) => {
        const ratio = e.target.value;
        console.log('Ratio:', ratio);
        // API call
        fetch(`/api/dimmer/ratio?value=${ratio}`)
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    console.log('Ratio set to', ratio);
                }
            })
            .catch(err => console.error('Ratio error:', err));
    });
    // Initialize
    ratioSlider.setAttribute('value', ratioSlider.value);
    updateSliderValue(ratioSlider);
}
// Theme Selection
document.querySelectorAll('input[name="theme"]').forEach(radio => {
    radio.addEventListener('change', (e) => {
        document.querySelectorAll('.option-card').forEach(card => {
            if (card.querySelector('input[name="theme"]')) {
                card.classList.remove('active');
            }
        });
        e.target.closest('.option-card').classList.add('active');
        const theme = e.target.value;
        console.log('Theme:', theme);
        // Apply theme
        document.body.setAttribute('data-theme', theme);
        // Store in localStorage
        localStorage.setItem('syndimm-theme', theme);
    });
});
// Language Selection
document.querySelectorAll('input[name="language"]').forEach(radio => {
    radio.addEventListener('change', (e) => {
        document.querySelectorAll('.option-card').forEach(card => {
            if (card.querySelector('input[name="language"]')) {
                card.classList.remove('active');
            }
        });
        e.target.closest('.option-card').classList.add('active');
        const lang = e.target.value;
        console.log('Language:', lang);
        // Apply translations
        switchLanguage(lang);
    });
});
// AP Mode Toggle
const apToggle = document.getElementById('ap-mode');
if (apToggle) {
    apToggle.addEventListener('change', (e) => {
        console.log('AP Mode:', e.target.checked);
        // API call: /api/network/ap?enabled=true/false
    });
}
// Scan Devices (Dynamic button - Start/Stop)
let isScanning = false;
let connectedDeviceIP = '';  // Track connected device

function scanDevices() {
    console.log('[Scan] scanDevices() called, isScanning:', isScanning);
    const scanBtn = document.querySelector('.btn-scan');
    
    if (!scanBtn) {
        console.error('[Scan] ERROR: btn-scan not found!');
        return;
    }
    
    console.log('[Scan] Button found, current text:', scanBtn.textContent);
    
    if (isScanning) {
        // Stop scanning
        console.log('[Scan] Stopping scan...');
        
        fetch('/api/devices/scan/stop')
            .then(r => r.json())
            .then(data => {
                console.log('[Scan] Scan stopped');
                isScanning = false;
                
                // Update button - get translation properly
                const scanText = translations?.dimmer?.scanNetwork?.[currentLang] || 'Ag Tara';
                scanBtn.textContent = scanText;
                scanBtn.classList.remove('scanning');
                console.log('[Scan] Button updated to:', scanBtn.textContent);
                
                // Show final results
                displayScanResults(data);
            })
            .catch(err => {
                console.error('[Scan] Stop error:', err);
            });
        return;
    }
    
    // Start scanning
    console.log('[Scan] Starting device scan...');
    isScanning = true;
    
    // Update button - get translation properly
    const stopText = translations?.dimmer?.stopScanning?.[currentLang] || 'Taramayi Durdur';
    scanBtn.textContent = stopText;
    scanBtn.classList.add('scanning');
    console.log('[Scan] Button updated to:', scanBtn.textContent);
    
    const deviceList = document.querySelector('.device-list');
    
    if (!deviceList) {
        console.error('[Scan] ERROR: device-list element not found!');
        return;
    }
    
    deviceList.innerHTML = '<div class="empty-state"><p>Scanning network...</p><span>Checking 254 IP addresses</span></div>';
    
    // Start scan
    fetch('/api/devices/scan')
        .then(r => {
            console.log('[Scan] API response status:', r.status);
            return r.json();
        })
        .then(data => {
            console.log('[Scan] Initial response:', data);
            displayScanResults(data);
        })
        .catch(err => {
            console.error('[Scan] ERROR:', err);
            deviceList.innerHTML = '<div class="empty-state"><p>Scan failed</p><span>Check network connection</span></div>';
            
            // Reset button on error
            isScanning = false;
            const scanText = translations?.dimmer?.scanNetwork?.[currentLang] || 'Ag Tara';
            scanBtn.textContent = scanText;
            scanBtn.classList.remove('scanning');
        });
}

function displayScanResults(data) {
    const deviceList = document.querySelector('.device-list');
    const scanBtn = document.querySelector('.btn-scan');
    
    console.log('[Scan] Data received:', data);
    console.log('[Scan] Scanning:', data.scanning);
    console.log('[Scan] Devices count:', data.devices ? data.devices.length : 0);
    
    if (data.scanning) {
        // Tarama devam ediyor - cihazları ve progress'i göster
        let html = '';
        
        // Bulunan cihazları listele
        if (data.devices && data.devices.length > 0) {
            data.devices.forEach(device => {
                console.log('[Scan] Device found:', device.ip, device.type, device.model);
                
                // Check if this device is connected
                const isConnected = (connectedDeviceIP === device.ip);
                const buttonHTML = isConnected 
                    ? '<button class="btn-connect btn-disconnect" onclick="disconnectDevice()">Disconnect</button>'
                    : `<button class="btn-connect" onclick="connectDevice('${device.ip}')">Connect</button>`;
                
                html += `
                    <div class="device-item">
                        <div class="device-info">
                            <span class="device-name">${device.type || 'Unknown'}</span>
                            <span class="device-detail">${device.ip} ${device.model ? '(' + device.model + ')' : ''}</span>
                        </div>
                        <div class="device-action">
                            ${buttonHTML}
                        </div>
                    </div>
                `;
            });
        }
        
        // Progress ekle
        html += `
            <div class="scan-progress">
                <p class="scan-status">Scanning network... ${data.progress}%</p>
                <span class="scan-info">Found ${data.devices.length} dimmer(s) - Scanning continues...</span>
            </div>
        `;
        
        deviceList.innerHTML = html;
        console.log('[Scan] Still scanning, fetching progress in 2s...');
        
        // 2 saniyede bir güncelle
        setTimeout(() => {
            if (isScanning) {  // Only continue if still scanning
                fetch('/api/devices/scan/progress')
                    .then(r => r.json())
                    .then(displayScanResults)
                    .catch(err => console.error('Progress error:', err));
            }
        }, 2000);
        return;
    }
    
    // Scan complete - update button and show final results
    console.log('[Scan] Scan complete! Showing device list...');
    isScanning = false;
    const scanText = translations?.dimmer?.scanNetwork?.[currentLang] || 'Ag Tara';
    scanBtn.textContent = scanText;
    scanBtn.classList.remove('scanning');
    
    if (data.devices && data.devices.length > 0) {
        let html = '';
        data.devices.forEach(device => {
            console.log('[Scan] Device:', device.ip, device.type, device.model);
            
            // Check if this device is connected
            const isConnected = (connectedDeviceIP === device.ip);
            const buttonHTML = isConnected 
                ? '<button class="btn-connect btn-disconnect" onclick="disconnectDevice()">Disconnect</button>'
                : `<button class="btn-connect" onclick="connectDevice('${device.ip}')">Connect</button>`;
            
            html += `
                <div class="device-item">
                    <div class="device-info">
                        <span class="device-name">${device.type || 'Unknown'}</span>
                        <span class="device-detail">${device.ip} ${device.model ? '(' + device.model + ')' : ''}</span>
                    </div>
                    <div class="device-action">
                        ${buttonHTML}
                    </div>
                </div>
            `;
        });
        deviceList.innerHTML = html;
        console.log('[Scan] Device list rendered:', data.devices.length, 'devices');
    } else {
        deviceList.innerHTML = '<div class="empty-state"><p>No dimmers found</p><span>Only Shelly Dimmer/DALI devices are detected</span></div>';
        console.log('[Scan] No devices found');
    }
}

// Connect to Device
function connectDevice(ip) {
    console.log('[Connect] Attempting to connect to:', ip);
    
    // API call
    fetch(`/api/shelly/connect?ip=${ip}`)
        .then(r => {
            console.log('[Connect] Response status:', r.status);
            return r.json();
        })
        .then(data => {
            console.log('[Connect] Response data:', data);
            if (data.success) {
                console.log('[Connect] SUCCESS - Connected to:', ip);
                connectedDeviceIP = ip;  // Update global variable
                alert('Connected to ' + ip);
                
                // Update connection status WITHOUT page reload
                updateConnectionStatus();
                
                // If scan results are visible, update the buttons
                const deviceList = document.querySelector('.device-list');
                if (deviceList && deviceList.querySelector('.device-item')) {
                    // Re-render device list with updated buttons
                    const buttons = deviceList.querySelectorAll('.btn-connect');
                    buttons.forEach(btn => {
                        const btnIP = btn.getAttribute('onclick')?.match(/'([^']+)'/)?.[1];
                        if (btnIP === ip) {
                            btn.textContent = 'Disconnect';
                            btn.classList.add('btn-disconnect');
                            btn.setAttribute('onclick', 'disconnectDevice()');
                        }
                    });
                }
            } else {
                console.error('[Connect] FAILED:', data.error);
                alert('Failed to connect: ' + (data.error || 'Unknown error'));
            }
        })
        .catch(err => {
            console.error('[Connect] ERROR:', err);
            alert('Connection error: ' + err);
        });
}

// Manual IP Connect Dialog - Modal Version
function showManualIPDialog() {
    const modal = document.getElementById('manualIPModal');
    const input = document.getElementById('manualIP');
    const error = document.getElementById('manualIPError');
    
    // Clear previous state
    input.value = '';
    error.textContent = '';
    
    // Show modal
    modal.classList.add('show');
    modal.style.display = 'flex';
    
    // Focus input and add Enter key handler
    setTimeout(() => {
        input.focus();
        input.addEventListener('keypress', handleManualIPKeyPress);
    }, 100);
}

function handleManualIPKeyPress(event) {
    if (event.key === 'Enter') {
        event.preventDefault();
        connectManualIP();
    }
}

function closeManualIPModal() {
    const modal = document.getElementById('manualIPModal');
    const input = document.getElementById('manualIP');
    const error = document.getElementById('manualIPError');
    
    // Remove event listener
    input.removeEventListener('keypress', handleManualIPKeyPress);
    
    // Hide modal
    modal.classList.remove('show');
    modal.style.display = 'none';
    
    // Clear fields
    input.value = '';
    error.textContent = '';
}

function modalBackdropClick(event) {
    // Close modal if clicking on backdrop (not content)
    if (event.target.id === 'manualIPModal') {
        closeManualIPModal();
    }
}

function connectManualIP() {
    const input = document.getElementById('manualIP');
    const error = document.getElementById('manualIPError');
    const ip = input.value.trim();
    
    // Clear previous error
    error.textContent = '';
    
    // Validate IP format
    const ipPattern = /^(\d{1,3}\.){3}\d{1,3}$/;
    if (!ipPattern.test(ip)) {
        error.textContent = 'Invalid IP address format';
        input.focus();
        return;
    }
    
    // Validate IP ranges (0-255)
    const parts = ip.split('.');
    for (let part of parts) {
        const num = parseInt(part);
        if (num < 0 || num > 255) {
            error.textContent = 'IP address must be between 0.0.0.0 and 255.255.255.255';
            input.focus();
            return;
        }
    }
    
    console.log('[Manual] Connecting to:', ip);
    error.textContent = 'Connecting...';
    
    // Disable input during connection
    input.disabled = true;
    
    // Call manual connect API
    fetch('/api/devices/manual?ip=' + encodeURIComponent(ip))
        .then(r => r.json())
        .then(data => {
            input.disabled = false;
            if (data.success) {
                console.log('[Manual] SUCCESS - Device added');
                closeManualIPModal();
                
                // Update connection status
                updateConnectionStatus();
                
                // Refresh device list to show the newly added device
                setTimeout(() => {
                    fetchDevices();
                }, 500);
            } else {
                console.error('[Manual] FAILED:', data.message);
                error.textContent = 'Failed: ' + data.message;
            }
        })
        .catch(err => {
            input.disabled = false;
            console.error('[Manual] ERROR:', err);
            error.textContent = 'Connection error: ' + err;
        });
}

// Disconnect Device
function disconnectDevice() {
    console.log('[Disconnect] Disconnecting device...');
    
    // API call
    fetch('/api/shelly/disconnect')
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('[Disconnect] SUCCESS');
                const oldIP = connectedDeviceIP;
                connectedDeviceIP = '';  // Clear global variable
                
                // Update connection status WITHOUT page reload
                updateConnectionStatus();
                
                // If scan results are visible, update the buttons
                const deviceList = document.querySelector('.device-list');
                if (deviceList && deviceList.querySelector('.device-item')) {
                    // Re-render device list with updated buttons
                    const buttons = deviceList.querySelectorAll('.btn-disconnect');
                    buttons.forEach(btn => {
                        btn.textContent = 'Connect';
                        btn.classList.remove('btn-disconnect');
                        btn.setAttribute('onclick', `connectDevice('${oldIP}')`);
                    });
                }
            }
        })
        .catch(err => console.error('[Disconnect] ERROR:', err));
}
// Toggle Device (ON/OFF)
function toggleDevice() {
    fetch('/api/shelly/toggle')
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                // Wait a moment then update status
                setTimeout(() => {
                    updateConnectionStatus();
                }, 300);
            } else {
                alert('Failed to toggle device');
            }
        })
        .catch(err => {
            console.error('Toggle error:', err);
            alert('Error toggling device');
        });
}
// Update Connection Status
function updateConnectionStatus() {
    fetch('/api/shelly/status', { signal: AbortSignal.timeout(3000) })
        .then(r => r.json())
        .then(data => {
            // Update global connected device IP
            connectedDeviceIP = data.connected ? (data.ip || '') : '';
            console.log('[Status] Connected device IP:', connectedDeviceIP);
            
            // Update dimmer status grid
            const dimmerConnection = document.getElementById('dimmer-connection');
            const dimmerDevice = document.getElementById('dimmer-device');
            const devicePower = document.getElementById('device-power');
            
            if (data.connected) {
                if (dimmerConnection) dimmerConnection.textContent = 'Connected';
                if (dimmerDevice) dimmerDevice.textContent = `${data.type || 'Shelly'} (${data.ip})`;
                if (devicePower) devicePower.checked = data.ison || false;
                
                // Update brightness slider - GERÇEK ZAMANLI SENKRONIZASYON
                const brightnessSlider = document.getElementById('brightness');
                const brightnessValue = document.getElementById('brightness-value');
                if (brightnessSlider && data.brightness !== undefined) {
                    // Sadece kullanıcı slider'ı kullanmıyorsa güncelle
                    if (document.activeElement !== brightnessSlider && !brightnessSlider.matches(':active')) {
                        brightnessSlider.value = data.brightness;
                        if (brightnessValue) brightnessValue.textContent = data.brightness;
                    }
                }
                
                // Update device list
                updateDeviceList(data.ip);
            } else {
                if (dimmerConnection) dimmerConnection.textContent = 'Not Connected';
                if (dimmerDevice) dimmerDevice.textContent = 'No Device';
                if (devicePower) devicePower.checked = false;
            }
        })
        .catch(err => {
            // Silently ignore temporary network errors during reconnection
            if (err.name === 'AbortError' || err.name === 'TimeoutError') return;
            console.debug('Status error:', err);
        });
}
// Update Encoder Brightness (real-time sync from encoder)
function updateEncoderBrightness() {
    fetch('/api/dimmer/level')
        .then(r => r.json())
        .then(data => {
            const brightnessSlider = document.getElementById('brightness');
            if (brightnessSlider && data.level !== undefined) {
                // Sadece kullanıcı slider'ı değiştirmiyorsa güncelle
                // (Slider'a focus yoksa veya mouse basılı değilse)
                if (document.activeElement !== brightnessSlider && !brightnessSlider.matches(':active')) {
                    brightnessSlider.value = data.level;
                    brightnessSlider.setAttribute('value', data.level);
                }
            }
            
            // ÜSTTEKİ ÇERÇEVE İÇİNDEKİ BRIGHTNESS DEĞERİNİ GÜNCELLE
            const connection = document.querySelector('.connection-status');
            if (connection && connection.classList.contains('connected')) {
                const brightnessDisplay = connection.querySelector('.value-number');
                if (brightnessDisplay && data.level !== undefined) {
                    brightnessDisplay.textContent = data.level;
                }
            }
        })
        .catch(err => console.error('Encoder brightness error:', err));
}
// Update Device List
function updateDeviceList(connectedIp) {
    // DISABLED: Bu fonksiyon cihaz listesini siliyordu
    // Artık scan sonuçları kalıcı - sadece butonlar "Connect"/"Disconnect" arasında değişiyor
    console.log('[DeviceList] Update called for IP:', connectedIp, '(list preserved)');
}
// Network Configuration Functions
function saveNetworkConfig() {
    // Get WiFi 1 values
    const wifi1_ssid = document.getElementById('wifi1-ssid').value;
    const wifi1_password = document.getElementById('wifi1-password').value;
    const wifi1_ip = document.getElementById('wifi1-ip').value;
    const wifi1_local = document.getElementById('wifi1-local').value;
    
    // Get WiFi 2 values
    const wifi2_ssid = document.getElementById('wifi2-ssid').value;
    const wifi2_password = document.getElementById('wifi2-password').value;
    const wifi2_ip = document.getElementById('wifi2-ip').value;
    const wifi2_local = document.getElementById('wifi2-local').value;
    
    let saveCount = 0;
    let successCount = 0;
    
    // Save WiFi 1 if SSID is provided
    if (wifi1_ssid) {
        saveCount++;
        fetch(`/api/network/wifi1?ssid=${encodeURIComponent(wifi1_ssid)}&pass=${encodeURIComponent(wifi1_password)}&ip=${encodeURIComponent(wifi1_ip)}&local=${encodeURIComponent(wifi1_local)}`)
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    successCount++;
                    checkComplete();
                } else {
                    alert('Failed to save WiFi 1 configuration');
                }
            })
            .catch(err => {
                console.error('WiFi 1 save error:', err);
                alert('Error saving WiFi 1 configuration');
            });
    }
    
    // Save WiFi 2 if SSID is provided
    if (wifi2_ssid) {
        saveCount++;
        fetch(`/api/network/wifi2?ssid=${encodeURIComponent(wifi2_ssid)}&pass=${encodeURIComponent(wifi2_password)}&ip=${encodeURIComponent(wifi2_ip)}&local=${encodeURIComponent(wifi2_local)}`)
            .then(r => r.json())
            .then(data => {
                if (data.success) {
                    successCount++;
                    checkComplete();
                } else {
                    alert('Failed to save WiFi 2 configuration');
                }
            })
            .catch(err => {
                console.error('WiFi 2 save error:', err);
                alert('Error saving WiFi 2 configuration');
            });
    }
    
    if (saveCount === 0) {
        alert('Please enter at least one WiFi SSID');
        return;
    }
    
    function checkComplete() {
        if (successCount === saveCount) {
            alert('Network configuration saved. Device is reconnecting...\n\nPage will reload in 5 seconds.');
            
            // Sayfa yenilemeden önce periyodik güncellemeleri durdur
            // (Bu, hataların görünmesini önler)
            
            // 5 saniye sonra sayfayı yenile
            setTimeout(() => {
                // Mevcut IP adresiyle yenile (mDNS henüz hazır olmayabilir)
                window.location.reload();
            }, 5000);
        }
    }
}
// Save Mode Configuration
function saveModeConfig() {
    console.log('Saving mode configuration...');
    
    // Safe Lock konfigürasyonunu kaydet
    saveSafeLockConfig()
        .then(() => {
            alert('Mode configuration saved successfully!');
            console.log('All settings saved');
        })
        .catch(err => {
            alert('Failed to save configuration: ' + err.message);
            console.error('Save error:', err);
        });
}
// Update Network Status
function updateNetworkStatus() {
    fetch('/api/network/status', { 
        signal: AbortSignal.timeout(3000) // 3 saniye timeout
    })
        .then(r => r.json())
        .then(data => {
            const statusItems = document.querySelectorAll('.network-status .status-value');
            if (statusItems.length >= 2) {
                statusItems[0].textContent = data.mode || 'Unknown';
                statusItems[1].textContent = data.ssid || 'N/A';
            }
            
            // Update IP and mDNS (stacked layout)
            const ipValue = document.querySelector('.ip-value');
            const mdnsStatus = document.getElementById('status-mdns');
            
            if (ipValue) {
                ipValue.textContent = data.ip || 'N/A';
            }
            if (mdnsStatus && data.mdns) {
                mdnsStatus.textContent = data.mdns;
            }
        })
        .catch(err => {
            // Hataları sessizce yoksay (cihaz yeniden bağlanıyorsa normal)
            if (err.name !== 'AbortError' && err.name !== 'TimeoutError') {
                console.debug('Network status temporarily unavailable');
            }
        });
}
// Load Network Info
function loadNetworkInfo() {
    fetch('/api/network/info')
        .then(r => r.json())
        .then(data => {
            // Update Chip ID in header
            if (data.chipID) {
                const chipIdValue = document.getElementById('chip-id-value');
                if (chipIdValue) {
                    chipIdValue.textContent = data.chipID;
                }
            }
            
            // AP Mode Info (read-only display)
            if (data.ap) {
                const apSSIDDisplay = document.getElementById('ap-ssid-display');
                const apMdnsDisplay = document.getElementById('ap-mdns-display');
                const apStatus = document.getElementById('ap-status');
                
                if (apSSIDDisplay) {
                    apSSIDDisplay.textContent = data.ap.ssid || 'SynDimm-XXXXXX';
                }
                
                if (apMdnsDisplay && data.mdns) {
                    apMdnsDisplay.textContent = data.mdns;
                }
                
                if (apStatus) {
                    if (data.ap.active) {
                        apStatus.textContent = t('network.ap.active');
                        apStatus.classList.remove('inactive');
                        apStatus.classList.add('connected');
                    } else {
                        apStatus.textContent = t('network.ap.inactive');
                        apStatus.classList.remove('connected');
                        apStatus.classList.add('inactive');
                    }
                }
            }
            
            // WiFi 1 Config
            if (data.wifi1) {
                const wifi1SSID = document.getElementById('wifi1-ssid');
                const wifi1IP = document.getElementById('wifi1-ip');
                const wifi1Local = document.getElementById('wifi1-local');
                if (wifi1SSID) wifi1SSID.value = data.wifi1.ssid || '';
                if (wifi1IP) wifi1IP.value = data.wifi1.ip || '';
                if (wifi1Local) wifi1Local.value = data.wifi1.local || '';
                // Update WiFi 1 status badge
                updateWiFiStatus('wifi1', data.wifi1.ssid);
            }
            
            // WiFi 2 Config
            if (data.wifi2) {
                const wifi2SSID = document.getElementById('wifi2-ssid');
                const wifi2IP = document.getElementById('wifi2-ip');
                const wifi2Local = document.getElementById('wifi2-local');
                if (wifi2SSID) wifi2SSID.value = data.wifi2.ssid || '';
                if (wifi2IP) wifi2IP.value = data.wifi2.ip || '';
                if (wifi2Local) wifi2Local.value = data.wifi2.local || '';
                // Update WiFi 2 status badge
                updateWiFiStatus('wifi2', data.wifi2.ssid);
            }
        })
        .catch(err => console.error('Network info error:', err));
}
// Update WiFi Status Badge
function updateWiFiStatus(wifiNum, ssid) {
    const headers = document.querySelectorAll('.accordion-header');
    headers.forEach(header => {
        const title = header.querySelector('.accordion-title').textContent;
        const statusBadge = header.querySelector('.accordion-status');
        if ((wifiNum === 'wifi1' && title === 'PRIMARY WIFI') ||
            (wifiNum === 'wifi2' && title === 'BACKUP WIFI')) {
            if (ssid && ssid !== '') {
                // Check if this WiFi is currently connected
                fetch('/api/network/status')
                    .then(r => r.json())
                    .then(data => {
                        if (data.mode === 'WiFi' && data.ssid === ssid) {
                            statusBadge.textContent = 'Connected';
                            statusBadge.classList.remove('inactive', 'not-configured');
                            statusBadge.classList.add('connected');
                        } else {
                            statusBadge.textContent = 'Configured';
                            statusBadge.classList.remove('inactive', 'connected');
                            statusBadge.classList.add('not-configured');
                        }
                    });
            } else {
                statusBadge.textContent = 'Not Configured';
                statusBadge.classList.remove('connected');
                statusBadge.classList.add('not-configured');
            }
        }
    });
}
// Load Encoder Values
function loadEncoderValues() {
    fetch('/api/encoder/values')
        .then(r => r.json())
        .then(data => {
            // Update encoder value displays if needed
            console.log('Encoder values:', data);
        })
        .catch(err => console.error('Encoder values error:', err));
}
// Load Current Mode
function loadCurrentMode() {
    fetch('/api/mode/get')
        .then(r => r.json())
        .then(data => {
            if (data.mode) {
                const modeRadio = document.querySelector(`input[name="mode"][value="${data.mode}"]`);
                if (modeRadio) {
                    modeRadio.checked = true;
                    // Update mode card UI
                    document.querySelectorAll('.mode-card').forEach(card => card.classList.remove('active'));
                    modeRadio.closest('.mode-card').classList.add('active');
                }
            }
        })
        .catch(err => console.error('Mode get error:', err));
}
// Load Dimmer Settings (brightness and ratio)
function loadDimmerSettings() {
    // Load brightness level
    fetch('/api/dimmer/level')
        .then(r => r.json())
        .then(data => {
            if (data.level !== undefined) {
                const brightnessSlider = document.getElementById('brightness');
                const brightnessValue = document.getElementById('brightness-value');
                if (brightnessSlider) {
                    brightnessSlider.value = data.level;
                    brightnessSlider.setAttribute('value', data.level);
                }
                if (brightnessValue) {
                    brightnessValue.textContent = data.level;
                }
            }
        })
        .catch(err => console.error('Load brightness error:', err));
    // Load dimm ratio
    fetch('/api/dimmer/ratio')
        .then(r => r.json())
        .then(data => {
            if (data.ratio !== undefined) {
                const ratioSlider = document.getElementById('ratio');
                const ratioValue = document.getElementById('ratio-value');
                if (ratioSlider) {
                    ratioSlider.value = data.ratio;
                    ratioSlider.setAttribute('value', data.ratio);
                }
                if (ratioValue) {
                    ratioValue.textContent = data.ratio;
                }
            }
        })
        .catch(err => console.error('Load ratio error:', err));
}
// Initialize - Load all data on page load
function initializePage() {
    console.log('SynDimm initializing...');
    
    // Load translations first
    loadTranslations().then(() => {
        // Set saved language
        const savedLang = localStorage.getItem('syndimm-language') || 'tr';
        const langRadio = document.querySelector(`input[name="language"][value="${savedLang}"]`);
        if (langRadio) {
            langRadio.checked = true;
            langRadio.closest('.option-card')?.classList.add('active');
        }
        
        // Fix scan button text after translations loaded
        const scanBtn = document.querySelector('.btn-scan');
        if (scanBtn && !isScanning) {
            const scanText = translations?.dimmer?.scanNetwork?.[currentLang] || 'Ag Tara';
            scanBtn.textContent = scanText;
        }
    });
    
    // Load theme from localStorage
    const savedTheme = localStorage.getItem('syndimm-theme') || 'dark';
    document.body.setAttribute('data-theme', savedTheme);
    // Set theme radio button
    const themeRadio = document.querySelector(`input[name="theme"][value="${savedTheme}"]`);
    if (themeRadio) {
        themeRadio.checked = true;
        themeRadio.closest('.option-card')?.classList.add('active');
    }
    
    // Load network configuration
    loadNetworkInfo();
    // Update network status
    updateNetworkStatus();
    // Update device connection status (this will set connectedDeviceIP)
    updateConnectionStatus();
    // Load current mode
    loadCurrentMode();
    // Load dimmer settings (brightness and ratio)
    loadDimmerSettings();
    // Load encoder values
    loadEncoderValues();
    // REMOVED: Automatic scan on startup - user must manually click "Scan Network"
    // scanDevices();
    // Load Safe Lock configuration
    loadSafeLockConfig();
    
    // Load OTA settings and setup listeners
    loadOTASettings();
    setupOTAListeners();
    
    // Start periodic status updates - HIZLI SENKRONIZASYON (her 500ms)
    setInterval(() => {
        updateConnectionStatus();
        updateNetworkStatus();
        loadCurrentMode();  // Poll mode changes from encoder
    }, 500);  // 5000ms -> 500ms (10x daha hızlı)
    
    // OTA check - her 5 saniyede bir (Info sekmesindeki versiyon için)
    setInterval(() => {
        loadOTASettings();
    }, 5000);
    
    console.log('SynDimm initialized');
}
// ===================================
// Safe Lock Functions
// ===================================
// Switch Safe password tabs
function switchSafeTab(index) {
    document.querySelectorAll('.safe-tab').forEach((tab, i) => {
        tab.classList.toggle('active', i === index);
    });
    document.querySelectorAll('.safe-tab-content').forEach((content, i) => {
        content.classList.toggle('active', i === index);
    });
}
// Toggle password enable/disable
function togglePassword(index, enabled) {
    console.log(`Password ${index} ${enabled ? 'enabled' : 'disabled'}`);
    const container = document.querySelector(`[data-password-index="${index}"]`);
    if (container) {
        container.style.opacity = enabled ? '1' : '0.5';
    }
}
// Toggle API configuration visibility
function toggleApiConfig(index, enabled) {
    console.log(`API ${index} ${enabled ? 'enabled' : 'disabled'}`);
}
// Validate password format
function validatePassword(password) {
    if (!password || password.trim() === '') {
        return { valid: false, message: '' };
    }
    const parts = password.split('-');
    if (parts.length < 3 || parts.length > 6) {
        return { valid: false, message: 'Password must have 3-6 steps' };
    }
    for (let part of parts) {
        const dir = part[0];
        if (dir !== 'L' && dir !== 'R' && dir !== 'B') {
            return { valid: false, message: `Invalid direction: ${dir}` };
        }
        if (dir !== 'B') {
            const ticks = parseInt(part.substring(1));
            if (isNaN(ticks) || ticks < 1 || ticks > 50) {
                return { valid: false, message: `Ticks must be 1-50: ${part}` };
            }
        }
    }
    return { valid: true, message: 'Valid password format' };
}
// Real-time password validation
for (let i = 0; i < 5; i++) {
    const pwdInput = document.getElementById(`pwd${i}-value`);
    const validationDiv = document.getElementById(`pwd${i}-validation`);
    if (pwdInput && validationDiv) {
        pwdInput.addEventListener('input', () => {
            const result = validatePassword(pwdInput.value);
            if (pwdInput.value === '') {
                validationDiv.textContent = '';
                validationDiv.className = 'password-validation';
            } else if (result.valid) {
                validationDiv.textContent = '✓ ' + result.message;
                validationDiv.className = 'password-validation valid';
            } else {
                validationDiv.textContent = '✗ ' + result.message;
                validationDiv.className = 'password-validation invalid';
            }
        });
    }
    // Toggle POST body field visibility
    const methodRadios = document.querySelectorAll(`input[name="pwd${i}-api-method"]`);
    const bodyField = document.getElementById(`pwd${i}-body-field`);
    if (methodRadios.length > 0 && bodyField) {
        methodRadios.forEach(radio => {
            radio.addEventListener('change', () => {
                bodyField.style.display = radio.value === 'POST' ? 'block' : 'none';
            });
        });
    }
}
// Load Safe Lock configuration from device
function loadSafeLockConfig() {
    fetch('/api/safe/config')
        .then(r => r.json())
        .then(data => {
            if (data.passwords) {
                for (let i = 0; i < 5; i++) {
                    const pwd = data.passwords[i];
                    if (pwd) {
                        // Password enabled
                        const pwdCheckbox = document.getElementById(`pwd${i}-enabled`);
                        pwdCheckbox.checked = pwd.enabled || false;
                        togglePassword(i, pwd.enabled || false);
                        
                        // Password value
                        document.getElementById(`pwd${i}-value`).value = pwd.password || '';
                        
                        if (pwd.api) {
                            // API enabled - ÖNEMLİ: dispatchEvent ile toggle switch'i güncelle
                            const apiCheckbox = document.getElementById(`pwd${i}-api-enabled`);
                            apiCheckbox.checked = pwd.api.enabled || false;
                            // Manuel event tetikle (CSS toggle için)
                            toggleApiConfig(i, pwd.api.enabled || false);
                            
                            document.getElementById(`pwd${i}-api-url`).value = pwd.api.url || '';
                            document.getElementById(`pwd${i}-api-header`).value = pwd.api.header || '';
                            document.getElementById(`pwd${i}-api-body`).value = pwd.api.body || '';
                            const methodRadio = document.querySelector(`input[name="pwd${i}-api-method"][value="${pwd.api.method || 'GET'}"]`);
                            if (methodRadio) {
                                methodRadio.checked = true;
                                methodRadio.closest('.option-card').classList.add('active');
                            }
                        }
                    }
                }
                console.log('Safe Lock config loaded');
            }
        })
        .catch(err => console.error('Failed to load Safe Lock config:', err));
}
// Save Safe Lock configuration (called from saveModeConfig)
function saveSafeLockConfig() {
    const passwords = [];
    for (let i = 0; i < 5; i++) {
        const enabled = document.getElementById(`pwd${i}-enabled`).checked;
        const password = document.getElementById(`pwd${i}-value`).value;
        const oldPassword = document.getElementById(`pwd${i}-old`).value;
        const apiEnabled = document.getElementById(`pwd${i}-api-enabled`).checked;
        const apiUrl = document.getElementById(`pwd${i}-api-url`).value;
        const apiMethod = document.querySelector(`input[name="pwd${i}-api-method"]:checked`).value;
        const apiHeader = document.getElementById(`pwd${i}-api-header`).value;
        const apiBody = document.getElementById(`pwd${i}-api-body`).value;
        passwords.push({
            index: i,
            enabled: enabled,
            password: password,
            oldPassword: oldPassword,
            api: {
                enabled: apiEnabled,
                url: apiUrl,
                method: apiMethod,
                header: apiHeader,
                body: apiBody
            }
        });
    }
    return fetch('/api/safe/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify({ passwords: passwords })
    })
    .then(r => r.json())
    .then(data => {
        if (data.success) {
            console.log('Safe Lock config saved');
            // Clear old password fields
            for (let i = 0; i < 5; i++) {
                document.getElementById(`pwd${i}-old`).value = '';
            }
            return true;
        } else {
            throw new Error(data.error || 'Failed to save');
        }
    });
}

// OTA Settings functions
function loadOTASettings() {
    // Load OTA info (versions)
    fetch('/api/ota/info')
        .then(r => r.json())
        .then(data => {
            document.getElementById('ota-current-version').textContent = data.current || '-';
            
            if (data.updateAvailable) {
                document.getElementById('ota-latest-version').textContent = data.latest || '-';
                document.getElementById('ota-latest-container').style.display = 'block';
            } else {
                document.getElementById('ota-latest-container').style.display = 'none';
            }
            
            // Update button visibility based on auto-update setting
            updateOTAButtonVisibility(data.updateAvailable);
        })
        .catch(err => console.error('Failed to load OTA info:', err));
    
    // Load OTA settings (auto-update toggle)
    fetch('/api/ota/settings')
        .then(r => r.json())
        .then(data => {
            const autoUpdateCheckbox = document.getElementById('ota-auto-update');
            autoUpdateCheckbox.checked = data.autoUpdate;
            
            // Update button visibility
            fetch('/api/ota/info')
                .then(r => r.json())
                .then(info => updateOTAButtonVisibility(info.updateAvailable));
        })
        .catch(err => console.error('Failed to load OTA settings:', err));
}

function updateOTAButtonVisibility(updateAvailable) {
    const autoUpdateCheckbox = document.getElementById('ota-auto-update');
    const updateButton = document.getElementById('ota-update-button');
    
    // Show button only if: auto-update OFF AND new version available
    if (!autoUpdateCheckbox.checked && updateAvailable) {
        updateButton.style.display = 'inline-block';
    } else {
        updateButton.style.display = 'none';
    }
}

function setupOTAListeners() {
    // Auto-update toggle
    const autoUpdateCheckbox = document.getElementById('ota-auto-update');
    autoUpdateCheckbox.addEventListener('change', function() {
        const enabled = this.checked;
        
        fetch('/api/ota/settings', {
            method: 'POST',
            headers: { 'Content-Type': 'application/x-www-form-urlencoded' },
            body: 'autoUpdate=' + enabled
        })
        .then(r => r.json())
        .then(data => {
            if (data.success) {
                console.log('Auto-update setting saved:', enabled);
                
                // Update button visibility
                fetch('/api/ota/info')
                    .then(r => r.json())
                    .then(info => updateOTAButtonVisibility(info.updateAvailable));
            }
        })
        .catch(err => console.error('Failed to save auto-update setting:', err));
    });
    
    // Manual update button
    const updateButton = document.getElementById('ota-update-button');
    updateButton.addEventListener('click', function() {
        if (confirm('Firmware güncellemesi başlatılsın mı? Cihaz yeniden başlatılacak.')) {
            this.disabled = true;
            this.textContent = 'Güncelleniyor...';
            
            fetch('/api/ota/update')
                .then(r => r.json())
                .then(data => {
                    if (data.success) {
                        alert('Güncelleme başladı. Cihaz yeniden başlatılıyor...');
                    } else {
                        alert('Güncelleme başlatılamadı: ' + (data.message || data.error));
                        this.disabled = false;
                        this.textContent = 'Şimdi Güncelle';
                    }
                })
                .catch(err => {
                    console.error('Update failed:', err);
                    alert('Güncelleme hatası: ' + err);
                    this.disabled = false;
                    this.textContent = 'Şimdi Güncelle';
                });
        }
    });
}

// Initialize
document.addEventListener('DOMContentLoaded', initializePage);
)=====";

#endif
