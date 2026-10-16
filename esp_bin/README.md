# esp_bin — build klasörü

Tüm ESP-IDF build çıktıları burada oluşur. `Code/` klasörüne hiçbir üretilen dosya yazılmaz.

| Yol | İçerik |
|---|---|
| `HW/CMakeLists.txt` | Sarmalayıcı proje. Kaynakları `Code/HW` ve `Code/GUI`'den alır. |
| `HW/build/` | idf.py çıktısı (bootloader, partition table, syndimm.bin, elf, map) |
| `HW/sdkconfig` | Üretilen yapılandırma (git'e girmez) |
| `HW/managed_components/` | Component manager indirmeleri (git'e girmez) |
| `HW/dependencies.lock` | Bağımlılık sürüm kilidi (git'te tutulur) |
| `firmware/` | Sürüm adıyla kopyalanmış `syndimm_vX.Y.Z.bin` dosyaları |

## Kullanım

```sh
./esp_bin/build.sh                 # build
./esp_bin/build.sh flash monitor   # yükle ve seri izle
./esp_bin/build.sh menuconfig
./esp_bin/build.sh fullclean
```

ESP-IDF varsayılan yolu `~/esp/esp-idf-v5.5.2`. Farklıysa `IDF_DIR=/yol ./esp_bin/build.sh`.

Firmware sürümü tek yerden gelir: `Code/HW/CMakeLists.txt` içindeki `PROJECT_VER`.
