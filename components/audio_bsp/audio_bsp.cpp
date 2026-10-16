#include "audio_bsp.h"
#include "user_config.h"

#include "codec_board.h"
#include "codec_init.h"
#include "esp_codec_dev.h"
#include "esp_log.h"

static const char *TAG = "audio";

static bool s_inited = false;
static esp_codec_dev_handle_t s_play = nullptr;
static esp_codec_dev_handle_t s_rec = nullptr;

esp_err_t audio_init(void)
{
    if (s_inited) {
        return ESP_OK;
    }
    set_codec_board_type(CODEC_BOARD_NAME);
    codec_init_cfg_t cfg = {};
    cfg.in_mode = CODEC_I2S_MODE_STD;
    cfg.out_mode = CODEC_I2S_MODE_STD;
    cfg.in_use_tdm = false;
    cfg.reuse_dev = false;
    int ret = init_codec(&cfg);
    if (ret != 0) {
        ESP_LOGE(TAG, "init_codec failed: %d", ret);
        return ESP_FAIL;
    }
    s_play = get_playback_handle();
    s_rec = get_record_handle();
    s_inited = true;
    ESP_LOGI(TAG, "codec ready");
    return ESP_OK;
}

esp_err_t audio_record_begin(uint32_t sample_rate, uint8_t gain)
{
    if (!s_rec) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = sample_rate;
    fs.channel = 1;
    fs.bits_per_sample = 16;
    esp_err_t ret = esp_codec_dev_open(s_rec, &fs);
    if (ret != ESP_OK) {
        return ret;
    }
    return esp_codec_dev_set_in_gain(s_rec, (float)gain);
}

esp_err_t audio_record_read(uint8_t *buf, size_t bytes)
{
    return esp_codec_dev_read(s_rec, buf, bytes);
}

esp_err_t audio_record_end(void)
{
    return s_rec ? esp_codec_dev_close(s_rec) : ESP_OK;
}

esp_err_t audio_play_begin(uint32_t sample_rate, uint16_t channels, uint8_t volume)
{
    if (!s_play) {
        return ESP_ERR_INVALID_STATE;
    }
    esp_codec_dev_sample_info_t fs = {};
    fs.sample_rate = sample_rate;
    fs.channel = channels;
    fs.bits_per_sample = 16;
    esp_err_t ret = esp_codec_dev_open(s_play, &fs);
    if (ret != ESP_OK) {
        return ret;
    }
    return esp_codec_dev_set_out_vol(s_play, volume);
}

esp_err_t audio_play_write(const uint8_t *buf, size_t bytes)
{
    return esp_codec_dev_write(s_play, (void *)buf, bytes);
}

esp_err_t audio_play_end(void)
{
    return s_play ? esp_codec_dev_close(s_play) : ESP_OK;
}
