idf_component_register(
    SRCS "src/sd_behaviors.c" "src/drv_dimmer.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json sd_buzzer
)
# sd_ctx_* sembolleri sd_mode_engine'de implemente edilir — CMake REQUIRES
# döngüsü olmasın diye burada bağımlılık YOKTUR; uygulama linkinde çözülür.
# T3.3: src/drv_shutter.c, drv_mqtt_remote.c; T4.2: drv_safe.c + sd_sequence.c
