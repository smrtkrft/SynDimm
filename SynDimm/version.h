/*
 * SynDimm - Version Information
 * 
 * VERSION DEĞİŞTİRME:
 * Sadece bu dosyadaki FIRMWARE_VERSION değerini değiştir!
 * Tüm sistem (OTA, Web UI, Serial) buradan okur.
 * 
 * Semantic Versioning: MAJOR.MINOR.PATCH
 * - MAJOR: Uyumsuz API değişiklikleri (1.0.0 -> 2.0.0)
 * - MINOR: Geriye uyumlu yeni özellikler (1.0.0 -> 1.1.0)
 * - PATCH: Geriye uyumlu bug fixler (1.0.0 -> 1.0.1)
 */

#ifndef VERSION_H
#define VERSION_H

// ============================================
// VERSION BURADAN DEĞİŞTİRİLİR!
// ============================================
#define FIRMWARE_VERSION "1.0.0"
// ============================================

// Build bilgileri (opsiyonel)
#define BUILD_DATE __DATE__
#define BUILD_TIME __TIME__

// Proje bilgileri
#define PROJECT_NAME "SynDimm"
#define PROJECT_AUTHOR "SmartKraft"
#define PROJECT_URL "https://github.com/smrtkrft/SynDimm"

#endif
