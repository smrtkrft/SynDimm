idf_component_register(
    SRCS "src/sd_buzzer.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES driver esp_timer sd_pins sd_prefs
)
# sd_quiet sd_prefs'e taşındı (gramer tek kaynak) — buzzer sd_prefs
# bağımlılığı üzerinden sd_quiet.h'yi görür.
