#pragma once
#include <stdbool.h>
#include "esp_err.h"

esp_err_t lanradio_ethernet_start(void);
bool lanradio_ethernet_has_ip(void);
