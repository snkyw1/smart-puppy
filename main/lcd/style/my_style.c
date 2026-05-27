#include "src/misc/lv_style.h"
#include "src/misc/lv_style_gen.h"
#include "font/my_fonts.h"
#include "style/my_style.h"

#define UI_HEADER_HEIGHT 40
#define UI_CONTENT_HEIGHT (CONFIG_LCD_HEIGHT-UI_HEADER_HEIGHT)

/** ========================= COMMON ============================== **/

static const lv_style_const_prop_t style_focus_key_props[] = {
    LV_STYLE_CONST_OUTLINE_COLOR(UI_COLOR_BLUE),
    LV_STYLE_CONST_OUTLINE_WIDTH(4),
    LV_STYLE_CONST_OUTLINE_PAD(4),
    LV_STYLE_CONST_OUTLINE_OPA(LV_OPA_50),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_focus_key, style_focus_key_props);

static const lv_style_const_prop_t style_msgbox_props[] = {
    LV_STYLE_CONST_WIDTH(200),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_msgbox, style_msgbox_props);

static const lv_style_const_prop_t style_line_props[] = {
    LV_STYLE_CONST_LINE_WIDTH(8),
    LV_STYLE_CONST_LINE_ROUNDED(true),
    LV_STYLE_CONST_LINE_COLOR(UI_COLOR_FORE),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_line, style_line_props);

static const lv_style_const_prop_t style_fore_color_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_FORE),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_fore_color, style_fore_color_props);

static const lv_style_const_prop_t style_back_color_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_bg_color, style_back_color_props);

static const lv_style_const_prop_t style_header_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_FORE),
    LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH),
    LV_STYLE_CONST_HEIGHT(UI_HEADER_HEIGHT),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_PAD_RIGHT(10),
    LV_STYLE_CONST_PAD_LEFT(10),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_header, style_header_props);

static const lv_style_const_prop_t style_header_title_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_header_title, style_header_title_props);

static const lv_style_const_prop_t style_content_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH),
    LV_STYLE_CONST_HEIGHT(UI_CONTENT_HEIGHT),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_content, style_content_props);

const lv_style_const_prop_t style_symbol_16_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&font_awesome_16),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_symbol_16, style_symbol_16_props);

const lv_style_const_prop_t style_cn_font_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&han_sans_cn_medium_16),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_cn_font, style_cn_font_props);

const lv_style_const_prop_t style_text_red_color_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_RED),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_text_color_red, style_text_red_color_props);

const lv_style_const_prop_t style_text_color_green_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREEN),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_text_color_green, style_text_color_green_props);

const lv_style_const_prop_t style_text_color_back_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_text_color_back, style_text_color_back_props);

/** ========================= UI_HOME ============================== **/
static const lv_style_const_prop_t style_panel_props[] = {
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH),
    LV_STYLE_CONST_HEIGHT(140),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_BG_OPA(LV_OPA_0),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_PAD_COLUMN(25),
    LV_STYLE_CONST_BORDER_WIDTH(8),
    LV_STYLE_CONST_BORDER_SIDE(LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_BOTTOM),
    LV_STYLE_CONST_BORDER_COLOR(UI_COLOR_FORE),
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_ROW),
    LV_STYLE_CONST_FLEX_TRACK_PLACE(LV_FLEX_ALIGN_CENTER),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_panel, style_panel_props);

static const lv_style_const_prop_t style_app_bg_props[] = {
    LV_STYLE_CONST_WIDTH(80),
    LV_STYLE_CONST_HEIGHT(80),
    LV_STYLE_CONST_BG_COLOR(LV_COLOR_MAKE(0x00, 0xBC, 0xD4)),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_app_bg, style_app_bg_props);

static const lv_style_const_prop_t style_app_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_TEXT_FONT(&font_awesome_64),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_app, style_app_props);

static const lv_style_const_prop_t style_app_title_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_app_title, style_app_title_props);

static const lv_style_const_prop_t style_status_bar_props[] = {
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH),
    LV_STYLE_CONST_HEIGHT(24),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_PAD_RIGHT(5),
    LV_STYLE_CONST_PAD_LEFT(5),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_status_bar, style_status_bar_props);

static const lv_style_const_prop_t style_status_left_props[] = {
    LV_STYLE_CONST_WIDTH(120),
    LV_STYLE_CONST_HEIGHT(24),
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_ROW),
    LV_STYLE_CONST_FLEX_MAIN_PLACE(LV_FLEX_ALIGN_START),
    LV_STYLE_CONST_FLEX_TRACK_PLACE(LV_FLEX_ALIGN_CENTER),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_LEFT_MID),
    LV_STYLE_CONST_PAD_COLUMN(5),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_status_left, style_status_left_props);

static const lv_style_const_prop_t style_status_right_props[] = {
    LV_STYLE_CONST_WIDTH(120),
    LV_STYLE_CONST_HEIGHT(24),
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_ROW_REVERSE),
    LV_STYLE_CONST_FLEX_MAIN_PLACE(LV_FLEX_ALIGN_END),
    LV_STYLE_CONST_FLEX_TRACK_PLACE(LV_FLEX_ALIGN_CENTER),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_RIGHT_MID),
    LV_STYLE_CONST_PAD_COLUMN(5),
    LV_STYLE_CONST_PAD_RIGHT(22),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_status_right, style_status_right_props);

static const lv_style_const_prop_t style_label_time_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&han_sans_cn_medium_16),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_label_time, style_label_time_props);

static const lv_style_const_prop_t style_symbol_battery_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&font_awesome_18),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_RIGHT_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_symbol_battery, style_symbol_battery_props);


/** ========================= chat.c ============================== **/
const lv_style_const_prop_t style_chat_label_message_props[] = {
    LV_STYLE_CONST_MAX_WIDTH(CONFIG_LCD_WIDTH-40),
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_TEXT_FONT(&han_sans_cn_medium_16),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_Y(-20),
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_BG_OPA(30),
    LV_STYLE_CONST_RADIUS(5),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_chat_label_message, style_chat_label_message_props);

const lv_style_const_prop_t style_chat_label_status_props[] = {
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH-100),
    LV_STYLE_CONST_TEXT_ALIGN(LV_TEXT_ALIGN_CENTER),
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_TEXT_FONT(&han_sans_cn_medium_16),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_Y(20),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_chat_label_status, style_chat_label_status_props);


/** ========================= servo_ctrl.c ============================== **/
const lv_style_const_prop_t style_matrix_servo_ctrl_props[] = {
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH),
    LV_STYLE_CONST_HEIGHT(CONFIG_LCD_HEIGHT),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_TEXT_FONT(&han_sans_cn_medium_16),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_BORDER_WIDTH(1),
    LV_STYLE_CONST_BORDER_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_PAD_TOP(0),
    LV_STYLE_CONST_PAD_BOTTOM(0),
    LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_PAD_COLUMN(0),
    LV_STYLE_CONST_PAD_ROW(0),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_matrix_servo_ctrl, style_matrix_servo_ctrl_props);

const lv_style_const_prop_t style_matrix_servo_ctrl_item_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_BORDER_WIDTH(1),
    LV_STYLE_CONST_BORDER_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_matrix_servo_ctrl_item, style_matrix_servo_ctrl_item_props);

const lv_style_const_prop_t style_matrix_item_on_focus_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_BLUE),
    LV_STYLE_CONST_OUTLINE_WIDTH(0),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_matrix_item_on_focus, style_matrix_item_on_focus_props);


/** ========================= wifi.c ============================== **/
const lv_style_const_prop_t style_wifi_list_props[] = {
    LV_STYLE_CONST_WIDTH(LV_PCT(100)),
    LV_STYLE_CONST_HEIGHT(LV_PCT(100)),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_list, style_wifi_list_props);

const lv_style_const_prop_t style_wifi_input_area_props[] = {
    LV_STYLE_CONST_WIDTH(CONFIG_LCD_WIDTH),
    LV_STYLE_CONST_HEIGHT(40),
    LV_STYLE_CONST_RADIUS(0),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_input_area, style_wifi_input_area_props);

const lv_style_const_prop_t style_wifi_keyboard_props[] = {
    LV_STYLE_CONST_OUTLINE_WIDTH(0),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_keyboard, style_wifi_keyboard_props);


const lv_style_const_prop_t style_wifi_symbol_config_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&font_awesome_24),
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_RIGHT_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_symbol_config, style_wifi_symbol_config_props);

const lv_style_const_prop_t style_wifi_spinner_props[] = {
    LV_STYLE_CONST_WIDTH(150),
    LV_STYLE_CONST_HEIGHT(150),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_spinner, style_wifi_spinner_props);

const lv_style_const_prop_t style_wifi_list_symbol_props[] = {
    LV_STYLE_CONST_WIDTH(14),
    LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(0x00, 0xFF, 0x00)),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_list_symbol, style_wifi_list_symbol_props);

const lv_style_const_prop_t style_wifi_list_symbol_fail_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(LV_COLOR_MAKE(0xFF, 0x00, 0x00)),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_wifi_list_symbol_fail, style_wifi_list_symbol_fail_props);

/** ========================== music.c ================================ **/
#define MUSIC_COLOR_PRIMARY  LV_COLOR_MAKE(0xE9, 0x45, 0x60) // 霓虹红
#define MUSIC_COLOR_ACCENT   LV_COLOR_MAKE(0x0F, 0x34, 0x60)

static const lv_style_const_prop_t style_music_list_label_artist_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_list_label_artist, style_music_list_label_artist_props);

static const lv_style_const_prop_t style_music_label_artist_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_Y(5),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_label_artist, style_music_label_artist_props);

static const lv_style_const_prop_t style_music_lyric_container_props[] = {
    LV_STYLE_CONST_WIDTH(300),
    LV_STYLE_CONST_HEIGHT(80),
    LV_STYLE_CONST_BG_COLOR(MUSIC_COLOR_ACCENT),
    LV_STYLE_CONST_RADIUS(12),
    LV_STYLE_CONST_PAD_TOP(8),
    LV_STYLE_CONST_PAD_BOTTOM(8),
    LV_STYLE_CONST_PAD_LEFT(8),
    LV_STYLE_CONST_PAD_RIGHT(8),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_Y(24),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_lyric_container, style_music_lyric_container_props);

static const lv_style_const_prop_t style_music_label_lyric_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
    LV_STYLE_CONST_WIDTH(280),
    LV_STYLE_CONST_TEXT_COLOR(MUSIC_COLOR_PRIMARY),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_label_lyric, style_music_label_lyric_props);

static const lv_style_const_prop_t style_music_label_lyric_next_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_16),
    LV_STYLE_CONST_WIDTH(280),
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_Y(-6),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_label_lyric_next, style_music_label_lyric_next_props);

static const lv_style_const_prop_t style_music_bar_progress_props[] = {
    LV_STYLE_CONST_WIDTH(290),
    LV_STYLE_CONST_HEIGHT(12),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_Y(-70),
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_bar_progress, style_music_bar_progress_props);

static const lv_style_const_prop_t style_music_bar_progress_indicator_props[] = {
    LV_STYLE_CONST_BG_COLOR(MUSIC_COLOR_PRIMARY),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_bar_progress_indicator, style_music_bar_progress_indicator_props);

static const lv_style_const_prop_t style_music_label_time_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_LEFT),
    LV_STYLE_CONST_X(16),
    LV_STYLE_CONST_Y(-50),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_label_time, style_music_label_time_props);

static const lv_style_const_prop_t style_music_label_time_full_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_RIGHT),
    LV_STYLE_CONST_X(-16),
    LV_STYLE_CONST_Y(-50),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_label_time_full, style_music_label_time_full_props);

static const lv_style_const_prop_t style_music_btn_container_props[] = {
    LV_STYLE_CONST_WIDTH(220),
    LV_STYLE_CONST_HEIGHT(80),
    LV_STYLE_CONST_BG_OPA(LV_OPA_TRANSP),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_PAD_BOTTOM(8),
    LV_STYLE_CONST_FLEX_FLOW(LV_FLEX_FLOW_ROW),
    LV_STYLE_CONST_LAYOUT(LV_LAYOUT_FLEX),
    LV_STYLE_CONST_FLEX_MAIN_PLACE(LV_FLEX_ALIGN_SPACE_EVENLY),
    LV_STYLE_CONST_FLEX_CROSS_PLACE(LV_FLEX_ALIGN_CENTER),
    LV_STYLE_CONST_FLEX_TRACK_PLACE(LV_FLEX_ALIGN_CENTER),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_btn_container, style_music_btn_container_props);

static const lv_style_const_prop_t style_music_btn_prev_props[] = {
    LV_STYLE_CONST_WIDTH(40),
    LV_STYLE_CONST_HEIGHT(40),
    LV_STYLE_CONST_BG_COLOR(MUSIC_COLOR_ACCENT),
    LV_STYLE_CONST_RADIUS(20),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_btn_prev, style_music_btn_prev_props);

static const lv_style_const_prop_t style_music_btn_play_props[] = {
    LV_STYLE_CONST_WIDTH(50),
    LV_STYLE_CONST_HEIGHT(50),
    LV_STYLE_CONST_BG_COLOR(MUSIC_COLOR_PRIMARY),
    LV_STYLE_CONST_RADIUS(25),
    LV_STYLE_CONST_SHADOW_WIDTH(10),
    LV_STYLE_CONST_SHADOW_COLOR(MUSIC_COLOR_PRIMARY),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_music_btn_play, style_music_btn_play_props);


/** ========================= recorder.c ============================== **/
static const lv_style_const_prop_t style_recorder_label_time_props[] = {
    LV_STYLE_CONST_TEXT_FONT(&lv_font_montserrat_28),
    LV_STYLE_CONST_WIDTH(126),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_Y(-60),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_label_time, style_recorder_label_time_props);

static const lv_style_const_prop_t style_recorder_btn_record_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_RED),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_WIDTH(90),
    LV_STYLE_CONST_HEIGHT(50),
    LV_STYLE_CONST_X(-50),
    LV_STYLE_CONST_Y(20),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_btn_record, style_recorder_btn_record_props);

static const lv_style_const_prop_t style_recorder_btn_play_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_GREEN),
    LV_STYLE_CONST_WIDTH(90),
    LV_STYLE_CONST_HEIGHT(50),
    LV_STYLE_CONST_X(50),
    LV_STYLE_CONST_Y(20),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_btn_play, style_recorder_btn_play_props);

static const lv_style_const_prop_t style_recorder_btn_running_props[] = {
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_ORANGE),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_btn_running, style_recorder_btn_running_props);

static const lv_style_const_prop_t style_recorder_progress_bar_props[] = {
    LV_STYLE_CONST_WIDTH(200),
    LV_STYLE_CONST_HEIGHT(12),
    LV_STYLE_CONST_Y(-25),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_BOTTOM_MID),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_progress_bar, style_recorder_progress_bar_props);

static const lv_style_const_prop_t style_recorder_btn_text_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_BACK),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_btn_text, style_recorder_btn_text_props);

static const lv_style_const_prop_t style_recorder_label_status_props[] = {
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_CENTER),
    LV_STYLE_CONST_Y(-25),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_recorder_label_status, style_recorder_label_status_props);

/** ========================= iot.c ============================== **/

const lv_style_const_prop_t style_iot_ali_status_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_LEFT),
    LV_STYLE_CONST_X(15),
    LV_STYLE_CONST_Y(10),
    LV_STYLE_CONST_TEXT_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_ali_status, style_iot_ali_status_props);

const lv_style_const_prop_t style_iot_label_light_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_LEFT),
    LV_STYLE_CONST_X(15),
    LV_STYLE_CONST_Y(50),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_label_light, style_iot_label_light_props);

const lv_style_const_prop_t style_iot_separator_props[] = {
    LV_STYLE_CONST_WIDTH(LV_PCT(90)),
    LV_STYLE_CONST_HEIGHT(2),
    LV_STYLE_CONST_BORDER_WIDTH(0),
    LV_STYLE_CONST_PAD_TOP(0),
    LV_STYLE_CONST_PAD_BOTTOM(0),
    LV_STYLE_CONST_PAD_LEFT(0),
    LV_STYLE_CONST_PAD_RIGHT(0),
    LV_STYLE_CONST_BG_COLOR(UI_COLOR_GREY),
    LV_STYLE_CONST_BG_OPA(LV_OPA_COVER),
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_Y(105),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_separator, style_iot_separator_props);

const lv_style_const_prop_t style_iot_label_espnow_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_LEFT),
    LV_STYLE_CONST_X(15),
    LV_STYLE_CONST_Y(115),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_label_espnow, style_iot_label_espnow_props);

const lv_style_const_prop_t style_iot_label_led_ctrl_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_LEFT),
    LV_STYLE_CONST_X(15),
    LV_STYLE_CONST_Y(155),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_label_led_ctrl, style_iot_label_led_ctrl_props);

const lv_style_const_prop_t style_iot_sw_led_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_MID),
    LV_STYLE_CONST_Y(135),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_sw_led, style_iot_sw_led_props);

const lv_style_const_prop_t style_iot_label_led_state_props[] = {
    LV_STYLE_CONST_ALIGN(LV_ALIGN_TOP_RIGHT),
    LV_STYLE_CONST_X(-70),
    LV_STYLE_CONST_Y(155),
    LV_STYLE_CONST_PROPS_END
};
LV_STYLE_CONST_INIT(style_iot_label_led_state, style_iot_label_led_state_props);

