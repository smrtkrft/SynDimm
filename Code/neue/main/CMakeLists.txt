idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    PRIV_REQUIRES sk_core sk_api nvs_flash esp_pm driver sd_pins sd_prefs
)
