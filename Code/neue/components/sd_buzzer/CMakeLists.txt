idf_component_register(
    SRCS "src/sd_buzzer.c" "src/sd_quiet.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES driver esp_timer sd_pins sd_prefs
)
