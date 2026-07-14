idf_component_register(
    SRCS "src/sd_template.c" "src/sd_proto.c" "src/sd_proto_http.c"
         "src/sd_proto_udp.c" "src/sd_proto_mqtt.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json sd_util
    PRIV_REQUIRES esp_http_client mqtt lwip
)
