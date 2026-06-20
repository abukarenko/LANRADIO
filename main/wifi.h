#pragma once

#include <stdbool.h>
#include "esp_err.h"

typedef void (*lanradio_wifi_connected_cb_t)(void);

esp_err_t lanradio_wifi_set_ssid(const char *ssid);
esp_err_t lanradio_wifi_set_password(const char *password);
esp_err_t lanradio_wifi_connect(void);
bool lanradio_wifi_has_ip(void);
const char *lanradio_wifi_ssid(void);
void lanradio_wifi_set_connected_callback(lanradio_wifi_connected_cb_t callback);
