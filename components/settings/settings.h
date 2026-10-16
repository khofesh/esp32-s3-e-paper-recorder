#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    int8_t tz_offset;        /* UTC offset in hours */
    bool hour12;             /* 12h clock */
    bool temp_f;             /* Fahrenheit */
    int8_t temp_offset;      /* self-heating calibration, whole degrees */
    uint8_t full_refresh;    /* full refresh every N partial refreshes */
    uint16_t sample_rate;    /* recording sample rate */
    uint8_t mic_gain;        /* mic gain 0..100 */
    uint8_t volume;          /* speaker volume 0..100 */
    uint16_t auto_sleep_s;   /* idle seconds before sleep, 0 = never */
} app_settings_t;

void settings_init(void);
app_settings_t *settings(void);
void settings_save(void);

#ifdef __cplusplus
}
#endif

#endif
