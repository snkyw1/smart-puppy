#pragma once
#include "src/misc/lv_style.h"

#define UI_COLOR_FORE     LV_COLOR_MAKE(0x26, 0x32, 0x38)
#define UI_COLOR_BACK     LV_COLOR_MAKE(0xF5, 0xF5, 0xF5)

#define UI_COLOR_GREY     LV_COLOR_MAKE(0xAA, 0xAA, 0xAA)
#define UI_COLOR_GREEN    LV_COLOR_MAKE(0x28, 0xA7, 0x45)
#define UI_COLOR_RED      LV_COLOR_MAKE(0xD7, 0x3A, 0x4A)
#define UI_COLOR_BLUE     LV_COLOR_MAKE(0x21, 0x96, 0xF3)
#define UI_COLOR_ORANGE   LV_COLOR_MAKE(0xFF, 0xA5, 0x00)

/** ============ COMMON ============= **/
extern const lv_style_t style_focus_key;
extern const lv_style_t style_msgbox;
extern const lv_style_t style_line;
extern const lv_style_t style_fore_color;
extern const lv_style_t style_bg_color;

extern const lv_style_t style_header;
extern const lv_style_t style_header_title;
extern const lv_style_t style_content;

extern const lv_style_t style_symbol_16;
extern const lv_style_t style_cn_font;
extern const lv_style_t style_text_color_red;
extern const lv_style_t style_text_color_green;
extern const lv_style_t style_text_color_back;

/** ============ UI_HOME ============= **/

extern const lv_style_t style_panel;

extern const lv_style_t style_app_bg;
extern const lv_style_t style_app;
extern const lv_style_t style_app_title;

extern const lv_style_t style_status_bar;
extern const lv_style_t style_status_left;
extern const lv_style_t style_status_right;
extern const lv_style_t style_label_time;
extern const lv_style_t style_symbol_battery;

/** ============ chat.c ============= **/
extern const lv_style_t style_chat_label_message;
extern const lv_style_t style_chat_label_status;

/** ============ servo_ctrl.c ============= **/
extern const lv_style_t style_matrix_servo_ctrl;
extern const lv_style_t style_matrix_servo_ctrl_item;
extern const lv_style_t style_matrix_item_on_focus;

/** ============ wifi.c ============= **/
extern const lv_style_t style_wifi_list;
extern const lv_style_t style_wifi_input_area;
extern const lv_style_t style_wifi_keyboard;
extern const lv_style_t style_wifi_switch_bg;
extern const lv_style_t style_wifi_symbol_config;
extern const lv_style_t style_wifi_spinner;
extern const lv_style_t style_wifi_list_symbol;
extern const lv_style_t style_wifi_list_symbol_fail;

/** ============ music.c ======================== **/
extern const lv_style_t style_music_list_label_artist;
extern const lv_style_t style_music_label_artist;
extern const lv_style_t style_music_lyric_container;
extern const lv_style_t style_music_label_lyric;
extern const lv_style_t style_music_label_lyric_next;
extern const lv_style_t style_music_bar_progress;
extern const lv_style_t style_music_bar_progress_indicator;
extern const lv_style_t style_music_label_time;
extern const lv_style_t style_music_label_time_full;
extern const lv_style_t style_music_btn_container;
extern const lv_style_t style_music_btn_prev;
extern const lv_style_t style_music_btn_play;

/** ============ recorder.c ============= **/
extern const lv_style_t style_recorder_label_time;
extern const lv_style_t style_recorder_btn_record;
extern const lv_style_t style_recorder_btn_play;
extern const lv_style_t style_recorder_btn_running;
extern const lv_style_t style_recorder_progress_bar;
extern const lv_style_t style_recorder_btn_text;
extern const lv_style_t style_recorder_label_status;

/** ============ iot.c ============= **/
extern const lv_style_t style_iot_ali_status;
extern const lv_style_t style_iot_label_light;
extern const lv_style_t style_iot_separator;
extern const lv_style_t style_iot_label_espnow;
extern const lv_style_t style_iot_label_led_ctrl;
extern const lv_style_t style_iot_sw_led;
extern const lv_style_t style_iot_label_led_state;

