idf_component_register(
    SRCS "src/sd_template.c" "src/sd_proto.c" "src/sd_proto_http.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json
    PRIV_REQUIRES esp_http_client
)
# T3.1-2: src/sd_proto_udp.c, sd_proto_mqtt.c (sd_proto_internal.h'te
# SD_PROTO_HAS_UDP/MQTT bayraklarını da 1 yap).
