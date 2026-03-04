/* ── Theme ────────────────────────────────────────── */
let currentTheme=localStorage.getItem('syndimm-theme')||'dark';
function setTheme(theme){
    currentTheme=theme;
    document.documentElement.setAttribute('data-theme',theme);
    localStorage.setItem('syndimm-theme',theme);
    const sel=document.getElementById('theme-select');
    if(sel) sel.value=theme;
}
function initTheme(){
    document.documentElement.setAttribute('data-theme',currentTheme);
    const sel=document.getElementById('theme-select');
    if(sel) sel.value=currentTheme;
}

/* ── Mode Config ──────────────────────────────────── */
let modeConfig={multiMode:true,modes:[]};
let activeMode=null;

function showEmptyState(){
    document.getElementById('empty-state').style.display='flex';
    document.getElementById('orbital-container').style.display='none';
    document.getElementById('device-info').style.display='none';
    activeMode=null;
}
function hideEmptyState(){
    document.getElementById('empty-state').style.display='none';
    document.getElementById('orbital-container').style.display='';
    document.getElementById('device-info').style.display='';
}

function loadModeConfig(){
    fetch('/api/modes').then(r=>r.json()).then(cfg=>{
        modeConfig=cfg;
        document.getElementById('multi-mode-toggle').classList.toggle('on',cfg.multiMode);
        renderModeNav();
        renderModeList();
        if(cfg.modes.length>0){
            hideEmptyState();
            activateMode(cfg.modes[0].id);
        } else {
            showEmptyState();
        }
    }).catch(()=>{
        // API basarisiz - bos state goster (hardcoded mod yok)
        modeConfig={multiMode:true,modes:[]};
        renderModeNav();
        renderModeList();
        showEmptyState();
    });
}

function renderModeNav(){
    const nav=document.getElementById('mode-nav');
    const gear=nav.querySelector('.gear-btn');
    // Remove old mode tabs
    nav.querySelectorAll('.mode-tab').forEach(t=>t.remove());
    if(modeConfig.multiMode&&modeConfig.modes.length>1){
        modeConfig.modes.forEach(m=>{
            const btn=document.createElement('button');
            btn.className='mode-tab';
            btn.textContent=m.name;
            btn.dataset.modeId=m.id;
            btn.onclick=()=>activateMode(m.id);
            nav.insertBefore(btn,gear);
        });
    }
    // Highlight active
    if(activeMode){
        const el=nav.querySelector('[data-mode-id="'+activeMode+'"]');
        if(el) el.classList.add('active');
    }
    // Tek gear varsa ortalamak icin class ekle
    if(!modeConfig.multiMode||modeConfig.modes.length<=1){
        nav.classList.add('gear-only');
    } else {
        nav.classList.remove('gear-only');
    }
}

function activateMode(modeId){
    const mode=modeConfig.modes.find(m=>m.id===modeId);
    if(!mode) return;
    activeMode=modeId;
    // Nav highlight
    document.querySelectorAll('.mode-tab').forEach(t=>t.classList.remove('active'));
    const el=document.querySelector('[data-mode-id="'+modeId+'"]');
    if(el) el.classList.add('active');
    // Body class (no mode-specific accents anymore, just for type detection)
    document.body.className='mode-'+mode.type;
    // Display
    const isSafe=mode.type==='safe';
    document.getElementById('value-display').style.display=isSafe?'none':'block';
    document.getElementById('safe-icon').style.display=isSafe?'block':'none';
    document.getElementById('safe-label').style.display=isSafe?'block':'none';
    document.getElementById('status-line').style.display=isSafe?'none':'block';
    if(!isSafe){
        document.getElementById('big-unit').textContent=(mode.params&&mode.params.unit)||'%';
    }
    // device-info: sadece multiMode && modes>1 ise goster
    const infoEl=document.getElementById('device-info');
    if(modeConfig.multiMode&&modeConfig.modes.length>1){
        infoEl.textContent=mode.name;
        infoEl.style.display='';
    } else {
        infoEl.style.display='none';
    }
    // GUI → HW mod senkronizasyonu
    fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({mode:mode.type})}).catch(()=>{});
}

/* ── Mode Management (Modlar tab) ─────────────────── */
function renderModeList(){
    const list=document.getElementById('mode-list');
    if(!list) return;
    list.innerHTML='';
    modeConfig.modes.forEach((m,i)=>{
        const isFirst=i===0, isLast=i===modeConfig.modes.length-1;
        const card=document.createElement('div');
        card.className='mode-card';
        card.innerHTML=
            '<div class="mode-card-order">'+
                '<button '+(isFirst?'disabled':'')+' onclick="moveMode(\''+m.id+'\',-1)">'+
                    '<svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="M18 15l-6-6-6 6"/></svg>'+
                '</button>'+
                '<span class="mode-card-num">'+(i+1)+'</span>'+
                '<button '+(isLast?'disabled':'')+' onclick="moveMode(\''+m.id+'\',1)">'+
                    '<svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="M6 9l6 6 6-6"/></svg>'+
                '</button>'+
            '</div>'+
            '<div class="mode-card-info">'+
                '<strong>'+esc(m.name)+'</strong>'+
                '<span class="mode-badge">'+esc(m.type)+'</span>'+
            '</div>'+
            '<div class="mode-card-actions">'+
                '<button title="Duzenle" onclick="renameMode(\''+m.id+'\')">'+
                    '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 4H4a2 2 0 00-2 2v14a2 2 0 002 2h14a2 2 0 002-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 013 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>'+
                '</button>'+
                '<button class="del" title="Sil" onclick="deleteMode(\''+m.id+'\')">'+
                    '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 6h18M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"/></svg>'+
                '</button>'+
            '</div>';
        list.appendChild(card);
    });
}

function moveMode(id,dir){
    const idx=modeConfig.modes.findIndex(m=>m.id===id);
    if(idx<0) return;
    const newIdx=idx+dir;
    if(newIdx<0||newIdx>=modeConfig.modes.length) return;
    const tmp=modeConfig.modes[idx];
    modeConfig.modes[idx]=modeConfig.modes[newIdx];
    modeConfig.modes[newIdx]=tmp;
    saveModeConfig();
}

function showAddMode(){
    document.getElementById('add-mode-form').style.display='block';
    document.getElementById('add-mode-btn').style.display='none';
    document.getElementById('new-mode-name').value='';
    document.getElementById('new-mode-type').value='dimmer';
}
function hideAddMode(){
    document.getElementById('add-mode-form').style.display='none';
    document.getElementById('add-mode-btn').style.display='';
}

function addMode(){
    const name=document.getElementById('new-mode-name').value.trim();
    const type=document.getElementById('new-mode-type').value;
    if(!name) return;
    const id='mode_'+Date.now();
    const params=type==='safe'?{}:{min:0,max:100,unit:'%'};
    const endpoint=type==='safe'?'':'/api/value';
    modeConfig.modes.push({id,name,type,endpoint,params});
    hideEmptyState();
    saveModeConfig();
    activateMode(id);
    hideAddMode();
}

function deleteMode(id){
    if(!confirm('Bu mod silinecek. Emin misiniz?')) return;
    modeConfig.modes=modeConfig.modes.filter(m=>m.id!==id);
    saveModeConfig();
    if(modeConfig.modes.length===0){
        showEmptyState();
    } else if(activeMode===id){
        activateMode(modeConfig.modes[0].id);
    }
}

function renameMode(id){
    const mode=modeConfig.modes.find(m=>m.id===id);
    if(!mode) return;
    const newName=prompt('Yeni mod adi:',mode.name);
    if(!newName||!newName.trim()) return;
    mode.name=newName.trim();
    saveModeConfig();
}

function toggleMultiMode(){
    modeConfig.multiMode=!modeConfig.multiMode;
    document.getElementById('multi-mode-toggle').classList.toggle('on',modeConfig.multiMode);
    if(!modeConfig.multiMode && modeConfig.modes.length>0){
        activateMode(modeConfig.modes[0].id);
    }
    saveModeConfig();
}

function saveModeConfig(){
    renderModeList();
    renderModeNav();
    fetch('/api/modes',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(modeConfig)}).catch(()=>{});
}

/* ── Settings Modal ──────────────────────────────── */
function toggleSettings(){
    const modal=document.getElementById('modal');
    const opening=!modal.classList.contains('open');
    modal.classList.toggle('open');
    if(opening){
        // Her zaman ilk sekmeden baslat
        document.querySelectorAll('.modal-tab').forEach(t=>t.classList.remove('active'));
        document.querySelectorAll('.tab-panel').forEach(p=>p.classList.remove('active'));
        const firstTab=document.querySelector('.modal-tab');
        const firstPanel=document.querySelector('.tab-panel');
        if(firstTab) firstTab.classList.add('active');
        if(firstPanel) firstPanel.classList.add('active');
        // Reset subpages
        document.querySelectorAll('.info-detail').forEach(d=>d.style.display='none');
        document.querySelectorAll('[id^="sistem-detail-"]').forEach(d=>d.style.display='none');
        if(document.getElementById('sistem-main')) document.getElementById('sistem-main').style.display='';
        if(document.getElementById('info-main')) document.getElementById('info-main').style.display='';
        subpageActive=false;updateFooterBtn();
    }
}
function switchTab(tabName,btn){
    document.querySelectorAll('.modal-tab').forEach(t=>t.classList.remove('active'));
    btn.classList.add('active');
    document.querySelectorAll('.tab-panel').forEach(p=>p.classList.remove('active'));
    document.querySelector('.tab-panel[data-tab="'+tabName+'"]').classList.add('active');
    // Reset detail views
    document.querySelectorAll('.info-detail').forEach(d=>d.style.display='none');
    document.querySelectorAll('[id^="sistem-detail-"]').forEach(d=>d.style.display='none');
    if(document.getElementById('sistem-main')) document.getElementById('sistem-main').style.display='';
    if(document.getElementById('info-main')) document.getElementById('info-main').style.display='';
    subpageActive=false;updateFooterBtn();
}
// Modal kapanma: sadece X butonu ile
// Overlay tiklamasini engelle — modal-box icindeki tiklamalar yukariya cikmaz
document.querySelector('.modal-box').addEventListener('click',function(e){e.stopPropagation()});
document.getElementById('modal').addEventListener('click',function(e){
    // Sadece dogrudan modal-bg'ye tiklama — hicbir sey yapma (eski: toggleSettings)
    // Kasitli olarak bos: overlay'e tiklama modali kapatmaz
});

/* ── Sistem Detail Navigation ────────────────────── */
function showSistemDetail(name){
    document.getElementById('sistem-main').style.display='none';
    document.getElementById('sistem-detail-'+name).style.display='block';
    subpageActive=true;updateFooterBtn();
}
function hideSistemDetail(){
    document.querySelectorAll('[id^="sistem-detail-"]').forEach(d=>d.style.display='none');
    document.getElementById('sistem-main').style.display='block';
    subpageActive=false;updateFooterBtn();
}

/* ── WiFi ─────────────────────────────────────────── */
function loadWifiConfig(){
    fetch('/api/wifi/config').then(r=>r.json()).then(d=>{
        document.getElementById('wifi-ssid').textContent=d.ssid||'--';
        document.getElementById('wifi-ip').textContent=d.ip||'--';
        document.getElementById('wifi-dot').classList.toggle('off',!d.ip||d.ip==='0.0.0.0');
        document.getElementById('wifi-rssi').textContent=d.rssi?d.rssi+'dBm':'';
        document.getElementById('ap-ssid').textContent=d.ap_ssid||'--';
        const apTog=document.getElementById('ap-toggle');
        if(d.ap_mode) apTog.classList.add('on'); else apTog.classList.remove('on');
        // mDNS
        const mdnsEl=document.getElementById('mdns-current');
        if(mdnsEl && d.mdns) mdnsEl.textContent=d.mdns+'.local';
        const mdnsInp=document.getElementById('mdns-hostname');
        if(mdnsInp && d.mdns) mdnsInp.value=d.mdns;
    }).catch(()=>{});
}
function saveMdns(){
    const hostname=document.getElementById('mdns-hostname').value.trim();
    fetch('/api/wifi/mdns',{method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({hostname})})
    .then(r=>r.json()).then(()=>{loadWifiConfig();})
    .catch(()=>{});
}
function wifiConnect(){
    const ssid=document.getElementById('wifi-inp-ssid').value.trim();
    const pass=document.getElementById('wifi-inp-pass').value;
    const ip=document.getElementById('wifi-inp-ip').value.trim();
    if(!ssid) return;
    const btn=document.getElementById('wifi-connect-btn');
    btn.disabled=true;btn.textContent='Baglaniyor...';
    fetch('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:pass,static_ip:ip})})
        .then(r=>r.json()).then(()=>{btn.textContent='Baglan';btn.disabled=false;setTimeout(loadWifiConfig,2000);})
        .catch(()=>{btn.textContent='Baglan';btn.disabled=false;});
}
function wifiBackupSave(){
    const ssid=document.getElementById('wifi-bak-ssid').value.trim();
    const pass=document.getElementById('wifi-bak-pass').value;
    const ip=document.getElementById('wifi-bak-ip').value.trim();
    if(!ssid) return;
    const btn=document.getElementById('wifi-backup-btn');
    btn.disabled=true;btn.textContent='Kaydediliyor...';
    fetch('/api/wifi/backup/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:pass,static_ip:ip})})
        .then(r=>r.json()).then(()=>{btn.textContent='Yedek Agi Kaydet';btn.disabled=false;})
        .catch(()=>{btn.textContent='Yedek Agi Kaydet';btn.disabled=false;});
}
function toggleAP(){
    const tog=document.getElementById('ap-toggle');
    const enable=!tog.classList.contains('on');
    fetch('/api/wifi/ap/set',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({enabled:enable})})
        .then(r=>r.json()).then(()=>{tog.classList.toggle('on',enable);})
        .catch(()=>{});
}

/* ── Subpage State ────────────────────────────────── */
let subpageActive=false;

function updateFooterBtn(){
    const btn=document.getElementById('footerCancelBtn');
    if(btn) btn.textContent=subpageActive?'Geri':'Iptal';
}

/* ── Info & System ────────────────────────────────── */
function showInfoDetail(name){
    document.getElementById('info-main').style.display='none';
    document.getElementById('info-detail-'+name).style.display='block';
    if(name==='sysinfo') loadSystemInfo();
    subpageActive=true;updateFooterBtn();
}
function hideInfoDetail(){
    document.querySelectorAll('.info-detail').forEach(d=>d.style.display='none');
    document.getElementById('info-main').style.display='block';
    subpageActive=false;updateFooterBtn();
}

function updateUsageBar(fillId,textId,used,total,unit){
    const pct=total>0?Math.round((used/total)*100):0;
    const fillEl=document.getElementById(fillId);
    const textEl=document.getElementById(textId);
    if(fillEl) fillEl.style.width=pct+'%';
    if(textEl){
        if(unit==='KB'){
            const freeVal=total-used;
            textEl.textContent=pct+'% kullaniliyor  ('+Math.round(freeVal)+' KB bos / '+Math.round(total)+' KB toplam)';
        } else {
            textEl.textContent=pct+'% kullaniliyor  ('+Math.round(total-used)+' / '+Math.round(total)+' KB)';
        }
    }
}

function loadSystemInfo(){
    fetch('/api/system/detail').then(r=>r.json()).then(d=>{
        document.getElementById('sys-chip').textContent=d.chip||'--';
        document.getElementById('sys-rev').textContent=d.cores?d.cores+' Core, '+d.cpu_mhz+' MHz':'--';
        document.getElementById('sys-flash').textContent=d.flash_kb?(d.flash_kb/1024).toFixed(0)+' MB':'--';
        document.getElementById('sys-mac').textContent=d.mac||'--';

        // RAM usage bar
        if(d.heap_total&&d.heap_free){
            const heapTotalKB=d.heap_total/1024;
            const heapFreeKB=d.heap_free/1024;
            const heapUsedKB=heapTotalKB-heapFreeKB;
            updateUsageBar('ram-fill','ram-text',heapUsedKB,heapTotalKB,'KB');
        }

        // Internal Flash usage bar
        if(d.flash_kb){
            const flashUsed=d.flash_used_kb||0;
            updateUsageBar('flash-fill','flash-text',flashUsed,d.flash_kb,'');
        }

        // Slot A usage bar
        if(d.slot_a_total){
            const slotAUsed=d.slot_a_total-d.slot_a_free;
            updateUsageBar('slot-a-fill','slot-a-text',slotAUsed,d.slot_a_total,'');
        }

        // Slot B usage bar
        if(d.slot_b_total){
            const slotBUsed=d.slot_b_total-d.slot_b_free;
            updateUsageBar('slot-b-fill','slot-b-text',slotBUsed,d.slot_b_total,'');
        }

        // Software
        document.getElementById('sys-fw').textContent=d.fw_version?'v'+d.fw_version:'--';
        document.getElementById('sys-idf').textContent=d.idf_version||'--';
        if(d.uptime!=null){
            const h=Math.floor(d.uptime/3600),m=Math.floor((d.uptime%3600)/60);
            document.getElementById('sys-uptime').textContent=h+'s '+m+'dk';
        }
    }).catch(()=>{});
}

/* ── Dev Detail Navigation ────────────────────────── */
function showDevDetail(name){
    document.getElementById('sistem-main').style.display='none';
    document.getElementById('dev-detail-'+name).style.display='block';
    subpageActive=true;updateFooterBtn();
}
function hideDevDetail(){
    document.querySelectorAll('#dev-detail-fw,#dev-detail-gui').forEach(d=>d.style.display='none');
    document.getElementById('sistem-main').style.display='block';
    subpageActive=false;updateFooterBtn();
}

/* ── Firmware OTA ─────────────────────────────────── */
let fwOtaPoll=null;
function loadFwOtaInfo(){
    fetch('/api/fw-ota/info').then(r=>r.json()).then(d=>{
        document.getElementById('fw-version').textContent=d.current_version?'v'+d.current_version:'--';
        document.getElementById('fw-partition').textContent=d.running_partition||'--';
        document.getElementById('fw-rollback').textContent=d.can_rollback?'v'+d.rollback_version:'Yok';
        if(d.can_rollback) document.getElementById('fw-rollback-btn').style.display='';
        else document.getElementById('fw-rollback-btn').style.display='none';
        // Kart versiyonu
        const cardEl=document.getElementById('fw-version-card');
        if(cardEl) cardEl.textContent=d.current_version?'v'+d.current_version:'--';
    }).catch(()=>{});
}
function fwOtaCheck(){
    const btn=document.getElementById('fw-check-btn');
    btn.disabled=true;btn.textContent='Kontrol ediliyor...';
    fetch('/api/fw-ota/check',{method:'POST'}).then(r=>r.json()).then(()=>{
        // Poll status for result
        setTimeout(()=>{
            fetch('/api/fw-ota/status').then(r=>r.json()).then(s=>{
                btn.textContent='Kontrol Et';btn.disabled=false;
                if(s.state===2){// UPDATE_AVAILABLE
                    document.getElementById('fw-start-btn').style.display='';
                    showOtaProgress('fw',s);
                } else if(s.state===3){// NO_UPDATE
                    showOtaProgress('fw',{progress_pct:100,message:'Guncel surumdesiniz.'});
                }
            });
        },500);
    }).catch(()=>{btn.textContent='Kontrol Et';btn.disabled=false;});
}
function fwOtaStart(){
    document.getElementById('fw-check-btn').disabled=true;
    document.getElementById('fw-start-btn').style.display='none';
    fetch('/api/fw-ota/start',{method:'POST'}).then(()=>{
        fwOtaPoll=setInterval(()=>{
            fetch('/api/fw-ota/status').then(r=>r.json()).then(s=>{
                showOtaProgress('fw',s);
                if(s.state>=5){// DONE or ERROR
                    clearInterval(fwOtaPoll);fwOtaPoll=null;
                    document.getElementById('fw-check-btn').disabled=false;
                    loadFwOtaInfo();
                }
            });
        },600);
    }).catch(()=>{document.getElementById('fw-check-btn').disabled=false;});
}
function fwOtaRollback(){
    if(!confirm('Onceki firmware surumune donulecek. Emin misiniz?')) return;
    fetch('/api/fw-ota/rollback',{method:'POST'}).then(()=>{
        alert('Rollback basarili. Cihaz yeniden baslatilacak.');
    }).catch(()=>{});
}

/* ── GUI OTA ──────────────────────────────────────── */
let guiOtaPoll=null;
function loadGuiOtaInfo(){
    fetch('/api/gui-ota/info').then(r=>r.json()).then(d=>{
        document.getElementById('gui-slot').textContent=d.active_slot||'--';
        document.getElementById('gui-version').textContent=d.active_version?'v'+d.active_version:'--';
        // Kart versiyonu
        const cardEl=document.getElementById('gui-version-card');
        if(cardEl) cardEl.textContent=d.active_version?'v'+d.active_version:'--';
    }).catch(()=>{});
}
function guiOtaStart(){
    document.getElementById('gui-start-btn').disabled=true;
    document.getElementById('gui-start-btn').textContent='Indiriliyor...';
    fetch('/api/gui-ota/start',{method:'POST'}).then(()=>{
        guiOtaPoll=setInterval(()=>{
            fetch('/api/gui-ota/status').then(r=>r.json()).then(s=>{
                showOtaProgress('gui',s);
                if(s.state>=5){// DONE or ERROR
                    clearInterval(guiOtaPoll);guiOtaPoll=null;
                    document.getElementById('gui-start-btn').disabled=false;
                    document.getElementById('gui-start-btn').textContent='Arayuzu Guncelle';
                    loadGuiOtaInfo();
                }
            });
        },600);
    }).catch(()=>{
        document.getElementById('gui-start-btn').disabled=false;
        document.getElementById('gui-start-btn').textContent='Arayuzu Guncelle';
    });
}
function guiOtaRollback(){
    if(!confirm('Onceki arayuz slotuna donulecek. Emin misiniz?')) return;
    fetch('/api/gui-ota/rollback',{method:'POST'}).then(()=>{
        alert('GUI rollback basarili.');loadGuiOtaInfo();
    }).catch(()=>{});
}

/* ── OTA helpers ──────────────────────────────────── */
function showOtaProgress(prefix,s){
    const wrap=document.getElementById(prefix+'-ota-progress');
    wrap.style.display='block';
    document.getElementById(prefix+'-ota-fill').style.width=s.progress_pct+'%';
    document.getElementById(prefix+'-ota-text').textContent=s.message||(''+s.progress_pct+'%');
}

/* ── Device Actions ───────────────────────────────── */
function deviceRestart(){
    if(!confirm('Cihaz yeniden baslatilacak. Emin misiniz?')) return;
    fetch('/api/device/restart',{method:'POST'}).catch(()=>{});
}
function factoryReset(){
    if(!confirm('TUM AYARLAR SILINECEK. Bu islem geri alinamaz. Emin misiniz?')) return;
    fetch('/api/device/factory-reset',{method:'POST'}).catch(()=>{});
}

/* ── SSE (Real-time) ──────────────────────────────── */
let sseSource=null;
function connectSSE(){
    if(sseSource) sseSource.close();
    sseSource=new EventSource('/api/events');
    sseSource.onmessage=function(e){
        try{
            const d=JSON.parse(e.data);
            if(d.type==='state'){
                // Mod senkronizasyonu: encoder mod degistirdiyse GUI'yi guncelle
                if(d.mode && modeConfig.modes.length>0){
                    const matchMode=modeConfig.modes.find(m=>m.type===d.mode);
                    if(matchMode && matchMode.id!==activeMode){
                        activateMode(matchMode.id);
                    }
                }
                // Update value display from server
                if(!activeMode) return;
                const mode=modeConfig.modes.find(m=>m.id===activeMode);
                if(mode&&mode.type!=='safe'){
                    document.getElementById('big-val').textContent=d.value;
                }
                // Update status line
                const sl=document.getElementById('status-line');
                if(mode&&mode.type!=='safe'){
                    const stateText=d.state==='on'?'<span class="on">On</span>':'Off';
                    const connText=d.connected!==false?'Connected':'Disconnected';
                    sl.innerHTML=stateText+' &middot; '+connText;
                }
            }
        }catch(err){}
    };
    sseSource.onerror=function(){
        sseSource.close();sseSource=null;
        // Reconnect after 5s
        setTimeout(connectSSE,5000);
    };
}

/* ── Language ─────────────────────────────────────── */
let currentLang=localStorage.getItem('syndimm-lang')||'tr';
function setLanguage(lang){
    currentLang=lang;
    localStorage.setItem('syndimm-lang',lang);
    // Ceviriler sonra eklenecek - simdilik sadece secimi kaydet
}
function initLang(){
    const sel=document.getElementById('lang-select');
    if(sel) sel.value=currentLang;
}

/* ── Footer Buttons ──────────────────────────────── */
function footerCancel(){
    if(subpageActive){
        const sistemPanel=document.querySelector('.tab-panel[data-tab="sistem"]');
        const infoPanel=document.querySelector('.tab-panel[data-tab="info"]');
        if(sistemPanel&&sistemPanel.classList.contains('active')){
            // Sistem tab: wifi/backup/ap veya fw/gui detay açık olabilir
            hideSistemDetail();hideDevDetail();
        } else if(infoPanel&&infoPanel.classList.contains('active')){
            hideInfoDetail();
        }
    } else {
        toggleSettings();
    }
}
function footerSave(){
    // Tum kaydet islemleri tek seferde
    // WiFi ayarlari kullanici "Baglan" veya "Kaydet" butonuna basarak zaten kaydediliyor
    // Burada genel ayarlari (tema, dil vb) kaydedip modalı kapat
    toggleSettings();
}

/* ── Helpers ──────────────────────────────────────── */
function esc(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');}

/* ── Init ─────────────────────────────────────────── */
initTheme();
loadModeConfig();
loadWifiConfig();
loadFwOtaInfo();
loadGuiOtaInfo();
connectSSE();
initLang();

// Dev kart click listener'lari
document.getElementById('dev-card-fw').addEventListener('click',function(){showDevDetail('fw')});
document.getElementById('dev-card-gui').addEventListener('click',function(){showDevDetail('gui')});
