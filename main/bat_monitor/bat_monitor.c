#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "adc_battery_estimation.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "ui/ui.h"
#include "application.h"
#include "mmap_generate_audio.h"

#if CONFIG_BAT_ADC_UNIT_1
    #define BAT_ADC_UNIT ADC_UNIT_1
#elif CONFIG_BAT_ADC_UNIT_2
    #define BAT_ADC_UNIT ADC_UNIT_2
#endif

#if CONFIG_BAT_ADC_ATTEN_DB_0
    #define BAT_ADC_ATTEN ADC_ATTEN_DB_0
#elif CONFIG_BAT_ADC_ATTEN_DB_2_5
    #define BAT_ADC_ATTEN ADC_ATTEN_DB_2_5
#elif CONFIG_BAT_ADC_ATTEN_DB_6
    #define BAT_ADC_ATTEN ADC_ATTEN_DB_6
#else
    #define BAT_ADC_ATTEN ADC_ATTEN_DB_12
#endif

static const char* TAG = "BAT_MONITOR";

static adc_battery_estimation_handle_t adc_bat_handle;

bool battery_charging_detect_cb(void *user_data)
{
    return gpio_get_level(CONFIG_BAT_GPIO_CHARGING) == 0;
}

static void IRAM_ATTR charge_isr_handler(void* arg)
{
    SetMainEventBitsFromISR(MAIN_EVENT_CHARGE_STATE_CHANGE);
}

void bat_monitor_info_update()
{
    static bool is_low_power = false;
    float capacity = 0;
    bool is_charging = false;
    adc_battery_estimation_get_capacity(adc_bat_handle, &capacity);
    adc_battery_estimation_get_charging_state(adc_bat_handle, &is_charging);
    ESP_LOGI(TAG, "percentage:%.2f%%, state: %s", capacity, is_charging ? "charging" : "idle");
    ui_label_battery_update(capacity, is_charging);
    if (!is_low_power) {
        if ((int)capacity <= 20) {
            SchedulePlaySound(MMAP_AUDIO_LOW_BATTERY_OGG);
            is_low_power = true;
        }
    }
    else {
        if ((int)capacity > 20) {
            is_low_power = false;
        }
    }
}

void bat_monitor_init()
{
    gpio_config_t bat_charging_cfg = {
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = 1ULL << CONFIG_BAT_GPIO_CHARGING,
        .pull_up_en = true,
        .intr_type = GPIO_INTR_ANYEDGE
    };
    gpio_config(&bat_charging_cfg);

     adc_battery_estimation_t config = {
        .internal = {
            .adc_unit = BAT_ADC_UNIT,
            .adc_bitwidth = ADC_BITWIDTH_DEFAULT,
            .adc_atten = BAT_ADC_ATTEN,
        },
        .adc_channel = CONFIG_BAT_ADC_CHANNEL,
        .lower_resistor = CONFIG_BAT_RESISTOR_LOWER,
        .upper_resistor = CONFIG_BAT_RESISTOR_UPPER,
        .charging_detect_cb = battery_charging_detect_cb,
    };

    adc_bat_handle = adc_battery_estimation_create(&config);

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);
    gpio_isr_handler_add(CONFIG_BAT_GPIO_CHARGING, charge_isr_handler, NULL);
    gpio_intr_enable(CONFIG_BAT_GPIO_CHARGING);

    bat_monitor_info_update();
}
