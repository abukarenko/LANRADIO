#include "console.h"
#include "ethernet.h"
#include "player.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "lanradio";

void app_main(void) {
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(player_init());
    err = lanradio_ethernet_start();
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Ethernet unavailable (%s); console remains available", esp_err_to_name(err));
    }
    console_start();
    ESP_LOGI(TAG, "LANRADIO started; awaiting Ethernet DHCP");
}
