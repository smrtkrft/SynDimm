# Vendored Components

## sk_core (v0.4.0) + sk_api (v0.3.0)

- **Kaynak:** `BlockingFocus/Code/components/{sk_core,sk_api}` (github.com/smrtkrft/BlockingFocus)
- **Kopyalanma tarihi:** 2026-07-02
- **Kopyalayan karar:** SynDimm reposu GitHub'da tek başına derlenebilir kalsın diye
  EXTRA_COMPONENT_DIRS referansı yerine vendor copy seçildi (plan: Kilitli Kararlar).

## Senkron kuralı

1. Bu kopyalar SynDimm içinde SERBESTÇE DEĞİŞTİRİLMEZ.
2. sk_core/sk_api'de değişiklik gerekirse: önce BlockingFocus'ta yapılır,
   BF derlenip gerçek donanımda test edilir, SONRA buraya aynı diff uygulanır.
3. Upstream'den senkron alırken bu dosyadaki sürüm+tarih güncellenir.
4. Sürüm kaynağı: her komponentin `idf_component.yml` dosyasındaki `version` alanı.

## Uygulanan senkron diff'leri

- **2026-07-03 · `sk_core/{src/sk_auth.c,include/sk_auth.h}` — "bond yokken pencere
  açık kal" opt-in'i.** Bond'suz cihaz 60 sn boot pencere timeout'unda BLE'yi
  kapatıyordu → cihaz görünmez oluyordu ("SKAPP bağlanmıyor" kök nedeni). Eklendi:
  `sk_auth_set_stay_open_when_bondless(bool)` + `pair_timeout_cb`'de bond yoksa +
  opt-in ise pencereyi yeniden kur (kapatma) → `pairing_state` OPEN kalır →
  `idle_timer_cb` advertising'i açık tutar. SynDimm (sabit güç) `main.c`'de
  `true` çağırır; piller (BF) çağırmaz → davranışı değişmez. **BF'de yapıldı+
  derlendi** (sk_auth.c.obj OK), SynDimm'e kopyalandı, gerçek cihazda doğrulandı
  (65 sn sonra pairing.status=open, ble=advertising).

- **2026-07-03 · `sk_core/src/sk_wifi.c` — WiFi reconnect üstel backoff.**
  Eskiden `WIFI_EVENT_STA_DISCONNECTED` `esp_wifi_connect()`'i ANINDA çağırıyordu
  (AP uzun süre kapalıysa sürekli re-assoc = boşa akım). Eklendi: `s_reconnect_timer`
  + `schedule_reconnect()` (1s,2s,4s,…,30s tavan; GOT_IP ile otomatik sıfırlanır).
  **BF'de yapıldı ve derlendi** (kaynak-of-truth), buraya birebir kopyalandı; SynDimm
  tam build yeşil. NOT: `LebensSpur/Neue/sk_core/src/sk_wifi.c` sürümü ıraksamış
  (farklı md5) — aynı backoff'u oraya körlemesine kopyalamayın, elle merge gerekir.

- **2026-08-13 · `sk_core/{include/sk_auth.h,src/sk_auth.c,src/sk_transport_ble_gatt.c,
  src/sk_transport_tcp.c}` — parola-kapılı eşleşme linki erken kapanıyordu.**
  `handle_ecdh_exchange` kapı açıkken bond'u RAM'de bekletip `need_passphrase:true`
  dönüyor ama `SK_AUTH_PAIRING_OK` raporluyordu; taşıyıcılar OK'i "bitti" sayıp linki
  kapatıyor, `close_pairing_mode` de bekleyen bond'u siliyordu → peer'in
  `pairing.passphrase.verify`'ı inecek yer yok = parola kapısı açıkken eşleşme HİÇ
  tamamlanamıyor. Eklendi: `SK_AUTH_PAIRING_PENDING` sonucu ("peer aynı link üzerinde
  bir satır daha gönderecek, KAPATMA"); taşıyıcılar PENDING'de linki açık tutar; TCP'ye
  `client_t.pairing_repair` bayrağı (onarım yolunda ikinci satır ecdh substring kapısına
  takılmasın, pencere kapalıysa düzgün `ERR_PAIRING_NOT_OPEN` dönsün). Ayrıca
  `sk_auth_clear_all` artık `pending_clear()` çağırıyor (ble.unpair sonrası RAM'de kalan
  bekleyen bond'un commit edilip erişimi geri vermesi kapandı). **BF'de yapıldı+derlendi**,
  SynDimm'e birebir kopyalandı, iki ağaç da build yeşil (%19 boş). Donanım doğrulaması
  BEKLİYOR.

- **2026-08-13 · `sk_core/{include/sk_auth.h,private_include/sk_secure_session.h,
  src/sk_auth_handshake.c,src/sk_auth_hmac.c,src/sk_secure_session.c}` — imzalı-zarf
  katmanı OTURUM KAPSAMINA alındı.** Cihaz aynı anda birden çok peer'e hizmet ediyor
  (4 TCP client + 1 BLE) ama hem 64'lük nonce halkası hem "aktif bond" GLOBAL'di:
  (a) her SKAPP CliSigner'ı nonce=1'den başladığı için ikinci peer'in nonce'ları
  birincininkiyle çakışıp replay sanılıyordu; (b) ikinci peer'in handshake'i
  `sk_auth_active_bond_clear()` ile birincinin aktif anahtarını siliyor, sonra kendi
  slotunu aktive ediyordu → birinci peer'in komutları yanlış anahtarla doğrulanıp
  `ERR_HMAC_INVALID` alıyordu; (c) `handshake_answer` global aktif anahtarla cevap
  ürettiği için iki peer birbirinin cevabını alıyordu. Eklendi: `sk_auth_handshake_t`
  içine `bond_set/bond_slot/bond_key`, `sk_auth_replay_t` (oturum başına halka),
  `sk_auth_verify_message_with()`, `sk_auth_handshake_answer(hs, ...)`. Global
  `sk_auth_verify_message`/`sk_auth_replay_reset` geriye-uyum için duruyor; global
  slot aktivasyonu yalnız `bond.list.active_slot` raporlaması için korundu.
  **BF'de yapıldı+derlendi**, SynDimm'e birebir kopyalandı (`diff -rq` temiz), iki ağaç
  da build yeşil. Çok-eşli donanım doğrulaması BEKLİYOR.
