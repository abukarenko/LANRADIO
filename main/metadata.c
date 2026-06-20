#include "metadata.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "esp_http_client.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"

#define META_URL_MAX        256
#define META_TEXT_MAX       192
#define META_BLOCK_MAX      4096

static const char *TAG = "metadata";
static volatile uint32_t s_generation;
static metadata_info_t s_current;
static SemaphoreHandle_t s_current_lock;

typedef struct {
    uint32_t generation;
    char url[META_URL_MAX];
} metadata_request_t;

typedef struct {
    int metaint;
    char station[META_TEXT_MAX];
    char genre[META_TEXT_MAX];
    char bitrate[64];
    char url[META_TEXT_MAX];
    char description[META_TEXT_MAX];
} icy_headers_t;

static void copy_header(char *destination, size_t size, const char *value) {
    if (!value || !*value) return;
    snprintf(destination, size, "%s", value);
}

static void clear_current(void) {
    if (!s_current_lock) s_current_lock = xSemaphoreCreateMutex();
    if (!s_current_lock) return;
    if (xSemaphoreTake(s_current_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
        memset(&s_current, 0, sizeof(s_current));
        xSemaphoreGive(s_current_lock);
    }
}

static void update_current_headers(const icy_headers_t *headers) {
    if (!s_current_lock) return;
    if (xSemaphoreTake(s_current_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
        snprintf(s_current.station, sizeof(s_current.station), "%s", headers->station);
        snprintf(s_current.genre, sizeof(s_current.genre), "%s", headers->genre);
        snprintf(s_current.bitrate, sizeof(s_current.bitrate), "%s", headers->bitrate);
        snprintf(s_current.station_url, sizeof(s_current.station_url), "%s", headers->url);
        snprintf(s_current.description, sizeof(s_current.description), "%s", headers->description);
        s_current.metaint = headers->metaint;
        xSemaphoreGive(s_current_lock);
    }
}

static void update_current_title(const char *title, const char *stream_url) {
    if (!s_current_lock) return;
    if (xSemaphoreTake(s_current_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
        if (title) snprintf(s_current.title, sizeof(s_current.title), "%s", title);
        if (stream_url) snprintf(s_current.stream_url, sizeof(s_current.stream_url), "%s", stream_url);
        xSemaphoreGive(s_current_lock);
    }
}

static esp_err_t http_event_handler(esp_http_client_event_t *event) {
    if (event->event_id != HTTP_EVENT_ON_HEADER || !event->header_key || !event->header_value) {
        return ESP_OK;
    }
    icy_headers_t *headers = (icy_headers_t *)event->user_data;
    if (!headers) return ESP_OK;

    if (!strcasecmp(event->header_key, "icy-metaint")) {
        headers->metaint = (int)strtol(event->header_value, NULL, 10);
    } else if (!strcasecmp(event->header_key, "icy-name")) {
        copy_header(headers->station, sizeof(headers->station), event->header_value);
    } else if (!strcasecmp(event->header_key, "icy-genre")) {
        copy_header(headers->genre, sizeof(headers->genre), event->header_value);
    } else if (!strcasecmp(event->header_key, "icy-br")) {
        copy_header(headers->bitrate, sizeof(headers->bitrate), event->header_value);
    } else if (!strcasecmp(event->header_key, "icy-url")) {
        copy_header(headers->url, sizeof(headers->url), event->header_value);
    } else if (!strcasecmp(event->header_key, "icy-description") ||
               !strcasecmp(event->header_key, "description")) {
        copy_header(headers->description, sizeof(headers->description), event->header_value);
    }
    return ESP_OK;
}

static void print_headers(const icy_headers_t *headers) {
    update_current_headers(headers);
    if (headers->station[0]) printf("[META] icy-name: %s\r\n", headers->station);
    if (headers->genre[0]) printf("[META] icy-genre: %s\r\n", headers->genre);
    if (headers->bitrate[0]) printf("[META] icy-br: %s kbps\r\n", headers->bitrate);
    if (headers->url[0]) printf("[META] icy-url: %s\r\n", headers->url);
    if (headers->description[0]) printf("[META] Description: %s\r\n", headers->description);
    if (headers->metaint > 0) printf("[META] ICY metadata enabled (interval %d)\r\n", headers->metaint);
}

static bool get_quoted_value(const char *metadata, const char *key, char *out, size_t out_size) {
    const char *value = strstr(metadata, key);
    if (!value) return false;
    value += strlen(key);
    const char quote = *value;
    if (quote != '\'' && quote != '\"') return false;
    ++value;
    const char *end = strchr(value, quote);
    if (!end) return false;
    size_t length = (size_t)(end - value);
    if (length >= out_size) length = out_size - 1;
    memcpy(out, value, length);
    out[length] = '\0';
    return true;
}

static void print_metadata_if_changed(const char *block,
                                      char *last_title, size_t last_title_size,
                                      char *last_url, size_t last_url_size) {
    char title[META_TEXT_MAX] = "";
    char stream_url[META_TEXT_MAX] = "";
    bool has_title = get_quoted_value(block, "StreamTitle=", title, sizeof(title));
    bool has_url = get_quoted_value(block, "StreamUrl=", stream_url, sizeof(stream_url));

    if (has_title && title[0] && strcmp(title, last_title)) {
        snprintf(last_title, last_title_size, "%s", title);
        update_current_title(title, NULL);
        printf("[META] StreamTitle: %s\r\n", title);
    }
    if (has_url && stream_url[0] && strcmp(stream_url, last_url)) {
        snprintf(last_url, last_url_size, "%s", stream_url);
        update_current_title(NULL, stream_url);
        printf("[META] StreamUrl: %s\r\n", stream_url);
    }
}

static void metadata_session(const metadata_request_t *request) {
    icy_headers_t headers = {0};
    esp_http_client_config_t cfg = {
        .url = request->url,
        .event_handler = http_event_handler,
        .user_data = &headers,
        .timeout_ms = 5000,
        .buffer_size = 1024,
        .disable_auto_redirect = false,
    };
    esp_http_client_handle_t client = esp_http_client_init(&cfg);
    if (!client) return;
    esp_http_client_set_header(client, "Icy-MetaData", "1");
    esp_http_client_set_header(client, "User-Agent", "LANRADIO/0.1");

    esp_err_t err = esp_http_client_open(client, 0);
    if (err != ESP_OK) {
        ESP_LOGD(TAG, "metadata connection unavailable: %s", esp_err_to_name(err));
        esp_http_client_cleanup(client);
        return;
    }
    esp_http_client_fetch_headers(client);
    print_headers(&headers);

    unsigned char read_buffer[512];
    char *metadata = calloc(1, META_BLOCK_MAX + 1);
    if (!metadata) {
        esp_http_client_close(client);
        esp_http_client_cleanup(client);
        return;
    }
    char last_title[META_TEXT_MAX] = "";
    char last_stream_url[META_TEXT_MAX] = "";
    int audio_remaining = headers.metaint;
    int metadata_remaining = -1;
    size_t metadata_length = 0;

    while (request->generation == s_generation) {
        int read = esp_http_client_read(client, (char *)read_buffer, sizeof(read_buffer));
        if (read <= 0) break;
        if (headers.metaint <= 0) continue;

        for (int i = 0; i < read && request->generation == s_generation; ++i) {
            unsigned char byte = read_buffer[i];
            if (metadata_remaining >= 0) {
                if (metadata_remaining > 0 && metadata_length < META_BLOCK_MAX) {
                    metadata[metadata_length++] = (char)byte;
                }
                if (--metadata_remaining == 0) {
                    metadata[metadata_length] = '\0';
                    print_metadata_if_changed(metadata, last_title, sizeof(last_title),
                                              last_stream_url, sizeof(last_stream_url));
                    metadata_length = 0;
                    audio_remaining = headers.metaint;
                    metadata_remaining = -1;
                }
            } else if (audio_remaining > 0) {
                --audio_remaining;
            } else {
                metadata_remaining = byte * 16;
                metadata_length = 0;
                if (metadata_remaining == 0) {
                    audio_remaining = headers.metaint;
                    metadata_remaining = -1;
                }
            }
        }
    }
    free(metadata);
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
}

static void metadata_task(void *arg) {
    metadata_request_t *request = (metadata_request_t *)arg;
    while (request->generation == s_generation) {
        metadata_session(request);
        if (request->generation == s_generation) vTaskDelay(pdMS_TO_TICKS(3000));
    }
    free(request);
    vTaskDelete(NULL);
}

esp_err_t metadata_start(const char *url) {
    if (!url || strncmp(url, "http://", 7) != 0) return ESP_ERR_NOT_SUPPORTED;
    metadata_stop();
    clear_current();
    metadata_request_t *request = calloc(1, sizeof(*request));
    if (!request) return ESP_ERR_NO_MEM;
    request->generation = s_generation;
    snprintf(request->url, sizeof(request->url), "%s", url);
    if (xTaskCreate(metadata_task, "icy_meta", 6144, request, 2, NULL) != pdPASS) {
        free(request);
        return ESP_ERR_NO_MEM;
    }
    return ESP_OK;
}

void metadata_stop(void) {
    ++s_generation;
}

bool metadata_get(metadata_info_t *info) {
    if (!info || !s_current_lock) return false;
    if (xSemaphoreTake(s_current_lock, pdMS_TO_TICKS(100)) != pdTRUE) return false;
    *info = s_current;
    xSemaphoreGive(s_current_lock);
    return info->station[0] || info->title[0] || info->description[0];
}
