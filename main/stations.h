#pragma once
#include <stddef.h>

typedef struct {
    const char *name;
    const char *url;
} station_t;

const station_t *stations_get(size_t one_based_index);
size_t stations_count(void);
