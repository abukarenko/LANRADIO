#include "wifi.h"

#include <string.h>
#include "esp_event.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "nvs.h"

#define WIFI_NVS_NAMESPACE "lanradio"
#define WIFI_SSID_KEY "wifi_ssid"
#define WIFI_PASS_KEY "wifi_pass"

static const char *TAG = "wifi";
static bool s_initialized;
static bool s_has_ip;
static char s_ssid[33];

static esp_err_t nvs_set_string(const char *key, const char *value) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (err != ESP_OK) return err;
    err = nvs_set_str(nvs, key, value);
    if (err == ESP_OK) err = nvs_commit(nvs);
    nvs_close(nvs);
    return err;
}

static esp_err_t nvs_get_string(const char *key, char *value, size_t size) {
    nvs_handle_t nvs;
    esp_err_t err = nvs_open(WIFI_NVS_NAMESPACE, NVS_READONLY, &nvs);
    if (err != ESP_OK) return err;
    size_t required = size;
    err = nvs_get_str(nvs, key, value, &required);
    nvs_close(nvs);
    return err;
}

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data) {
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        s_has_ip = false;
        ESP_LOGW(TAG, "disconnected; retrying");
        esp_wifi_connect();
    }
    if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = data;
        s_has_ip = true;
        ESP_LOGI(TAG, "DHCP address: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t lanradio_wifi_set_ssid(const char *ssid) {
    if (!ssid || !ssid[0] || strlen(ssid) >= sizeof(s_ssid)) return ESP_ERR_INVALID_ARG;
    ESP_RETURN_ON_ERROR(nvs_set_string(WIFI_SSID_KEY, ssid), TAG, "save SSID");
    strlcpy(s_ssid, ssid, sizeof(s_ssid));
    return ESP_OK;
}

esp_err_t lanradio_wifi_set_password(const char *password) {
    if (!password || strlen(password) > 63) return ESP_ERR_INVALID_ARG;
    return nvs_set_string(WIFI_PASS_KEY, password);
}

const char *lanradio_wifi_ssid(void) { return s_ssid[0] ? s_ssid : "not configured"; }
bool lanradio_wifi_has_ip(void) { return s_has_ip; }

esp_err_t lanradio_wifi_connect(void) {
    char password[65] = {0};
    if (!s_ssid[0]) nvs_get_string(WIFI_SSID_KEY, s_ssid, sizeof(s_ssid));
    if (!s_ssid[0]) return ESP_ERR_INVALID_STATE;
    if (nvs_get_string(WIFI_PASS_KEY, password, sizeof(password)) != ESP_OK) return ESP_ERR_INVALID_STATE;

    if (!s_initialized) {
        esp_netif_create_default_wifi_sta();
        wifi_init_config_t init_cfg = WIFI_INIT_CONFIG_DEFAULT();
        ESP_RETURN_ON_ERROR(esp_wifi_init(&init_cfg), TAG, "init");
        ESP_RETURN_ON_ERROR(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL), TAG, "Wi-Fi event");
        ESP_RETURN_ON_ERROR(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL), TAG, "IP event");
        ESP_RETURN_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), TAG, "mode");
        ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "start");
        ESP_RETURN_ON_ERROR(esp_wifi_set_ps(WIFI_PS_NONE), TAG, "disable power save");
        s_initialized = true;
    }

    wifi_config_t config = {0};
    strlcpy((char *)config.sta.ssid, s_ssid, sizeof(config.sta.ssid));
    strlcpy((char *)config.sta.password, password, sizeof(config.sta.password));
    config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &config), TAG, "configuration");
    return esp_wifi_connect();
}
