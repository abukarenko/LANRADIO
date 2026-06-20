#include "ethernet.h"
#include "lanradio_config.h"

#include "esp_eth.h"
#include "esp_eth_mac_w5500.h"
#include "esp_eth_phy_w5500.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "driver/spi_master.h"

static const char *TAG = "ethernet";
static bool s_has_ip;
static esp_eth_handle_t s_eth_handle;

static void eth_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (id == ETHERNET_EVENT_CONNECTED) ESP_LOGI(TAG, "link up");
    if (id == ETHERNET_EVENT_DISCONNECTED) {
        s_has_ip = false;
        ESP_LOGW(TAG, "link down");
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    ip_event_got_ip_t *event = data;
    s_has_ip = true;
    ESP_LOGI(TAG, "DHCP address: " IPSTR, IP2STR(&event->ip_info.ip));
}

bool lanradio_ethernet_has_ip(void) { return s_has_ip; }

esp_err_t lanradio_ethernet_start(void) {
    esp_netif_config_t netif_cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *netif = esp_netif_new(&netif_cfg);
    if (!netif) return ESP_ERR_NO_MEM;

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = LANRADIO_W5500_MOSI_GPIO,
        .miso_io_num = LANRADIO_W5500_MISO_GPIO,
        .sclk_io_num = LANRADIO_W5500_SCLK_GPIO,
        .quadwp_io_num = -1, .quadhd_io_num = -1,
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LANRADIO_W5500_HOST, &bus_cfg, SPI_DMA_CH_AUTO), TAG, "SPI bus");

    spi_device_interface_config_t dev_cfg = {
        .mode = 0, .clock_speed_hz = LANRADIO_W5500_SPI_HZ,
        .spics_io_num = LANRADIO_W5500_CS_GPIO, .queue_size = 20,
    };
    spi_device_handle_t spi_handle;
    ESP_RETURN_ON_ERROR(spi_bus_add_device(LANRADIO_W5500_HOST, &dev_cfg, &spi_handle), TAG, "W5500 SPI device");

    eth_mac_config_t mac_cfg = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_cfg = ETH_PHY_DEFAULT_CONFIG();
    phy_cfg.phy_addr = 1;
    phy_cfg.reset_gpio_num = LANRADIO_W5500_RST_GPIO;
    eth_w5500_config_t w5500_cfg = ETH_W5500_DEFAULT_CONFIG(spi_handle);
    w5500_cfg.int_gpio_num = LANRADIO_W5500_INT_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_w5500(&mac_cfg, &w5500_cfg);
    esp_eth_phy_t *phy = esp_eth_phy_new_w5500(&phy_cfg);
    esp_eth_config_t eth_cfg = ETH_DEFAULT_CONFIG(mac, phy);
    ESP_RETURN_ON_ERROR(esp_eth_driver_install(&eth_cfg, &s_eth_handle), TAG, "driver");
    ESP_RETURN_ON_ERROR(esp_netif_attach(netif, esp_eth_new_netif_glue(s_eth_handle)), TAG, "netif attach");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(ETH_EVENT, ESP_EVENT_ANY_ID, eth_event_handler, NULL), TAG, "event handler");
    ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_ETH_GOT_IP, ip_event_handler, NULL), TAG, "IP handler");
    return esp_eth_start(s_eth_handle);
}
