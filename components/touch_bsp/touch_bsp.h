#ifndef TOUCH_BSP_H
#define TOUCH_BSP_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
class I2cMasterBus;
extern "C" {
#endif

#ifdef __cplusplus
void touch_init(I2cMasterBus *bus, int reset_pin, int x_max, int y_max);
#endif

/* Polls the FT6336; returns true when a point is currently pressed. */
bool touch_read_point(uint16_t *x, uint16_t *y);

#ifdef __cplusplus
}
#endif

#endif
