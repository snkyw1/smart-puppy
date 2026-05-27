#include "lcd.h"
#include "servo.h"
#include "accel.h"
#include "wifi_manager.h"
#include "bat_monitor/bat_monitor.h"
#include "low_power/low_power.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "application.h"
#include "nvs_flash.h"


void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    lcd_show();
    accel_init();
    bat_monitor_init();
    servo_init();
    low_power_init();

    xiaozhi_application_init();
    wifi_manager_init();
    xiaozhi_application_run();
}
