idf_component_register(
    SRCS "device_driver.c" "proto_http.c" "proto_udp.c" "proto_mqtt.c" "safe_lock.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES ext_flash nvs_flash esp_http_client mqtt json esp_event esp_netif buzzer lwip sys_log sd_util
    EMBED_TXTFILES
        "profiles/dimmer/hue_bulb.json"
        "profiles/dimmer/shelly_dimmer2.json"
        "profiles/dimmer/tasmota_dimmer.json"
        "profiles/dimmer/wiz_bulb.json"
        "profiles/shutter/shelly_25_shutter.json"
)
