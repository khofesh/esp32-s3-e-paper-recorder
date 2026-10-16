#ifndef APP_STATE_H
#define APP_STATE_H

#include <stdbool.h>
#include <stdint.h>
#include "storage_bsp.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    APP_IDLE = 0,
    APP_RECORDING,
    APP_PLAYING,
    APP_SLEEPING,
} app_state_t;

typedef enum {
    EVT_REC_START = 0,
    EVT_REC_STOP,
    EVT_PLAY_START,
    EVT_PLAY_STOP,
} app_event_type_t;

typedef struct {
    app_event_type_t type;
    char path[320];
} app_event_t;

typedef struct {
    app_state_t state;
    bool rtc_ok;
    bool sd_ok;
    bool sensor_ok;
    bool in_player;               /* Files -> player sub-view */
    char status[32];              /* Record page status line */
    char rec_name[STORAGE_NAME_LEN];
    uint32_t rec_seconds;
    /* Home */
    bool home_valid;
    int hour;
    int minute;
    int day;
    int month;
    int year;
    float temp_c;
    float humidity;
    uint8_t battery;
} app_snapshot_t;

void app_state_init(void);

app_state_t app_state_state(void);
void app_state_set_state(app_state_t s);

/* Writers: begin() locks, commit() bumps version and unlocks. */
app_snapshot_t *app_state_begin(void);
void app_state_commit(void);

/* Readers: copy out without bumping the version. */
void app_state_read(app_snapshot_t *out);
uint32_t app_state_version(void);

void app_state_set_status(const char *fmt, ...);

/* User-activity timestamp in ms (from esp_timer_get_time) for idle detection. */
void app_state_note_activity(void);
uint64_t app_state_last_activity_ms(void);

bool app_state_post(app_event_type_t type, const char *path);
bool app_state_wait(app_event_t *e, uint32_t timeout_ms);

#ifdef __cplusplus
}
#endif

#endif
