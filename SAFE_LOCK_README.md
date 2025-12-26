# 🔐 SynDimm Safe Lock System

## Genel Bakış

SynDimm Safe Lock, rotary encoder (KY040) kullanılarak kasa kilidi mantığında çalışan güvenlik sistemidir. Kullanıcı encoder'ı çevirerek ve butona basarak şifre girebilir, doğru şifre girildiğinde API tetiklenir.

## 🎯 Özellikler

### ✅ Temel Özellikler
- **5 Farklı Şifre**: Her biri ayrı API tetiklemesi yapabilir
- **Akıllı Buffer Sistemi**: Son 10 hareketi tutar, sürekli kontrol eder
- **Sınırsız Deneme**: Hata sayacı veya blokaj yok
- **Progressive Matching**: Şifreler adım adım eşleştirilir
- **Zaman Sınırı Yok**: Kullanıcı istediği kadar bekleyebilir

### 🔒 Şifre Formatı
```
Format: L<tik>-R<tik>-L<tik>-B
Örnek: L13-R2-L14-R11-B
       R5-L2-R4-L12

L = Left (Sola çevirme)
R = Right (Sağa çevirme)
B = Button (Buton basma - opsiyonel)

Min: 3 adım
Max: 6 adım
Tık aralığı: 1-50
```

### 🌐 API Tetikleme
- **HTTP Method**: GET veya POST
- **Custom Headers**: API key desteği
- **Request Body**: POST için JSON body
- **Retry**: 3 deneme, her biri 2 saniye timeout
- **Buzzer Feedback**: Her durum için farklı ses

## 🔊 Buzzer Sinyalleri

### GPIO Pin
```cpp
#define BUZZER_PIN 13  // GPIO13 (D7)
```

### Ses Kodları
| Durum | Ses Paterni | Açıklama |
|-------|-------------|----------|
| **Doğru Şifre** | 1 uzun dit (500ms) | Şifre eşleşti, API tetikleniyor |
| **API Başarılı** | 2 kısa + 1 uzun dit | API başarıyla çalıştı |
| **API Timeout** | 5 kısa dit | API yanıt vermedi |
| **WiFi Hatası** | 1 uzun + 3 uzun dit | WiFi bağlantısı yok |

## 📁 Dosya Yapısı

```
SynDimm/
├── SK_mode_safe.h          # Ana kütüphane (buffer, şifre kontrolü, Preferences + LittleFS)
├── SK_mode_safe_api.h      # API tetikleme modülü
├── SynDimm.ino             # Ana program
├── SK_mode_manager.h       # Mod yönetimi (Safe Lock entegrasyonu)
├── SK_webserver.h          # Web arayüzü endpoint'leri
├── SK_html.h               # Web arayüzü HTML
├── SK_css.h                # Web arayüzü CSS
└── SK_js.h                 # Web arayüzü JavaScript
```

## 🚀 Kurulum ve Kullanım

### 1. İlk Kurulum
```cpp
// SynDimm.ino içinde otomatik başlatılır
safeLock.begin();                         // Preferences (NVS) + LittleFS başlat
safeApiHandler.setSafeLock(&safeLock);    // API handler bağla
webServer.setSafeLock(&safeLock);         // Web server bağla
```

### 2. Web Arayüzünden Şifre Oluşturma
1. `http://syndimm-XXXXXX.local` adresine gidin
2. **Modlar** sekmesini açın
3. **Safe** akordionunu genişletin
4. Password 1-5 sekmelerinden birini seçin
5. Şifre formatını girin: `L5-R3-L10-B`
6. API URL'sini girin
7. **Save Mode Configuration** butonuna basın

### 3. Şifre Girişi
1. **Safe** modunu aktif edin
2. Encoder'ı şifre kombinasyonuna göre çevirin
   - Sola 5 tık → Sağa 3 tık → Sola 10 tık
3. Şifrede `B` varsa butona basın
4. Doğru şifrede buzzer çalar ve API tetiklenir

## 🧠 Çalışma Mantığı

### Buffer Sistemi
```cpp
// Son 10 hareket circular buffer'da tutulur
Movement moveBuffer[10];

// Örnek:
Kullanıcı: L1 R2 L2 R2 L2 R2
Şifre:         R2 L2 R2 L2 R2
               └──────────────┘
               Bu kısım eşleşir!
```

### Progressive Matching
```cpp
1. İlk hareket → 5 şifreden 3'ü eşleşti
2. İkinci hareket → 3 şifreden 2'si eşleşti
3. Üçüncü hareket → 2 şifreden 1'i eşleşti
4. Son hareket → Tek şifre eşleşti → API tetikle!
```

### Ardışık Hareket Birleştirme
```
Girilen: L2 L3 L1
Sistem:  L6 olarak kabul eder

Girilen: R5 R2
Sistem:  R7 olarak kabul eder
```

## 📡 API Endpoint'leri

### Konfigürasyon Al
```
GET /api/safe/config
Response: {
  "passwords": [
    {
      "index": 0,
      "enabled": true,
      "password": "L5-R3-L10-B",
      "api": {
        "enabled": true,
        "url": "https://example.com/unlock",
        "method": "GET",
        "header": "X-API-Key: abc123",
        "body": "{\"action\":\"unlock\"}"
      }
    },
    ...
  ]
}
```

### Şifre Ayarla
```
GET /api/safe/password/set?index=0&password=L5-R3-L10-B&old=L2-R2-L2
Response: {"success": true}
```

### API Test
```
POST /api/safe/test
Body: {
  "index": 0,
  "url": "https://example.com/test",
  "method": "GET",
  "header": "X-API-Key: test",
  "body": ""
}
Response: {"success": true, "message": "API test completed"}
```

### Şifre Aktif/Pasif
```
GET /api/safe/password/toggle?index=0&enabled=true
Response: {"success": true}
```

## 🔧 Depolama Yapısı (Preferences + LittleFS)

### NVS (Preferences) - Küçük Veriler
```cpp
Namespace: "safelock"
Keys:
  pwd_0..4      : String  // Şifre pattern'leri ("L5-R3-B")
  active_0..4   : Bool    // Şifre aktif mi?
  hasapi_0..4   : Bool    // API config var mı?
```

### LittleFS - Büyük Veriler (Sınırsız)
```cpp
Dizin: /safe/
Dosyalar:
  api_0.json    // Password 1 API config
  api_1.json    // Password 2 API config
  api_2.json    // Password 3 API config
  api_3.json    // Password 4 API config
  api_4.json    // Password 5 API config
```

## 🎨 Web Arayüzü Özellikleri

### Password Tabs
- 5 ayrı sekme (Password 1-5)
- Her sekme bağımsız konfigürasyon
- Enable/Disable toggle

### Password Configuration
- Şifre girişi (real-time validation)
- Eski şifre girişi (değişiklik için)
- Format kontrolü

### API Configuration
- Enable/Disable toggle
- URL girişi
- Method seçimi (GET/POST)
- Custom header
- Request body (POST için)

### Test Özellikleri
- Password test modu
- API test modu
- Sonuç gösterimi

## 🐛 Debug

### Serial Monitor Çıktıları
```
[SafeLock] LittleFS hazir
[SafeLock] Preferences yukleniyor...
[SafeLock] Sifre 0 yuklendi: L5-R3-L10-B (Aktif: Evet)
L5 Buffer (3/10): L5-R3-L10
R3 Buffer (4/10): L5-R3-L10-R3
[SafeLock] Password matched: #0
[API] GET: https://example.com/unlock
[API] HTTP Code: 200
[API] ✓ API başarıyla tetiklendi
```

### Debug API
```
GET /api/safe/debug
→ Serial Monitor'da tüm şifreleri yazdırır
```

## 📊 Performans

- **Buffer Boyutu**: 8 hareket (sliding window)
- **Şifre Kontrolü**: Her harekette (real-time)
- **API Timeout**: 1 saniye (watchdog güvenliği)
- **API Retry**: 1 deneme
- **Preferences Yazma**: Sadece konfigürasyon değişikliğinde
- **LittleFS Yazma**: API config değişikliğinde
- **RAM Kullanımı**: ~1KB (şifre pattern'leri NVS'te)

## ⚠️ Güvenlik Notları

1. **Şifre Değiştirme**: Web arayüzünden kolayca değiştirilebilir
2. **NVS Depolama**: Şifre pattern'leri plaintext, API config'ler LittleFS'te JSON
3. **Web Arayüzü**: Sadece lokal AP üzerinden erişilebilir (internet erişimi YOK)
4. **API Keys**: HTTPS kullanmanız önerilir

## 🎯 Örnek Kullanım Senaryoları

### 1. Akıllı Ev Kilidi
```
Şifre: L10-R5-L3-B
API: POST https://home.local/api/door/unlock
Header: X-API-Key: secret123
Body: {"door": "main", "action": "unlock"}
```

### 2. Garaj Kapısı
```
Şifre: R12-L8-R15
API: GET http://192.168.1.50/garage/open
```

### 3. Alarm Sistemi Devre Dışı
```
Şifre: L5-R5-L5-R5-B
API: POST https://alarm.local/api/disable
Header: Authorization: Bearer token123
```

### 4. IoT Cihaz Tetikleme
```
Şifre: R3-L7-R11-L4
API: GET http://shelly.local/relay/0?turn=on
```

### 5. Webhook Tetikleme
```
Şifre: L8-R8-L8-B
API: POST https://maker.ifttt.com/trigger/safe_unlock/with/key/...
Body: {"value1": "Safe unlocked", "value2": "2025-10-25"}
```

## 📝 Geliştirme Notları

### Gelecek Özellikler (Opsiyonel)
- [ ] NVS şifreleme (AES-128)
- [ ] Master şifre sistemi
- [ ] Şifre geçmişi logları
- [ ] Çoklu API tetikleme (1 şifre → N API)
- [ ] Zaman bazlı şifre aktifleştirme
- [ ] NFC/RFID entegrasyonu
- [ ] Telegram bot entegrasyonu

### Bilinen Sınırlamalar
- Maksimum 5 şifre (artırılabilir)
- Maksimum 6 adım (artırılabilir)
- Tek encoder desteği
- WiFi gerekli (API için)

---

**Geliştirici**: SmartKraft  
**Versiyon**: 1.0  
**Tarih**: Ekim 2025  
**Lisans**: Özel kullanım

🔐 **Safe Lock ile cihazlarınızı güvende tutun!**
