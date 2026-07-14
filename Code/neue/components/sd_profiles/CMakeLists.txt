idf_component_register(
    SRCS "src/sd_profiles.c" "src/sd_profiles_validate.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json sd_util
    PRIV_REQUIRES nvs_flash
)
