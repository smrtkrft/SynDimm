idf_component_register(
    SRCS "src/sd_prefs.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core
    PRIV_REQUIRES nvs_flash
)
