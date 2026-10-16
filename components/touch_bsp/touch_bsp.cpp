#include "touch_bsp.h"
#include "i2c_bus.h"
#include "user_config.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "touch";

static i2c_master_dev_handle_t s_dev = nullptr;
static int s_x_max = 200;
static int s_y_max = 200;

void touch_init(I2cMasterBus *bus, int reset_pin, int x_max, int y_max)
{
    s_x_max = x_max;
    s_y_max = y_max;
    s_dev = bus->add_device(I2C_FT6336_DEV_Address, 400000);

    gpio_config_t rst = {};
    rst.intr_type = GPIO_INTR_DISABLE;
    rst.mode = GPIO_MODE_OUTPUT;
    rst.pin_bit_mask = (1ULL << reset_pin);
    rst.pull_down_en = GPIO_PULLDOWN_DISABLE;
    rst.pull_up_en = GPIO_PULLUP_DISABLE;
    ESP_ERROR_CHECK(gpio_config(&rst));
    gpio_set_level((gpio_num_t)reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)reset_pin, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    gpio_set_level((gpio_num_t)reset_pin, 1);
    vTaskDelay(pdMS_TO_TICKS(100));
    ESP_LOGI(TAG, "FT6336 ready");
}

bool touch_read_point(uint16_t *x, uint16_t *y)
{
    uint8_t reg = 0x02;
    uint8_t touched = 0;
    if (i2c_master_transmit_receive(s_dev, &reg, 1, &touched, 1, pdMS_TO_TICKS(50)) != ESP_OK) {
        return false;
    }
    if (!touched) {
        return false;
    }
    uint8_t buf[4] = {0};
    reg = 0x03;
    if (i2c_master_transmit_receive(s_dev, &reg, 1, buf, 4, pdMS_TO_TICKS(50)) != ESP_OK) {
        return false;
    }
    *x = (((uint16_t)buf[0] & 0x0f) << 8) | buf[1];
    *y = (((uint16_t)buf[2] & 0x0f) << 8) | buf[3];
    if (*x > s_x_max) *x = s_x_max;
    if (*y > s_y_max) *y = s_y_max;
    return true;
}
