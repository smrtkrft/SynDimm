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
