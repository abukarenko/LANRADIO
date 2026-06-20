#include "player.h"
#include "lanradio_config.h"

#include "audio_pipeline.h"
#include "audio_element.h"
#include "http_stream.h"
#include "i2s_stream.h"
#include "mp3_decoder.h"
#include "esp_check.h"
#include "esp_log.h"

static const char *TAG = "player";
static audio_pipeline_handle_t s_pipeline;
static audio_element_handle_t s_http, s_i2s;
static int s_volume = 50;
static bool s_playing;

esp_err_t player_init(void) {
    audio_pipeline_cfg_t pipeline_cfg = DEFAULT_AUDIO_PIPELINE_CONFIG();
    s_pipeline = audio_pipeline_init(&pipeline_cfg);
    if (!s_pipeline) return ESP_ERR_NO_MEM;

    http_stream_cfg_t http_cfg = HTTP_STREAM_CFG_DEFAULT();
    http_cfg.type = AUDIO_STREAM_READER;
    http_cfg.out_rb_size = LANRADIO_STREAM_BUFFER_BYTES;
    s_http = http_stream_init(&http_cfg);
    mp3_decoder_cfg_t mp3_cfg = DEFAULT_MP3_DECODER_CONFIG();
    mp3_cfg.out_rb_size = LANRADIO_PCM_BUFFER_BYTES;
    audio_element_handle_t mp3 = mp3_decoder_init(&mp3_cfg);
    i2s_stream_cfg_t i2s_cfg = I2S_STREAM_CFG_DEFAULT_WITH_PARA(
        LANRADIO_I2S_PORT, 44100, I2S_DATA_BIT_WIDTH_16BIT, AUDIO_STREAM_WRITER);
    i2s_cfg.out_rb_size = LANRADIO_PCM_BUFFER_BYTES;
    i2s_cfg.std_cfg.gpio_cfg.bclk = LANRADIO_I2S_BCLK_GPIO;
    i2s_cfg.std_cfg.gpio_cfg.ws = LANRADIO_I2S_WS_GPIO;
    i2s_cfg.std_cfg.gpio_cfg.dout = LANRADIO_I2S_DOUT_GPIO;
    i2s_cfg.std_cfg.gpio_cfg.din = I2S_GPIO_UNUSED;
    s_i2s = i2s_stream_init(&i2s_cfg);
    if (!s_http || !mp3 || !s_i2s) return ESP_ERR_NO_MEM;
    audio_pipeline_register(s_pipeline, s_http, "http");
    audio_pipeline_register(s_pipeline, mp3, "mp3");
    audio_pipeline_register(s_pipeline, s_i2s, "i2s");
    const char *link[3] = {"http", "mp3", "i2s"};
    ESP_RETURN_ON_ERROR(audio_pipeline_link(s_pipeline, link, 3), TAG, "pipeline link");
    return player_set_volume(s_volume);
}

esp_err_t player_play(const char *url) {
    if (!url) return ESP_ERR_INVALID_ARG;
    player_stop();
    ESP_RETURN_ON_ERROR(audio_element_set_uri(s_http, url), TAG, "set URL");
    ESP_RETURN_ON_ERROR(audio_pipeline_run(s_pipeline), TAG, "start stream");
    s_playing = true;
    ESP_LOGI(TAG, "playing %s", url);
    return ESP_OK;
}

esp_err_t player_stop(void) {
    if (!s_playing) return ESP_OK;
    audio_pipeline_stop(s_pipeline);
    audio_pipeline_wait_for_stop(s_pipeline);
    audio_pipeline_terminate(s_pipeline);
    audio_pipeline_reset_ringbuffer(s_pipeline);
    audio_pipeline_reset_elements(s_pipeline);
    s_playing = false;
    return ESP_OK;
}

esp_err_t player_set_volume(int volume) {
    if (volume < 0 || volume > 100) return ESP_ERR_INVALID_ARG;
    s_volume = volume;
    /* UDA1334A has no programmable hardware volume. Keep software state until
     * a DSP/volume element is added to the pipeline. */
    return ESP_OK;
}

int player_get_volume(void) { return s_volume; }
bool player_is_playing(void) { return s_playing; }
