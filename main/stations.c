#include "stations.h"

/* Replace these starter URLs with the intended local/NVS station catalogue. */
static const station_t s_stations[] = {
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

const station_t *stations_get(size_t one_based_index) {
    return one_based_index && one_based_index <= stations_count()
               ? &s_stations[one_based_index - 1] : NULL;
}

size_t stations_count(void) { return sizeof(s_stations) / sizeof(s_stations[0]); }
