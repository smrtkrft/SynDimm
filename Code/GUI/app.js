/* ── i18n ─────────────────────────────────────────── */
/* Ceviriler /lang/<kod>.json dosyalarindan yuklenir (Slot A'da, firmware ile gelir).
 * data-i18n (textContent), data-i18n-html (innerHTML), data-i18n-ph (placeholder),
 * data-i18n-title (title). t(key, vars) dinamik metinler icin. */
const LANGS=['tr','en','de','es','fr','it'];
let currentLang=localStorage.getItem('syndimm-lang')||'tr';
if(LANGS.indexOf(currentLang)<0) currentLang='tr';
let I18N={};
function t(key,vars){
    let s=I18N[key];
    if(s==null) s=key;
    if(vars) for(const k in vars) s=s.split('{'+k+'}').join(vars[k]);
    return s;
}
function applyI18n(){
    document.documentElement.lang=currentLang;
    document.querySelectorAll('[data-i18n]').forEach(el=>{const k=el.getAttribute('data-i18n');if(I18N[k]!=null) el.textContent=I18N[k];});
    document.querySelectorAll('[data-i18n-html]').forEach(el=>{const k=el.getAttribute('data-i18n-html');if(I18N[k]!=null) el.innerHTML=I18N[k];});
    document.querySelectorAll('[data-i18n-ph]').forEach(el=>{const k=el.getAttribute('data-i18n-ph');if(I18N[k]!=null) el.placeholder=I18N[k];});
    document.querySelectorAll('[data-i18n-title]').forEach(el=>{const k=el.getAttribute('data-i18n-title');if(I18N[k]!=null) el.title=I18N[k];});
    updateFooterBtn();
    if(typeof logData!=='undefined' && logData) renderLogs();
}
function loadLanguage(lang){
    return fetch('/lang/'+lang+'.json').then(r=>r.json()).then(d=>{I18N=d;applyI18n();}).catch(()=>{});
}
function setLanguage(lang){
    if(LANGS.indexOf(lang)<0) return;
    currentLang=lang;
    localStorage.setItem('syndimm-lang',lang);
    loadLanguage(lang);
}
function initLang(){
    const sel=document.getElementById('lang-select');
    if(sel) sel.value=currentLang;
    return loadLanguage(currentLang);
}

/* ── Auth (cihaz parolasi) ───────────────────────── */
/* Tum fetch'ler sarilir: 401 gelirse giris ekrani acilir. Oturum HttpOnly cerezde,
 * JS'in token tutmasi gerekmez. */
let authState={enabled:false,authed:true};
(function(){
    const _fetch=window.fetch.bind(window);
    window.fetch=function(url,opts){
        return _fetch(url,opts).then(r=>{
            if(r.status===401 && String(url).indexOf('/api/')===0) showLogin();
            return r;
        });
    };
})();
function showLogin(){
    const el=document.getElementById('login');
    if(el.classList.contains('open')) return;
    el.classList.add('open');
    setTimeout(()=>{const i=document.getElementById('login-pass');if(i){i.value='';i.focus();}},50);
}
function doLogin(){
    const pass=document.getElementById('login-pass').value;
    const btn=document.getElementById('login-btn');
    const err=document.getElementById('login-err');
    if(!pass) return;
    btn.disabled=true;err.textContent='';
    fetch('/api/auth/login',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({password:pass})})
    .then(r=>r.json().then(d=>({ok:r.ok,d})))
    .then(({ok,d})=>{
        btn.disabled=false;
        if(ok && d.status==='ok'){ location.reload(); }
        else { err.textContent=(d&&d.message)||t('auth.wrong'); }
    }).catch(()=>{btn.disabled=false;err.textContent=t('app.conn_error');});
}
document.getElementById('login-pass').addEventListener('keydown',e=>{if(e.key==='Enter') doLogin();});
function loadAuthStatus(){
    fetch('/api/auth/status').then(r=>r.json()).then(d=>{
        authState=d;
        const sub=document.getElementById('auth-card-sub');
        if(sub) sub.textContent=d.enabled?t('auth.set'):t('auth.not_set');
        const st=document.getElementById('auth-state');
        if(st) st.textContent=d.enabled?t('auth.protected'):t('auth.open');
        const cw=document.getElementById('auth-cur-wrap');
        if(cw) cw.style.display=d.enabled?'':'none';
        const rb=document.getElementById('auth-remove-btn');
        if(rb) rb.style.display=d.enabled?'':'none';
        const lb=document.getElementById('auth-logout-btn');
        if(lb) lb.style.display=d.enabled?'':'none';
        if(d.enabled && !d.authed) showLogin();
    }).catch(()=>{});
}
function setPassword(remove){
    const cur=document.getElementById('auth-cur').value;
    const p1=document.getElementById('auth-new').value;
    const p2=document.getElementById('auth-new2').value;
    const msg=document.getElementById('auth-msg');
    msg.textContent='';
    if(remove){
        if(!confirm(t('auth.remove_confirm'))) return;
    } else {
        if(p1.length<4||p1.length>64){msg.textContent=t('auth.len_error');return;}
        if(p1!==p2){msg.textContent=t('auth.mismatch');return;}
    }
    fetch('/api/auth/set',{method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({current:cur,password:remove?'':p1})})
    .then(r=>r.json()).then(d=>{
        if(d.status==='ok'){
            msg.textContent=remove?t('auth.removed'):t('auth.saved');
            ['auth-cur','auth-new','auth-new2'].forEach(id=>document.getElementById(id).value='');
            loadAuthStatus();
        } else msg.textContent=d.message||t('app.error');
    }).catch(()=>{msg.textContent=t('app.conn_error');});
}
function doLogout(){
    fetch('/api/auth/logout',{method:'POST'}).then(()=>location.reload()).catch(()=>{});
}

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
            // Acilista donanimin mevcut modunu sec; yoksa ilk mod (donanima yazmadan)
            fetch('/api/mode').then(r=>r.json()).then(h=>{
                const hw=(h&&h.mode)?cfg.modes.find(m=>m.type===h.mode):null;
                activateMode(hw?hw.id:cfg.modes[0].id,false);
            }).catch(()=>activateMode(cfg.modes[0].id,false));
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
            btn.onclick=()=>activateMode(m.id,true);
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

/* syncHw=true: kullanici secti, donanima bildir. false: sayfa acilisi / SSE'den gelen
 * senkron — donanima POST edilmez. Eskiden her acilista modes[0] donanima yazilip
 * encoder'la secilmis mod ve aktif profil sifirlaniyordu. */
function activateMode(modeId,syncHw){
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
    // GUI → HW mod senkronizasyonu (sadece kullanici secimi)
    if(syncHw){
        fetch('/api/mode',{method:'POST',headers:{'Content-Type':'application/json'},
            body:JSON.stringify({mode:mode.type})}).catch(()=>{});
    }
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
        // Mod id'si HTML/JS string'ine gomulmez (XSS): data-act + data-id ile delegasyon
        card.innerHTML=
            '<div class="mode-card-order">'+
                '<button '+(isFirst?'disabled':'')+' data-act="up">'+
                    '<svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="M18 15l-6-6-6 6"/></svg>'+
                '</button>'+
                '<span class="mode-card-num">'+(i+1)+'</span>'+
                '<button '+(isLast?'disabled':'')+' data-act="down">'+
                    '<svg width="10" height="10" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="3"><path d="M6 9l6 6 6-6"/></svg>'+
                '</button>'+
            '</div>'+
            '<div class="mode-card-info">'+
                '<strong>'+esc(m.name)+'</strong>'+
                '<span class="mode-badge">'+esc(m.type)+'</span>'+
            '</div>'+
            '<div class="mode-card-actions">'+
                '<button title="'+esc(t('modes.edit'))+'" data-act="rename">'+
                    '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M11 4H4a2 2 0 00-2 2v14a2 2 0 002 2h14a2 2 0 002-2v-7"/><path d="M18.5 2.5a2.121 2.121 0 013 3L12 15l-4 1 1-4 9.5-9.5z"/></svg>'+
                '</button>'+
                '<button class="del" title="'+esc(t('modes.delete'))+'" data-act="delete">'+
                    '<svg width="14" height="14" viewBox="0 0 24 24" fill="none" stroke="currentColor" stroke-width="2"><path d="M3 6h18M19 6v14a2 2 0 01-2 2H7a2 2 0 01-2-2V6m3 0V4a2 2 0 012-2h4a2 2 0 012 2v2"/></svg>'+
                '</button>'+
            '</div>';
        card.dataset.id=m.id;
        list.appendChild(card);
    });
}
document.getElementById('mode-list').addEventListener('click',function(e){
    const btn=e.target.closest('button[data-act]');
    if(!btn||btn.disabled) return;
    const card=btn.closest('.mode-card');
    if(!card) return;
    const id=card.dataset.id;
    switch(btn.dataset.act){
        case 'up': moveMode(id,-1); break;
        case 'down': moveMode(id,1); break;
        case 'rename': renameMode(id); break;
        case 'delete': deleteMode(id); break;
    }
});

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
    modeConfig.modes.push({id,name,type,params});
    hideEmptyState();
    saveModeConfig();
    activateMode(id,true);
    hideAddMode();
}

function deleteMode(id){
    if(!confirm(t('modes.delete_confirm'))) return;
    modeConfig.modes=modeConfig.modes.filter(m=>m.id!==id);
    saveModeConfig();
    if(modeConfig.modes.length===0){
        showEmptyState();
    } else if(activeMode===id){
        activateMode(modeConfig.modes[0].id,true);
    }
}

function renameMode(id){
    const mode=modeConfig.modes.find(m=>m.id===id);
    if(!mode) return;
    const newName=prompt(t('modes.rename_prompt'),mode.name);
    if(!newName||!newName.trim()) return;
    mode.name=newName.trim();
    saveModeConfig();
}

function toggleMultiMode(){
    modeConfig.multiMode=!modeConfig.multiMode;
    document.getElementById('multi-mode-toggle').classList.toggle('on',modeConfig.multiMode);
    if(!modeConfig.multiMode && modeConfig.modes.length>0){
        activateMode(modeConfig.modes[0].id,true);
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
        // TX gucu / guc yonetimi
        const txNow=document.getElementById('tx-now');
        if(txNow) txNow.textContent=(d.tx_now!=null)?d.tx_now+' dBm':'--';
        const txMax=document.getElementById('tx-max');
        if(txMax && d.tx_max!=null) txMax.value=String(d.tx_max);
        const txAd=document.getElementById('tx-adapt');
        if(txAd) txAd.classList.toggle('on',!!d.tx_adapt);
        const apCl=document.getElementById('ap-clients');
        if(apCl) apCl.textContent=(d.ap_clients!=null)?d.ap_clients:'--';
    }).catch(()=>{});
}
function saveTxPower(){
    const max=parseInt(document.getElementById('tx-max').value,10);
    const adaptive=document.getElementById('tx-adapt').classList.contains('on');
    fetch('/api/wifi/txpower',{method:'POST',headers:{'Content-Type':'application/json'},
        body:JSON.stringify({max,adaptive})})
    .then(r=>r.json()).then(()=>{setTimeout(loadWifiConfig,3000);})
    .catch(()=>{});
}
function toggleTxAdapt(){
    const tog=document.getElementById('tx-adapt');
    tog.classList.toggle('on');
    saveTxPower();
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
    btn.disabled=true;btn.textContent=t('wifi.connecting');
    fetch('/api/wifi/connect',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:pass,static_ip:ip})})
        .then(r=>r.json()).then(()=>{btn.textContent=t('wifi.connect');btn.disabled=false;setTimeout(loadWifiConfig,2000);})
        .catch(()=>{btn.textContent=t('wifi.connect');btn.disabled=false;});
}
function wifiBackupSave(){
    const ssid=document.getElementById('wifi-bak-ssid').value.trim();
    const pass=document.getElementById('wifi-bak-pass').value;
    const ip=document.getElementById('wifi-bak-ip').value.trim();
    if(!ssid) return;
    const btn=document.getElementById('wifi-backup-btn');
    btn.disabled=true;btn.textContent=t('wifi.saving');
    fetch('/api/wifi/backup/save',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify({ssid,password:pass,static_ip:ip})})
        .then(r=>r.json()).then(()=>{btn.textContent=t('wifi.backup_save');btn.disabled=false;})
        .catch(()=>{btn.textContent=t('wifi.backup_save');btn.disabled=false;});
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
    if(btn) btn.textContent=subpageActive?t('app.back'):t('app.cancel');
}

/* ── Info & System ────────────────────────────────── */
function showInfoDetail(name){
    document.getElementById('info-main').style.display='none';
    document.getElementById('info-detail-'+name).style.display='block';
    if(name==='sysinfo') loadSystemInfo();
    if(name==='logs') loadLogs();
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
        textEl.textContent=t('sysinfo.used',{pct:pct,free:Math.round(total-used),total:Math.round(total)});
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
            document.getElementById('sys-uptime').textContent=fmtDur(d.uptime);
        }
        const boots=document.getElementById('sys-boots');
        if(boots) boots.textContent=(d.boot_count!=null)?('#'+d.boot_count+(d.short_boots?' ('+t('sysinfo.short_boots',{n:d.short_boots})+')':'')):'--';
    }).catch(()=>{});
}

/* ── Olay Kayitlari (sys_log) ─────────────────────── */
let logData=null, logFilter='all';
function fmtDur(sec){
    sec=Math.max(0,sec|0);
    const d=Math.floor(sec/86400),h=Math.floor(sec%86400/3600),m=Math.floor(sec%3600/60),s=sec%60;
    const D=t('log.dur_d'),H=t('log.dur_h'),M=t('log.dur_m'),S=t('log.dur_s');
    if(d>0) return d+D+' '+h+H+' '+m+M;
    if(h>0) return h+H+' '+m+M;
    if(m>0) return m+M+' '+s+S;
    return s+S;
}
/* Firmware'den gelen sebep/olay kodlari yerel dile cevrilir; cevirisi yoksa cihaz metni */
function causeText(e){const k='cause.'+e.cause;return I18N[k]!=null?I18N[k]:(e.msg||e.cause||'');}
function hintText(e){
    if(!e.hint) return '';
    if(e.cause==='brownout') return t(/akim yetersiz/i.test(e.hint)?'hint.brownout_load':'hint.brownout_other');
    if(e.cause==='power_loss') return t('hint.power_loss');
    if(e.cause==='task_wdt') return t('hint.task_wdt');
    return e.hint;
}
function fmtTs(ts){
    if(!ts) return '';
    const dt=new Date(ts*1000);
    const p=n=>String(n).padStart(2,'0');
    return p(dt.getDate())+'.'+p(dt.getMonth()+1)+'.'+dt.getFullYear()+' '+p(dt.getHours())+':'+p(dt.getMinutes());
}
function loadLogs(){
    const list=document.getElementById('log-list');
    fetch('/api/system/logs').then(r=>r.json()).then(d=>{
        logData=d;
        const sum=document.getElementById('log-summary');
        if(sum){
            let txt=esc(t('log.this_boot'))+': <strong>#'+d.boot+'</strong> &middot; '+esc(t('log.running'))+' '+esc(fmtDur(d.uptime));
            if(d.short) txt+=' &middot; '+esc(t('log.short_skipped',{n:d.short}));
            txt+=' &middot; '+esc(d.epoch?t('log.time_synced'):t('log.time_none'));
            sum.innerHTML=txt;
        }
        renderLogs();
    }).catch(()=>{list.innerHTML='<div class="log-empty">'+esc(t('log.fetch_error'))+'</div>';});
}
function setLogFilter(f,btn){
    logFilter=f;
    document.querySelectorAll('.log-filter').forEach(b=>b.classList.remove('active'));
    btn.classList.add('active');
    renderLogs();
}
function renderLogs(){
    const list=document.getElementById('log-list');
    if(!logData||!logData.entries){list.innerHTML='<div class="log-empty">'+esc(t('log.empty'))+'</div>';return;}
    const items=logData.entries.filter(e=>{
        if(logFilter==='all') return true;
        if(logFilter==='boot') return e.type==='boot';
        if(logFilter==='power') return e.cat==='power';
        if(logFilter==='wifi') return e.cat==='wifi';
        return true;
    });
    if(items.length===0){list.innerHTML='<div class="log-empty">'+esc(t('log.empty_filter'))+'</div>';return;}
    list.innerHTML=items.map(e=>{
        if(e.type==='boot'){
            const cat=e.cat||'unknown';
            const chips=[];
            chips.push(t('log.run',{d:(e.approx?'~':'')+fmtDur(e.up)}));
            if(e.ev&&e.ev!=='NONE') chips.push(t('log.last_event',{e:t('ev.'+e.ev)})+(e.ev_dt>=0?' ('+t('log.ago',{d:fmtDur(e.ev_dt)})+')':''));
            if(e.wifi&&e.wifi!=='?') chips.push('wifi '+e.wifi+(e.cl?' / '+t('log.clients',{n:e.cl}):''));
            if(e.rssi) chips.push('rssi '+e.rssi);
            if(e.tx) chips.push('tx '+e.tx+' dBm');
            if(e.heap_min) chips.push(t('log.heap_min',{n:Math.round(e.heap_min/1024)}));
            if(e.short) chips.push(t('log.short_boots',{n:e.short}));
            const hint=hintText(e);
            return '<div class="log-item cat-'+cat+'">'+
                '<div class="log-head"><span class="log-title">'+esc(t('cat.'+cat))+' &middot; '+esc(t('log.boot_end',{n:e.n}))+'</span>'+
                '<span class="log-time">'+esc(fmtTs(e.ts)||t('log.boot_ref',{n:e.n}))+'</span></div>'+
                '<div class="log-msg">'+esc(causeText(e))+'</div>'+
                (hint?'<div class="log-hint">'+esc(hint)+'</div>':'')+
                '<div class="log-chips">'+chips.map(c=>'<span class="log-chip">'+esc(c)+'</span>').join('')+'</div>'+
                '</div>';
        }
        const lvl=e.lvl||'INFO';
        return '<div class="log-item cat-'+(e.cat||'sys')+' lvl-'+lvl+'">'+
            '<div class="log-head"><span class="log-title">'+esc(I18N['cat.'+(e.cat||'sys')]||(e.cat||'sys').toUpperCase())+' &middot; '+esc(lvl)+'</span>'+
            '<span class="log-time">'+esc(fmtTs(e.ts)||('#'+e.n+' +'+fmtDur(e.up)))+'</span></div>'+
            '<div class="log-msg">'+esc(e.msg||'')+'</div>'+
            '</div>';
    }).join('');
}
function clearLogs(){
    if(!confirm(t('log.clear_confirm'))) return;
    fetch('/api/system/logs',{method:'DELETE'}).then(()=>loadLogs()).catch(()=>{});
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
        document.getElementById('fw-rollback').textContent=d.can_rollback?'v'+d.rollback_version:t('app.none');
        if(d.can_rollback) document.getElementById('fw-rollback-btn').style.display='';
        else document.getElementById('fw-rollback-btn').style.display='none';
        // Kart versiyonu
        const cardEl=document.getElementById('fw-version-card');
        if(cardEl) cardEl.textContent=d.current_version?'v'+d.current_version:'--';
    }).catch(()=>{});
}
function fwOtaCheck(){
    const btn=document.getElementById('fw-check-btn');
    btn.disabled=true;btn.textContent=t('ota.checking');
    fetch('/api/fw-ota/check',{method:'POST'}).then(r=>r.json()).then(()=>{
        // GitHub TLS istegi birkac saniye surer: CHECKING (1) bitene kadar poll et
        let tries=0;
        const poll=()=>{
            fetch('/api/fw-ota/status').then(r=>r.json()).then(s=>{
                if(s.state===1 && tries++<30){setTimeout(poll,700);return;}
                btn.textContent=t('ota.check');btn.disabled=false;
                if(s.state===2){// UPDATE_AVAILABLE
                    document.getElementById('fw-start-btn').style.display='';
                    showOtaProgress('fw',s);
                } else if(s.state===3){// NO_UPDATE
                    showOtaProgress('fw',{progress_pct:100,message:t('ota.up_to_date')});
                } else {
                    showOtaProgress('fw',{progress_pct:0,message:s.message||t('ota.check_failed')});
                }
            }).catch(()=>{btn.textContent=t('ota.check');btn.disabled=false;});
        };
        setTimeout(poll,700);
    }).catch(()=>{btn.textContent=t('ota.check');btn.disabled=false;});
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
    if(!confirm(t('ota.rollback_confirm'))) return;
    fetch('/api/fw-ota/rollback',{method:'POST'}).then(r=>r.json()).then(d=>{
        if(d&&d.status==='ok'){
            alert(t('ota.rollback_ok'));
            fetch('/api/device/restart',{method:'POST'}).catch(()=>{});
        } else {
            alert(t('ota.rollback_fail')+': '+((d&&d.message)||''));
        }
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
    document.getElementById('gui-start-btn').textContent=t('ota.downloading');
    fetch('/api/gui-ota/start',{method:'POST'}).then(()=>{
        guiOtaPoll=setInterval(()=>{
            fetch('/api/gui-ota/status').then(r=>r.json()).then(s=>{
                showOtaProgress('gui',s);
                if(s.state>=5){// DONE or ERROR
                    clearInterval(guiOtaPoll);guiOtaPoll=null;
                    document.getElementById('gui-start-btn').disabled=false;
                    document.getElementById('gui-start-btn').textContent=t('ota.gui_update');
                    loadGuiOtaInfo();
                }
            });
        },600);
    }).catch(()=>{
        document.getElementById('gui-start-btn').disabled=false;
        document.getElementById('gui-start-btn').textContent=t('ota.gui_update');
    });
}
function guiOtaRollback(){
    if(!confirm(t('ota.gui_rollback_confirm'))) return;
    fetch('/api/gui-ota/rollback',{method:'POST'}).then(()=>{
        alert(t('ota.gui_rollback_ok'));loadGuiOtaInfo();
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
    if(!confirm(t('sys.restart_confirm'))) return;
    fetch('/api/device/restart',{method:'POST'}).catch(()=>{});
}
function factoryReset(){
    if(!confirm(t('sys.factory_confirm'))) return;
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
                        activateMode(matchMode.id,false);
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
                    const stateText=d.state==='on'?'<span class="on">'+esc(t('app.on'))+'</span>':esc(t('app.off'));
                    const connText=d.connected!==false?esc(t('app.connected')):esc(t('app.disconnected'));
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
function esc(s){return String(s==null?'':s).replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;').replace(/"/g,'&quot;');}

/* ── Init ─────────────────────────────────────────── */
initTheme();
initLang().then(()=>{
    loadAuthStatus();
    loadModeConfig();
    loadWifiConfig();
    loadFwOtaInfo();
    loadGuiOtaInfo();
    connectSSE();
});

// Dev kart click listener'lari
document.getElementById('dev-card-fw').addEventListener('click',function(){showDevDetail('fw')});
document.getElementById('dev-card-gui').addEventListener('click',function(){showDevDetail('gui')});
