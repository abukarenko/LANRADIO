idf_component_register(
    SRCS "app_main.c" "console.c" "debug.c" "ethernet.c" "metadata.c" "player.c" "stations.c" "wifi.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_netif esp_eth esp_wifi nvs_flash driver esp_http_client audio_pipeline audio_stream audio_board
)
