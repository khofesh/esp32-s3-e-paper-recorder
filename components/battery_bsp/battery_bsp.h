#ifndef BATTERY_BSP_H
#define BATTERY_BSP_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void battery_init(void);
float battery_voltage(void);
uint8_t battery_level(void);

#ifdef __cplusplus
}
#endif

#endif
