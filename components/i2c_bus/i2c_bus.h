#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "driver/i2c_master.h"

class I2cMasterBus
{
private:
    i2c_master_bus_handle_t bus_handle = nullptr;

    I2cMasterBus(int scl_pin, int sda_pin, int i2c_port);
    ~I2cMasterBus();

public:
    static I2cMasterBus *request_instance(int scl_pin, int sda_pin, int i2c_port);

    i2c_master_bus_handle_t handle() { return bus_handle; }
    i2c_master_dev_handle_t add_device(uint8_t addr, uint32_t scl_hz);

    int write(i2c_master_dev_handle_t dev, int reg, const uint8_t *buf, uint8_t len);
    int read(i2c_master_dev_handle_t dev, int reg, uint8_t *buf, uint8_t len);
    int write_read(i2c_master_dev_handle_t dev, const uint8_t *wbuf, uint8_t wlen,
                   uint8_t *rbuf, uint8_t rlen);

    static I2cMasterBus *instance_;
};

#endif
