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

static int reg_read(uint8_t reg, uint8_t *buf, size_t len)
{
    return i2c_master_transmit_receive(s_dev, &reg, 1, buf, len, 1000);
}

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

    gpio_config_t int_cfg = {};
    int_cfg.intr_type = GPIO_INTR_DISABLE;
    int_cfg.mode = GPIO_MODE_INPUT;
    int_cfg.pin_bit_mask = (1ULL << EPD_TP_INT_PIN);
    int_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    int_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    ESP_ERROR_CHECK(gpio_config(&int_cfg));

    int probe = i2c_master_probe(bus->handle(), I2C_FT6336_DEV_Address, 200);
    ESP_LOGI(TAG, "FT6336 probe=%s", esp_err_to_name(probe));
    ESP_LOGI(TAG, "FT6336 ready");
}

bool touch_read_point(uint16_t *x, uint16_t *y)
{
    static uint32_t err_count;
    uint8_t touched = 0;
    if (reg_read(0x02, &touched, 1) != ESP_OK) {
        if (err_count++ < 5) {
            ESP_LOGW(TAG, "TD_STATUS read failed");
        }
        return false;
    }
    if (!touched) {
        return false;
    }
    uint8_t buf[4] = {0};
    if (reg_read(0x03, buf, 4) != ESP_OK) {
        return false;
    }
    *x = (((uint16_t)buf[0] & 0x0f) << 8) | buf[1];
    *y = (((uint16_t)buf[2] & 0x0f) << 8) | buf[3];
    if (*x > s_x_max) *x = s_x_max;
    if (*y > s_y_max) *y = s_y_max;
    return true;
}
