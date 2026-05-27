#include "lvgl.h"
#include "font/my_fonts.h"
#include "ui/ui.h"
#include "style/my_style.h"
#include "esp_lvgl_port.h"
#include "espnow/espnow_mgr.h"
#include "ali_iot/ali_iot.h"
#include "esp_log.h"
#include "wifi_manager.h"

#define TAG "IOT"

static lv_obj_t* sw_led;
static lv_obj_t* label_led_state;
static lv_obj_t* label_light;
static lv_obj_t* label_ali_status;
static lv_obj_t* label_espnow;

static bool led_is_on = false;
static bool active_level = 0;
static int current_level = -1;

static void light_recv_cb(int level)
{
    current_level = level;

    lvgl_port_lock(0);

    if (label_light) {
        lv_label_set_text_fmt(label_light, "light level: %d", level);
    }

    lvgl_port_unlock();
}

static void conn_status_cb(bool connected)
{
    lvgl_port_lock(0);

    if (!label_ali_status) {
        lvgl_port_unlock();
        return;
    }

    if (connected) {
        lv_label_set_text(label_ali_status, "aliyun: connected");
        lv_obj_add_style(label_ali_status, &style_text_color_green, LV_PART_MAIN);
    } else {
        lv_label_set_text(label_ali_status, "aliyun: disconnected");
        lv_obj_remove_style(label_ali_status, &style_text_color_green, LV_PART_MAIN);
        lv_obj_add_style(label_ali_status, &style_text_color_red, LV_PART_MAIN);
    }

    lvgl_port_unlock();

    if (label_ali_status) {
        lv_label_set_text(label_ali_status, "aliyun: connected");
        lv_obj_add_style(label_ali_status, &style_text_color_green, LV_PART_MAIN);
    }

    lvgl_port_unlock();
}

static void recv_cb(const espnow_led_cmd_t* cmd)
{
    if (strncmp(cmd->cmd, "led", ESPNOW_LED_CMD_LEN) != 0) return;

    lvgl_port_lock(0);

    if (sw_led == NULL || label_led_state == NULL) {
        lvgl_port_unlock();
        return;
    }

    if (cmd->value == active_level) {
        led_is_on = true;
        lv_obj_add_state(sw_led, LV_STATE_CHECKED);
        lv_label_set_text(label_led_state, "ON");
        lv_obj_set_style_text_color(label_led_state, (lv_color_t)UI_COLOR_GREEN, 0);
    } else {
        led_is_on = false;
        lv_obj_remove_state(sw_led, LV_STATE_CHECKED);
        lv_label_set_text(label_led_state, "OFF");
        lv_obj_set_style_text_color(label_led_state, (lv_color_t)UI_COLOR_RED, 0);
    }
    lvgl_port_unlock();
}

static void switch_led_cb(lv_event_t* e)
{
    lv_obj_t* sw = lv_event_get_target(e);
    bool checked = lv_obj_has_state(sw, LV_STATE_CHECKED);

    esp_err_t ret = espnow_mgr_send_led(checked ? active_level : !active_level);

    if (ret == ESP_OK) {
        lv_obj_set_style_text_color(label_led_state, (lv_color_t)UI_COLOR_FORE, 0);
        lv_label_set_text(label_led_state, "Send success");
    } else {
        lv_obj_set_style_text_color(label_led_state, (lv_color_t)UI_COLOR_RED, 0);
        lv_label_set_text(label_led_state, "Send failed!");
    }
}

static void screen_exit_cb(void)
{
    ali_iot_deinit();
    espnow_mgr_deinit();
    sw_led = NULL;
    label_led_state = NULL;
    label_light = NULL;
    label_ali_status = NULL;
    label_espnow = NULL;
}

static void screen_loaded_cb()
{
    if (!wifi_manager_is_start()) return;

    esp_err_t ret = espnow_mgr_init();
    if (ret != ESP_OK) {
        lv_label_set_text(label_espnow, "ESP-NOW: init failed");
        lv_obj_add_style(label_espnow, &style_text_color_red, LV_PART_MAIN);
        lv_obj_add_state(sw_led, LV_STATE_DISABLED);
    } else {
        espnow_mgr_set_recv_cb(recv_cb);
    }

    ali_iot_set_light_cb(light_recv_cb);
    ali_iot_set_conn_cb(conn_status_cb);
    ali_iot_init();
}

void app_init_iot(void)
{
    lv_obj_t* screen = my_screen_create("IOT", screen_exit_cb);
    lv_obj_add_event_cb(screen, screen_loaded_cb, LV_EVENT_SCREEN_LOADED, NULL);
    lv_obj_t* content = my_screen_get_content(screen);

    /* ---- Aliyun section (top half) ---- */
    label_ali_status = lv_label_create(content);
    lv_label_set_text(label_ali_status, "aliyun: connecting...");
    lv_obj_add_style(label_ali_status, &style_iot_ali_status, 0);

    label_light = lv_label_create(content);
    lv_label_set_text(label_light, "Light level: ---");
    lv_obj_add_style(label_light, &style_iot_label_light, 0);

    /* ---- Split line (visual middle) ---- */
    lv_obj_t* separator = lv_obj_create(content);
    lv_obj_add_style(separator, &style_iot_separator, 0);

    /* ---- ESP-NOW section (bottom half) ---- */
    label_espnow = lv_label_create(content);
    lv_label_set_text(label_espnow, "ESP-NOW");
    lv_obj_add_style(label_espnow, &style_iot_label_espnow, 0);

    lv_obj_t* label_led_ctrl = lv_label_create(content);
    lv_label_set_text(label_led_ctrl, "LED Control");
    lv_obj_add_style(label_led_ctrl, &style_iot_label_led_ctrl, 0);

    sw_led = lv_switch_create(content);
    if (led_is_on) {
        lv_obj_add_state(sw_led, LV_STATE_CHECKED);
    }
    lv_obj_add_style(sw_led, &style_iot_sw_led, 0);
    lv_obj_add_event_cb(sw_led, switch_led_cb, LV_EVENT_VALUE_CHANGED, NULL);

    label_led_state = lv_label_create(content);
    lv_label_set_text(label_led_state, "--");
    lv_obj_add_style(label_led_state, &style_iot_label_led_state, 0);


    add_path(screen);
}
