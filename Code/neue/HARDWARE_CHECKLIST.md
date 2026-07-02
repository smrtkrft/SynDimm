# Donanım Doğrulama Listesi (cihaz USB'ye takılınca)

> Firmware kod-tamam; bu liste plandaki tüm "hedefte doğrula" adımlarının
> birikimidir. Flash: `source ~/esp/esp-idf-v5.5.2/export.sh && idf.py -p <port> flash monitor`
> ⚠️ Önce PCB rev B pin haritasını doğrula (sd_pins.h: CLK=19 DT=20 SW=18 BUZZER=17).

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
