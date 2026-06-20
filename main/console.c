#include "console.h"
#include "ethernet.h"
#include "wifi.h"
#include "player.h"
#include "stations.h"
#include "lanradio_config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "console";

static void print_help(void) {
    printf("help | list | wifi <SSID> | pass <password> | connect | play <1..%u> | stop | vol <0..100> | info\r\n", (unsigned)stations_count());
}

static void handle_command(char *line) {
    char *command = strtok(line, " \t");
    char *argument = strtok(NULL, " \t");
    if (!command) return;
    if (!strcmp(command, "help")) { print_help(); return; }
    if (!strcmp(command, "list")) {
        for (size_t i = 1; i <= stations_count(); ++i) {
            const station_t *s = stations_get(i);
            printf("%u. %s\r\n", (unsigned)i, s->name);
        }
        return;
    }
    if (!strcmp(command, "wifi")) {
        esp_err_t err = lanradio_wifi_set_ssid(argument);
        printf(err == ESP_OK ? "Wi-Fi SSID saved.\r\n" : "SSID must be 1..32 characters.\r\n");
        return;
    }
    if (!strcmp(command, "pass")) {
        esp_err_t err = lanradio_wifi_set_password(argument);
        printf(err == ESP_OK ? "Wi-Fi password saved.\r\n" : "Password must be 0..63 characters.\r\n");
        return;
    }
    if (!strcmp(command, "connect")) {
        esp_err_t err = lanradio_wifi_connect();
        printf(err == ESP_OK ? "Connecting to Wi-Fi...\r\n" : "Set wifi and pass first.\r\n");
        return;
    }
    if (!strcmp(command, "play")) {
        const station_t *s = argument ? stations_get((size_t)strtoul(argument, NULL, 10)) : NULL;
        if (!s) { printf("Invalid station. Use list.\r\n"); return; }
        if (!lanradio_ethernet_has_ip() && !lanradio_wifi_has_ip()) { printf("No Ethernet or Wi-Fi DHCP address yet.\r\n"); return; }
        esp_err_t err = player_play(s->url);
        printf(err == ESP_OK ? "Playing: %s\r\n" : "Could not start stream: %s\r\n",
               err == ESP_OK ? s->name : esp_err_to_name(err));
        return;
    }
    if (!strcmp(command, "stop")) { player_stop(); printf("Stopped.\r\n"); return; }
    if (!strcmp(command, "vol")) {
        int v = argument ? atoi(argument) : -1;
        esp_err_t err = player_set_volume(v);
        printf(err == ESP_OK ? "Volume: %d\r\n" : "Volume must be 0..100.\r\n", v);
        return;
    }
    if (!strcmp(command, "info")) {
        printf("Ethernet: %s; Wi-Fi: %s (%s); player: %s; volume: %d\r\n",
               lanradio_ethernet_has_ip() ? "DHCP ready" : "waiting for DHCP",
               lanradio_wifi_has_ip() ? "DHCP ready" : "disconnected", lanradio_wifi_ssid(),
               player_is_playing() ? "playing" : "stopped", player_get_volume());
        return;
    }
    printf("Unknown command. "); print_help();
}

static void console_task(void *arg) {
    uint8_t character;
    char line[96] = {0};
    size_t length = 0;
    printf("LANRADIO v0.1 ready. Type help.\r\n");
    while (true) {
        int read = uart_read_bytes(LANRADIO_UART_NUM, &character, 1, portMAX_DELAY);
        if (read != 1) continue;
        if (character == '\r' || character == '\n') {
            if (length) { line[length] = 0; handle_command(line); length = 0; }
        } else if (character >= 32 && length < sizeof(line) - 1) {
            line[length++] = (char)character;
        }
    }
}

void console_start(void) {
    uart_config_t cfg = {
        .baud_rate = LANRADIO_UART_BAUD, .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE, .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };
    ESP_ERROR_CHECK(uart_param_config(LANRADIO_UART_NUM, &cfg));
    ESP_ERROR_CHECK(uart_driver_install(LANRADIO_UART_NUM, 512, 0, 0, NULL, 0));
    xTaskCreate(console_task, "console", 4096, NULL, 4, NULL);
    ESP_LOGI(TAG, "UART console started");
}
