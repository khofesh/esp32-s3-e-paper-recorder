#include "i2c_bus.h"
#include "esp_err.h"
#include "freertos/FreeRTOS.h"

I2cMasterBus *I2cMasterBus::instance_ = nullptr;

I2cMasterBus *I2cMasterBus::request_instance(int scl_pin, int sda_pin, int i2c_port)
{
    if (instance_ == nullptr) {
        instance_ = new I2cMasterBus(scl_pin, sda_pin, i2c_port);
    }
    return instance_;
}

I2cMasterBus::I2cMasterBus(int scl_pin, int sda_pin, int i2c_port)
{
    i2c_master_bus_config_t cfg = {};
    cfg.clk_source = I2C_CLK_SRC_DEFAULT;
    cfg.i2c_port = (i2c_port_t)i2c_port;
    cfg.scl_io_num = (gpio_num_t)scl_pin;
    cfg.sda_io_num = (gpio_num_t)sda_pin;
    cfg.glitch_ignore_cnt = 7;
    cfg.flags.enable_internal_pullup = true;
    ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &bus_handle));
}

I2cMasterBus::~I2cMasterBus() {}

i2c_master_dev_handle_t I2cMasterBus::add_device(uint8_t addr, uint32_t scl_hz)
{
    i2c_device_config_t cfg = {};
    cfg.dev_addr_length = I2C_ADDR_BIT_LEN_7;
    cfg.device_address = addr;
    cfg.scl_speed_hz = scl_hz;
    i2c_master_dev_handle_t dev = nullptr;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &cfg, &dev));
    return dev;
}

int I2cMasterBus::write(i2c_master_dev_handle_t dev, int reg, const uint8_t *buf, uint8_t len)
{
    if (reg < 0) {
        return i2c_master_transmit(dev, buf, len, pdMS_TO_TICKS(1000));
    }
    uint8_t tmp[8];
    if (len + 1 > sizeof(tmp)) {
        return ESP_ERR_INVALID_SIZE;
    }
    tmp[0] = (uint8_t)reg;
    for (uint8_t i = 0; i < len; i++) {
        tmp[i + 1] = buf[i];
    }
    return i2c_master_transmit(dev, tmp, len + 1, pdMS_TO_TICKS(1000));
}

int I2cMasterBus::read(i2c_master_dev_handle_t dev, int reg, uint8_t *buf, uint8_t len)
{
    if (reg < 0) {
        return i2c_master_receive(dev, buf, len, pdMS_TO_TICKS(1000));
    }
    uint8_t addr = (uint8_t)reg;
    return i2c_master_transmit_receive(dev, &addr, 1, buf, len, pdMS_TO_TICKS(1000));
}

int I2cMasterBus::write_read(i2c_master_dev_handle_t dev, const uint8_t *wbuf, uint8_t wlen,
                             uint8_t *rbuf, uint8_t rlen)
{
    return i2c_master_transmit_receive(dev, wbuf, wlen, rbuf, rlen, pdMS_TO_TICKS(1000));
}
