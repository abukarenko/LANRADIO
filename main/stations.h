#pragma once
#include <stddef.h>
#include "esp_err.h"

typedef struct {
    const char *name;
    const char *url;
} station_t;

const station_t *stations_get(size_t one_based_index);
size_t stations_count(void);
size_t stations_capacity(void);
esp_err_t stations_set_last(size_t one_based_index);
size_t stations_last(void);
esp_err_t stations_set_user(size_t one_based_index, const char *name, const char *url);
