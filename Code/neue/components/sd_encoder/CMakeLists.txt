idf_component_register(
    SRCS "src/sd_encoder.c" "src/sd_gesture.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES driver esp_timer sd_pins sd_prefs sd_buzzer sd_mode_engine
)
