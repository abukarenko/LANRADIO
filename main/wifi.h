#pragma once

#include <stdbool.h>
#include "esp_err.h"

esp_err_t lanradio_wifi_set_ssid(const char *ssid);
esp_err_t lanradio_wifi_set_password(const char *password);
esp_err_t lanradio_wifi_connect(void);
bool lanradio_wifi_has_ip(void);
const char *lanradio_wifi_ssid(void);
