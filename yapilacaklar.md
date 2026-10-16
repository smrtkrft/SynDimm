# GUI'ye eklenebilecek donanım özellikleri

Donanımda var, arayüzde yok. Hangilerini istediğinizi seçin:

1. Kasa profilleri yönetimi. Şifre dizisi ve açılınca tetiklenecek HTTP isteği. Şu an yalnızca API ile.
2. Uzaktan yeniden başlatma anahtarı. Anahtar üret, aç/kapat, dış sistemden `GET /api/reset?key=` ile cihazı yeniden başlat. Artık gerçekten çalışıyor.
3. MQTT broker ayarı. Donanımda `device_driver_set_mqtt_config` var ama ne API ne GUI karşılığı var.
4. Cihaz kimlik bilgileri kartı. `/api/system/info` içindeki cihaz adı, AP IP, STA IP, mDNS adı tek ekranda.
5. Profil JSON yükleme. Kendi profil dosyasını arayüzden gönderme (`/api/profiles/upload`).
6. "trigger_api" bayrağı. Donanımda kaydediliyor ama hiçbir şey okumuyor. Önerim kaldırmak.
