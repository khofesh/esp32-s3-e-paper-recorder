#include "settings.h"

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include <string.h>

static const char *TAG = "settings";
#define NVS_NAMESPACE "recorder"
#define NVS_KEY "cfg"

static app_settings_t s_cfg;

static void settings_defaults(void)
{
    s_cfg.tz_offset = 7;
    s_cfg.hour12 = false;
    s_cfg.temp_f = false;
    s_cfg.temp_offset = 0;
    s_cfg.full_refresh = 10;
    s_cfg.sample_rate = 16000;
    s_cfg.mic_gain = 45;
    s_cfg.volume = 80;
    s_cfg.auto_sleep_s = 0;
}

void settings_init(void)
{
    settings_defaults();

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "nvs init failed: %s", esp_err_to_name(err));
        return;
    }

    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK) {
        return;
    }
    app_settings_t loaded;
    size_t len = sizeof(loaded);
    if (nvs_get_blob(h, NVS_KEY, &loaded, &len) == ESP_OK && len == sizeof(loaded)) {
        s_cfg = loaded;
    }
    nvs_close(h);
}

app_settings_t *settings(void)
{
    return &s_cfg;
}

void settings_save(void)
{
    nvs_handle_t h;
    if (nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h) != ESP_OK) {
        return;
    }
    nvs_set_blob(h, NVS_KEY, &s_cfg, sizeof(s_cfg));
    nvs_commit(h);
    nvs_close(h);
}
