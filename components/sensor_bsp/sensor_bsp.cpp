#include "sensor_bsp.h"
#include "i2c_bus.h"
#include "user_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include <math.h>
#include <string.h>

static const char *TAG = "shtc3";

#define SHTC3_CMD_WAKEUP 0x3517
#define SHTC3_CMD_SLEEP 0xB098
#define SHTC3_CMD_SOFT_RESET 0x805D
#define SHTC3_CMD_READ_ID 0xEFC8
#define SHTC3_CMD_MEAS_T_RH 0x7866

static i2c_master_dev_handle_t s_dev = nullptr;

static int shtc3_send(uint16_t cmd)
{
    uint8_t buf[2] = {(uint8_t)(cmd >> 8), (uint8_t)(cmd & 0xff)};
    return i2c_master_transmit(s_dev, buf, 2, pdMS_TO_TICKS(1000));
}

static uint8_t crc8(const uint8_t *data, int len)
{
    uint8_t crc = 0xff;
    for (int i = 0; i < len; i++) {
        crc ^= data[i];
        for (int b = 0; b < 8; b++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

esp_err_t sensor_init(I2cMasterBus *bus)
{
    s_dev = bus->add_device(I2C_SHTC3_DEV_Address, 400000);
    shtc3_send(SHTC3_CMD_WAKEUP);
    vTaskDelay(pdMS_TO_TICKS(50));
    shtc3_send(SHTC3_CMD_SOFT_RESET);
    vTaskDelay(pdMS_TO_TICKS(20));
    shtc3_send(SHTC3_CMD_WAKEUP);
    vTaskDelay(pdMS_TO_TICKS(50));

    uint8_t id[3] = {0};
    uint8_t cmd[2] = {(uint8_t)(SHTC3_CMD_READ_ID >> 8), (uint8_t)(SHTC3_CMD_READ_ID & 0xff)};
    if (i2c_master_transmit_receive(s_dev, cmd, 2, id, 3, pdMS_TO_TICKS(1000)) != ESP_OK) {
        ESP_LOGE(TAG, "Sensor not responding");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "SHTC3 ID: %04x", (id[0] << 8) | id[1]);
    return ESP_OK;
}

bool sensor_read(float *temp_c, float *humidity)
{
    if (shtc3_send(SHTC3_CMD_WAKEUP) != ESP_OK) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(50));
    if (shtc3_send(SHTC3_CMD_MEAS_T_RH) != ESP_OK) {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    uint8_t d[6] = {0};
    if (i2c_master_receive(s_dev, d, 6, pdMS_TO_TICKS(1000)) != ESP_OK) {
        shtc3_send(SHTC3_CMD_SLEEP);
        return false;
    }
    shtc3_send(SHTC3_CMD_SLEEP);

    if (crc8(d, 2) != d[2] || crc8(&d[3], 2) != d[5]) {
        ESP_LOGW(TAG, "CRC error");
        return false;
    }
    uint16_t raw_t = ((uint16_t)d[0] << 8) | d[1];
    uint16_t raw_h = ((uint16_t)d[3] << 8) | d[4];
    *temp_c = 175.0f * raw_t / 65536.0f - 45.0f;
    *humidity = 100.0f * raw_h / 65536.0f;
    return true;
}
