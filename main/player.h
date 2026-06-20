#pragma once
#include <stdbool.h>
#include "esp_err.h"

esp_err_t player_init(void);
esp_err_t player_play(const char *url);
esp_err_t player_stop(void);
esp_err_t player_set_volume(int volume);
int player_get_volume(void);
bool player_is_playing(void);
