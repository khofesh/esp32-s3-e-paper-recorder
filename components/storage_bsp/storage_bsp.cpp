#include "storage_bsp.h"
#include "user_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"

#include <dirent.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>

static const char *TAG = "storage";

static sdmmc_card_t *s_card = nullptr;
static SemaphoreHandle_t s_mutex = nullptr;

#define WAV_HDR_SIZE 44

static void write_le16(FILE *f, uint16_t v)
{
    fputc(v & 0xff, f);
    fputc((v >> 8) & 0xff, f);
}

static void write_le32(FILE *f, uint32_t v)
{
    fputc(v & 0xff, f);
    fputc((v >> 8) & 0xff, f);
    fputc((v >> 16) & 0xff, f);
    fputc((v >> 24) & 0xff, f);
}

void wav_write_header(FILE *f, uint32_t sample_rate, uint16_t channels, uint16_t bits)
{
    uint32_t byte_rate = sample_rate * channels * bits / 8;
    uint16_t block_align = channels * bits / 8;
    fwrite("RIFF", 1, 4, f);
    write_le32(f, 36);
    fwrite("WAVE", 1, 4, f);
    fwrite("fmt ", 1, 4, f);
    write_le32(f, 16);
    write_le16(f, 1);
    write_le16(f, channels);
    write_le32(f, sample_rate);
    write_le32(f, byte_rate);
    write_le16(f, block_align);
    write_le16(f, bits);
    fwrite("data", 1, 4, f);
    write_le32(f, 0);
}

void wav_finalize(FILE *f, uint32_t data_bytes)
{
    if (!f) return;
    long end = ftell(f);
    fseek(f, 4, SEEK_SET);
    write_le32(f, 36 + data_bytes);
    fseek(f, 40, SEEK_SET);
    write_le32(f, data_bytes);
    fseek(f, end, SEEK_SET);
    fflush(f);
}

esp_err_t storage_init(void)
{
    if (s_mutex == nullptr) {
        s_mutex = xSemaphoreCreateMutex();
    }

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {};
    mount_cfg.format_if_mount_failed = false;
    mount_cfg.max_files = 5;
    mount_cfg.allocation_unit_size = 2 * 1024;

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;

    sdmmc_slot_config_t slot = SDMMC_SLOT_CONFIG_DEFAULT();
    slot.width = 1;
    slot.clk = SD_CLK_PIN;
    slot.cmd = SD_MOSI_CMD_PIN;
    slot.d0 = SD_MISO_D0_PIN;

    esp_err_t ret = esp_vfs_fat_sdmmc_mount(SD_MOUNT_POINT, &host, &slot, &mount_cfg, &s_card);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "SD mount failed: %s", esp_err_to_name(ret));
        s_card = nullptr;
        return ret;
    }
    ESP_LOGI(TAG, "SD card mounted");
    sdmmc_card_print_info(stdout, s_card);
    return ESP_OK;
}

bool storage_mounted(void)
{
    return s_card != nullptr;
}

void storage_lock(void)
{
    if (s_mutex) xSemaphoreTake(s_mutex, portMAX_DELAY);
}

void storage_unlock(void)
{
    if (s_mutex) xSemaphoreGive(s_mutex);
}

int storage_list(storage_file_t *out, int max)
{
    int count = 0;
    DIR *dir = opendir(SD_MOUNT_POINT);
    if (!dir) {
        return 0;
    }
    struct dirent *ent;
    while ((ent = readdir(dir)) != nullptr && count < max) {
        if (ent->d_name[0] == '.') {
            continue;
        }
        char path[320];
        snprintf(path, sizeof(path), "%s/%s", SD_MOUNT_POINT, ent->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) {
            continue;
        }
        storage_file_t *e = &out[count++];
        memset(e, 0, sizeof(*e));
        strncpy(e->name, ent->d_name, STORAGE_NAME_LEN - 1);
        e->size = (uint32_t)st.st_size;
        size_t len = strlen(e->name);
        e->is_wav = len > 4 && strcasecmp(e->name + len - 4, ".wav") == 0;
    }
    closedir(dir);
    return count;
}

esp_err_t wav_read_info(const char *path, uint32_t *data_bytes, uint32_t *sample_rate,
                        uint16_t *channels, uint16_t *bits)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        return ESP_ERR_NOT_FOUND;
    }
    uint8_t hdr[WAV_HDR_SIZE];
    esp_err_t ret = ESP_FAIL;
    if (fread(hdr, 1, WAV_HDR_SIZE, f) == WAV_HDR_SIZE &&
        memcmp(hdr, "RIFF", 4) == 0 && memcmp(hdr + 8, "WAVE", 4) == 0) {
        uint32_t rate;
        uint16_t ch, bps, fmt;
        memcpy(&rate, hdr + 24, 4);
        memcpy(&ch, hdr + 22, 2);
        memcpy(&bps, hdr + 34, 2);
        memcpy(&fmt, hdr + 20, 2);
        uint32_t bytes;
        memcpy(&bytes, hdr + 40, 4);
        if (fmt == 1 && bps == 16 && ch >= 1 && ch <= 2) {
            if (data_bytes) *data_bytes = bytes;
            if (sample_rate) *sample_rate = rate;
            if (channels) *channels = ch;
            if (bits) *bits = bps;
            ret = ESP_OK;
        } else {
            ret = ESP_ERR_NOT_SUPPORTED;
        }
    }
    fclose(f);
    return ret;
}

void storage_recording_name(char *out, size_t out_len, const struct tm *t, int seq)
{
    if (t && t->tm_year > 100) {
        snprintf(out, out_len, "%s/REC_%04d%02d%02d_%02d%02d%02d.wav", SD_MOUNT_POINT,
                 t->tm_year + 1900, t->tm_mon + 1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    } else {
        snprintf(out, out_len, "%s/REC_%04d.wav", SD_MOUNT_POINT, seq);
    }
}
