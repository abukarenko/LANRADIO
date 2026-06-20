#pragma once

#include <stdbool.h>

/* Runtime control for ESP-IDF/ADF diagnostic messages sent to UART. */
void lanradio_debug_set(bool enabled);
bool lanradio_debug_is_enabled(void);
