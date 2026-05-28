#include "lvgl.h"
#include "font/my_fonts.h"
#include "ui/ui.h"
#include "style/my_style.h"
#include "key/key.h"
#include "lcd.h"
#include "low_power/low_power.h"
#include "esp_lvgl_port.h"
#include "esp_wifi.h"
#include "wifi_manager.h"

#define TAG "Settings"

static lv_obj_t* slider_brightness;
static lv_obj_t* label_brightness_val;
static lv_obj_t* switch_lp;

static void slider_brightness_cb(lv_event_t* e)
{
    lv_obj_t* slider = lv_event_get_target(e);
    uint8_t step = (uint8_t)lv_slider_get_value(slider);
    uint8_t brightness = step * 5;
    lv_label_set_text_fmt(label_brightness_val, "%d%%", brightness);
    lcd_set_brightness(brightness);
}

static void init_brightness_screen()
{
    lv_obj_t* screen = my_screen_create("Brightness", key_edit_mode_invert);
    lv_obj_t* content = my_screen_get_content(screen);

    slider_brightness = lv_slider_create(content);
    lv_slider_set_range(slider_brightness, 1, 20);
    lv_obj_set_width(slider_brightness, lv_pct(80));
    lv_obj_center(slider_brightness);

    uint8_t val = lcd_get_brightness();
    uint8_t step = val / 5;
    if (step < 1) step = 1;
    if (step > 20) step = 20;
    lv_slider_set_value(slider_brightness, step, LV_ANIM_OFF);

    label_brightness_val = lv_label_create(content);
    lv_label_set_text_fmt(label_brightness_val, "%d%%", step * 5);
    lv_obj_align(label_brightness_val, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_group_t* group = my_screen_get_group(screen);
    lv_group_add_obj(group, slider_brightness);

    lv_obj_add_event_cb(slider_brightness, slider_brightness_cb, LV_EVENT_VALUE_CHANGED, NULL);

    key_edit_mode_invert();
    add_path(screen);
}


static lv_obj_t* slider_volume;
static lv_obj_t* label_volume_val;

static void slider_volume_cb(lv_event_t* e)
{
    lv_obj_t* slider = lv_event_get_target(e);
    uint8_t step = (uint8_t)lv_slider_get_value(slider);
    uint8_t vol = step * 5;
    lv_label_set_text_fmt(label_volume_val, "%d%%", vol);
}

static void init_volume_screen()
{
    lv_obj_t* screen = my_screen_create("Volume", key_edit_mode_invert);
    lv_obj_t* content = my_screen_get_content(screen);

    slider_volume = lv_slider_create(content);
    lv_slider_set_range(slider_volume, 0, 20);
    lv_obj_set_width(slider_volume, lv_pct(80));
    lv_obj_center(slider_volume);
    lv_slider_set_value(slider_volume, 50, LV_ANIM_OFF);

    label_volume_val = lv_label_create(content);
    lv_label_set_text_fmt(label_volume_val, "%d%%", 50);
    lv_obj_align(label_volume_val, LV_ALIGN_BOTTOM_MID, 0, -30);

    lv_group_t* group = my_screen_get_group(screen);
    lv_group_add_obj(group, slider_volume);

    lv_obj_add_event_cb(slider_volume, slider_volume_cb, LV_EVENT_VALUE_CHANGED, NULL);

    key_edit_mode_invert();
    add_path(screen);
}

static void init_theme_screen()
{
    lv_obj_t* screen = my_screen_create("Theme", NULL);
    lv_obj_t* content = my_screen_get_content(screen);

    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, "Dark Mode");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t* switch_theme = lv_switch_create(content);
    lv_obj_align(switch_theme, LV_ALIGN_CENTER, 0, 0);

    add_path(screen);
}


static void switch_wifi_cb(lv_event_t* e)
{
    lv_obj_t* sw = lv_event_get_target(e);
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);

    if (checked) {
        esp_wifi_start();
        esp_wifi_connect();
    } else {
        esp_wifi_stop();
    }
}

static void init_wifi_screen()
{
    lv_obj_t* screen = my_screen_create("Wi-Fi", NULL);
    lv_obj_t* content = my_screen_get_content(screen);

    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, "Enable Wi-Fi");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t* switch_wifi = lv_switch_create(content);
    lv_obj_align(switch_wifi, LV_ALIGN_CENTER, 0, 0);
    lv_obj_add_event_cb(switch_wifi, switch_wifi_cb, LV_EVENT_VALUE_CHANGED, NULL);
    if (wifi_manager_is_started()) {
        lv_obj_add_state(switch_wifi, LV_STATE_CHECKED);
    }
    
    add_path(screen);
}

static void switch_lp_cb(lv_event_t* e)
{
    lv_obj_t* sw = lv_event_get_target(e);
    if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        low_power_enter();
    } else {
        low_power_exit();
    }
}

static void lp_wake_cb(void)
{
    if (switch_lp) {
        lvgl_port_lock(0);
        lv_obj_remove_state(switch_lp, LV_STATE_CHECKED);
        lvgl_port_unlock();
    }
}

static void lp_screen_exit(void)
{
    switch_lp = NULL;
    low_power_set_wake_cb(NULL);
}

static void init_lp_screen()
{
    lv_obj_t* screen = my_screen_create("Low Power", lp_screen_exit);
    lv_obj_t* content = my_screen_get_content(screen);

    lv_obj_t* label = lv_label_create(content);
    lv_label_set_text(label, "Low Power Mode");
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 30);

    switch_lp = lv_switch_create(content);
    lv_obj_align(switch_lp, LV_ALIGN_CENTER, 0, 0);
    if (low_power_is_sleeping()) {
        lv_obj_add_state(switch_lp, LV_STATE_CHECKED);
    }
    lv_obj_add_event_cb(switch_lp, switch_lp_cb, LV_EVENT_VALUE_CHANGED, NULL);

    low_power_set_wake_cb(lp_wake_cb);

    add_path(screen);
}

static void init_about_screen()
{
    lv_obj_t* screen = my_screen_create("About", NULL);
    lv_obj_t* content = my_screen_get_content(screen);

    lv_obj_t* label_name = lv_label_create(content);
    lv_label_set_text(label_name, "Smart Puppy v1.0");
    lv_obj_align(label_name, LV_ALIGN_TOP_MID, 0, 30);

    lv_obj_t* label_chip = lv_label_create(content);
    lv_label_set_text(label_chip, "ESP32-S3-WROOM-1");
    lv_obj_align(label_chip, LV_ALIGN_CENTER, 0, -20);

    lv_obj_t* label_blank = lv_label_create(content);
    lv_label_set_text(label_blank, "xiaozhi_esp32 v2.2.6");
    lv_obj_align(label_blank, LV_ALIGN_CENTER, 0, 10);

    add_path(screen);
}

void app_init_settings(void)
{
    lv_obj_t* screen = my_screen_create("Settings", NULL);
    lv_obj_t* content = my_screen_get_content(screen);

    lv_obj_t* list = lv_list_create(content);
    lv_obj_set_size(list, lv_pct(100), lv_pct(100));

    lv_obj_t* btn;
    lv_obj_t* img;

    btn = lv_list_add_button(list, FONT_SYMBOL_16_BRIGHTNESS, "Brightness");
    img = lv_obj_get_child(btn, 0);
    lv_obj_add_style(img, &style_symbol_16, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, init_brightness_screen, LV_EVENT_PRESSED, NULL);

    btn = lv_list_add_button(list, FONT_SYMBOL_16_VOLUME, "Volume");
    img = lv_obj_get_child(btn, 0);
    lv_obj_add_style(img, &style_symbol_16, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, init_volume_screen, LV_EVENT_PRESSED, NULL);

    btn = lv_list_add_button(list, FONT_SYMBOL_16_THEME, "Theme");
    img = lv_obj_get_child(btn, 0);
    lv_obj_add_style(img, &style_symbol_16, LV_PART_MAIN);
    lv_obj_add_event_cb(btn, init_theme_screen, LV_EVENT_PRESSED, NULL);

    btn = lv_list_add_button(list, LV_SYMBOL_WIFI, "Wi-Fi");
    lv_obj_add_event_cb(btn, init_wifi_screen, LV_EVENT_PRESSED, NULL);

    btn = lv_list_add_button(list, LV_SYMBOL_POWER, "Sleep");
    lv_obj_add_event_cb(btn, init_lp_screen, LV_EVENT_PRESSED, NULL);

    btn = lv_list_add_button(list, LV_SYMBOL_HOME, "About");
    lv_obj_add_event_cb(btn, init_about_screen, LV_EVENT_PRESSED, NULL);

    add_path(screen);
}
