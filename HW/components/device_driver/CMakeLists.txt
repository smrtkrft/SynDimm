idf_component_register(
    SRCS "device_driver.c" "proto_http.c" "proto_udp.c" "proto_mqtt.c" "safe_lock.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES ext_flash nvs_flash esp_http_client mqtt json esp_event esp_netif buzzer lwip
)
