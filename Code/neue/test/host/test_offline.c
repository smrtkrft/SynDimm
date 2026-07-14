// sd_offline birim testleri (host, assert tabanlı, sahte saat).

#include <assert.h>
#include <stdio.h>

#include "sd_offline.h"

static void test_three_failures_then_offline(void)
{
    sd_offline_t o;
    sd_offline_init(&o, 3);

    assert(sd_offline_feed(&o, 0, false, 1000) == SD_OFFLINE_NONE);
    assert(sd_offline_feed(&o, 0, false, 1100) == SD_OFFLINE_NONE);
    assert(!sd_offline_is_offline(&o, 0));
    assert(sd_offline_feed(&o, 0, false, 1200) == SD_OFFLINE_WENT);   // 3.
    assert(sd_offline_is_offline(&o, 0));
    // Offline'dayken ek hata olay üretmez.
    assert(sd_offline_feed(&o, 0, false, 6300) == SD_OFFLINE_NONE);
}

static void test_success_resets_counter(void)
{
    sd_offline_t o;
    sd_offline_init(&o, 3);
    sd_offline_feed(&o, 0, false, 1000);
    sd_offline_feed(&o, 0, false, 1100);
    assert(sd_offline_feed(&o, 0, true, 1200) == SD_OFFLINE_NONE);   // sayaç sıfır
    sd_offline_feed(&o, 0, false, 1300);
    sd_offline_feed(&o, 0, false, 1400);
    assert(!sd_offline_is_offline(&o, 0));                            // 2 < 3
}

static void test_probe_gating(void)
{
    sd_offline_t o;
    sd_offline_init(&o, 3);
    // Online → her gönderim serbest.
    assert(sd_offline_should_send(&o, 0, 1000));
    assert(sd_offline_should_send(&o, 0, 1001));
    // Offline'a düşür (t=2000'de).
    sd_offline_feed(&o, 0, false, 1800);
    sd_offline_feed(&o, 0, false, 1900);
    sd_offline_feed(&o, 0, false, 2000);
    assert(sd_offline_is_offline(&o, 0));
    // 5 sn dolmadan probe yok.
    assert(!sd_offline_should_send(&o, 0, 2000 + 4999));
    // 5 sn'de bir probe.
    assert( sd_offline_should_send(&o, 0, 2000 + 5000));
    assert(!sd_offline_should_send(&o, 0, 2000 + 5001));   // kapı yeniden kuruldu
    assert( sd_offline_should_send(&o, 0, 2000 + 10001));
}

static void test_recovery_via_probe(void)
{
    sd_offline_t o;
    sd_offline_init(&o, 3);
    sd_offline_feed(&o, 0, false, 1000);
    sd_offline_feed(&o, 0, false, 1100);
    sd_offline_feed(&o, 0, false, 1200);
    assert(sd_offline_is_offline(&o, 0));
    // Başarılı probe → geri geldi.
    assert(sd_offline_feed(&o, 0, true, 9000) == SD_OFFLINE_CAME);
    assert(!sd_offline_is_offline(&o, 0));
    assert(sd_offline_should_send(&o, 0, 9001));   // normal akış
}

static void test_wifi_down_up(void)
{
    sd_offline_t o;
    sd_offline_init(&o, 3);
    // Slot 1 zaten offline; 0 ve 2 online.
    sd_offline_feed(&o, 1, false, 100);
    sd_offline_feed(&o, 1, false, 200);
    sd_offline_feed(&o, 1, false, 300);
    uint32_t newly = sd_offline_wifi_down(&o);
    assert(newly == 0b101);                        // yalnız 0 ve 2 YENİ offline
    assert(sd_offline_is_offline(&o, 0));
    assert(sd_offline_is_offline(&o, 1));
    assert(sd_offline_is_offline(&o, 2));
    // WiFi geldi: bayrak kalır ama probe kapısı hemen açık.
    sd_offline_wifi_up(&o);
    assert(sd_offline_is_offline(&o, 0));
    assert(sd_offline_should_send(&o, 0, 50000));  // uzlaştırma gönderimi geçer
    // Uzlaştırma başarılı → online.
    assert(sd_offline_feed(&o, 0, true, 50010) == SD_OFFLINE_CAME);
}

static void test_slot_isolation(void)
{
    sd_offline_t o;
    sd_offline_init(&o, 3);
    sd_offline_feed(&o, 0, false, 100);
    sd_offline_feed(&o, 0, false, 200);
    sd_offline_feed(&o, 0, false, 300);
    assert( sd_offline_is_offline(&o, 0));
    assert(!sd_offline_is_offline(&o, 1));
    assert(sd_offline_should_send(&o, 1, 301));    // komşu etkilenmez
}

int main(void)
{
    test_three_failures_then_offline();
    test_success_resets_counter();
    test_probe_gating();
    test_recovery_via_probe();
    test_wifi_down_up();
    test_slot_isolation();
    printf("test_offline: OK\n");
    return 0;
}
