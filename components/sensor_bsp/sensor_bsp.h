#ifndef SENSOR_BSP_H
#define SENSOR_BSP_H

#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
class I2cMasterBus;
extern "C" {
#endif

#ifdef __cplusplus
esp_err_t sensor_init(I2cMasterBus *bus);
#endif
bool sensor_read(float *temp_c, float *humidity);

#ifdef __cplusplus
}
#endif

#endif
