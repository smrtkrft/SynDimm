# Donanım Doğrulama Listesi (cihaz USB'ye takılınca)

> Firmware kod-tamam; bu liste plandaki tüm "hedefte doğrula" adımlarının
> birikimidir. Flash: `source ~/esp/esp-idf-v5.5.2/export.sh && idf.py -p <port> flash monitor`
> ⚠️ Önce PCB rev B pin haritasını doğrula (sd_pins.h: CLK=19 DT=20 SW=18 BUZZER=17).

## İlk donanım oturumu sonuçları (2026-07-03, port /dev/cu.usbmodem101, ESP32-C6FH4)

**CLI ile OTONOM DOĞRULANANLAR (✓):**
- Temiz boot, kimlik `SD-BNBWT4RDP` (regex ✓), `recovery:false`; app_main tamam.
- CLI: `device.info` / `device.capabilities` (18 book) / `mode.list` / `prefs.list`
  (gestures/buzzer/quiet/tz) / `profile.list` / `wifi.status` / `debug.heap` cevap veriyor.
- 60 sn pairing penceresi açılıp temiz kapandı (`pairing mode closed: timeout`).
- **Yeni stabilite (bu oturumda eklendi):** periyodik `health` logu canlı
  (`free=126k largest=104k stack in=1404 cmd=4464`); heap SAĞLIKLI, fragmantasyon yok.
  `sd_input` yığın marjı 1404 B boş → C5 "ince marj" endişesi ÇÜRÜTÜLDÜ.
- **eng_run_test refcount:** `mode.test 1` ×4 → heap BİREBİR SABİT (124232→124232) = leak yok.
- **Motor E2E (dummy target):** `profile.add shelly_dimmer2` → `mode.set 1 dimmer` →
  `mode.value 1 50` gerçek HTTP GET tetikledi (`brightness=50`, ulaşılamaz→temiz hata,
  hang/crash yok) → reboot → `value:50` NVS'ten geri geldi (kalıcılık ✓).
- **Offline tracker:** 3 hata → hedef offline → sonraki test'ler ağı atladı (`ok:true`).
- **Güvenlik:** `safe.list` USB'de `ERR_NOT_AUTHENTICATED` (requires_auth ✓);
  `mode.clear 1` confirm-token akışı (`ERR_CONFIRM_REQUIRED` + tek-kullanımlık token) ✓.
- **Recovery-boot:** kaza eseri kanıtlandı (aşağıdaki bug 12 crash→RECOVERY BOOT tetikledi,
  düzeltme sonrası recovery=0 temiz boot). Cihaz 5+ dk kesintisiz (TWDT yanlış reset atmadı).

**DONANIM BUG BULUNDU + DÜZELTİLDİ (commit f3af5dd):** `sk_auth_open_pairing_mode`
ikili-açılış — bond YOKKEN sk_auth_init pencereyi zaten açıyor, main.c'nin ESP_ERROR_CHECK'li
ikinci çağrısı INVALID_STATE'te abort ediyordu = **her fabrika-taze cihaz sonsuz boot-loop**.
Artık tolere ediliyor. (Bu ayrıca C3 "her-zaman-yüklü yol crash'i recovery ile düzelmez"
senaryosunun canlı örneğiydi — recovery devreye girdi ama pairing hâlâ her boot'ta crash'liyordu.)

**KOZMETİK NOT (sk_core USB formatter, düşük öncelik):** insan-okunur USB çıktısında bir
JSON string alanı BOŞ ("") olduğunda pretty-printer satıra ham JSON'un kalanını basıyor
(ör. `ssid: ","rssi":0,...`). Makine/SKAPP yolu ham JSON kullandığından ETKİLENMEZ; yalnız
USB insan görünümü. sk_core paylaşımlı (BF-first) — ayrı ele alınmalı.

**FİZİKSEL ETKİLEŞİM GEREKENLER (kullanıcı yapacak — aşağıdaki fazlar):** enkoder çevirme,
buton (tık/çift/uzun/5sn/10sn), buzzer sesi, WiFi (SSID/parola), gerçek Shelly/Hue/MQTT,
BLE pairing (SKAPP), OTA, 48 saat soak. `debug.panic ×3 → recovery` de opsiyonel (cihazı 3×
reboot eder, sonra ~10 dk recovery — yeni SD_STABLE_UPTIME_MS=600sn).

## Faz 0 — İskelet
- [ ] USB'de `SD-XXXXXXXX> ` promptu; `device.info` / `help` / `device.capabilities` cevap veriyor
- [ ] Kimlik `^SD-[A-Z0-9]{6,12}$`; boot'ta tek bip + 60 sn pairing ('par' reklamı)
- [ ] `wifi.connect --ssid ... --password ...` → bağlanıyor; mDNS `_skapp._tcp` görünüyor

## Faz 1 — Girdi/çıktı
- [ ] Çevirme: detent başına tepki (4 adım/detent), hızlı çevirmede watchdog yok
- [ ] Tek tık / çift tık / uzun basış logları; `prefs.set gestures off` → tık ANINDA
- [ ] 5 sn basılı: bip (restart uyarısı) → bırakınca yeniden başlama
- [ ] 10 sn basılı: bip (factory uyarısı) → bırakınca NVS silinip sıfırlanma
- [ ] Basılı+çevir: slot 1↔2↔3 geçişi (1/2/3 bip), sınırda sarmıyor
- [ ] `buzzer.play dit3` vb. desenler; `prefs.set buzzer off` susturuyor;
      `prefs.set quiet <şimdiki aralık>` + `time.set` sonrası susuyor (tz'yi ayarla!)

## Faz 2 — Motor + dimmer (gerçek Shelly/Hue ile)
- [ ] `profile.add {"json":"<shelly_dimmer2.json içeriği>"}` → `profile.list`te
- [ ] `mode.set 1 {"v":2,"behavior":"dimmer","name":"Salon","profile":"shelly_dimmer2","targets":[{"host":"<ip>"}]}`
- [ ] Düğme kısar/açar; tık toggle; çift tık preset (params.presets ekleyince)
- [ ] `mode.value 1 50` çalışıyor; değer reboot sonrası korunuyor (2 sn debounce)
- [ ] `mode.test 1` → `{"ok":true,...,"status":200}`
- [ ] Hedefi fişten çek → 3 hata → ERR bip + `target.offline`; düğme yerel akıcı;
      probe 5 sn'de bir; tak → `target.online` + lambada uzlaşmış değer

## Faz 3 — Protokoller
- [ ] WiZ (UDP): slot'a bağla, çevirme parlaklık değiştiriyor (`nc -ul 38899` ile de izlenebilir)
- [ ] MQTT (mosquitto): tasmota profili publish ediyor; broker restart → auto-reconnect
- [ ] mqtt_remote slotu: çevirme `params.topic`'e {value} basıyor (`mosquitto_sub` ile izle)
- [ ] İkinci farklı broker'lı slot → `mode.error {mqtt_broker_conflict}`

## Faz 4 — Safe (kimlikli TCP oturumu gerekir)
- [ ] `api.endpoint.add --name kapi --url https://webhook.site/...`
- [ ] `safe.set 1 {"v":2,"sequence":"R4-L2-R3","endpoint":"kapi","lockout":{"enabled":true,"after":3,"seconds":30}}`
- [ ] Düğmede R4-L2-R3 + 2 sn bekle → 3 bip + `api.sent` + webhook.site'ta istek
- [ ] Yanlış dizi → uzun bip; 3 yanlış → ERR bip + 30 sn girdi yok + `safe.lockout`
- [ ] USB'den `safe.list` → `ERR_NOT_AUTHENTICATED`

## Faz 5 — SKAPP wire (BLE/TCP)
- [ ] BLE pairing: 5 sn bas → restart → SKAPP 60 sn içinde eşleşiyor
- [ ] Makine modunda `mode.list`/`mode.value` zarfları (`data` alanı) + olay akışı (`evt`)
- [ ] En büyük `profile.get` BLE'de sorunsuz mu? (MTU ölçümü — sonucu SKAPP_CONTRACT.md'ye işle)

## Faz 6 — Dayanıklılık
- [ ] `debug.panic` ×3 (confirm-token'lı) → RECOVERY boot: slotlar yüklenmiyor,
      `mode.list` `recovery:true`, transportlar + OTA açık; 120 sn stabil → sayaç sıfır
- [ ] `config.export` → factory reset → `config.import` → birebir aynı konfig
- [ ] OTA (T6.3, release altyapısı kurulunca): manifest URL + A→B→A döngüsü + rollback
- [ ] 48 saat soak (T6.4): sürekli çevirme, broker/AP flap, `debug.heap` watermark takibi
