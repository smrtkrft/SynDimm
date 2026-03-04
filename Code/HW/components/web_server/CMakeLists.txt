idf_component_register(
    SRCS "web_server.c"
    INCLUDE_DIRS "include"
    PRIV_REQUIRES esp_http_server esp_http_client ext_flash wifi_manager system_info device_driver nvs_flash esp_event esp_netif esp_system esp_wifi gui_ota fw_ota json
    EMBED_TXTFILES
        "setup.html"
)
