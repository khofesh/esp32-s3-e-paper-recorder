#ifndef AUDIO_BSP_H
#define AUDIO_BSP_H

#include <stddef.h>
#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

esp_err_t audio_init(void);

esp_err_t audio_record_begin(uint32_t sample_rate, uint8_t gain);
esp_err_t audio_record_read(uint8_t *buf, size_t bytes);
esp_err_t audio_record_end(void);

esp_err_t audio_play_begin(uint32_t sample_rate, uint16_t channels, uint8_t volume);
esp_err_t audio_play_write(const uint8_t *buf, size_t bytes);
esp_err_t audio_play_end(void);

#ifdef __cplusplus
}
#endif

#endif
