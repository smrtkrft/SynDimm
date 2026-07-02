idf_component_register(
    SRCS "src/sd_binding.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json
    PRIV_REQUIRES nvs_flash
)
# T2.5+: src/sd_mode_engine.c, sd_mode_cli.c, sd_offline.c, sd_coalesce.c
# T6.2:  src/sd_config_io.c
