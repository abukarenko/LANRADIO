#include "console.h"
#include "debug.h"
#include "ethernet.h"
#include "player.h"
#include "stations.h"
#include "wifi.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "lanradio";

static void play_saved_station(void) {
    size_t index = stations_last();
    const station_t *station = stations_get(index);
    if (!station) return;
    if (player_play(station->url) == ESP_OK) {
        ESP_LOGI(TAG, "Autoplay: %s", station->name);
    }
}

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    /* Keep the monitor readable by default; use `debug 1` for SDK/ADF logs. */
    lanradio_debug_set(false);
    ESP_ERROR_CHECK(player_init());
    err = lanradio_ethernet_start();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Ethernet unavailable (%s); console remains available", esp_err_to_name(err));
    }
    lanradio_wifi_set_connected_callback(play_saved_station);
    err = lanradio_wifi_connect();
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "Connecting to saved Wi-Fi network: %s", lanradio_wifi_ssid());
    } else {
        ESP_LOGI(TAG, "No saved Wi-Fi credentials; use wifi, pass, then connect");
    }
    console_start();
    ESP_LOGI(TAG, "LANRADIO started; awaiting Ethernet DHCP");
}
