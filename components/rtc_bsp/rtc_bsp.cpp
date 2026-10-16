#include "rtc_bsp.h"
#include "i2c_bus.h"
#include "user_config.h"
#include "pcf85063a.h"
#include "esp_log.h"

static const char *TAG = "rtc";
static pcf85063a_dev_t s_dev = {};
static bool s_ok = false;

esp_err_t rtc_dev_init(I2cMasterBus *bus)
{
    esp_err_t ret = pcf85063a_init(&s_dev, bus->handle(), PCF85063A_ADDRESS);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "PCF85063A init failed: %s", esp_err_to_name(ret));
        s_ok = false;
        return ret;
    }
    s_ok = true;
    return ESP_OK;
}

bool rtc_ok(void)
{
    return s_ok;
}

void rtc_get(rtc_datetime_t *out)
{
    pcf85063a_datetime_t dt = {};
    if (!s_ok || pcf85063a_get_time_date(&s_dev, &dt) != ESP_OK) {
        *out = (rtc_datetime_t){};
        return;
    }
    out->year = dt.year;
    out->month = dt.month;
    out->day = dt.day;
    out->dotw = dt.dotw;
    out->hour = dt.hour;
    out->min = dt.min;
    out->sec = dt.sec;
}

void rtc_set(const rtc_datetime_t *in)
{
    if (!s_ok) {
        return;
    }
    pcf85063a_datetime_t dt = {};
    dt.year = in->year;
    dt.month = in->month;
    dt.day = in->day;
    dt.dotw = in->dotw;
    dt.hour = in->hour;
    dt.min = in->min;
    dt.sec = in->sec;
    pcf85063a_set_time_date(&s_dev, dt);
}
