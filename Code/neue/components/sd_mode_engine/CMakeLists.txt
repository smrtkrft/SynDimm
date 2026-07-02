idf_component_register(
    SRCS "src/sd_binding.c" "src/sd_offline.c"
         "src/sd_mode_engine.c" "src/sd_mode_cli.c" "src/sd_config_io.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json sd_behaviors sd_proto sd_util
    PRIV_REQUIRES nvs_flash esp_timer sd_profiles sd_buzzer sd_prefs sk_api
)
