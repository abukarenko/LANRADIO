idf_component_register(
    SRCS "app_main.c" "console.c" "ethernet.c" "player.c" "stations.c"
    INCLUDE_DIRS "."
    REQUIRES esp_event esp_netif esp_eth nvs_flash driver audio_pipeline audio_stream audio_board
)
