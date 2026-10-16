#include "user_app.h"

#include "board_power_bsp.h"
#include "i2c_bus.h"
#include "touch_bsp.h"
#include "rtc_bsp.h"
#include "sensor_bsp.h"
#include "battery_bsp.h"
#include "storage_bsp.h"
#include "audio_bsp.h"
#include "app_state.h"
#include "settings.h"
#include "ui_pages.h"
#include "user_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"

#include <stdio.h>
#include <string.h>
#include <time.h>

static const char *TAG = "user_app";

epaper_driver_display *driver = nullptr;
static board_power_bsp_t board_div(EPD_PWR_PIN, Audio_PWR_PIN, VBAT_PWR_PIN);

#define REC_CHUNK 8192
#define PLAY_CHUNK 4096

/* recording state */
static FILE *s_rec_file;
static uint32_t s_rec_bytes;
static uint32_t s_rec_rate;
static volatile bool s_rec_stop;
static volatile bool s_rec_error;
static volatile bool s_rec_lowbat;
static int s_rec_seq = 1;

/* playback state */
static FILE *s_play_file;
static char s_play_path[320];
static char s_play_name[STORAGE_NAME_LEN];
static uint32_t s_play_bytes;
static uint32_t s_play_rate;
static uint16_t s_play_ch;
static volatile bool s_play_stop;

static void app_task(void *arg);
static void sensor_task(void *arg);
static void power_task(void *arg);
static void recorder_task(void *arg);
static void player_task(void *arg);

static const char *base_name(const char *path)
{
    const char *p = strrchr(path, '/');
    return p ? p + 1 : path;
}

/* --------------------------- audio power ------------------------- */

static esp_err_t audio_power_begin(void)
{
    board_div.POWEER_Audio_ON();
    vTaskDelay(pdMS_TO_TICKS(20));
    return audio_init();
}

static void audio_power_end(void)
{
    audio_deinit();
    board_div.POWEER_Audio_OFF();
}

/* ----------------------------- init ----------------------------- */

void user_app_init(void)
{
    board_div.VBAT_POWER_ON();
    board_div.POWEER_EPD_ON();

    settings_init();
    app_state_init();

    custom_lcd_spi_t cfg = {};
    cfg.cs = EPD_CS_PIN;
    cfg.dc = EPD_DC_PIN;
    cfg.rst = EPD_RST_PIN;
    cfg.busy = EPD_BUSY_PIN;
    cfg.mosi = EPD_MOSI_PIN;
    cfg.scl = EPD_SCK_PIN;
    cfg.spi_host = EPD_SPI_NUM;
    cfg.buffer_len = 5000;
    driver = new epaper_driver_display(EPD_WIDTH, EPD_HEIGHT, cfg);

    I2cMasterBus *bus = I2cMasterBus::request_instance(ESP32_I2C_SCL_PIN, ESP32_I2C_SDA_PIN, ESP32_I2C_DEV_NUM);
    touch_init(bus, EPD_TP_RST_PIN, EPD_WIDTH, EPD_HEIGHT);
    rtc_dev_init(bus);
    sensor_init(bus);
    battery_init();

    esp_err_t sd = storage_init();

    app_snapshot_t *s = app_state_begin();
    s->rtc_ok = rtc_ok();
    s->sd_ok = (sd == ESP_OK);
    s->sensor_ok = true;
    app_state_commit();

    if (sd != ESP_OK) {
        app_state_set_status("No SD");
    }
}

void user_ui_init(void)
{
    ui_pages_init();
}

/* --------------------------- recorder --------------------------- */

static void start_recording(void)
{
    if (app_state_state() != APP_IDLE) {
        return;
    }
    if (!storage_mounted()) {
        app_state_set_status("No SD");
        return;
    }

    app_settings_t *cfg = settings();
    char path[320];
    struct tm tmv = {};
    struct tm *tmp = nullptr;
    if (rtc_ok()) {
        rtc_datetime_t t = {};
        rtc_get(&t);
        if (t.year > 2024) {
            tmv.tm_year = t.year - 1900;
            tmv.tm_mon = t.month - 1;
            tmv.tm_mday = t.day;
            tmv.tm_hour = t.hour;
            tmv.tm_min = t.min;
            tmv.tm_sec = t.sec;
            tmp = &tmv;
        }
    }
    storage_recording_name(path, sizeof(path), tmp, s_rec_seq);

    storage_lock();
    FILE *f = fopen(path, "wb");
    if (!f && tmp) {
        /* name collision: fall back to a sequence name */
        storage_recording_name(path, sizeof(path), nullptr, s_rec_seq);
        f = fopen(path, "wb");
    }
    if (!f) {
        storage_unlock();
        app_state_set_status("SD error");
        return;
    }
    wav_write_header(f, cfg->sample_rate, 1, 16);
    storage_unlock();

    s_rec_file = f;
    s_rec_bytes = 0;
    s_rec_rate = cfg->sample_rate;
    s_rec_stop = false;
    s_rec_error = false;
    s_rec_lowbat = false;

    app_snapshot_t *s = app_state_begin();
    s->state = APP_RECORDING;
    strncpy(s->rec_name, base_name(path), sizeof(s->rec_name) - 1);
    s->rec_name[sizeof(s->rec_name) - 1] = '\0';
    s->rec_seconds = 0;
    app_state_commit();

    xTaskCreatePinnedToCore(recorder_task, "recorder", 6 * 1024, nullptr, 6, nullptr, 1);
}

static void recorder_task(void *arg)
{
    uint8_t *buf = (uint8_t *)heap_caps_malloc(REC_CHUNK, MALLOC_CAP_SPIRAM);
    if (!buf) {
        app_state_set_status("No memory");
    } else if (audio_power_begin() != ESP_OK) {
        app_state_set_status("Mic error");
        free(buf);
    } else if (audio_record_begin(s_rec_rate, settings()->mic_gain) != ESP_OK) {
        app_state_set_status("Mic error");
        audio_power_end();
        free(buf);
    } else {
        uint32_t since_flush = 0;
        uint32_t flush_interval = s_rec_rate * 2 * 2; /* ~2 s of audio */
        while (!s_rec_stop) {
            if (audio_record_read(buf, REC_CHUNK) != ESP_OK) {
                s_rec_error = true;
                break;
            }
            storage_lock();
            size_t w = fwrite(buf, 1, REC_CHUNK, s_rec_file);
            storage_unlock();
            if (w != REC_CHUNK) {
                s_rec_error = true;
                break;
            }
            s_rec_bytes += REC_CHUNK;
            since_flush += REC_CHUNK;

            if (since_flush >= flush_interval) {
                since_flush = 0;
                storage_lock();
                wav_finalize(s_rec_file, s_rec_bytes);
                storage_unlock();
                if (battery_level() < 10) {
                    s_rec_lowbat = true;
                    break;
                }
            }
        }
        audio_record_end();
        audio_power_end();
        free(buf);
    }

    if (s_rec_file) {
        storage_lock();
        wav_finalize(s_rec_file, s_rec_bytes);
        fclose(s_rec_file);
        storage_unlock();
        s_rec_file = nullptr;
    }
    s_rec_seq++;

    app_snapshot_t *s = app_state_begin();
    s->state = APP_IDLE;
    app_state_commit();

    if (s_rec_error) {
        app_state_set_status("SD error");
    } else if (s_rec_lowbat) {
        app_state_set_status("Battery low");
    } else {
        app_state_set_status("Ready");
    }
    vTaskDelete(nullptr);
}

static void stop_recording(void)
{
    s_rec_stop = true;
}

/* --------------------------- playback --------------------------- */

static void start_playback(const char *path)
{
    if (app_state_state() != APP_IDLE) {
        return;
    }

    uint32_t bytes = 0, rate = 0;
    uint16_t ch = 0, bits = 0;
    storage_lock();
    esp_err_t ret = wav_read_info(path, &bytes, &rate, &ch, &bits);
    storage_unlock();
    if (ret != ESP_OK) {
        app_state_set_status("Unsupported");
        return;
    }

    strncpy(s_play_path, path, sizeof(s_play_path) - 1);
    s_play_path[sizeof(s_play_path) - 1] = '\0';
    strncpy(s_play_name, base_name(path), sizeof(s_play_name) - 1);
    s_play_name[sizeof(s_play_name) - 1] = '\0';
    s_play_bytes = bytes;
    s_play_rate = rate;
    s_play_ch = ch;
    s_play_stop = false;

    app_snapshot_t *s = app_state_begin();
    s->state = APP_PLAYING;
    strncpy(s->rec_name, s_play_name, sizeof(s->rec_name) - 1);
    s->rec_name[sizeof(s->rec_name) - 1] = '\0';
    app_state_commit();

    xTaskCreatePinnedToCore(player_task, "player", 6 * 1024, nullptr, 5, nullptr, 1);
}

static void player_task(void *arg)
{
    uint8_t *buf = (uint8_t *)heap_caps_malloc(PLAY_CHUNK, MALLOC_CAP_SPIRAM);
    if (buf) {
        storage_lock();
        s_play_file = fopen(s_play_path, "rb");
        storage_unlock();
        if (!s_play_file) {
            free(buf);
        } else if (audio_power_begin() != ESP_OK) {
            storage_lock();
            fclose(s_play_file);
            storage_unlock();
            s_play_file = nullptr;
            free(buf);
        } else if (audio_play_begin(s_play_rate, s_play_ch, settings()->volume) != ESP_OK) {
            audio_power_end();
            storage_lock();
            fclose(s_play_file);
            storage_unlock();
            s_play_file = nullptr;
            free(buf);
        } else {
            fseek(s_play_file, 44, SEEK_SET);
            uint32_t remaining = s_play_bytes;
            while (!s_play_stop && remaining > 0) {
                size_t want = remaining > PLAY_CHUNK ? PLAY_CHUNK : remaining;
                storage_lock();
                size_t rd = fread(buf, 1, want, s_play_file);
                storage_unlock();
                if (rd == 0) {
                    break;
                }
                if (audio_play_write(buf, rd) != ESP_OK) {
                    break;
                }
                remaining -= rd;
            }
            audio_play_end();
            audio_power_end();
            storage_lock();
            fclose(s_play_file);
            storage_unlock();
            s_play_file = nullptr;
            free(buf);
        }
    }

    app_snapshot_t *s = app_state_begin();
    s->state = APP_IDLE;
    app_state_commit();
    app_state_set_status("Ready");
    vTaskDelete(nullptr);
}

static void stop_playback(void)
{
    s_play_stop = true;
}

/* ---------------------------- app task --------------------------- */

static void app_task(void *arg)
{
    app_event_t e;
    for (;;) {
        if (!app_state_wait(&e, portMAX_DELAY)) {
            continue;
        }
        switch (e.type) {
        case EVT_REC_START: start_recording(); break;
        case EVT_REC_STOP: stop_recording(); break;
        case EVT_PLAY_START: start_playback(e.path); break;
        case EVT_PLAY_STOP: stop_playback(); break;
        default: break;
        }
    }
}

/* --------------------------- sensor task -------------------------- */

static void sensor_task(void *arg)
{
    int last_min = -1;
    int tick = 0;
    for (;;) {
        rtc_datetime_t t = {};
        rtc_get(&t);
        bool minute_changed = (t.min != last_min) || (tick == 0);

        float temp = 0, hum = 0;
        bool sen_ok = false;
        if (minute_changed) {
            sen_ok = sensor_read(&temp, &hum);
        }
        uint8_t batt = battery_level();
        bool sd_ok = storage_mounted();

        app_snapshot_t cur;
        app_state_read(&cur);
        bool changed = minute_changed
            || cur.battery != batt
            || cur.sd_ok != sd_ok
            || (minute_changed && (cur.sensor_ok != sen_ok || cur.temp_c != temp || cur.humidity != hum));

        if (changed) {
            app_snapshot_t *s = app_state_begin();
            if (minute_changed) {
                if (rtc_ok() && t.year > 2024) {
                    s->home_valid = true;
                    s->year = t.year;
                    s->month = t.month;
                    s->day = t.day;
                    s->hour = t.hour;
                    s->minute = t.min;
                    s->rtc_ok = true;
                } else {
                    s->home_valid = false;
                    s->rtc_ok = false;
                }
                s->sensor_ok = sen_ok;
                if (sen_ok) {
                    s->temp_c = temp;
                    s->humidity = hum;
                }
            }
            s->battery = batt;
            s->sd_ok = sd_ok;
            app_state_commit();
        }

        last_min = t.min;
        tick++;
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

/* ---------------------------- power task -------------------------- */

static void enter_sleep(void)
{
    ESP_LOGI(TAG, "Entering deep sleep");
    app_state_set_state(APP_SLEEPING);
    audio_deinit();
    board_div.POWEER_Audio_OFF();
    driver->EPD_Shutdown();
    board_div.POWEER_EPD_OFF();

    esp_sleep_enable_timer_wakeup(60ULL * 1000000ULL);
    uint64_t mask = (1ULL << BOOT_BUTTON_PIN) | (1ULL << PWR_BUTTON_PIN);
    rtc_gpio_pullup_en(BOOT_BUTTON_PIN);
    rtc_gpio_pulldown_dis(BOOT_BUTTON_PIN);
    rtc_gpio_pullup_en(PWR_BUTTON_PIN);
    rtc_gpio_pulldown_dis(PWR_BUTTON_PIN);
    esp_sleep_enable_ext1_wakeup(mask, ESP_EXT1_WAKEUP_ANY_LOW);
    esp_deep_sleep_start();
}

static void power_task(void *arg)
{
    uint64_t last_activity = app_state_last_activity_ms();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        app_settings_t *cfg = settings();
        if (cfg->auto_sleep_s == 0 || app_state_state() != APP_IDLE) {
            last_activity = app_state_last_activity_ms();
            continue;
        }
        uint64_t now = (uint64_t)(esp_timer_get_time() / 1000);
        if (now - last_activity >= (uint64_t)cfg->auto_sleep_s * 1000ULL) {
            enter_sleep();
            last_activity = now;
        }
    }
}

/* ----------------------------- start ----------------------------- */

void user_app_start(void)
{
    xTaskCreatePinnedToCore(app_task, "app", 5 * 1024, nullptr, 5, nullptr, 1);
    xTaskCreatePinnedToCore(sensor_task, "sensor", 4 * 1024, nullptr, 3, nullptr, 0);
    xTaskCreatePinnedToCore(power_task, "power", 3 * 1024, nullptr, 2, nullptr, 0);
}
