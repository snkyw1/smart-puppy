#include "low_power.h"
#include "lcd.h"
#include "esp_log.h"

#define TAG "LowPower"

static bool g_lp_sleeping = false;
static uint8_t g_saved_brightness = 100;
static void (*g_wake_cb)(void) = NULL;

void low_power_init(void)
{
    g_lp_sleeping = false;
    ESP_LOGI(TAG, "init");
}

bool low_power_is_sleeping(void)
{
    return g_lp_sleeping;
}

void low_power_set_wake_cb(void (*cb)(void))
{
    g_wake_cb = cb;
}

void low_power_enter(void)
{
    if (g_lp_sleeping) return;
    g_saved_brightness = lcd_get_brightness();
    lcd_set_brightness(0);
    g_lp_sleeping = true;
    ESP_LOGI(TAG, "enter sleep");
}

void low_power_exit(void)
{
    if (!g_lp_sleeping) return;
    lcd_set_brightness(g_saved_brightness);
    g_lp_sleeping = false;
    ESP_LOGI(TAG, "exit sleep");
}

void low_power_wake(void)
{
    low_power_exit();
    if (g_wake_cb) {
        g_wake_cb();
    }
}
