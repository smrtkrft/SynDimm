# SynDimm neue — Mimari Plan

> Durum: ONAYLANDI (2026-07-02) — implementasyon fazları en altta.
> Eski kod (`Code/HW`, `Code/GUI`) referans olarak duruyor, silinmeyecek.

## 0. Karar Kaydı

| Karar | Sonuç |
|---|---|
| Davranış dağıtımı | **Native sürücü + JSON veri.** Yeni mantık = firmware OTA; mod atamaları/profiller = çalışma zamanında JSON. Script motoru YOK (kapı açık: ileride `sd_behavior_t` arkasına yorumlanan sürücü eklenebilir). |
| Transportlar | **USB + BLE + TCP** (BlockingFocus ile aynı, sk_core transportları). |
| Mod sayısı | **Sabit 3 slot** (buzzer 1-2-3 bip). |
| Protokoller | **HTTP + UDP + MQTT üçü de V1'de** (eski proto_*.c taşınır). |
| Donanım | PCB revizyonu sadece 32MB harici flash'ın çıkarılması. **LED yok**, buzzer kalır. **Sabit güç** (şebeke/adaptör) — BLE+WiFi sürekli açık kalabilir. |
| Arayüz | Cihazda GUI yok. SKAPP (cli_contract.md) + bağımsız CLI. web_server/ext_flash/gui_ota/GUI klasörü ölü. |
| Seçimlik özellik ilkesi | **Her seçimlik özellik CLI ve GUI'den açılıp kapanabilir** (`prefs.*`), istemeyen kullanıcı kapatır. |

V1 seçimlik özellikleri: jest sözlüğü + preset'ler, MQTT uzaktan-kumanda sürücüsü, sessiz saatler.
V1 yapısal özellikleri: recovery boot, config export/import, mode.test (test ateşleme), gönderim birleştirme (coalescing), çevrimdışı dayanıklılık.
V1'e girmeyen (aday havuzu): çocuk kilidi, çoklu-hedef grubu (şemada `targets[]` gün 1'de var, motor desteği sonra), RGB LED.

## 1. Büyük Resim

```
                        ┌─────────────────────────────────────────┐
                        │                 SKAPP                   │
                        │  (device.manifest → ekranlar, komutlar) │
                        └───────┬──────────────┬──────────────────┘
                            BLE │          TCP │        USB (insan CLI)
                        ┌───────┴──────────────┴──────────┬───────┐
                        │        sk_core (paylaşılan)      │       │
                        │  identity/cli/event_bus/auth/    │       │
                        │  wifi/mdns/ota/capabilities      │       │
                        ├──────────────────────────────────┴───────┤
                        │              sd_mode_engine               │
                        │   3 slot × binding{behavior, profile,     │
                        │   target, params} + coalescing + offline  │
                        ├───────────────┬───────────────────────────┤
                        │ sd_behaviors  │  dimmer │ shutter │ safe  │
                        │  (registry)   │  mqtt_remote │ (gelecek…) │
                        ├───────────────┴───────────────────────────┤
                        │ sd_proto: http │ udp │ mqtt (şablon motoru)│
                        └───────────────────────────────────────────┘
  giriş:  sd_encoder (jest algılama) ──event──▶ mode_engine
  çıkış:  sd_buzzer (sessiz saatler) ◀──event── her katman
```

Temel ilkeler:
1. **Tek kontrol düzlemi:** her özellik bir `sk_cli` komutudur. USB'de insan yazar,
   SKAPP makine modunda aynı handler'ı çağırır. GUI-CLI uyumu yapısaldır, elle korunmaz.
2. **Davranışlar kum havuzunda:** sürücüler donanıma ve ağa doğrudan DOKUNMAZ.
   Girdi event olarak gelir, ağ `sd_proto`/`sk_api` servisinden istenir,
   geri bildirim `sd_feedback`'ten. Sürücü çökerse motor slotu devre dışı bırakır,
   cihaz erişilebilir kalır.
3. **Ekleme, değiştirme değil:** yeni davranış = registry'ye yeni kayıt (OTA ile).
   Yeni hedef cihaz = yeni profil JSON (OTA'sız). Manifest runtime üretilir,
   eski SKAPP bilmediği alanı yok sayar (forward-compat, contract §5).

## 2. Bileşenler (`neue/components/`)

| Bileşen | Görev | Kaynak |
|---|---|---|
| `sk_core`, `sk_api` | temel kütüphane (identity, cli, event bus, auth, wifi, mdns, ota, webhooks) | BlockingFocus'tan paylaşım — bkz. §8 |
| `sd_pins` | pin haritası: ENC CLK=19 DT=20 SW=18, BUZZER=17 (PCB rev B'de doğrula) | yeni (bf_pins deseni) |
| `sd_encoder` | quadrature + buton + **jest algılama** (tek/çift tık, uzun basış, basılı+çevir) → event bus'a semantik olaylar | eski `encoder.c` port + jest katmanı |
| `sd_buzzer` | bip desenleri + **sessiz saatler** + global aç/kapa | eski `buzzer.c` port |
| `sd_feedback` | sistem olayı → bip deseni eşlemesi (mod=1/2/3 dit, hata, kilit, onay) | yeni, küçük |
| `sd_proto` | HTTP/UDP/MQTT yürütücüleri + şablon motoru (`{value}` `{toggle}` `{auth_key}` `{device_id}`, min/max eşleme) | eski `proto_*.c` port |
| `sd_profiles` | profil deposu (NVS blob), şema v2 doğrulama, CRUD | eski profil JSON'ları şema v2'ye taşınır |
| `sd_mode_engine` | slot/bağlama yönetimi, aktif mod, event yönlendirme, coalescing, offline önbellek, test ateşleme, durum yayını | yeni (mantık `device_driver.c`'den derlenir) |
| `sd_behaviors` | sürücü registry + yerleşik sürücüler: `dimmer`, `shutter`, `safe`, `mqtt_remote` | yeni; safe dizi-eşleyici `safe_lock.c`'den port |
| `main` | `sk_core_init({prefix:"SD", hw_rev:'B', ...})` + bileşen dizilimi + recovery boot denetimi | yeni |

Ölen eski bileşenler: `web_server`, `ext_flash`, `flash_sync`, `gui_ota`, `fw_ota` (→`sk_ota`),
`wifi_manager` (→`sk_wifi`), `system_info` (→`device.info`), `GUI/` (→SKAPP).

## 3. Davranış Sürücüsü Sözleşmesi (`sd_behavior_t`, API v1)

```c
// sd_behaviors/include/sd_behavior.h — SABİT SÖZLEŞME, versiyonlu.
#define SD_BEHAVIOR_API_V1  1

typedef struct sd_behavior_ctx sd_behavior_ctx_t;   // motorun sürücüye verdiği hizmetler:
// ctx üzerinden: bağlama config'i (cJSON), sd_proto çağrıları, sk_api tetikleme,
// sd_feedback istekleri, durum-değişti bildirimi (event yayını motor yapar).

typedef struct {
    const char *id;              // "dimmer" — bağlama JSON'undaki behavior alanı
    uint16_t    api_version;     // SD_BEHAVIOR_API_V1
    esp_err_t (*init)   (sd_behavior_ctx_t *ctx);          // bağlama yüklenince
    void      (*deinit) (sd_behavior_ctx_t *ctx);          // slot boşaltılınca
    esp_err_t (*on_input)(sd_behavior_ctx_t *ctx,
                          const sd_input_event_t *evt);    // ROTATE(±n)/CLICK/DBL/LONG
    esp_err_t (*get_state)(sd_behavior_ctx_t *ctx,
                           char *json, size_t cap);        // {"value":72,"state":true,...}
    esp_err_t (*test)   (sd_behavior_ctx_t *ctx);          // mode.test — deneme ateşi (NULL olabilir)
    size_t    (*manifest)(char *json, size_t cap);         // SKAPP ekran/komut parçası
} sd_behavior_t;

esp_err_t sd_behavior_register(const sd_behavior_t *drv);  // boot'ta statik kayıt
```

Kurallar:
- Sürücü kendi task'ını açmaz; motor tek task'tan çağırır (yarış yok, determinizm).
- `on_input` bloklamaz; ağ işleri motorun coalescing kuyruğuna gider.
- Hata dönen sürücü çağrısı N kez üst üste olursa motor slotu `error` durumuna alır,
  event yayınlar (`mode.error`), diğer slotlar ve transportlar çalışmaya devam eder.
- Yeni sürücü eklemek = bu struct'ı doldurup register etmek. Mevcut koda dokunulmaz.

## 4. Veri Şemaları (NVS)

### 4.1 Bağlama (slot başına 1 kayıt, ns=`sd_mode`, key=`slot1..3`)
```json
{
  "v": 2,
  "behavior": "dimmer",
  "enabled": true,
  "name": "Salon",
  "profile": "shelly_dimmer2",
  "targets": [{"host":"192.168.1.40","port":80,"device_id":"","auth_key":""}],
  "params": {
    "step": 1, "accel": true,
    "presets": {"double_click": 100, "long_press": 10},
    "gestures_enabled": true
  }
}
```
`targets[]` dizidir (çoklu-hedef V1'de tek elemanla sınırlı — motor `targets[0]` kullanır,
şema hazır). Slot 3 örneği: `{"behavior":"dimmer","enabled":false,"name":"Rezerve"}`.

### 4.2 Profil (ns=`sd_profile`, key=id) — eski şablon şemasının v2'si
Eski alanlar korunur (`commands.set_value/toggle/status`, map_min/max, protocol),
eklenenler: `"v":2`, `"behaviors":["dimmer"]` (hangi sürücülerle uyumlu).
Katalog dağıtımı: profil kataloğu GitHub'da/SKAPP içinde yaşar, SKAPP seçilen profili
cihaza `profile.add` ile basar. Cihaz yalnızca atanmış profilleri saklar (NVS bütçesi).

### 4.3 Safe yapılandırması (ns=`sd_safe`, 5 kayıt)
```json
{"v":2, "enabled":true, "sequence":"L3-R5-L2-B", "endpoint":"kapi_kilidi",
 "lockout":{"enabled":true,"after":5,"seconds":30}}
```
`endpoint` bir **sk_api USER slotu** adıdır — eski safe'in kendi API-config'i yerine
sk_api'nin hazır deposu/auth tipleri/async gönderimi kullanılır (kod tekrarı ölür).
Kısıt değişikliği: URL ≤191, body ≤768 bayt (eski "sınırsız" harici flash'la gitti — kabul).

### 4.4 Tercihler (ns=`sd_prefs`) — SEÇİMLİK ÖZELLİK ANAHTARLARI
```
gestures=on|off          çift tık/uzun basış jestleri (varsayılan: on)
quiet_hours=off|"23:00-07:00"   buzzer sessiz aralığı (varsayılan: off)
buzzer=on|off            tüm sesler (varsayılan: on)
```
Her anahtar: CLI `prefs.set <key> <val>` + SKAPP'te toggle_list ekranı (BF deseni).
Yeni seçimlik özellik eklerken kural: prefs anahtarı olmadan gemiye binemez.

## 5. Komut Ağacı (cihaza özgü — sk_baseline zorunluları hariç)

```
mode.list | mode.get <slot> | mode.select <slot>
mode.set <slot> {binding-json}        (critical: hayır)
mode.clear <slot>                     (critical: evet — confirm token)
mode.test <slot>                      hedefe deneme komutu, sonucu envelope'ta döner
mode.value <slot> <0-100|toggle>      GUI slider/butonların çağırdığı komut
profile.list | profile.get <id> | profile.add {json} | profile.remove <id>
safe.list | safe.set <n> {json} | safe.clear <n>     (requires_auth: evet)
prefs.list | prefs.set <key> <val>
```
Hazır gelenler: `device.*` (info/commands/status/manifest/restart/factory-reset/capabilities),
`wifi.*`, `logs.get`, `ota.fw.*`, `time.set` (sk_baseline) ve `api.*` (sk_api endpoint editörü).
Factory-reset: her sd_* bileşeni kendi silme kancasını kaydeder (sk_api deseni).

## 6. Olaylar (event bus → SKAPP `events[]`)

```
mode.changed {slot}            mode.value {slot,value,state}    mode.error {slot,err}
target.offline {slot}          target.online {slot}
safe.triggered {n,ok}          safe.lockout {n,seconds}
input.gesture {type}           (mqtt_remote + tanılama için)
api.sent {name,ok,status}      (sk_api'den hazır)
```

## 7. Çalışma Zamanı Nitelikleri

- **Coalescing:** enkoder tıkları anında yerel değeri günceller (buzzer/SKAPP eventi),
  ağ gönderimi ≤10 Hz'e birleştirilir; çevirme durunca son değer garantili gönderilir.
- **Çevrimdışı:** hedef yanıt vermezse değer yerelde ilerler, `target.offline` yayınlanır,
  bağlantı dönünce son değer uzlaştırılır. Düğme asla "donmaz".
- **Recovery boot:** RTC-noinit crash sayacı; 3 ardışık panik → sürücüler ve mode engine
  yüklenmeden boot (transportlar + CLI + OTA açık). `device.status` `recovery:true` bildirir.
- **Kalıcılık:** son mod + son değerler NVS'e yazılır (debounce'lu, flash aşınma korumalı);
  elektrik kesintisi sonrası cihaz kaldığı yerden döner.
- **Config export/import:** `config.export` → tüm sd_* NVS ad alanları tek JSON;
  `config.import` tersini yapar (critical + confirm token). Cihaz klonlama senaryosu.

## 8. sk_core Paylaşım Stratejisi

Kısa vade (V1): `EXTRA_COMPONENT_DIRS` ile `BlockingFocus/Code/components`'a referans —
fork yok, tek kaynak. Orta vade: `sk_core`+`sk_api` kendi reposuna (`smartkraft-core`)
çıkarılır, iki cihaz da oradan tüketir (BF'de SynDimm yüzünden regresyon olmasın diye
sürümlenmiş: git tag/component manager). SynDimm gereksinimleri sk_core'a dokunmayı
gerektirirse değişiklik BF tarafında da derlenip test edilmeden merge edilmez.

## 9. Bölümleme (dahili 4MB)

```
nvs      64KB   (0x10000 — profiller/bağlamalar/prefs NVS blob'ları için büyütüldü)
otadata   8KB
phy       4KB
ota_0   ~1.9MB
ota_1   ~1.9MB
```
LittleFS yok — her şey NVS'te (atomik, wear-leveled, yedeklemesi kolay).
⚠️ RİSK: BLE+WiFi+mbedTLS+MQTT'li firmware'in 1.9MB'a sığması Faz 0'da iskelet
derlemesiyle ÖLÇÜLECEK; sığmazsa önce BLE/TLS config budaması, son çare LittleFS'siz
kalmak kaydıyla slot küçültme değil `sdkconfig` optimizasyonu.

## 10. Fazlar

- **Faz 0 — İskelet:** ESP-IDF projesi, sk_core bağlama, boot + kimlik (SD-xxx) + USB CLI
  + wifi + mdns. Firmware boyut bütçesi ölçümü. Çıktı: `device.info` USB'den cevap veriyor.
- **Faz 1 — Girdi/çıktı:** sd_encoder (jest algılama dahil) + sd_buzzer (sessiz saatler)
  + sd_feedback + prefs. Çıktı: jestler event bus'ta loglanıyor, bipler çalıyor.
- **Faz 2 — Motor + dimmer:** sd_mode_engine + bağlama deposu + `dimmer` sürücüsü +
  sd_proto HTTP + coalescing + mode.test. Çıktı: gerçek bir Shelly/Hue USB CLI'dan
  kurulup düğmeyle kontrol ediliyor.
- **Faz 3 — Protokol tamamlama:** UDP + MQTT yürütücüleri, `shutter` + `mqtt_remote`
  sürücüleri, profil kataloğu v2 taşıma (eski 6 profil).
- **Faz 4 — Safe:** `safe` sürücüsü (dizi eşleyici portu) + sk_api entegrasyonu + lockout. Ürün kuralı (2026-07-03): dizi 3-6 segment, segment 1-50 tık; canlı girişte 6+ segment taşma sayılır ve deneme BAŞARISIZ kaydedilir (lockout beslenir).
- **Faz 5 — SKAPP:** BLE/TCP secure session + eşleşme, device.manifest ekranları
  (knob dashboard ekran türü SKAPP tarafında renderer ister — SKAPP iş kalemi!),
  event aboneliği, endpoint editörü.
- **Faz 6 — Dayanıklılık:** sk_ota + recovery boot + config export/import +
  factory-reset kancaları + uzun-süre (soak) testleri.

Her faz sonu: önceki fazların testleri yeşil + gerçek donanımda duman testi.

## 11. Açık Konular

1. SKAPP'te "knob_dashboard" ekran türünün tasarımı/renderer'ı (Faz 5'ten önce netleşmeli).
2. PCB rev B pin haritası doğrulaması (flash çıkınca boşalan SPI pinleri: yeniden kullanım yok, NC).
3. Eski cihaz kimliği (FNV-1a `SD-XXXXXXXXXX`) ile sk_identity formatı farklı —
   yeni cihazlar sk_identity kullanır, geriye dönük kaygı yok (saha cihazı yok varsayımı — DOĞRULA).
4. Profil kataloğunun yaşayacağı yer: SKAPP gömülü mü, ayrı GitHub reposu mu.
