/*
 * SynDimm - Multi-language Translation System
 * Supports: English (EN), German (DE), Turkish (TR)
 */

#ifndef TRANSLATIONS_H
#define TRANSLATIONS_H

const char TRANSLATIONS_JSON[] PROGMEM = R"=====(
{
  "header": {
    "title": {
      "en": "SmartKraft SynDimm",
      "de": "SmartKraft SynDimm",
      "tr": "SmartKraft SynDimm"
    },
    "chipId": {
      "en": "Chip ID",
      "de": "Chip-ID",
      "tr": "Chip ID"
    },
    "version": {
      "en": "Version",
      "de": "Version",
      "tr": "Versiyon"
    }
  },
  "nav": {
    "control": {
      "en": "Quick Control",
      "de": "Schnellsteuerung",
      "tr": "Hizli Kontrol"
    },
    "settings": {
      "en": "Modes",
      "de": "Modi",
      "tr": "Modlar"
    },
    "network": {
      "en": "Connection",
      "de": "Verbindung",
      "tr": "Baglanti"
    },
    "info": {
      "en": "Info",
      "de": "Info",
      "tr": "Info"
    }
  },
  "control": {
    "selectMode": {
      "en": "Select Your Active Mode",
      "de": "Wählen Sie Ihren aktiven Modus",
      "tr": "Aktif Modunuzu Secin"
    },
    "mode": {
      "dimmer": {
        "name": {
          "en": "Dimmer",
          "de": "Dimmer",
          "tr": "Dimmer"
        },
        "desc": {
          "en": "Control brightness levels",
          "de": "Helligkeitsstufen steuern",
          "tr": "Parlaklik seviyelerini kontrol et"
        }
      },
      "shutter": {
        "name": {
          "en": "Shutter",
          "de": "Rollladen",
          "tr": "Panjur"
        },
        "desc": {
          "en": "Control shutter position",
          "de": "Rollladenposition steuern",
          "tr": "Panjur konumunu kontrol et"
        }
      },
      "safe": {
        "name": {
          "en": "Safe",
          "de": "Tresor",
          "tr": "Safe"
        },
        "desc": {
          "en": "Counter display mode",
          "de": "Zähler-Anzeigemodus",
          "tr": "Sayac gösterim modu"
        }
      },
      "alarm": {
        "name": {
          "en": "Alarm",
          "de": "Alarm",
          "tr": "Alarm"
        },
        "desc": {
          "en": "Emergency mode",
          "de": "Notfallmodus",
          "tr": "Acil durum modu"
        }
      }
    },
    "theme": {
      "label": {
        "en": "Theme",
        "de": "Thema",
        "tr": "Tema"
      },
      "dark": {
        "en": "Dark",
        "de": "Dunkel",
        "tr": "Koyu"
      },
      "light": {
        "en": "Light",
        "de": "Hell",
        "tr": "Açik"
      }
    },
    "language": {
      "label": {
        "en": "Language",
        "de": "Sprache",
        "tr": "Dil"
      }
    }
  },
  "settings": {
    "dimmer": {
      "title": {
        "en": "Dimmer",
        "de": "Dimmer",
        "tr": "Dimmer"
      },
      "connection": {
        "en": "Connection",
        "de": "Verbindung",
        "tr": "Baglanti"
      },
      "connected": {
        "en": "Connected",
        "de": "Verbunden",
        "tr": "Bagli"
      },
      "disconnected": {
        "en": "Disconnected",
        "de": "Getrennt",
        "tr": "Bagli Degil"
      },
      "device": {
        "en": "Device",
        "de": "Gerät",
        "tr": "Cihaz"
      },
      "power": {
        "en": "Power",
        "de": "Leistung",
        "tr": "Güc"
      },
      "brightness": {
        "en": "Brightness",
        "de": "Helligkeit",
        "tr": "Parlaklik"
      },
      "dimmRatio": {
        "en": "Dimm Ratio",
        "de": "Dimm-Verhältnis",
        "tr": "Dimm Orani"
      },
      "scanNetwork": {
        "en": "Scan Network",
        "de": "Netzwerk scannen",
        "tr": "Ag Tara"
      },
      "manualIP": {
        "en": "Manual IP",
        "de": "Manuelle IP",
        "tr": "Manuel IP"
      },
      "manualIPTitle": {
        "en": "Enter Device IP",
        "de": "Geräte-IP eingeben",
        "tr": "Cihaz IP Girin"
      },
      "enterIP": {
        "en": "IP Address:",
        "de": "IP-Adresse:",
        "tr": "IP Adresi:"
      },
      "stopScanning": {
        "en": "Stop Scanning",
        "de": "Scan stoppen",
        "tr": "Taramayi Durdur"
      },
      "noDevices": {
        "en": "No devices found",
        "de": "Keine Geräte gefunden",
        "tr": "Cihaz bulunamadi"
      },
      "startScanning": {
        "en": "Start scanning to discover devices",
        "de": "Starten Sie den Scan, um Geräte zu finden",
        "tr": "Cihazlari bulmak icin taramayi baslatin"
      }
    },
    "shutter": {
      "title": {
        "en": "Shutter",
        "de": "Rollladen",
        "tr": "Panjur"
      },
      "position": {
        "en": "Position",
        "de": "Position",
        "tr": "Konum"
      },
      "status": {
        "en": "Status",
        "de": "Status",
        "tr": "Durum"
      },
      "stopped": {
        "en": "Stopped",
        "de": "Gestoppt",
        "tr": "Durdu"
      },
      "opening": {
        "en": "Opening",
        "de": "Öffnet",
        "tr": "Aciliyor"
      },
      "closing": {
        "en": "Closing",
        "de": "Schließt",
        "tr": "Kapaniyor"
      },
      "setPosition": {
        "en": "Set Position",
        "de": "Position einstellen",
        "tr": "Konum Ayarla"
      },
      "open": {
        "en": "Open",
        "de": "Öffnen",
        "tr": "Ac"
      },
      "close": {
        "en": "Close",
        "de": "Schließen",
        "tr": "Kapat"
      },
      "stop": {
        "en": "Stop",
        "de": "Stopp",
        "tr": "Durdur"
      },
      "info": {
        "en": "Use encoder to control shutter position. Rotate left to close, right to open.",
        "de": "Verwenden Sie den Encoder, um die Rollladenposition zu steuern. Links drehen zum Schließen, rechts zum Öffnen.",
        "tr": "Panjur konumunu kontrol etmek icin encoder kullanin. Kapatmak icin sola, acmak icin saga cevirin."
      }
    },
    "safe": {
      "title": {
        "en": "Safe",
        "de": "Tresor",
        "tr": "Safe"
      },
      "password": {
        "en": "Password",
        "de": "Passwort",
        "tr": "Sifre"
      },
      "enablePassword": {
        "en": "Enable Password",
        "de": "Passwort aktivieren",
        "tr": "Sifreyi Aktif Et"
      },
      "passwordConfig": {
        "en": "Password Configuration",
        "de": "Passwort-Konfiguration",
        "tr": "Sifre Yapilandirmasi"
      },
      "passwordFormat": {
        "en": "Format: L3-R12-L11-R3-B (Min: 3 steps, Max: 6 steps)",
        "de": "Format: L3-R12-L11-R3-B (Min: 3 Schritte, Max: 6 Schritte)",
        "tr": "Format: L3-R12-L11-R3-B (Min: 3 adim, Max: 6 adim)"
      },
      "passwordPlaceholder": {
        "en": "e.g., L5-R3-L10-B",
        "de": "z.B. L5-R3-L10-B",
        "tr": "örn., L5-R3-L10-B"
      },
      "oldPassword": {
        "en": "Old Password (for changes)",
        "de": "Altes Passwort (für Änderungen)",
        "tr": "Eski Sifre (degisiklikler icin)"
      },
      "oldPasswordHint": {
        "en": "Required when modifying existing password",
        "de": "Erforderlich beim Ändern des vorhandenen Passworts",
        "tr": "Mevcut sifreyi degistirirken gereklidir"
      },
      "oldPasswordPlaceholder": {
        "en": "Enter old password",
        "de": "Altes Passwort eingeben",
        "tr": "Eski sifreyi girin"
      },
      "apiConfig": {
        "en": "API Configuration",
        "de": "API-Konfiguration",
        "tr": "API Yapilandirmasi"
      },
      "enableApi": {
        "en": "Enable API",
        "de": "API aktivieren",
        "tr": "API'yi Aktif Et"
      },
      "apiUrl": {
        "en": "API URL",
        "de": "API-URL",
        "tr": "API URL"
      },
      "apiUrlPlaceholder": {
        "en": "https://example.com/api/unlock",
        "de": "https://beispiel.de/api/unlock",
        "tr": "https://ornek.com/api/unlock"
      },
      "httpMethod": {
        "en": "HTTP Method",
        "de": "HTTP-Methode",
        "tr": "HTTP Metodu"
      },
      "customHeader": {
        "en": "Custom Header (optional)",
        "de": "Benutzerdefinierter Header (optional)",
        "tr": "Özel Baslik (opsiyonel)"
      },
      "customHeaderHint": {
        "en": "Format: HeaderName: Value",
        "de": "Format: HeaderName: Wert",
        "tr": "Format: BaslikAdi: Deger"
      },
      "customHeaderPlaceholder": {
        "en": "X-API-Key: your-api-key",
        "de": "X-API-Key: ihr-api-schlüssel",
        "tr": "X-API-Key: api-anahtariniz"
      },
      "requestBody": {
        "en": "Request Body (JSON)",
        "de": "Anfrage-Body (JSON)",
        "tr": "Istek Govdesi (JSON)"
      },
      "requestBodyHint": {
        "en": "Only for POST requests",
        "de": "Nur für POST-Anfragen",
        "tr": "Sadece POST istekleri icin"
      }
    },
    "alarm": {
      "title": {
        "en": "Alarm",
        "de": "Alarm",
        "tr": "Alarm"
      },
      "description": {
        "en": "Alarm mode is a security feature designed to instantly notify your loved ones or request help in emergencies. When you touch or turn the encoder (regardless of direction or amount), a signal is sent via API to the mobile application. This signal triggers the ringtone on phones with the app installed and displays your predefined message on the screen.\n\nExample Use Case:\n\nWhen your SynDimm device is in Alarm mode and you interact with the encoder, an emergency request is automatically sent to the SynDimm mobile app on Android or iOS devices. Phones with the app installed will ring loudly and display your custom emergency message on the screen. This provides you with an intelligent solution that you can use as a panic button for your personal safety.\n\nNote: The system is currently under development and will be available soon.",
        "de": "Der Alarm-Modus ist eine Sicherheitsfunktion, die entwickelt wurde, um Ihre Angehörigen sofort zu benachrichtigen oder in Notfällen Hilfe anzufordern. Wenn Sie den Encoder berühren oder drehen (unabhängig von Richtung oder Menge), wird ein Signal über die API an die mobile Anwendung gesendet. Dieses Signal löst den Klingelton auf Telefonen mit installierter App aus und zeigt Ihre vordefinierte Nachricht auf dem Bildschirm an.\n\nBeispiel-Anwendungsszenario:\n\nWenn sich Ihr SynDimm-Gerät im Alarm-Modus befindet und Sie mit dem Encoder interagieren, wird automatisch eine Notfallanfrage an die SynDimm-Mobile-App auf Android- oder iOS-Geräten gesendet. Telefone mit installierter App klingeln laut und zeigen Ihre benutzerdefinierte Notfallnachricht auf dem Bildschirm an. Dies bietet Ihnen eine intelligente Lösung, die Sie als Panik-Button für Ihre persönliche Sicherheit verwenden können.\n\nHinweis: Das System befindet sich derzeit in der Entwicklung und wird in Kürze verfügbar sein.",
        "tr": "Alarm modu, acil durumlarda sevdiklerinize anında haber vermek veya yardım istemek için tasarlanmış bir güvenlik özelliğidir. Encoder'a dokunduğunuzda veya çevirdiğinizde (yön ve miktar fark etmeksizin), API üzerinden mobil uygulamaya sinyal gönderilir. Bu sinyal uygulamanın yüklü olduğu telefonların zil sesini tetikleyerek önceden belirlediğiniz mesajı ekranda gösterir.\n\nÖrnek Kullanım Senaryosu:\n\nSynDimm cihazınız Alarm modundayken encoder ile herhangi bir etkileşime girdiğinizde, Android veya iOS cihazlarınızdaki SynDimm mobil uygulamasına otomatik olarak acil durum isteği gönderilir. Uygulamanın yüklü olduğu telefonlar yüksek sesle çalmaya başlar ve önceden ayarladığınız özel acil durum mesajını ekranda görüntüler. Bu sayede, kişisel güvenliğiniz için bir panik butonu olarak kullanabileceğiniz akıllı bir çözüm sunar.\n\nNot: Sistem şu anda geliştirme aşamasındadır ve yakında kullanıma sunulacaktır."
      }
    },
    "saveConfig": {
      "en": "Save Mode Configuration",
      "de": "Modus-Konfiguration speichern",
      "tr": "Mod Yapilandirmasini Kaydet"
    }
  },
  "network": {
    "status": {
      "mode": {
        "en": "Mode",
        "de": "Modus",
        "tr": "Mod"
      },
      "ssid": {
        "en": "SSID",
        "de": "SSID",
        "tr": "SSID"
      },
      "ip": {
        "en": "IP Address",
        "de": "IP-Adresse",
        "tr": "IP Adresi"
      },
      "wifi": {
        "en": "WiFi",
        "de": "WLAN",
        "tr": "WiFi"
      }
    },
    "ap": {
      "title": {
        "en": "AP MODE SETTINGS",
        "de": "AP-MODUS-EINSTELLUNGEN",
        "tr": "AP MODU AYARLARI"
      },
      "active": {
        "en": "Active",
        "de": "Aktiv",
        "tr": "Aktif"
      },
      "inactive": {
        "en": "Inactive",
        "de": "Inaktiv",
        "tr": "Pasif"
      },
      "ssid": {
        "en": "SSID",
        "de": "SSID",
        "tr": "SSID"
      },
      "password": {
        "en": "Password",
        "de": "Passwort",
        "tr": "Sifre"
      },
      "noPassword": {
        "en": "None (Open Network)",
        "de": "Keine (Offenes Netzwerk)",
        "tr": "Yok (Açik Ag)"
      },
      "ip": {
        "en": "IP Address",
        "de": "IP-Adresse",
        "tr": "IP Adresi"
      },
      "mdns": {
        "en": "mDNS Address",
        "de": "mDNS-Adresse",
        "tr": "mDNS Adresi"
      },
      "autoFailover": {
        "en": "Automatic Failover",
        "de": "Automatisches Failover",
        "tr": "Otomatik Failover"
      },
      "failoverDesc": {
        "en": "Primary WiFi → Backup WiFi → AP Mode<br><br>AP Mode activates automatically. Operates as an open network without password.",
        "de": "Primäres WLAN → Backup-WLAN → AP-Modus<br><br>Der AP-Modus wird automatisch aktiviert. Funktioniert als offenes Netzwerk ohne Passwort.",
        "tr": "Primary WiFi → Backup WiFi → AP Mode<br><br>AP Mode otomatik olarak devreye girer. Sifresiz acik ag olarak calisir."
      }
    },
    "wifi1": {
      "title": {
        "en": "PRIMARY WIFI",
        "de": "PRIMÄRES WLAN",
        "tr": "PRIMARY WIFI"
      },
      "connected": {
        "en": "Connected",
        "de": "Verbunden",
        "tr": "Bagli"
      },
      "notConfigured": {
        "en": "Not Configured",
        "de": "Nicht konfiguriert",
        "tr": "Yapilandirilmadi"
      },
      "ssid": {
        "en": "SSID",
        "de": "SSID",
        "tr": "SSID"
      },
      "ssidPlaceholder": {
        "en": "Enter WiFi SSID",
        "de": "WLAN-SSID eingeben",
        "tr": "WiFi SSID girin"
      },
      "password": {
        "en": "Password",
        "de": "Passwort",
        "tr": "Sifre"
      },
      "passwordPlaceholder": {
        "en": "Enter WiFi password",
        "de": "WLAN-Passwort eingeben",
        "tr": "WiFi sifresi girin"
      },
      "staticIp": {
        "en": "Static IP (optional)",
        "de": "Statische IP (optional)",
        "tr": "Statik IP (opsiyonel)"
      },
      "staticIpPlaceholder": {
        "en": "Leave empty for DHCP",
        "de": "Für DHCP leer lassen",
        "tr": "DHCP icin bos birakin"
      },
      "localDomain": {
        "en": ".local Domain (optional)",
        "de": ".local-Domain (optional)",
        "tr": ".local Alan Adi (opsiyonel)"
      },
      "localDomainPlaceholder": {
        "en": "e.g., mysyndimm",
        "de": "z.B. meinsyndimm",
        "tr": "örn., benimsyndimm"
      }
    },
    "wifi2": {
      "title": {
        "en": "BACKUP WIFI",
        "de": "BACKUP-WLAN",
        "tr": "BACKUP WIFI"
      },
      "connected": {
        "en": "Connected",
        "de": "Verbunden",
        "tr": "Bagli"
      },
      "notConfigured": {
        "en": "Not Configured",
        "de": "Nicht konfiguriert",
        "tr": "Yapilandirilmadi"
      },
      "ssid": {
        "en": "SSID",
        "de": "SSID",
        "tr": "SSID"
      },
      "ssidPlaceholder": {
        "en": "Enter WiFi SSID",
        "de": "WLAN-SSID eingeben",
        "tr": "WiFi SSID girin"
      },
      "password": {
        "en": "Password",
        "de": "Passwort",
        "tr": "Sifre"
      },
      "passwordPlaceholder": {
        "en": "Enter WiFi password",
        "de": "WLAN-Passwort eingeben",
        "tr": "WiFi sifresi girin"
      },
      "staticIp": {
        "en": "Static IP (optional)",
        "de": "Statische IP (optional)",
        "tr": "Statik IP (opsiyonel)"
      },
      "staticIpPlaceholder": {
        "en": "Leave empty for DHCP",
        "de": "Für DHCP leer lassen",
        "tr": "DHCP icin bos birakin"
      },
      "localDomain": {
        "en": ".local Domain (optional)",
        "de": ".local-Domain (optional)",
        "tr": ".local Alan Adi (opsiyonel)"
      },
      "localDomainPlaceholder": {
        "en": "e.g., mysyndimm",
        "de": "z.B. meinsyndimm",
        "tr": "örn., benimsyndimm"
      }
    },
    "saveConfig": {
      "en": "Save Network Configuration",
      "de": "Netzwerkkonfiguration speichern",
      "tr": "Ag Yapilandirmasini Kaydet"
    }
  },
  "info": {
    "userGuide": {
      "en": "User Guide",
      "de": "Benutzerhandbuch",
      "tr": "Kullanim Kilavuzu"
    },
    "dimmerMode": {
      "title": {
        "en": "Dimmer Mode",
        "de": "Dimmer-Modus",
        "tr": "Dimmer Modu"
      },
      "encoderControl": {
        "title": {
          "en": "Encoder Control:",
          "de": "Encoder-Steuerung:",
          "tr": "Encoder ile Kontrol:"
        },
        "text": {
          "en": "Turn clockwise to increase brightness, counterclockwise to decrease. Press the encoder button to turn the connected device on/off.",
          "de": "Im Uhrzeigersinn drehen, um die Helligkeit zu erhöhen, gegen den Uhrzeigersinn, um sie zu verringern. Drücken Sie die Encoder-Taste, um das verbundene Gerät ein-/auszuschalten.",
          "tr": "Saat yönünde cevirerek parlakligt artirin, ters yönde azaltin. Encoder butonuna basarak bagli cihazi acip/kapatabilirsiniz."
        }
      },
      "deviceConnection": {
        "title": {
          "en": "Device Connection:",
          "de": "Geräteverbindung:",
          "tr": "Cihaz Baglantisi:"
        },
        "text": {
          "en": "From Settings → Dimmer section, use Scan Network to find smart dimmer devices on your network and synchronize with the Connect button.",
          "de": "Unter Einstellungen → Dimmer-Bereich können Sie mit Netzwerk scannen intelligente Dimmer-Geräte in Ihrem Netzwerk finden und mit der Schaltfläche Verbinden synchronisieren.",
          "tr": "Settings → Dimmer bölümünden Scan Network ile aginizidaki akilli dimmer cihazlarini bulun ve Connect butonuyla senkronize olun."
        }
      },
      "webControl": {
        "title": {
          "en": "Web Control:",
          "de": "Web-Steuerung:",
          "tr": "Web Kontrolü:"
        },
        "text": {
          "en": "From the Control tab, you can adjust Brightness (brightness level) and Dimm Ratio (change amount per encoder movement, 1-5 range).",
          "de": "Auf der Registerkarte Steuerung können Sie Helligkeit (Helligkeitsstufe) und Dimm-Verhältnis (Änderungsmenge pro Encoder-Bewegung, Bereich 1-5) anpassen.",
          "tr": "Control sekmesinden Brightness (parlaklik) ve Dimm Ratio (her encoder hareketi icin degisim miktari 1-5 arasi) ayarlarini yapabilirsiniz."
        }
      }
    },
    "safeMode": {
      "title": {
        "en": "Safe Lock Mode",
        "de": "Tresor-Modus",
        "tr": "Safe Lock Modu"
      },
      "passwordSystem": {
        "title": {
          "en": "Password System:",
          "de": "Passwort-System:",
          "tr": "Sifre Sistemi:"
        },
        "text": {
          "en": "Passwords are entered via encoder movements. Example: R5-L3-R2-B (right 5, left 3, right 2, button press).",
          "de": "Passwörter werden über Encoder-Bewegungen eingegeben. Beispiel: R5-L3-R2-B (rechts 5, links 3, rechts 2, Tastendruck).",
          "tr": "Encoder hareketleriyle sifre girilir. Örnek: R5-L3-R2-B (saga 5, sola 3, saga 2, buton bas)."
        }
      },
      "configuration": {
        "title": {
          "en": "Configuration:",
          "de": "Konfiguration:",
          "tr": "Ayarlama:"
        },
        "text": {
          "en": "From Settings → Safe tab, you can save 5 different passwords. Define an API URL for each password.",
          "de": "Unter Einstellungen → Tresor-Registerkarte können Sie 5 verschiedene Passwörter speichern. Definieren Sie eine API-URL für jedes Passwort.",
          "tr": "Settings → Safe sekmesinden 5 farkli sifre kaydedebilirsiniz. Her sifre icin API URL tanimlain."
        }
      },
      "triggering": {
        "title": {
          "en": "Triggering:",
          "de": "Auslösung:",
          "tr": "Tetikleme:"
        },
        "text": {
          "en": "When the correct password is entered, the API registered to that password is automatically called (HTTP GET/POST).",
          "de": "Wenn das richtige Passwort eingegeben wird, wird die für dieses Passwort registrierte API automatisch aufgerufen (HTTP GET/POST).",
          "tr": "Dogru sifre girildiginde, o sifreye kayitli API otomatik olarak cagrilir (HTTP GET/POST)."
        }
      },
      "useCases": {
        "title": {
          "en": "Use Cases:",
          "de": "Anwendungsfälle:",
          "tr": "Kullanim Alanlari:"
        },
        "text": {
          "en": "Smart lock opening, garage door control, custom automation scenarios.",
          "de": "Intelligentes Türschloss öffnen, Garagentorsteuerung, benutzerdefinierte Automatisierungsszenarien.",
          "tr": "Akilli kilit acma, garaj kapisi kontrolü, özel otomasyon senaryolari."
        }
      }
    },
    "alarmMode": {
      "title": {
        "en": "Alarm Mode",
        "de": "Alarm-Modus",
        "tr": "Alarm Modu"
      },
      "emergencyNotification": {
        "title": {
          "en": "Emergency Notification:",
          "de": "Notfall-Benachrichtigung:",
          "tr": "Acil Bildirim:"
        },
        "text": {
          "en": "When you touch or turn the encoder (direction/amount irrelevant), a signal is sent to the mobile app via API.",
          "de": "Wenn Sie den Encoder berühren oder drehen (Richtung/Menge irrelevant), wird ein Signal über die API an die mobile App gesendet.",
          "tr": "Encoder'a dokundugunuzda veya cevirdiginizde (yön/miktar önemsiz) API üzerinden mobil uygulamaya sinyal gönderilir."
        }
      },
      "function": {
        "title": {
          "en": "Function:",
          "de": "Funktion:",
          "tr": "Islev:"
        },
        "text": {
          "en": "Ringtone is triggered on the target phone and emergency message is delivered.",
          "de": "Der Klingelton wird auf dem Zieltelefon ausgelöst und die Notfallnachricht wird übermittelt.",
          "tr": "Hedef telefonda zil sesi tetiklenir ve acil durum mesaji iletilir."
        }
      },
      "warning": {
        "en": "Alarm mode is under development.",
        "de": "Der Alarm-Modus befindet sich in der Entwicklung.",
        "tr": "Alarm modu gelistirme asmasindadir."
      }
    },
    "networkSettings": {
      "title": {
        "en": "Network Settings",
        "de": "Netzwerkeinstellungen",
        "tr": "Ag Ayarlari"
      },
      "wifiConnection": {
        "title": {
          "en": "WiFi Connection:",
          "de": "WLAN-Verbindung:",
          "tr": "WiFi Baglantisi:"
        },
        "text": {
          "en": "Connect to a WiFi network by entering SSID and password from the Network tab.",
          "de": "Verbinden Sie sich mit einem WLAN-Netzwerk, indem Sie SSID und Passwort auf der Registerkarte Netzwerk eingeben.",
          "tr": "Network sekmesinden SSID ve sifre girerek WiFi agina baglanin."
        }
      },
      "ipConfiguration": {
        "title": {
          "en": "IP Configuration:",
          "de": "IP-Konfiguration:",
          "tr": "IP Yapilandirmasi:"
        },
        "text": {
          "en": "You can use DHCP (automatic) or Static IP options.",
          "de": "Sie können DHCP (automatisch) oder statische IP-Optionen verwenden.",
          "tr": "DHCP (otomatik) veya Statik IP seceneklerini kullanabilirsiniz."
        }
      }
    },
    "otaSettings": {
      "title": {
        "en": "Version Update",
        "de": "Versions-Update",
        "tr": "Versiyon Güncelleme"
      },
      "currentVersion": {
        "en": "Device Version:",
        "de": "Geräteversion:",
        "tr": "Cihaz Sürümü:"
      },
      "latestVersion": {
        "en": "New Version:",
        "de": "Neue Version:",
        "tr": "Yeni Sürüm:"
      },
      "autoUpdate": {
        "en": "Auto Update",
        "de": "Automatisches Update",
        "tr": "Otomatik Güncelleme"
      },
      "updateNow": {
        "en": "Update Now",
        "de": "Jetzt aktualisieren",
        "tr": "Simdi Güncelle"
      },
      "description": {
        "en": "When auto-update is enabled, the device checks for new versions and updates itself. When disabled, you only receive notifications.",
        "de": "Wenn das automatische Update aktiviert ist, überprüft das Gerät auf neue Versionen und aktualisiert sich selbst. Wenn deaktiviert, erhalten Sie nur Benachrichtigungen.",
        "tr": "Otomatik güncelleme acik oldugunda cihaz yeni sürümleri kontrol eder ve kendini günceller. Kapali oldugunda sadece bildirim alirsiniz."
      }
    },
    "documentation": {
      "title": {
        "en": "Support and Documentation",
        "de": "Support und Dokumentation",
        "tr": "Destek ve Dokümantasyon"
      },
      "description": {
        "en": "For detailed user guide, example scenarios, and updates:",
        "de": "Für detailliertes Benutzerhandbuch, Beispielszenarien und Updates:",
        "tr": "Detayli kullanim kilavuzu, örnek senaryolar ve güncellemeler icin:"
      },
      "smartkraftButton": {
        "en": "SmartKraft.ch",
        "de": "SmartKraft.ch",
        "tr": "SmartKraft.ch"
      },
      "githubButton": {
        "en": "GitHub",
        "de": "GitHub",
        "tr": "GitHub"
      }
    }
  },
  "common": {
    "save": {
      "en": "Save",
      "de": "Speichern",
      "tr": "Kaydet"
    },
    "cancel": {
      "en": "Cancel",
      "de": "Abbrechen",
      "tr": "Iptal"
    },
    "connect": {
      "en": "Connect",
      "de": "Verbinden",
      "tr": "Baglan"
    },
    "disconnect": {
      "en": "Disconnect",
      "de": "Trennen",
      "tr": "Baglanti Kes"
    },
    "success": {
      "en": "Success",
      "de": "Erfolgreich",
      "tr": "Basarili"
    },
    "error": {
      "en": "Error",
      "de": "Fehler",
      "tr": "Hata"
    },
    "loading": {
      "en": "Loading...",
      "de": "Laden...",
      "tr": "Yükleniyor..."
    }
  }
}
)=====";

#endif
