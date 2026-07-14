# Firmware Boyut Kayıtları

OTA slotu: 0x1E0000 = 1.966.080 B. Kapılar plandadır (Boyut Kapıları tablosu).
Ölçüm: `idf.py size`, IDF v5.5.2, -Os, sdkconfig.defaults.

| # | Tarih | Aşama | .bin | Slot boş | Kapı | Durum |
|---|---|---|---|---|---|---|
| 1a | 2026-07-02 | T0.1 iskelet (sk_core+sk_api: BLE+WiFi+TLS+HTTP) | 0x16DE30 = 1.498.672 B (1,43 MB) | %24 | – | bilgi |
| 1b | 2026-07-02 | T0.2 en-kötü link (üstüne esp-mqtt probe) | 0x1747B0 = 1.525.680 B (1,46 MB) | %22 | ≤1,6 MB | ✅ GEÇTİ (~150 KB pay) |

| 2 | 2026-07-02 | Faz 2 sonu (motor+dimmer+HTTP+inceleme düzeltmeleri) | 0x178C80 = 1.542.272 B (1,47 MB) | %22 | ≤1,65 MB | ✅ |
| 3 | 2026-07-02 | Faz 3 sonu (UDP+MQTT+3 sürücü+katalog) | 0x1806B0 = 1.574.576 B (1,50 MB) | %20 | ≤1,7 MB | ✅ |
| 4 | 2026-07-02 | Faz 4-6 kod-tamam (safe+recovery+config io) | 0x182C50 = 1.584.208 B (1,51 MB) | %19 | ≤1,75 MB | ✅ (~166 KB OTA payı) |

Checkpoint 1 notları:
- esp-mqtt maliyeti yalnızca ~27 KB (TLS/HTTP zaten sk_api üzerinden bağlı).
- Statik RAM: DIRAM %55,8 (.bss 96 KB) — Faz 2'de task stack'leri eklenirken izlenecek.
- Budama merdivenine gerek kalmadı; Checkpoint 2 Faz 2 sonunda (kapı ≤1,65 MB).
