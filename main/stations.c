#include "stations.h"
#include "nvs.h"
#include <stdio.h>
#include <string.h>

#define STATIONS_NVS_NAMESPACE "lanradio"
#define STATIONS_LAST_KEY "station"
#define STATIONS_MAX 64
#define STATION_NAME_MAX 48
#define STATION_URL_MAX 192

/* Replace these starter URLs with the intended local/NVS station catalogue. */
static const station_t s_stations[STATIONS_MAX] = {
    {"SomaFM Groove Salad", "http://ice1.somafm.com/groovesalad-128-mp3"},
    {"SomaFM Drone Zone", "http://ice1.somafm.com/dronezone-128-mp3"},
    {"SomaFM Secret Agent", "http://ice1.somafm.com/secretagent-128-mp3"},
    {"SomaFM DEF CON", "http://ice1.somafm.com/defcon-128-mp3"},
    {"SomaFM Illinois Street", "http://ice1.somafm.com/illstreet-128-mp3"},
    {"SomaFM SF 10-33", "http://ice1.somafm.com/sf1033-128-mp3"},
    {"SomaFM Vaporwaves", "http://ice1.somafm.com/vaporwaves-128-mp3"},
    {"SomaFM Space Station", "http://ice1.somafm.com/spacestation-128-mp3"},
    {"SomaFM PopTron", "http://ice1.somafm.com/poptron-128-mp3"},
    {"SomaFM Digitalis", "http://ice1.somafm.com/digitalis-128-mp3"},
};
static char s_user_names[STATIONS_MAX][STATION_NAME_MAX];
static char s_user_urls[STATIONS_MAX][STATION_URL_MAX];
static bool s_user_loaded[STATIONS_MAX];

static void load_user_station(size_t one_based_index) {
    size_t i = one_based_index - 1;
    if (s_user_loaded[i]) return;
    s_user_loaded[i] = true;
    char name_key[8], url_key[8];
    snprintf(name_key, sizeof(name_key), "n%u", (unsigned)one_based_index);
    snprintf(url_key, sizeof(url_key), "u%u", (unsigned)one_based_index);
    nvs_handle_t nvs;
    if (nvs_open(STATIONS_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return;
    size_t name_size = sizeof(s_user_names[i]), url_size = sizeof(s_user_urls[i]);
    if (nvs_get_str(nvs, name_key, s_user_names[i], &name_size) != ESP_OK ||
        nvs_get_str(nvs, url_key, s_user_urls[i], &url_size) != ESP_OK) {
        s_user_names[i][0] = 0;
        s_user_urls[i][0] = 0;
    }
    nvs_close(nvs);
}

const station_t *stations_get(size_t one_based_index) {
    if (!one_based_index || one_based_index > STATIONS_MAX) return NULL;
    load_user_station(one_based_index);
    size_t i = one_based_index - 1;
    if (s_user_names[i][0] && s_user_urls[i][0]) {
        static station_t user_station;
        user_station.name = s_user_names[i];
        user_station.url = s_user_urls[i];
        return &user_station;
    }
    const station_t *station = &s_stations[i];
    return station && station->name && station->url ? station : NULL;
}

esp_err_t stations_set_user(size_t one_based_index, const char *name, const char *url) {
    if (!one_based_index || one_based_index > STATIONS_MAX || !name || !url ||
        !name[0] || !url[0] || strlen(name) >= STATION_NAME_MAX || strlen(url) >= STATION_URL_MAX) {
        return ESP_ERR_INVALID_ARG;
    }
    char name_key[8], url_key[8];
    snprintf(name_key, sizeof(name_key), "n%u", (unsigned)one_based_index);
    snprintf(url_key, sizeof(url_key), "u%u", (unsigned)one_based_index);
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(STATIONS_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, name_key, name);
    if (err == ESP_OK) err = nvs_set_str(nvs, url_key, url);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    if (err == ESP_OK) {
        size_t i = one_based_index - 1;
        strlcpy(s_user_names[i], name, sizeof(s_user_names[i]));
        strlcpy(s_user_urls[i], url, sizeof(s_user_urls[i]));
        s_user_loaded[i] = true;
    }
    return err;
}

size_t stations_count(void) {
    size_t count = 0;
    for (size_t i = 0; i < STATIONS_MAX; ++i) if (s_stations[i].name) ++count;
    return count;
}

size_t stations_capacity(void) { return STATIONS_MAX; }

esp_err_t stations_set_last(size_t one_based_index) {
    if (!stations_get(one_based_index)) return ESP_ERR_INVALID_ARG;
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(STATIONS_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_u8(nvs, STATIONS_LAST_KEY, (uint8_t)one_based_index);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

size_t stations_last(void) {
    nvs_handle_t nvs;
    uint8_t index = 0;
    if (nvs_open(STATIONS_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) return 0;
    esp_err_t err = nvs_get_u8(nvs, STATIONS_LAST_KEY, &index);
    nvs_close(nvs);
    return err == ESP_OK && stations_get(index) ? index : 0;
}
