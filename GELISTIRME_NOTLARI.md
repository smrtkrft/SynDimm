# SynDimm v0.9.1 - Geliştirme Notları
**Tarih:** 19 Kasım 2025

---ReStart ve Facktory Reset butonlari eklenecek

✅ Shelly Dimmer 1, 2, L (Gen1)
✅ Shelly Dimmer 2 Gen3 (SNSW-001P16EU)
✅ Shelly 0-10V Dimmer Gen3
✅ Shelly 1-10V Dimmer Gen3
✅ Shelly Plus Dimmer 0-10V PM
✅ Shelly Plus Dimmer PM
✅ Shelly Pro Dimmer
✅ Shelly DALI Dimmer Gateway
✅ Shelly Duo RGBW (bulb)
✅ Shelly Vintage (bulb)
✅ Shelly RGBW2 (white mode)
✅ Shelly Dimmer SL

## 📋 GELECEK ÖZELLİKLER

### 1. Çoklu Dimmer Kontrol Modu
- **Dosya:** `SK_mode_multi.h` (yeni oluşturulacak)
- **Özellikler:**
  - Birden fazla dimmer'ı grup halinde kontrol
  - Encoder ile dimmer seçimi
  - Grup halinde parlaklık ayarı
  - Senkronize açma/kapama
  - Preset'ler (tüm ışıklar, salon, yatak odası, vb.)

### 2. Developer Mode (Geliştirici Modu)
- **Dosya:** `SK_mode_dev.h` (yeni oluşturulacak)
- **Özellikler:**
  - Gerçek zamanlı log görüntüleme
  - Network diagnostics (ping, traceroute)
  - HTTP istek/yanıt detayları
  - Bellek kullanımı monitörü
  - Komut test arayüzü
  - Web arayüzüne dev paneli

---

## 🐛 MEVCUT SORUNLAR VE ÇÖZÜMLER

### SORUN 1: Dimmer Keşif Süresi (Ortalama 25 Dakika)

**Mevcut Durum:**
```cpp
// SK_dimmer.h - scanNetwork() fonksiyonu
// Sıralı tarama: 254 IP × 8 saniye timeout = ~34 dakika (en kötü durum)
for (int i = 1; i <= 254; i++) {
    String testIP = subnet + String(i);
    ShellyModelInfo modelInfo = detectShellyModel(testIP);
    // Her IP için 8 saniye bekliyor
    yield();
    delay(10);
}
```

**Neden Uzun Süruyor:**
- 254 IP adresi tek tek taranıyor (sıralı)
- Her başarısız bağlantı 8 saniye bekliyor
- Gen3 + Gen1 kontrolü = IP başına 16 saniye olabilir
- Ağda olmayan cihazlar için tam timeout bekleniyor
/// seu - öneri /// ilk tarama ip adresi dolu mu bos mu 
/// ikinci tarama agdaki baglanti shelly dimmer mi degil mi 

**Çözüm Önerileri:**
1. **Paralel Tarama:** 5-10 IP'yi eşzamanlı tara
2. **Timeout Azaltma:** 8 saniye → 2-3 saniye
3. **Retry Mekanizması:** 2-3 deneme, hızlı fail
4. **mDNS Keşfi:** Shelly cihazlarını servis keşfi ile bul (`_http._tcp.local`)
5. **Cihaz Önbellekleme:** Daha önce bulunan cihazları hatırla
6. **Progresif Tarama:** İlk hızlı tarama (1s timeout), sonra derin tarama

**Beklenen İyileşme:** 25 dakika → 2-3 dakika

---

### SORUN 2: Bazı Dimmer'ları Bulamama

**Neden Kaçırıyor:**
- Tek deneme - retry yok
- Ağ gecikmeleri
- Cihazlar geçici olarak yanıt vermiyor
- Timeout çok agresif (8 saniye yine de bazı ağlar için yetersiz)
- HTTP istekleri blocking - `yield()` yetersiz

**Çözüm:**
- Retry mekanizması ekle (2-3 deneme)
- Timeout'ları optimize et (2-3 saniye × 3 deneme)
- Bulunan cihazları kaydet, sonraki taramalarda önce onları kontrol et
- mDNS ile otomatik keşif

---

### SORUN 3: Açma/Kapama 11 Döngü Sorunu

**Mevcut Kod:**
```cpp
// SK_dimmer.h - toggleShelly() fonksiyonu (satır 740-760)
bool toggleShelly() {
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + "/rpc/Light.Toggle?id=0";
    
    http.begin(url);
    http.setTimeout(DIMMER_REQUEST_TIMEOUT);  // 5000ms
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        dimmerDevice.isOn = !dimmerDevice.isOn;  // SORUN: Hemen değiştiriyor
        getShellyStatus(); // SORUN: Anında doğrulama çağrısı
        return true;
    }
    return false;
}
```

**Sorun Analizi:**
1. Toggle komutu gönderiliyor
2. Cihaz durum değişikliğini henüz yapmadan state değiştiriliyor
3. `getShellyStatus()` hemen çağrılıyor → ESKİ durumu görüyor
4. State geri dönüyor (race condition)
5. Bu 11 kez tekrar ediyor, ta ki timing uyuşana kadar

**Neden 11 Döngü:**
- Shelly cihazların yanıt süresi: ~100-500ms
- Verification anında yapılıyor (bekleme yok)
- State senkronizasyonu yok
- Her denemede %10-20 şans, ortalama ~11 deneme

**Çözüm Önerileri:**
```cpp
bool toggleShelly() {
    HTTPClient http;
    String url = "http://" + dimmerDevice.ip + "/rpc/Light.Toggle?id=0";
    
    http.begin(url);
    http.setTimeout(5000);
    
    int httpCode = http.GET();
    http.end();
    
    if (httpCode == HTTP_CODE_OK) {
        // ÇÖZÜM 1: Bekleme ekle
        delay(300);  // Cihazın durumu değiştirmesi için zaman tanı
        
        // ÇÖZÜM 2: Polling ile doğrula
        for (int retry = 0; retry < 5; retry++) {
            getShellyStatus();
            if (dimmerDevice.isOn != previousState) {
                return true;  // Durum değişti
            }
            delay(100);  // Kısa bekle, tekrar kontrol et
        }
    }
    return false;
}
```

**Alternatif Çözüm - Exponential Backoff:**
```cpp
// 3 deneme: 100ms, 200ms, 400ms
for (int attempt = 0; attempt < 3; attempt++) {
    if (sendToggleCommand()) {
        delay(100 * pow(2, attempt));
        if (verifyStateChange()) {
            return true;
        }
    }
}
```

---

## 🏗️ MİMARİ İYİLEŞTİRMELER

### Komut Kuyruğu Sistemi
**Sorun:** Eşzamanlı dimmer işlemleri çakışıyor (toggle + status update)

**Çözüm:**
```cpp
// Yeni dosya: SK_command_queue.h
struct DimmerCommand {
    enum Type { TOGGLE, SET_BRIGHTNESS, GET_STATUS };
    Type type;
    String targetIP;
    int value;
    unsigned long timestamp;
};

class CommandQueue {
    std::queue<DimmerCommand> commands;
    bool processing = false;
    
    void enqueue(DimmerCommand cmd);
    void processNext();
    void loop();  // Ana loop'ta çağrılacak
};
```

### Mod Sistemi Genişletme
**Mevcut:** Sadece Safe Mode var  
**Hedef:** Çoklu mod desteği

```cpp
// Yeni dosya: SK_mode_base.h
class ModeBase {
public:
    virtual void init() = 0;
    virtual void handleEncoder(char event) = 0;
    virtual void handleWeb() = 0;
    virtual String getName() = 0;
};

// SK_mode_manager.h
class ModeManager {
    ModeBase* modes[4];  // Safe, Multi, Dev, Normal
    int currentMode = 0;
    
    void switchMode(int newMode);
    void handleEncoderLongPress();  // Mod değiştir
};
```

---

## 📊 TEKNİK DETAYLAR

### Timeout Değerleri (Mevcut)
| İşlem | Timeout | Konum |
|-------|---------|-------|
| Dimmer Keşfi (IP başına) | 8000ms | `detectShellyModel()` |
| Dimmer Kontrolü | 5000ms | `DIMMER_REQUEST_TIMEOUT` |
| Durum Güncelleme | 10000ms | `getShellyStatus()` |
| Safe Mode API | 1000ms | `SK_mode_safe_api.h` |
| OTA Güncelleme | 10000ms | `SK_ota.h` |

### Önerilen Yeni Timeout Değerleri
| İşlem | Yeni Timeout | Retry | Toplam Max |
|-------|--------------|-------|------------|
| Dimmer Keşfi | 2000ms | 3 | 6000ms |
| Dimmer Kontrolü | 3000ms | 3 | 9000ms |
| Durum Güncelleme | 5000ms | 2 | 10000ms |

### Bellek Optimizasyonu
**Sorun:** `std::vector<SavedDevice> savedDevices` sınırsız büyüyebilir

**Çözüm:**
```cpp
#define MAX_SAVED_DEVICES 30  // Bellek sınırı

void addDevice(SavedDevice device) {
    if (savedDevices.size() >= MAX_SAVED_DEVICES) {
        // En eski veya en az kullanılanı sil
        savedDevices.erase(savedDevices.begin());
    }
    savedDevices.push_back(device);
}
```
//////// seu eksi listlemedeki cihazlar silinebilir olmali , gereksizse silinmeli.
///////// elle girilen cihazlarda listelenmeli
---

## 🎯 UYGULAMA ÖNCELİĞİ

### Faz 1: Kritik Sorun Çözümleri (ÖNCE)
1. ✅ **Toggle Fix** (11 döngü sorunu) - 300ms delay + retry
2. ✅ **Keşif Optimizasyonu** - Timeout azaltma + retry

### Faz 2: Performans İyileştirmeleri
3. ✅ **Paralel Tarama** - 5-10 IP eşzamanlı
4. ✅ **mDNS Keşfi** - Otomatik cihaz bulma
5. ✅ **Komut Kuyruğu** - Race condition önleme

### Faz 3: Yeni Özellikler
6. 🔲 **Çoklu Dimmer Modu** - Grup kontrolü
7. 🔲 **Developer Mode** - Debug araçları
8. 🔲 **Mod Yöneticisi** - Encoder ile mod geçişi

### Faz 4: İyileştirmeler
9. 🔲 **Bellek Optimizasyonu** - Buffer sınırları
10. 🔲 **Web UI İyileştirme** - Mod seçimi, canlı log

---///////// seu mod secimi ve buzzer henüz yok encoder 3 saniye sonra mod degistirme ekele sonra 3 saniye basili tutma ve onaylama

## 💡 EK ÖNERİLER

### mDNS Servis Keşfi Örneği
```cpp
#include <ESPmDNS.h>

void discoverShellyDevices() {
    int n = MDNS.queryService("http", "tcp");
    for (int i = 0; i < n; i++) {
        String hostname = MDNS.hostname(i);
        if (hostname.startsWith("shelly")) {
            IPAddress ip = MDNS.IP(i);
            int port = MDNS.port(i);
            // Cihazı listeye ekle
        }
    }
}
```

### Connection Pooling
```cpp
// HTTP bağlantılarını yeniden kullan
class ConnectionPool {
    std::map<String, HTTPClient*> connections;
    
    HTTPClient* get(String ip) {
        if (connections.find(ip) == connections.end()) {
            connections[ip] = new HTTPClient();
        }
        return connections[ip];
    }
};
```

### Watchdog Yönetimi
```cpp
// Uzun işlemlerde watchdog besle
void scanNetworkSafe() {
    for (int i = 1; i <= 254; i++) {
        yield();  // Her 10 IP'de bir
        if (i % 10 == 0) {
            esp_task_wdt_reset();  // Watchdog'u sıfırla
        }
        // Tarama işlemi
    }
}
```

---

## 📝 NOTLAR

- ESP32 RAM sınırı: 320KB (SRAM)
- Maximum eşzamanlı HTTP bağlantısı: ~5-10 (bellek bağımlı)
- Encoder uzun basma süresi: 1000ms (ayarlanabilir)
- EEPROM alanı Safe Mode için: 2048+ offset
- Preferences alanı WiFi/config için: NVS

---

## 🔗 İLGİLİ DOSYALAR

- `SynDimm.ino` - Ana program
- `SK_dimmer.h` - Dimmer kontrolü (SORUN 1, 2, 3)
- `SK_encoder.h` - KY-040 encoder
- `SK_mode_safe.h` - Safe mode implementasyonu
- `SK_webserver.h` - Web arayüzü
- `SKwifi.h` - WiFi yönetimi

---

## ⚠️ DİKKAT EDİLMESİ GEREKENLER

1. **Test Ortamı:** Her değişiklik test edilmeden production'a alınmamalı
2. **Geri Dönüş:** `yedek/` klasöründe mevcut çalışan versiyon korunmalı
3. **Bellek:** `Serial.println("Free heap: " + String(ESP.getFreeHeap()))` ile izle
4. **Watchdog:** Uzun işlemlerde `yield()` ve `esp_task_wdt_reset()` kullan
5. **HTTP Cleanup:** Her HTTP işlemi sonrası `http.end()` çağrılmalı

---

**Son Güncelleme:** 19 Kasım 2025  
**Versiyon:** v0.9.1  
**Durum:** Planlama Aşaması
