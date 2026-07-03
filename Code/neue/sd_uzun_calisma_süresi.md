# SynDimm — Uzun Çalışma Süresi Dayanıklılık Analizi

> **Bağlam:** SynDimm (ESP32-C6, ESP-IDF 5.5.2) sürekli elektriğe bağlı,
> 7/24, aylarca kesintisiz çalışacak bir cihaz. Bu belge bellek taşması,
> donma/bloke olma, bozulma, flash aşınması ve reconnect fırtınası gibi
> uzun-vade risklerini firmware kodunu satır satır okuyarak envanterler;
> mevcut korumaları ve önerilen eklemeleri öncelik sırasıyla listeler.
>
> **Yöntem:** `components/sd_*`, `sk_core`/`sk_api` (vendored), `main.c` ve
> derlenmiş `sdkconfig` üç bağımsız derin-okuma turuyla tarandı (bellek/leak,
> bloke/watchdog/deadlock, NVS/zaman/reconnect/boot). Bulgular kaynak
> satırlarıyla verilir. Sınıflandırma: **VAR** = mevcut koruma yeterli,
> **EKSİK** = gerçek açık/öneri, **POTANSİYEL** = koşullu/latent risk.
>
> Durum (2026-07-03): Analiz tamamlandı. Kod düzeltmeleri DONANIM
> OTURUMUNDA uygulanacak — TWDT panic ve reconnect backoff gibi değişiklikler
> çalışma-zamanı davranışını değiştirir ve gerçek cihazda doğrulanmalıdır
> (host testleri yalnız saf mantık modüllerini kapsar).

---

## 0. Görev ve kilit topolojisi (zemin gerçeği)

| Task | Öncelik | Yığın | Nerede | Bloke bekleme |
|---|---|---|---|---|
| `sd_input` | 5 | 3072 B | `sd_encoder.c:267` | yok; `vTaskDelay(10ms)` poll (`sd_encoder.c:216`) |
| `sd_cmd` | 4 | 6144 B | `sd_mode_engine.c:1013` | `xQueueReceive(...portMAX_DELAY)` (`:870`) |
| `sd_restart` | 3 | 2048 B | `sd_config_io.c:241` | `vTaskDelay(500ms)`→`esp_restart` |
| IDF sistem-olay task | (IDF) | 6144 B | IDF | `on_wifi_state` → `eng_lock` (`:901,907`) |
| esp_timer task | (IDF) | — | IDF | nvs/slot/buzzer/stable geri-çağrımları |

**Muteksler:** `eng_lock` (özyinelemeli, motor-geneli, `sd_mode_engine.c:49,88`);
mqtt `s_mtx` (`sd_proto_mqtt.c:30`); prefs `s_lock`; event-bus `s_mtx`;
safe-store `s_lock`. Tek kuyruk `s_queue` uzunluk **16** (`SD_ENGINE_QUEUE_LEN`).
Watchdog: Task WDT @ **5 sn** (uyarı-modu), Int WDT @ 300 ms (`sdkconfig:1638-1644`).

**Tüm dinamik durum sabit boyutlu dizilerde:** slot=3, MQTT abone=4, sürücü
registry=8, safe kayıt=5, bekleyen olay=4, offline slot=8, binding hedef=4.
Sınırsız büyüyen hiçbir koleksiyon yok.

---

## 1. Riskler tablosu

| # | Risk | Sınıf | Öncelik | Kaynak |
|---|---|---|---|---|
| **A1** | TWDT yalnız-uyarı (PANIC kapalı) — hung task cihazı yeniden başlatMAZ | **EKSİK** | **P0** | `sdkconfig:1642` |
| **A2** | Hiçbir uygulama task'ı TWDT'ye kayıtlı değil (`esp_task_wdt_add` yok) — yalnız IDLE0 izleniyor | **EKSİK** | **P0** | grep: boş |
| **A3** | Sessiz hang recovery sayacını artırmaz → deadlock = kalıcı brick | **EKSİK** | **P0** | `main.c:248-289` |
| **B1** | `eng_run_test` timeout'ta job + semafor sızdırır (CONFIRMED, nadir, sınırlı) | **EKSİK** | **P1** | `sd_mode_engine.c:816-821` |
| **B2** | WiFi anında reconnect, exponential backoff yok — uzun AP kesintisinde sürekli deneme/akım | **EKSİK** | **P1** | `sk_wifi.c:165` |
| **B3** | Recovery sayacı 120 sn'de sıfırlanır → periyodu >120 sn olan yavaş crash-loop recovery'ye HİÇ girmez | **EKSİK** | **P1** | `main.c:117,123-131` |
| **B4** | Periyodik health/heap-watermark izleme yok; self-heal yalnız watchdog→crash→recovery | **EKSİK** | **P1** | `main.c:152-162` |
| **C1** | Sürücü geri-çağrımı `eng_lock` altında çalışır — bloke eden ileride bir sürücü tüm motoru dondurur (latent) | **POTANSİYEL** | **P2** | `sd_mode_engine.c:535,778-791` |
| **C2** | `execute_test` sürücü hata dönerse `captured` job'u free etmez (latent, mevcut sürücülerle erişilemez) | **POTANSİYEL** | **P2** | `sd_mode_engine.c:794-797` |
| **C3** | Recovery yalnız slot/proto/behaviors kapatır — her-zaman-yüklü çekirdek yolundaki (sk_core/wifi/prefs) crash mitigasyonsuz | **POTANSİYEL** | **P2** | `main.c:180-200` |
| **C4** | Brownout/power-glitch "crash" sayılır → marjinal güçte yanlış recovery | **POTANSİYEL** | **P3** | `main.c:252,264` |
| **C5** | `sd_input` 3072 B yığın + senkron event-bus fan-out — marj ince, gelecekteki derin subscriber overflow riski | **POTANSİYEL** | **P2** | `sd_encoder.c:267`; event-bus publish |
| **D1** | Değer-cache "2 sn debounce" = ilk değişimden 2 sn (trailing DEĞİL) → sürekli çevirmede 0.5 Hz NVS yazma | **POTANSİYEL** | **P3** | `sd_mode_engine.c:209-228,254-263` |
| **D2** | Aktif-slot her değişimde debounce'suz NVS'e yazılır | **POTANSİYEL** | **P3** | `sd_mode_engine.c:500-520` |
| **D3** | Send yolunda per-SEND cJSON churn (hafifletilmiş) + HTTP client re-init churn → aylarca fragmantasyon | **POTANSİYEL** | **P3** | `sd_mode_engine.c:643-695`; `sd_proto_http.c:40-59` |
| **D4** | `time.set` ile duvar-saati sıçraması — kırık delta yok; quiet fail-open; yalnız HMAC pencere churn'ü | **POTANSİYEL** | **P4** | `sk_baseline.c:586`; `sd_buzzer.c:108-116` |
| **D5** | `s_nvs_pending` kilitsiz volatile — yarış en fazla fazladan/eksik bir debounce timer'ı kurar (zararsız) | **POTANSİYEL** | **P4** | `sd_mode_engine.c:52` |

---

## 2. Mevcut korumalar (VAR — dokunma)

Kod tabanı uzun-vade dayanıklılığında **güçlü**; aşağıdakiler doğrulandı:

**Bellek / kaynak ömrü**
- **Sınırsız büyüme YOK, hot-path leak YOK.** Her koleksiyon sabit dizi (§0).
- Tüm `cJSON_PrintUnformatted` dönüşleri free'lenir; **double-free yok**;
  `cJSON_Duplicate` bağımsız kopya üretir, orijinal sahipliği bozulmaz
  (`sd_config_io.c`, `sd_mode_engine.c:659`, `sd_profiles.c`, `sd_safe_store.c`).
- **UDP soketi her zaman kapanır** (`sd_proto_udp.c:43-48`); MQTT client_init
  hatasında `esp_mqtt_client_destroy` (`sd_proto_mqtt.c:133-137`); HTTP
  keep-alive singleton dengeli (`sd_proto_http.c:26-42,118-119`).
- `materialize` hata yolunda free eder (`:689-693`); `load_slot` doğrulama
  hatasında binding'i siler (`:404-411`); `unload_slot` her reload başında
  binding/profil/priv'i serbest bırakır.
- Tüm NVS string yüklemeleri uzunluk-sınırlı (binding≤1024, profil≤2048,
  safe 384 B sabit); template genişletme yalnız-yığın, truncation-safe.

**Canlılık / kilitler**
- **ABBA kilit döngüsü YOK** — tek motor-geneli özyinelemeli mutex; MQTT rx
  geri-çağrımı `s_mtx` DIŞINDA çağrılır (`sd_proto_mqtt.c:87-88`); ISR hiç
  kilit almaz (`sd_encoder.c:74-94`).
- Ağ çağrıları **kilitsiz** yapılır (`eng_lock` ağ öncesi bırakılır,
  `sd_mode_engine.c:751,756`) → yavaş/ölü hedef yalnız `sd_cmd` throughput'unu
  geciktirir, motoru dondurmaz.
- HTTP `perform` **3 sn timeout** (`sd_proto_http.c:19,112`); MQTT publish
  bağlı-değilse döner, qos0 enqueue (non-blocking, `:193-196`).
- Kuyruk-16 **drop-oldest-SEND**, non-blocking, kayıp-güvenli `job_discard`
  (`sd_mode_engine.c:190-207`) — atılan TEST semaforu CLI'ye devreder,
  atılan NVS_SAVE debounce'u yeniden kurar.
- Girdi seli **VALUE coalescing** (drain-keep-latest, 100 ms) ile çökertilir
  (`:874-888`); tüm esp_timer geri-çağrımları **yalnız-enqueue** (kilit/NVS/ağ
  yok) (`:212,296`; `sd_buzzer.c:75`).
- Öncelik ters çevrimi FreeRTOS öncelik-kalıtımıyla sınırlı; tutan taraf işi
  yalnız kilit-içi (ağ kilitsiz).

**Zaman / NVS / reconnect / boot**
- **Tüm geçen-süre matematiği int64** (`esp_timer_get_time()` µs) — int32
  truncation bulunmadı; ~292k yıl sarma. Offline/accel/handshake hepsi int64.
- Değer-cache 2 sn cap → sürekli çevirmede en kötü **0.5 Hz** NVS commit
  (uygulama-yolu da aynı cap'ten geçer); IDF compare-before-write aynı değeri
  no-op yapar. Config blob'ları yalnız provisioning'de yazılır (aşınma yok).
- MQTT subscribe **her CONNECTED'de** yeniden uygulanır (eski
  subscribe-before-connect bug fix'i, `sd_proto_mqtt.c:45-68`); esp-mqtt
  otomatik reconnect'e bırakılır (uygulama-döngüsü yok).
- Offline 5 sn probu bir **kapı**, üretici değil — slot başına ≤1 deneme/5 sn,
  yalnız zaten dispatch edilmiş bir job varken (`sd_offline.c:39-49`); WiFi
  reconnect'te reconcile ≤3 slotla sınırlı.
- sk_api webhook retry sınırlı (2 deneme, tek 200 ms backoff, 4xx/5xx'te retry
  YOK — çift teslim önlenir, `sk_api.c:888-947`).
- Recovery boot mekanizması: `RTC_NOINIT` crash sayacı + magic power-loss
  koruması doğru (soğuk güç döngüsü birikmez); PANIC/WDT/BROWNOUT'ta ++,
  temiz reset'te 0; ≥3'te slot/proto/behaviors atlanır ama CLI/transport/OTA
  canlı kalır (anti-brick).
- `debug.heap` (`main.c:152-162`) free/min-free/en-büyük-blok raporlar (soak
  gözlemi için hazır).

---

## 3. Önerilen eklemeler (öncelik sırasıyla)

### P0 — Sessiz hang'i otomatik kurtarmaya bağla (A1+A2+A3)
**Sorun:** En tehlikeli birleşim. Bir deadlock (ör. C1'deki latent
`eng_lock`-tutan sürücü yolu) oluşursa: TWDT reset atmaz (PANIC kapalı),
uygulama task'ları TWDT'ye kayıtlı değil (yalnız IDLE0 izleniyor; bloke task
CPU'yu meşgul etmediğinden IDLE0 çalışmaya devam eder ve uyarı bile tetiklenmez),
ve sessiz hang recovery sayacını artırmaz. Sonuç: manuel güç döngüsüne kadar
**kalıcı brick**. Şu an sevk edilen tüm yollar tek tek korumalı, yani bu bir
**savunma-derinliği açığı** — ama 7/24 dağıtımda tek bir latent hang'i
felakete çevirir.

**Öneri:**
1. `sdkconfig.defaults`'a `CONFIG_ESP_TASK_WDT_PANIC=y` ekle → TWDT süresi
   dolduğunda chip reset olur, bu da `ESP_RST_TASK_WDT` → recovery sayacı ++.
2. `sd_cmd` ve `sd_input` task'larını `esp_task_wdt_add(NULL)` ile abone et;
   her döngü turunda `esp_task_wdt_reset()` besle. `sd_cmd` için: kuyruk
   `xQueueReceive` timeout'unu portMAX_DELAY yerine ~2 sn yapıp her tur besle
   (idle beslemesi). `sd_input` zaten 10 ms poll'da — turda bir besle.
3. Böylece bloke bir task 5 sn içinde reset→recovery zincirini tetikler.

**Risk/doğrulama:** Çalışma-zamanı reset davranışını değiştirir; DONANIMDA
doğrulanmalı (kasıtlı `debug.panic` + kasıtlı deadlock testi). Yığın/CPU
maliyeti ihmal edilebilir.

### P1 — Dört somut iyileştirme

**B1 · `eng_run_test` sızıntısını kapat.** Timeout'ta job+semafor kasıtlı
sızdırılıyor (cmd task hâlâ `give` edebilir diye). Sahiplik-devri bayrağı ekle:
job'a atomik `owner_freed` alanı koy; timeout eden CLI bayrağı set eder ve
free ETMEZ, cmd task işi bitirince bayrağı görüp free eder (veya tersi — son
dokunan free eder). Böylece ne use-after-free ne de leak. Sınırlı/nadir ama
aylarca tekrarlı offline `mode.test` altında birikebilir.

**B2 · WiFi reconnect backoff.** `sk_wifi.c:165` anında `esp_wifi_connect()`
çağırıyor. Üstel + tavanlı backoff ekle (ör. 1→2→4→…→30 sn tavan), got-IP'de
sıfırla. Uzun AP kesintisinde sürekli re-assoc akımını/radyo aktivitesini
düşürür. **Not:** Bu sk_core dosyası — vendored; değişiklik önce
BlockingFocus'ta derlenip test edilmeli, sonra iki kopyaya da uygulanmalı
(VENDORED.md kuralı). BF/LS de aynı iyileştirmeden yararlanır.

**B3 · Yavaş crash-loop kaçağını kapat.** Recovery sayacı her boot'ta 120
sn'de sıfırlanıyor; periyodu >120 sn olan bir fault sayacı hiç 3'e ulaştırmaz.
İki seçenek: (a) sayacı ancak N ardışık **temiz** uzun-uptime sonrası sıfırla
(tek 120 sn yerine), veya (b) ayrı bir "son 24 saatteki toplam unclean reset"
RTC sayacı tut, eşik aşılırsa recovery'ye gir. (a) daha basit ve yeterli.

**B4 · Periyodik health/heap izleme.** Düşük öncelikli periyodik esp_timer
(ör. 60 sn) ekle: `esp_get_free_heap_size` + `esp_get_minimum_free_heap_size`
+ en-büyük-blok örnekle; belirlenen kritik eşiğin altına düşerse `heap.low`
olayı yayınla (SKAPP loglar) ve isteğe bağlı olarak kontrollü restart tetikle
(veri kaybı olmadan — NVS zaten güncel). Ayrıca task yüksek-su-işaretlerini
(`uxTaskGetStackHighWaterMark`) logla → C5 (sd_input yığın marjı) gözlemi.
Bu, "yalnız crash'te fark et" yerine proaktif erken uyarı sağlar.

### P2 — Savunma-derinliği (latent riskler)

**C1 · Sürücü sözleşmesini kilitten ayır (ileriye dönük).** Sürücü
geri-çağrımları `eng_lock` altında çalışıyor; bloke eden bir sürücü tüm motoru
(input+cmd+CLI+wifi) dondurur. Mevcut dört sürücü doğrulanmış non-blocking, ama
`sd_behavior.h` sözleşmesine "on_input/test/on_timeout BLOKLAMAZ, kilit-altında
çalışır" uyarısını yaz (zaten kısmen var) ve ileride bir debug derlemesinde
geri-çağrım süresini ölçüp eşik aşımını logla.

**C2 · `execute_test` `captured` free'i.** Sürücü `test()` non-OK dönerse
`captured` job'u free et (şu an yalnız OK yolunda free ediliyor). Tek satırlık
defensive fix; mevcut sürücülerle erişilemez ama ucuz.

**C3 · Çekirdek-yol crash mitigasyonu.** Her-zaman-yüklü bileşende (sk_core,
wifi, prefs) crash recovery ile düzelmez. Uzun vadede: recovery'nin ikinci
kademesi olarak, sayaç çok yüksekse (ör. ≥6) transport'ları da minimal moda al
(yalnız USB CLI). Düşük öncelik — mevcut sürücü hataları zaten birinci kademe
recovery'de izole.

**C5 · `sd_input` yığınını izle/büyüt.** 3072 B + senkron event-bus fan-out
marjı ince. B4'teki high-water-mark izlemeyle gözle; marj <512 B görülürse
4096 B'ye çıkar.

### P3–P4 — Düşük öncelik / kozmetik

**D1 · Gerçek trailing debounce (opsiyonel).** Değer-cache'i "ilk değişimden
2 sn" yerine gerçek trailing (her değişimde timer reset) yaparsan sürekli
çevirme tek yazıya çöker (hareket durunca). NVS aşınmasını 0.5 Hz'den daha da
düşürür. Mevcut cap zaten güvenli (~1+ yıl sürekli çevirme = 100k döngü), bu
yalnız ek pay. **Dikkat:** trailing, hareket asla durmuyorsa yazmayı sonsuz
erteler — üst sınır (ör. 30 sn'de bir zorunlu flush) ile birleştir.

**D2 · Aktif-slot yazımına küçük debounce** ekle (hızlı slot-stepping için).
Düşük etki.

**D3 · Fragmantasyon gözlemi.** Send yolundaki cJSON churn'ü azaltmak için
`materialize`'ı önceden-tahsisli sabit buffer'a taşımak düşünülebilir; ama
mevcut coalescing+offline-gate zaten hafifletiyor. Önce B4 ile en-büyük-blok
trendini izle, gerçek düşüş görülürse müdahale et.

**D4 · time.set sıçraması** — kırık delta üretmiyor; müdahale gereksiz,
yalnız dokümante.

**D5 · `s_nvs_pending` yarışı** — zararsız; istenirse `eng_lock` altına al.

---

## 4. Öncelik özeti ve eylem planı

| Öncelik | Ne | Nerede | Donanım testi? |
|---|---|---|---|
| **P0** | TWDT PANIC=y + sd_cmd/sd_input abone + besle | `sdkconfig.defaults`, `sd_mode_engine.c`, `sd_encoder.c` | **Evet** (kasıtlı deadlock/panic) |
| **P1** | eng_run_test sahiplik-devri | `sd_mode_engine.c:804-821` | Evet (offline mode.test soak) |
| **P1** | WiFi üstel backoff (vendored — BF-first) | `sk_wifi.c` | Evet |
| **P1** | Yavaş crash-loop kaçağı (N-uptime reset) | `main.c:117-131` | Evet |
| **P1** | Periyodik heap/health timer + high-water log | `main.c` (yeni) | Evet (soak) |
| **P2** | Sürücü-kilit sözleşme uyarısı + süre ölçüm | `sd_behavior.h`, debug build | Kısmi |
| **P2** | execute_test captured free (1 satır) | `sd_mode_engine.c:794-797` | Host + build |
| **P2** | sd_input high-water izle → gerekirse 4096 B | `sd_encoder.c` | Evet |
| **P3** | Trailing debounce + zorunlu-flush tavanı | `sd_mode_engine.c` | Evet |
| **P3** | Aktif-slot debounce; fragmentasyon gözlemi | `sd_mode_engine.c` | Evet |
| **P4** | Brownout ayrımı; time.set doküman; nvs_pending kilit | `main.c`, `sd_mode_engine.c` | Evet |

**Genel değerlendirme:** Firmware uzun-vade dayanıklılığında olgun — sınırsız
büyüme yok, leak yok (bir kasıtlı-nadir hariç), tüm ağ/kilit/timer yolları
korumalı, int64 zaman matematiği, bounded kuyruk, coalescing, recovery boot.
**Tek en önemli açık P0'dır:** watchdog'un reset atmaması + task'ların kayıtlı
olmaması + sessiz hang'in recovery'yi tetiklememesi. Bu üçlü kapatıldığında,
teorik bir deadlock bile 5 sn içinde otomatik reset→recovery'ye dönüşür ve
cihaz 7/24 dağıtımda kendini iyileştirir.

## 5. Soak-izleme sinyalleri (48+ saat)

- `debug.heap` en-büyük-boş-blok haftalar boyunca izle — free sabitken düşerse
  send-yolu cJSON churn (D3) + HTTP client re-init şüphelidir.
- Free-heap yavaş düşüşü ↔ offline koşulda `mode.test` timeout'larıyla
  ilişkiliyse B1 sızıntısıdır.
- Task high-water-mark'ları (özellikle `sd_input`) — B4 eklendikten sonra.
- `target.offline/online` çırpınması + WiFi reconnect kadansı — B2 backoff
  öncesi/sonrası karşılaştır.
- Reset nedeni dağılımı (`esp_reset_reason`) — beklenmez BROWNOUT/WDT var mı.
