#include "debug.h"

#include "esp_log.h"

static bool s_enabled;

void lanradio_debug_set(bool enabled) {
    s_enabled = enabled;
    /* printf() output, including [META], is deliberately not affected. */
    esp_log_level_set("*", enabled ? ESP_LOG_INFO : ESP_LOG_NONE);
}

bool lanradio_debug_is_enabled(void) {
    return s_enabled;
}
