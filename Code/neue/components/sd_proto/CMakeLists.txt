idf_component_register(
    SRCS "src/sd_template.c"
    INCLUDE_DIRS "include"
    REQUIRES sk_core json
    PRIV_REQUIRES nvs_flash
)
# T2.3+: src/sd_proto.c, sd_proto_http.c; T3.1-2: sd_proto_udp.c, sd_proto_mqtt.c
