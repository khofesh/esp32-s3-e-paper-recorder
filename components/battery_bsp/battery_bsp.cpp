#include "battery_bsp.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

static adc_oneshot_unit_handle_t s_adc = nullptr;
static adc_cali_handle_t s_cali = nullptr;

#define VBAT_ADC_CHANNEL ADC_CHANNEL_3
#define VBAT_SAMPLES 16

void battery_init(void)
{
    adc_cali_curve_fitting_config_t cali_cfg = {};
    cali_cfg.unit_id = ADC_UNIT_1;
    cali_cfg.atten = ADC_ATTEN_DB_12;
    cali_cfg.bitwidth = ADC_BITWIDTH_12;
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali));

    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = ADC_UNIT_1;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &s_adc));

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.bitwidth = ADC_BITWIDTH_12;
    chan_cfg.atten = ADC_ATTEN_DB_12;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, VBAT_ADC_CHANNEL, &chan_cfg));
}

float battery_voltage(void)
{
    int32_t acc = 0;
    for (int i = 0; i < VBAT_SAMPLES; i++) {
        int raw = 0;
        if (adc_oneshot_read(s_adc, VBAT_ADC_CHANNEL, &raw) != ESP_OK) {
            return 0.0f;
        }
        acc += raw;
    }
    int mv = 0;
    if (adc_cali_raw_to_voltage(s_cali, (int)(acc / VBAT_SAMPLES), &mv) != ESP_OK) {
        return 0.0f;
    }
    return 0.001f * mv * 2.0f;
}

uint8_t battery_level(void)
{
    const float full = 4.12f;
    const float empty = 3.0f;
    float v = battery_voltage();
    if (v <= empty) return 0;
    if (v >= full) return 100;
    return (uint8_t)((v - empty) / (full - empty) * 100.0f);
}
