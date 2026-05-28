#include "key/key.h"
#include "lvgl.h"
#include "style/my_style.h"
#include "font/my_fonts.h"
#include "ui/ui.h"
#include "src/core/lv_obj_private.h"

#include "wifi_manager.h"
#include <stdlib.h>
#include <string.h>

#include "esp_lvgl_port.h"

static lv_obj_t* list_wifi;
static lv_obj_t* input_area;
static lv_obj_t* label_pwd_title;
static lv_obj_t* spinner;
static lv_obj_t* switch_smartconfig;

static bool is_checked = false;

void wifi_smartconfig_cb(bool is_done)
{
    lvgl_port_lock(0);
    if (!is_done)
    {
        if (spinner)
            lv_obj_remove_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        is_checked = !is_checked;
        status_net_config_show(is_checked);
        if (switch_smartconfig)
            lv_obj_remove_state(switch_smartconfig, LV_STATE_CHECKED);
    }
    lvgl_port_unlock();
}

void wifi_connect_cb(bool is_connected, const char* wifi_ssid)
{
    lvgl_port_lock(0);

    status_wifi_show(is_connected);

    if (!list_wifi) {
        lvgl_port_unlock();
        return;
    }

    for (uint32_t i = 0; i < lv_obj_get_child_count(list_wifi); i++)
    {
        lv_obj_t* btn = lv_obj_get_child(list_wifi, i);
        wifi_network* net = btn->user_data;
        lv_obj_t* img = lv_obj_get_child(btn, 0);

        const char* src = lv_image_get_src(img);
        if (src != NULL && strcmp(src, LV_SYMBOL_OK) == 0)
        {
            lv_image_set_src(img, NULL);
        }

        if (strncmp(wifi_ssid, net->ssid, sizeof(net->ssid)) == 0)
        {
            if (is_connected)
            {
                lv_image_set_src(img, LV_SYMBOL_OK);
                lv_obj_remove_style(img, &style_wifi_list_symbol_fail, LV_PART_MAIN);
            }
            else
            {
                lv_image_set_src(img, LV_SYMBOL_CLOSE);
                lv_obj_add_style(img, &style_wifi_list_symbol_fail, LV_PART_MAIN);
            }
        }
    }

    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);

    lvgl_port_unlock();
}

void wifi_connect_load(const char* ssid, const char* pwd)
{
    lv_obj_remove_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    wifi_manager_connect(ssid, pwd);
}

static void wifi_pwd_ready_handler(lv_event_t* e)
{
    const char* ssid = lv_label_get_text(label_pwd_title);
    const char* pwd = lv_textarea_get_text(input_area);
    delete_path();
    wifi_connect_load(ssid, pwd);
}

static void wifi_pwd_page_init(const char* ssid)
{
    lv_obj_t* screen_pwd = my_screen_create(NULL, key_edit_mode_invert);
    label_pwd_title = my_screen_get_title(screen_pwd);
    lv_label_set_text_static(label_pwd_title, ssid);
    lv_obj_add_style(label_pwd_title, &style_cn_font, LV_PART_MAIN);
    
    lv_obj_t* content_pwd = my_screen_get_content(screen_pwd);

    input_area = lv_textarea_create(content_pwd);
    lv_textarea_set_one_line(input_area, true);
    lv_textarea_set_max_length(input_area, 63);
    char password[64] = "";
    wifi_manager_get_config_password(ssid, password);
    lv_textarea_set_text(input_area, password);
    lv_textarea_set_placeholder_text(input_area, "input password");
    lv_obj_add_style(input_area, &style_wifi_input_area, LV_PART_MAIN);
    lv_obj_set_scrollbar_mode(input_area, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_event_cb(input_area, wifi_pwd_ready_handler, LV_EVENT_READY, NULL);

    lv_obj_t* keyboard = lv_keyboard_create(content_pwd);
    lv_keyboard_set_textarea(keyboard, input_area);
    lv_obj_add_style(keyboard, &style_wifi_keyboard, LV_STATE_FOCUS_KEY);

    lv_buttonmatrix_set_selected_button(keyboard, 39);

    add_path(screen_pwd);

    key_edit_mode_invert();
}

static void wifi_list_btn_event_handler(lv_event_t* e)
{
    lv_obj_t* btn = lv_event_get_target(e);

    wifi_network* net = btn->user_data;
    if (net->is_secure == false)
    {
        wifi_connect_load(net->ssid, "");
        return;
    }

    wifi_pwd_page_init(net->ssid);
}

static void wifi_add_network(wifi_network* net)
{
    lv_obj_t* btn = lv_list_add_button(list_wifi, "", net->ssid);
    lv_obj_add_event_cb(btn, wifi_list_btn_event_handler, LV_EVENT_PRESSED, NULL);
    btn->user_data = net;
    lv_obj_add_style(btn, &style_cn_font, LV_PART_MAIN);

    lv_obj_t* img = lv_obj_get_child(btn, 0);
    lv_image_set_src(img, NULL);
    lv_obj_add_style(img, &style_wifi_list_symbol, LV_PART_MAIN);

    if (net->is_secure)
    {
        lv_obj_t* label_lock = lv_label_create(btn);
        lv_label_set_text(label_lock, FONT_SYMBOL_16_LOCK);
        lv_obj_add_style(label_lock, &style_symbol_16, LV_PART_MAIN);
    }

    lv_obj_t* label_signal = lv_label_create(btn);
    if (net->rssi < -66)
        lv_label_set_text(label_signal, FONT_SYMBOL_16_SIGNAL_WEAK);
    else if (net->rssi < -33)
        lv_label_set_text(label_signal, FONT_SYMBOL_16_SIGNAL_HAIR);
    else
        lv_label_set_text(label_signal, FONT_SYMBOL_16_SIGNAL_FULL);

    lv_obj_add_style(label_signal, &style_symbol_16, LV_PART_MAIN);
}

void wifi_scan_cb(wifi_network* nets, uint8_t count, const char* connected_ssid)
{
    lvgl_port_lock(0);

    for (uint8_t i = 0; i < count; i++)
    {
        wifi_add_network(&nets[i]);
        if (connected_ssid != NULL && strncmp(connected_ssid, nets[i].ssid, sizeof(nets[i].ssid)) == 0)
        {
            lv_obj_t* img = lv_obj_get_child(lv_obj_get_child(list_wifi, i), 0);
            lv_image_set_src(img, LV_SYMBOL_OK);
        }
    }

    lv_obj_add_flag(spinner, LV_OBJ_FLAG_HIDDEN);
    lvgl_port_unlock();
}

static void switch_smartconfig_cb(lv_event_t* e)
{
    is_checked = !is_checked;
    status_net_config_show(is_checked);
    wifi_manager_smartconfig(is_checked);
}

static void symbol_config_cb(lv_event_t* e)
{
    if (!wifi_manager_is_started()) return;
    lv_obj_t* msgbox = my_msgbox_create(true);
    lv_msgbox_add_title(msgbox, "smartconfig on-off");

    lv_obj_t* msgbox_content = lv_msgbox_get_content(msgbox);
    switch_smartconfig = lv_switch_create(msgbox_content);
    if (is_checked)
        lv_obj_add_state(switch_smartconfig, LV_STATE_CHECKED);
    lv_obj_add_event_cb(switch_smartconfig, switch_smartconfig_cb, LV_EVENT_VALUE_CHANGED, NULL);

    add_path(msgbox);
}

static void screen_list_exit_cb(void)
{
    lv_obj_t* btn = lv_obj_get_child(list_wifi, 0);
    if (btn != NULL && btn->user_data != NULL)
        free(btn->user_data);
    list_wifi = NULL;
    // lv_obj_clean(list_wifi);
}

static void wifi_list_page_init()
{
    lv_obj_t* screen_list = my_screen_create("Wi-Fi", screen_list_exit_cb);
    lv_obj_t* header = my_screen_get_header(screen_list);

    lv_obj_t* symbol_config = lv_label_create(header);
    lv_label_set_text(symbol_config, FONT_SYMBOL_16_24_NET_CONFIG);
    lv_obj_add_style(symbol_config, &style_wifi_symbol_config, LV_PART_MAIN);
    lv_obj_add_style(symbol_config, &style_focus_key, LV_STATE_FOCUS_KEY);
    lv_group_add_obj(my_screen_get_group(screen_list), symbol_config);

    lv_obj_add_event_cb(symbol_config, symbol_config_cb, LV_EVENT_PRESSED, NULL);

    lv_obj_t* content_list = my_screen_get_content(screen_list);

    // 创建Wi-Fi列表
    list_wifi = lv_list_create(content_list);
    lv_obj_add_style(list_wifi, &style_wifi_list, LV_PART_MAIN);

    spinner = lv_spinner_create(content_list);
    lv_obj_add_style(spinner, &style_wifi_spinner, LV_PART_MAIN);

    add_path(screen_list);

    wifi_manager_scan();
}


void app_init_wifi(void)
{
    wifi_list_page_init();
}
