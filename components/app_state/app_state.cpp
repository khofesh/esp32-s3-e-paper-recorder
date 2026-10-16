#include "app_state.h"

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "esp_timer.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static SemaphoreHandle_t s_mutex;
static QueueHandle_t s_events;
static app_snapshot_t s_snap;
static uint32_t s_version;
static uint64_t s_activity_ms;

void app_state_init(void)
{
    s_mutex = xSemaphoreCreateMutex();
    s_events = xQueueCreate(8, sizeof(app_event_t));
    memset(&s_snap, 0, sizeof(s_snap));
    s_snap.state = APP_IDLE;
    strncpy(s_snap.status, "Ready", sizeof(s_snap.status) - 1);
    s_version = 1;
    s_activity_ms = (uint64_t)(esp_timer_get_time() / 1000);
}

app_state_t app_state_state(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    app_state_t s = s_snap.state;
    xSemaphoreGive(s_mutex);
    return s;
}

void app_state_set_state(app_state_t s)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    s_snap.state = s;
    s_version++;
    xSemaphoreGive(s_mutex);
}

app_snapshot_t *app_state_begin(void)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    return &s_snap;
}

void app_state_commit(void)
{
    s_version++;
    xSemaphoreGive(s_mutex);
}

void app_state_read(app_snapshot_t *out)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    *out = s_snap;
    xSemaphoreGive(s_mutex);
}

uint32_t app_state_version(void)
{
    return s_version;
}

void app_state_set_status(const char *fmt, ...)
{
    xSemaphoreTake(s_mutex, portMAX_DELAY);
    va_list ap;
    va_start(ap, fmt);
    vsnprintf(s_snap.status, sizeof(s_snap.status), fmt, ap);
    va_end(ap);
    s_version++;
    xSemaphoreGive(s_mutex);
}

bool app_state_post(app_event_type_t type, const char *path)
{
    app_event_t e = {};
    e.type = type;
    if (path) {
        strncpy(e.path, path, sizeof(e.path) - 1);
    }
    return xQueueSend(s_events, &e, 0) == pdTRUE;
}

bool app_state_wait(app_event_t *e, uint32_t timeout_ms)
{
    return xQueueReceive(s_events, e, pdMS_TO_TICKS(timeout_ms)) == pdTRUE;
}

void app_state_note_activity(void)
{
    s_activity_ms = (uint64_t)(esp_timer_get_time() / 1000);
}

uint64_t app_state_last_activity_ms(void)
{
    return s_activity_ms;
}
