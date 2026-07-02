idf_component_register(
    SRCS "src/sd_encoder.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES driver esp_timer sd_pins
)
