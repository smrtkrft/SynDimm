# SynDimm — CLI Komut Referansı

> Cihaz: **SmartKraft SynDimm** (ESP32-C6), fw **2.0.0**, protocol **0.2.0**.
> Bu liste cihazın kendi `help all` + `help <komut>` çıktısından üretildi
> (2026-07-03, gerçek donanım). Toplam **70 komut** (14'ü gizli).

## Genel kullanım

- **Bağlantı yolları:**
  - **USB Serial/JTAG** — insan CLI, kimlik-doğrulamasız. Yazdığın satır
    `<komut> <arg...>`. Örn: `idf.py -p /dev/cu.usbmodemXXX monitor` veya
    115200 baud herhangi bir terminal.
  - **BLE / TCP (port 8080)** — SKAPP; ECDH eşleşme + HMAC oturumu sonrası.
    `requires_auth` komutlar YALNIZ bu kimlikli kanalda çalışır.
- **Makine modu:** `json.on` → NDJSON cevaplar (`{"id",ok,data}` / `err`),
  `json.off` → insan-okunur mod. SKAPP hep makine modunda konuşur.
- **Argümanlar:** konumsal (`mode set 1 {...}`) veya adlı bayrak
  (`wifi connect --ssid X --password Y`). Sayısal argümanlar string de gidebilir
  (firmware toleranslı) — SKAPP bundan yararlanır.
- **`critical` komutlar (confirm-token):** yıkıcı işlemler önce
  `ERR_CONFIRM_REQUIRED` + 30 sn geçerli tek-kullanımlık token döner; komutu
  `... --confirm-token <token>` ile tekrar gönderirsin. (SKAPP bunu otomatik
  onay diyaloğuyla yapar.) Bkz. `device.confirm-token`.
- **`requires_auth` komutlar:** yalnız kimlikli BLE/TCP; USB'de
  `ERR_NOT_AUTHENTICATED` döner. (SynDimm'de: tüm `safe.*`.)
- **Gizli komutlar** (`[H]`): `help`te listelenmez, `help all` gösterir;
  bench/tanı/SKAPP-içi kullanım.

**Bayrak lejantı:** 🔒 = critical (confirm-token) · 🔑 = requires_auth (kimlikli kanal) · `[H]` = gizli

---

## SYSTEM — taban (tüm SmartKraft cihazlarında ortak)

### device — kimlik, yeniden başlatma, fabrika
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `device.info` | `device info` | | Kimlik + çalışma anlık görüntüsü (info+status birleşik): model, serial, fw/hw, uptime, wifi, ble, passphrase, bonds, `user_configured`, time |
| `device.capabilities` | `device capabilities` | `[H]` | Yüklü "book"lar + komut sürümleri (18 bileşen) |
| `device.commands` | `device commands` | `[H]` | Kayıtlı her komutun adı (JSON dizi) |
| `device.manifest` | `device manifest` | `[H]` | SKAPP için çalışma-zamanı UI manifest'i (SKAPP çağırıp atar) |
| `device.confirm-token` | `device confirm-token` | `[H]` | Bir kritik komutu yetkilendiren tek-kullanımlık token üretir |
| `device.restart` | `device restart` | 🔒 | Cihazı yeniden başlatır |
| `device.factory-reset` | `device factory-reset` | 🔒 | Bond + ayarlar + tüm NVS silinir, fabrika durumu |

### wifi — ağ bağlantısı
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `wifi.status` | `wifi status` | | Bağlantı durumu, SSID, IP, sinyal, aktif slot, primary/backup var mı |
| `wifi.scan` | `wifi scan` | | Görünen ağları tarar (en çok 12) |
| `wifi.list` | `wifi list` | | Kayıtlı ağları listeler (primary/backup) |
| `wifi.connect` | `wifi connect <ssid> [password] [ip[/24]] [primary\|backup]` | | Bağlanır + kimlik bilgilerini kaydeder (bkz. altta detay) |
| `wifi.disconnect` | `wifi disconnect` | | Mevcut ağdan ayrılır (kalıcı niyet) |
| `wifi.forget` | `wifi forget [--slot primary\|backup]` | 🔒 | Kayıtlı kimlik bilgilerini siler (slot yoksa ikisi de) |

**`wifi.connect` formları:**
```
wifi connect HomeWiFi pass1234                 # primary (varsayılan)
wifi connect HomeWiFi pass1234 192.168.1.111   # statik IP (/24 varsayılan)
wifi connect HomeWiFi pass1234 192.168.1.111/24
wifi connect OfficeWiFi pass5678 backup        # yedek slota kaydet
wifi connect --ssid X --password Y [--static-ip 192.168.1.50/24] [--slot primary|backup]
```
Primary birkaç denemede bağlanamazsa backup'a düşer. (Reconnect artık üstel
backoff'lu: 1s→2s→…→30s tavan, bağlanınca sıfırlanır.)

### ble — Bluetooth transport
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `ble.status` | `ble status` | | BLE radyo durumu + bağlı peer (varsa) |
| `ble.unpair` | `ble unpair` | 🔒 | Tüm eşleşmiş telefonları unutur (tüm bond slotları temizlenir) |

### ota — firmware güncelleme
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `ota.status` | `ota status` | | Güncelleme durumu, mevcut/olası sürüm, aktif partition |
| `ota.check` | `ota check` | | Güncelleme sunucusuna yeni sürüm var mı sorar (kurmaz) |
| `ota.update` | `ota update` | 🔒 | `ota check`'in bulduğu firmware'i kurar (sha256 doğrular, reboot eder) |
| `ota.rollback` | `ota rollback` | 🔒 | Önceki firmware'e döner (yeni build bozuksa) |

### logs — günlük halka tamponu
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `logs.get` | `logs get [--limit N] [--level debug\|info\|warn\|error]` | | Cihaz-içi ring buffer'dan son olay girişleri (varsayılan 50, en çok 200) |

### time
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `time.set` | `time set <unix>` | `[H]` | UTC unix zamanı it (SKAPP her bağlanışta; NTP yoksa log/webhook zaman damgası için) |

---

## SKAPP — eşleşmiş telefon yönetimi

### pairing — eşleşme penceresi
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `pairing.status` | `pairing status` | | Pencere durumu + dolu bond slot sayısı |
| `pairing.start` | `pairing start` | | 60 sn'lik BLE eşleşme penceresi aç (SKAPP keşfetsin) |
| `pairing.stop` | `pairing stop` | | Pencereyi erken kapat |

> Not: her boot'ta 60 sn pencere otomatik açılır (bond yoksa). Yeni telefon =
> 5 sn bas → restart → ilk 60 sn açık. Pencere kapanınca kurulu bağlantı kopmaz.

### auth — bağlantı parolası (passphrase)
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `auth.passphrase.status` | `auth passphrase status` | | Parola yapılandırması (set/mode/kalan deneme) |
| `auth.passphrase.set` | `auth passphrase set <plain>` | 🔒 | İlk parolayı belirle (6-32 karakter) |
| `auth.passphrase.change` | `auth passphrase change <old> <new>` | 🔒 | Parolayı döndür (eskiyi doğrular) |
| `auth.passphrase.clear` | `auth passphrase clear <old>` | 🔒 | Parolayı kaldır (eskiyi doğrular) |
| `auth.passphrase.mode.set` | `auth passphrase mode set --pairing <0\|1> --always <0\|1>` | 🔒 | İki zorlama anahtarı: `pairing`=yalnız ilk eşleşmede sor, `always`=her bağlantıda |
| `auth.token.rotate` | `auth token rotate` | `[H]` | Aktif oturum token'ını döndür (SKAPP-içi) |

### bond — eşleşmiş kurulumlar
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `bond.list` | `bond list` | | Eşleşmiş SKAPP kurulumları (slot, peer_id, label, paired_at) |
| `bond.remove` | `bond remove --slot <0..7>` | 🔒 | Bir slotu unut (o telefon yeniden eşleşmeli) |

---

## DEVICE — SynDimm'in işi

### mode — mod slotları (3 sabit slot)
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `mode.list` | `mode list` | | 3 slotun özeti + `recovery`, `active` |
| `mode.get` | `mode get <slot>` | | Slotun binding JSON'u |
| `mode.set` | `mode set <slot> {json}` | | Slotu bağla (doğrula + kaydet + hot-reload) |
| `mode.select` | `mode select <slot>` | | Aktif slotu değiştir |
| `mode.clear` | `mode clear <slot>` | 🔒 | Slotu çöz (boşalt) |
| `mode.value` | `mode value <slot> <0-100\|toggle>` | | Değer/aç-kapa (SKAPP slider yolu) |
| `mode.test` | `mode test <slot>` | | Slotun aksiyonunu bir kez test-ateşle |

**Binding JSON v2** (`mode.set` gövdesi):
```json
{"v":2,"behavior":"dimmer|shutter|safe|mqtt_remote","enabled":true,
 "name":"Salon","profile":"shelly_dimmer2",
 "targets":[{"host":"192.168.1.40","port":80,"device_id":"","auth_key":""}],
 "params":{"step":1,"accel":true,"gestures_enabled":true,
           "presets":{"double_click":100,"long_press":10},
           "topic":"...","payload_value":"{value}","payload_gesture":"{toggle}",
           "broker":"192.168.1.2","broker_port":1883}}
```
Kısıtlar: `name` JSON-güvenli (tırnak/backslash yok); `behavior`/`profile`
`[A-Za-z0-9_-]`; profil id ≤15. `safe`/`mqtt_remote` profil istemez. MQTT
broker'ı binding'e özgü: `params.broker` > profil broker alanı.

### profile — hedef cihaz profilleri (katalog)
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `profile.list` | `profile list` | | Kayıtlı profiller (kompakt) |
| `profile.get` | `profile get <id>` | | Tam profil JSON'u |
| `profile.add` | `profile add {json}` | | Profil ekle/değiştir (JSON v2; aynı id üstüne yazar) |
| `profile.remove` | `profile remove <id>` | | Profili sil (bir binding kullanıyorsa `ERR_IN_USE`) |

**Profil JSON v2** (`profile.add`):
```json
{"v":2,"id":"shelly_dimmer2","name":"...","protocol":"http|udp|mqtt",
 "port":80,"behaviors":["dimmer"],
 "commands":{"set_value":{"method":"GET","path":"/light/0?brightness={value}",
                          "min":0,"max":100,"step":1},
             "toggle":{"method":"GET","path":"/light/0?turn=toggle"}}}
```
`id` en fazla 15 karakter (NVS anahtar sınırı). Şablon belirteçleri:
`{value}` `{toggle}` `{device_id}` `{auth_key}` `{prefix}` `{state}`.
Repo kataloğu: `profiles/*.json` — dimmer: hue_bulb, mqtt_dimmer, shelly_dim_g2, shelly_dimmer2, tasmota_dimhttp, tasmota_dimmer, wiz_bulb, wled · shutter: mqtt_cover, shelly25_cover, shelly_cover_g2, tasmota_shutter.

### safe — dizi kilidi → webhook tetikleyici
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `safe.list` | `safe list` | 🔑 | Kasa kayıtları (**diziler GİZLİ** — yalnız segment sayısı) |
| `safe.set` | `safe set <1-5> {json}` | 🔑 | Kasa kaydı ayarla (dizi → webhook) |
| `safe.clear` | `safe clear <1-5>` | 🔑 | Kasa kaydını sil |

**Safe kaydı JSON** (`safe.set`):
```json
{"v":2,"enabled":true,"sequence":"L3-R5-L2",
 "endpoint":"<api.endpoint.add ile tanımlı ad>",
 "lockout":{"enabled":true,"after":5,"seconds":30}}
```
Dizi grameri: `L`/`R` + tur (1-50), `-` ile ayrılır, sona isteğe bağlı `B`
(buton); **3-6 segment** (ürün kuralı — kısası `sequence_too_short`, uzunu
`sequence_too_long` reddedilir; parser eski kayıtlar için 16'ya kadar okur ama
`safe.set` 6'da keser). Cihazda 6+ segmentlik kilit açma denemeleri BAŞARISIZ
sayılır ve lockout sayacını besler. **Kayıtlı diziler sırdır**: hiçbir çıktıda
görünmez, `config.export`'ta da `"safe":"omitted"`. `safe.*` yalnız kimlikli kanalda.

### prefs — özellik anahtarları
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `prefs.list` | `prefs list` | | gestures / buzzer / quiet / tz |
| `prefs.set` | `prefs set <gestures\|buzzer\|quiet\|tz> <value>` | | Bir anahtarı ayarla |

```
prefs set gestures off        # çift tık/uzun basış jestlerini kapat
prefs set buzzer off          # tüm sesler kapalı
prefs set quiet 23:00-07:00   # sessiz saat aralığı (gece sarmalı ok)
prefs set quiet off
prefs set tz +02:00           # yerel saat dilimi (quiet için; cihaz UTC)
```

### api — giden webhook preset'leri
| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `api.on` | `api on` | | Ana anahtar — kayıtlı endpoint'ler ateşleyebilsin |
| `api.off` | `api off` | | Ana anahtar — tüm API çağrılarını engelle (kayıtlar durur) |
| `api.status` | `api status` | | Ana anahtar durumu + endpoint sayıları |
| `api.endpoint.list` | `api endpoint list` | | Kayıtlı endpoint'ler (USER + SYSTEM) |
| `api.endpoint.add` | `api endpoint add --name X --type generic\|ifttt\|webhook_post --url ... [--token] [--method] [--auth none\|bearer\|basic\|header] [--header-name] [--content-type] [--delay-after 0-300]` | 🔒 | USER (manuel) endpoint ekle/güncelle (NVS, en çok 5) |
| `api.endpoint.remove` | `api endpoint remove --name X` | 🔒 | USER endpoint'i sil |
| `api.system.add` | `api system add` | | Çağıran peer'in SKAPP listener'ını SYSTEM slotu olarak kaydet |
| `api.system.remove` | `api system remove` | | Çağıran peer'in SYSTEM slotunu sil |
| `api.system.purge` | `api system purge` | | TÜM SYSTEM slotlarını sil (orphan temizliği) |
| `api.system.list` | `api system list` | | Yalnız SYSTEM slotları (eşleşmiş SKAPP'lerce yönetilir) |
| `api.send` | `api send --name X [--payload '...']` | | Bir endpoint'i hemen ateşle (async; sonuç `api.sent` olayıyla) |
| `api.chain.run` | `api chain run` | | Her endpoint'i sırayla ateşle (USER sonra SYSTEM) |

`--type`: `ifttt` (IFTTT Maker), `webhook_post` (auth kontrollü POST),
`generic` (her method/auth/header). `--delay-after` = zincirde sonraki
endpoint'ten önce bekleme (sn).

---

## Yapılandırma yedekleme

| Komut | Kullanım | Bayrak | Açıklama |
|---|---|---|---|
| `config.export` | `config export` | | binding'ler + profiller + prefs tek JSON (safe HARİÇ — `"safe":"omitted"`) |
| `config.import` | `config import {json}` | 🔒 | `config.export` dökümünü geri yükle (hepsini doğrular, sonra reboot) |

---

## Gizli / bench / tanı komutları

| Komut | Kullanım | Açıklama |
|---|---|---|
| `help` | `help [topic\|command\|all]` | Komutları listele / detay göster |
| `json.on` / `json.off` | — | NDJSON makine modu ↔ insan modu |
| `debug.heap` | `debug heap` | Heap watermark: `{free, min_free, largest}` |
| `debug.panic` | `debug panic` | Kasıtlı panik (recovery-boot testi) — 🔒 |
| `buzzer.play` | `buzzer play <ok\|err\|dit1\|dit2\|dit3\|long>` | Bip deseni çal (bench) |
| `proto.http.test` | `proto http test <host:port/path>` | Bench: bir GET ateşle |

---

## Olaylar (kimlikli istemcilere otomatik akar — `{"evt","seq","data"}`)

`mode.changed{slot,name}` · `mode.value{slot,value,state}` (≤10 Hz) ·
`mode.error{slot,err,count}` · `target.offline/online{slot}` ·
`safe.triggered{n,ok}` · `safe.lockout{n,seconds}` ·
`input.gesture{type[,delta]}` · `prefs.changed{key,value}` ·
`profile.added/removed{id}` · `device.recovery{count}` ·
`heap.low{free,largest,min_free}` (uzun-çalışma sağlık uyarısı) +
sk katmanından `api.sent`, `wifi.state`, `ota.fw.state`.

---

## Notlar

- **Kritik komut örneği (USB):**
  ```
  mode clear 1
  → error: ERR_CONFIRM_REQUIRED — to confirm, copy/paste this within 30s:
           mode clear 1 --confirm-token <token>
  mode clear 1 --confirm-token <token>
  → ok.
  ```
- **safe.* USB'de:** `error: ERR_NOT_AUTHENTICATED — Handshake not completed`
  (yalnız kimlikli BLE/TCP'den; diziler fiziksel-varlık kanıtı gereği korunur).
- Kozmetik: insan-modu USB çıktısında boş string alan varsa formatter satıra
  ham JSON basabilir (makine/SKAPP JSON yolu etkilenmez).
