# SynDimm ↔ SKAPP Sözleşmesi (firmware tarafı — plugin planının girdisi)

> Durum: kod-tamam; canlı BLE/TCP doğrulaması donanım oturumunda yapılacak
> (bkz. HARDWARE_CHECKLIST). Wire format sk_cli/sk_core'dan gelir — BF ile
> birebir aynı: ok → `{"id",ok:true,"data":...}`, hata → `{"id",ok:false,
> "err":"ERR_*","params":...}`, olay → `{"evt":"...","seq":N,"data":...}`.

## Kimlik ve ulaşım

| Alan | Değer |
|---|---|
| Prefix / regex | `SD` — `^SD-[A-Z0-9]{6,12}$` |
| Ürün / marka | SmartKraft SynDimm, fw 2.0.0, protocol_version 0.2.0 |
| TCP | NDJSON, port **8080**, mDNS `_skapp._tcp` |
| BLE | sk_transport_ble (servis `f100d001-…d01`, BF ile aynı) |
| Pairing | **Jest yok.** Her boot'ta 60 sn pencere (`sk_auth_open_pairing_mode`). Yeni telefon akışı: kullanıcı 5 sn basılı tutar → cihaz restart → ilk 60 sn 'par' reklamı. Kurulu bağlantı pencere kapanınca KOPMAZ. |
| Auth | sk_auth ECDH X25519 + HMAC zarf (BF ile aynı); `safe.*` requires_auth |

## SKAPP pano (knob dashboard) veri kaynakları

- Durum: `mode.list` → `{"recovery":bool,"active":N,"slots":[{"slot","assigned","behavior","name","enabled","error","value","state"}]}`
- Değer yazma: `mode.value <slot> <0-100|toggle>` (slider/buton yolu)
- Slot seçimi: `mode.select <slot>`; bağlama düzenleme: `mode.get/set/clear` (clear = confirm-token'lı)
- Kurulum testi: `mode.test <slot>` → `{"ok","err","status"}` (≤5 sn bekler)
- Profiller: `profile.list` (kompakt) / `profile.get/add/remove`; katalog repoda `profiles/*.json`, SKAPP seçileni `profile.add {"json":"<string>"}` ile basar
- Safe: `safe.list/set/clear` (yalnız kimlikli kanal; sequence listede gizli, yalnız segment sayısı döner)
- Tercihler: `prefs.list/set` — `gestures|buzzer|quiet|tz` (BF toggle_list ekran deseni)

## Olaylar (kimlikli istemcilere otomatik akar)

`mode.changed{slot,name}` · `mode.value{slot,value,state}` (≤10 Hz) ·
`mode.error{slot,err,count}` · `target.offline/online{slot}` ·
`safe.triggered{n,ok}` · `safe.lockout{n,seconds}` ·
`input.gesture{type[,delta]}` · `prefs.changed{key,value}` ·
`profile.added/removed{id}` + sk katmanından `api.sent`, `wifi.*`, `ota.*`.

## Bağlama (binding) JSON v2 — mode.set gövdesi

```json
{"v":2,"behavior":"dimmer|shutter|safe|mqtt_remote","enabled":true,
 "name":"Salon","profile":"shelly_dimmer2",
 "targets":[{"host":"192.168.1.40","port":80,"device_id":"","auth_key":""}],
 "params":{"step":1,"accel":true,"gestures_enabled":true,
           "presets":{"double_click":100,"long_press":10},
           "topic":"...","payload_value":"{value}","payload_gesture":"{toggle}",
           "broker":"192.168.1.2","broker_port":1883}}
```
Kısıtlar: name JSON-güvenli (tırnak/backslash yok), behavior/profile
`[A-Za-z0-9_-]`, profile id ≤15. MQTT broker'ı KURULUMA özgüdür: binding
params.broker > profil broker alanı.

## SKAPP plugin planı için iş listesi (keşif bulgularıyla)

1. `SdPlugin` (`features/devices/syndimm/sd_plugin.dart`, `bf_plugin.dart` aynası)
2. Registry satırı `device_plugin_registry.dart` + `device_type_visual.dart`'a `SD` case'leri (ikinci düzenleme noktası!)
3. `SdSession` (InheritedWidget, `bf_session.dart` deseni)
4. Pano: BF dashboard iskeleti + 3 slot kartı (LS `ls_section` deseni) + değer kadranı (`ls_countdown_ring` CustomPainter fork'u)
5. `sd_event_catalog.dart` (SKAPI tetikleyicileri: safe.triggered, mode.changed, input.gesture)
6. Sayısal argümanlar string gidebilir (firmware `sk_cli_arg_long` toleranslı) ✓

## Bilinen sınırlar / ölçülecekler

- BLE MTU: en büyük cevaplar `profile.get` (≤2 KB) ve `device.manifest` —
  donanım oturumunda BLE üzerinden ölçülecek (parçalama v0.3 açık maddesi).
- `mode.value` olay temposu ≤10 Hz (coalescing) — slider canlılığı yeterli.
- UDP hedeflerde offline tespiti WiFi-seviyesi (dürüstlük notu).
