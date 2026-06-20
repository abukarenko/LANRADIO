idf_component_register(
    SRCS "app_main.c" "console.c" "ethernet.c" "player.c" "stations.c" "wifi.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_netif esp_eth esp_wifi nvs_flash driver audio_pipeline audio_stream audio_board
)
