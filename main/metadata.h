#pragma once

#include <stdbool.h>
#include "esp_err.h"

/*
 * Reads ICY information through a second, low-priority HTTP connection.
 * Audio itself stays in the ESP-ADF pipeline and is never touched here.
 */
esp_err_t metadata_start(const char *url);
void metadata_stop(void);

typedef struct {
    char station[192];
    char genre[192];
    char bitrate[64];
    char station_url[192];
    char description[192];
    char title[192];
    char stream_url[192];
    int metaint;
} metadata_info_t;

/* Copies the last received ICY headers and StreamTitle. */
bool metadata_get(metadata_info_t *info);
