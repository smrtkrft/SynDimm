idf_component_register(
    SRCS "src/sd_profiles.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json sd_util
    PRIV_REQUIRES nvs_flash
)
