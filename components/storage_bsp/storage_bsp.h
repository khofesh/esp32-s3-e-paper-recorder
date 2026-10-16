#ifndef STORAGE_BSP_H
#define STORAGE_BSP_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>
#include <stdio.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

#define STORAGE_MAX_FILES 64
#define STORAGE_NAME_LEN 64

typedef struct {
    char name[STORAGE_NAME_LEN];
    uint32_t size;
    bool is_wav;
} storage_file_t;

esp_err_t storage_init(void);
bool storage_mounted(void);

void storage_lock(void);
void storage_unlock(void);

/* Files in the mount root, directories skipped. Returns count (<= max). */
int storage_list(storage_file_t *out, int max);

/* WAV helpers (PCM). */
void wav_write_header(FILE *f, uint32_t sample_rate, uint16_t channels, uint16_t bits);
void wav_finalize(FILE *f, uint32_t data_bytes);

esp_err_t wav_read_info(const char *path, uint32_t *data_bytes, uint32_t *sample_rate,
                        uint16_t *channels, uint16_t *bits);

/* Build "REC_YYYYMMDD_HHMMSS.wav"; falls back to "REC_nnnn.wav" via seq. */
void storage_recording_name(char *out, size_t out_len, const struct tm *t, int seq);

#ifdef __cplusplus
}
#endif

#endif
