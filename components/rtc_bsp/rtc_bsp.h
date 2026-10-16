#ifndef RTC_BSP_H
#define RTC_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
class I2cMasterBus;
extern "C" {
#endif

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t dotw;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
} rtc_datetime_t;

#ifdef __cplusplus
esp_err_t rtc_dev_init(I2cMasterBus *bus);
#endif
bool rtc_ok(void);
void rtc_get(rtc_datetime_t *out);
void rtc_set(const rtc_datetime_t *in);

#ifdef __cplusplus
}
#endif

#endif
