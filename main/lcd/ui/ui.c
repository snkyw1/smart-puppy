#include "ui.h"
#include "esp_lvgl_port.h"
#include "font/my_fonts.h"
#include "lvgl.h"
#include "src/core/lv_group_private.h" // IWYU pragma: keep
#include "src/core/lv_obj_private.h"
#include "src/themes/lv_theme_private.h" // IWYU pragma: keep
#include "string.h"
#include "style/my_style.h"
#include <stdlib.h>
#include <time.h>

#define SEL_SIZE   94
#define CORNER_LEN 20

// 一行定义一个角点：{起点}, {角点}, {终点}
#define CORNER_POINTS(cx, cy, dx, dy)                \
    {                                                \
        { cx, cy + dy }, { cx, cy }, { cx + dx, cy } \
    }

const lv_point_precise_t sel_points[][3] = {
    // 左上
    CORNER_POINTS(CONFIG_LCD_WIDTH / 2 - SEL_SIZE / 2, CONFIG_LCD_HEIGHT / 2 - SEL_SIZE / 2, CORNER_LEN, CORNER_LEN),
    // 左下
    CORNER_POINTS(CONFIG_LCD_WIDTH / 2 - SEL_SIZE / 2, CONFIG_LCD_HEIGHT / 2 + SEL_SIZE / 2 - 1, CORNER_LEN, -CORNER_LEN),
    // 右上
    CORNER_POINTS(CONFIG_LCD_WIDTH / 2 + SEL_SIZE / 2 - 1, CONFIG_LCD_HEIGHT / 2 - SEL_SIZE / 2, -CORNER_LEN, CORNER_LEN),
    // 右下
    CORNER_POINTS(CONFIG_LCD_WIDTH / 2 + SEL_SIZE / 2 - 1, CONFIG_LCD_HEIGHT / 2 + SEL_SIZE / 2 - 1, -CORNER_LEN, -CORNER_LEN),
};

static const lv_point_precise_t left_arrow_points[5] = {
    { 25, CONFIG_LCD_HEIGHT - 15 },
    { 15, CONFIG_LCD_HEIGHT - 25 },
    { 25, CONFIG_LCD_HEIGHT - 35 },
    { 15, CONFIG_LCD_HEIGHT - 25 },
    { 45, CONFIG_LCD_HEIGHT - 25 }
};

static const lv_point_precise_t right_arrow_points[5] = {
    { CONFIG_LCD_WIDTH - 25 - 1, CONFIG_LCD_HEIGHT - 15 },
    { CONFIG_LCD_WIDTH - 15 - 1, CONFIG_LCD_HEIGHT - 25 },
    { CONFIG_LCD_WIDTH - 25 - 1, CONFIG_LCD_HEIGHT - 35 },
    { CONFIG_LCD_WIDTH - 15 - 1, CONFIG_LCD_HEIGHT - 25 },
    { CONFIG_LCD_WIDTH - 45 - 1, CONFIG_LCD_HEIGHT - 25 }
};

static lv_indev_t* indev;
static lv_obj_t* arrow_left;
static lv_obj_t* arrow_right;
static lv_obj_t* label_app_title;
static lv_obj_t* status_left;
static lv_obj_t* status_right;
static lv_obj_t* label_time;
static lv_obj_t* label_battery;
static lv_obj_t* label_battery_percentage;

lv_anim_t delay_scroll_anim;

static Path* current_path;
void app_init_chat(void);
void app_init_iot(void);
void app_init_servo_ctrl(void);
void app_init_music(void);
void app_init_settings(void);
void app_init_wifi(void);
void app_init_recorder(void);

static App apps[] = {
    { "Xiaozhi", FONT_SYMBOL_64_BILIBILI, app_init_chat },
    { "Music", FONT_SYMBOL_64_MUSIC, app_init_music },
    { "Servo-Ctrl", FONT_SYMBOL_64_CONTROL, app_init_servo_ctrl },
    { "Wi-Fi", FONT_SYMBOL_64_WIFI, app_init_wifi },
    { "IOT", FONT_SYMBOL_64_PAPERPLANE, app_init_iot },
    { "Recorder", FONT_SYMBOL_64_RECORDER, app_init_recorder },
    { "Settings", FONT_SYMBOL_64_SETTINGS, app_init_settings },
};

#define APP_NUM (sizeof(apps) / sizeof(App))

static void group_switch_cb(lv_event_t* e)
{
    lv_obj_t* screen = lv_event_get_target(e);
    lv_group_t* group = my_screen_get_group(screen);
    lv_indev_set_group(indev, group);
}

lv_obj_t* my_msgbox_create(bool has_group)
{
    lv_obj_t* msgbox = lv_msgbox_create(NULL);
    lv_obj_add_style(msgbox, &style_msgbox, LV_PART_MAIN);
    if (has_group)
    {
        lv_group_t* group = lv_group_create();
        msgbox->user_data = group;
        lv_obj_get_screen(msgbox)->user_data = group;
    }

    lv_obj_add_event_cb(msgbox, group_switch_cb, LV_EVENT_DRAW_POST_END, NULL);

    return msgbox;
}

lv_obj_t* my_screen_create(const char* title, func_screen_cb screen_exit_cb)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_group_t* group = lv_group_create();
    screen->user_data = group;
    group->user_data = screen_exit_cb;
    lv_obj_add_event_cb(screen, group_switch_cb, LV_EVENT_SCREEN_LOADED, NULL);

    lv_obj_t* header = lv_obj_create(screen);
    lv_obj_remove_style_all(header);
    lv_obj_add_style(header, &style_header, LV_PART_MAIN);

    lv_obj_t* label_title = lv_label_create(header);
    if (title != NULL)
    {
        lv_label_set_text_static(label_title, title);
    }
    lv_obj_add_style(label_title, &style_header_title, LV_PART_MAIN);

    lv_obj_t* content = lv_obj_create(screen);
    lv_obj_remove_style_all(content);
    lv_obj_add_style(content, &style_content, LV_PART_MAIN);

    return screen;
}

lv_obj_t* my_empty_screen_create(func_screen_cb screen_exit_cb)
{
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_group_t* group = lv_group_create();
    screen->user_data = group;
    group->user_data = screen_exit_cb;
    lv_obj_add_event_cb(screen, group_switch_cb, LV_EVENT_SCREEN_LOADED, NULL);

    return screen;
}

func_screen_cb my_screen_get_exit_cb(lv_obj_t* screen)
{
    lv_group_t* group = (lv_group_t*)screen->user_data;
    return group ? group->user_data : NULL;
}

lv_group_t* my_screen_get_group(lv_obj_t* screen)
{
    return screen->user_data;
}

lv_obj_t* my_screen_get_content(lv_obj_t* screen)
{
    return lv_obj_get_child(screen, 1);
}

lv_obj_t* my_screen_get_title(lv_obj_t* screen)
{
    return lv_obj_get_child(lv_obj_get_child(screen, 0), 0);
}

lv_obj_t* my_screen_get_header(lv_obj_t* screen)
{
    return lv_obj_get_child(screen, 0);
}

void add_path(lv_obj_t* obj)
{
    if (lv_obj_check_type(obj, &lv_obj_class) && lv_obj_get_parent(obj) == NULL)
    {
        lv_screen_load_anim(obj, LV_SCR_LOAD_ANIM_OVER_BOTTOM, ANIM_DURATION, 0, false);

        lv_group_t* group = my_screen_get_group(obj);
        if (group != NULL)
        {
            lv_obj_t* child0 = lv_group_get_obj_by_index(group, 0);
            if (child0 != NULL) lv_group_focus_obj(child0);
        }
    }

    Path* new_path = malloc(sizeof(Path));
    new_path->obj = obj;
    new_path->pre = current_path;
    current_path = new_path;
}

void delete_path()
{

    Path* pre_page = current_path->pre;

    if (pre_page == NULL)
    {
        lv_group_focus_obj(lv_group_get_obj_by_index(my_screen_get_group(current_path->obj), 0));
        return;
    }

    if (lv_obj_check_type(current_path->obj, &lv_msgbox_class))
    {
        lv_group_t* group = my_screen_get_group(current_path->obj);
        if (group != NULL) lv_group_delete(group);
        lv_msgbox_close(current_path->obj);
    }
    else
    {
        lv_group_t* group = my_screen_get_group(current_path->obj);
        if (group != NULL) lv_group_delete(group);
        func_screen_cb screen_exit_cb = my_screen_get_exit_cb(current_path->obj);
        lv_screen_load_anim(pre_page->obj, LV_SCR_LOAD_ANIM_OUT_TOP, ANIM_DURATION, 0, true);
        if (screen_exit_cb != NULL) screen_exit_cb();
    }
    lv_indev_set_group(indev, my_screen_get_group(pre_page->obj));

    Path* temp = current_path;
    current_path = current_path->pre;
    free(temp);
}

static void focus_change_cb(lv_group_t* g)
{
    lv_obj_t* obj = lv_group_get_focused(g);
    int32_t index = lv_obj_get_index(obj);
    lv_label_set_text_static(label_app_title, apps[index].title);
    if (index == 0)
    {
        lv_obj_add_flag(arrow_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(arrow_right, LV_OBJ_FLAG_HIDDEN);
    }
    else if (index == APP_NUM - 1)
    {
        lv_obj_remove_flag(arrow_left, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(arrow_right, LV_OBJ_FLAG_HIDDEN);
    }
    else if (index == 1)
        lv_obj_remove_flag(arrow_left, LV_OBJ_FLAG_HIDDEN);
    else if (index == APP_NUM - 1 - 1)
        lv_obj_remove_flag(arrow_right, LV_OBJ_FLAG_HIDDEN);
}

static void app_enter_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    int32_t index = lv_obj_get_index(obj);
    apps[index].app_init_cb();
}

static void key_esc_cb(lv_event_t* e)
{
    if (lv_indev_get_state(indev) == LV_INDEV_STATE_RELEASED)
    {
        if (lv_indev_get_key(indev) == LV_KEY_ESC)
        {
            delete_path();
        }
    }
}

static void my_theme_apply_cb(lv_theme_t* th, lv_obj_t* obj)
{
    LV_UNUSED(th);
    if (lv_obj_has_class(obj, &lv_button_class) || lv_obj_has_class(obj, &lv_buttonmatrix_class)
        || lv_obj_has_class(obj, &lv_switch_class))
    {
        lv_obj_t* screen = lv_obj_get_screen(obj);
        lv_group_t* group = my_screen_get_group(screen);
        lv_group_add_obj(group, obj);
    }
    else if (lv_obj_check_type(obj, &lv_label_class) || lv_obj_check_type(obj, &lv_image_class))
    {
        lv_obj_add_style(obj, &style_fore_color, LV_PART_MAIN);
    }
    else if (lv_obj_check_type(obj, &lv_line_class))
    {
        lv_obj_add_style(obj, &style_line, LV_PART_MAIN);
    }
}

static void my_theme_init_and_set(void)
{
    lv_theme_t* th_act = lv_display_get_theme(NULL);
    static lv_theme_t th_new;
    th_new = *th_act;

    /*Set the parent theme and the style apply callback for the new theme*/
    lv_theme_set_parent(&th_new, th_act);
    lv_theme_set_apply_cb(&th_new, my_theme_apply_cb);

    /*Assign the new theme to the current display*/
    lv_display_set_theme(NULL, &th_new);
}

void ui_home()
{
    my_theme_init_and_set();

    indev = lv_indev_get_next(NULL);
    lv_group_t* group = lv_group_create();
    lv_indev_set_group(indev, group);

    lv_obj_t* screen_home = lv_screen_active();
    lv_obj_add_style(screen_home, &style_bg_color, LV_PART_MAIN);
    lv_obj_remove_flag(screen_home, LV_OBJ_FLAG_SCROLLABLE);
    screen_home->user_data = group;

    add_path(screen_home);

    lv_obj_t* panel = lv_obj_create(screen_home);
    lv_obj_add_style(panel, &style_panel, LV_PART_MAIN);
    lv_obj_set_scroll_snap_x(panel, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scrollbar_mode(panel, LV_SCROLLBAR_MODE_OFF);
    lv_obj_add_flag(panel, LV_OBJ_FLAG_SCROLL_ONE);

    for (uint8_t i = 0; i < APP_NUM; i++)
    {
        lv_obj_t* app_icon_bg = lv_obj_create(panel);
        lv_obj_add_style(app_icon_bg, &style_app_bg, LV_PART_MAIN);
        lv_obj_add_flag(app_icon_bg, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_add_event_cb(app_icon_bg, app_enter_cb, LV_EVENT_PRESSED, NULL);
        lv_group_add_obj(group, app_icon_bg);
        lv_obj_remove_flag(app_icon_bg, LV_OBJ_FLAG_SCROLLABLE);

        lv_obj_t* app_icon = lv_label_create(app_icon_bg);
        lv_label_set_text_static(app_icon, apps[i].icon);
        lv_obj_add_style(app_icon, &style_app, LV_PART_MAIN);
    }

    label_app_title = lv_label_create(screen_home);
    lv_obj_add_style(label_app_title, &style_app_title, LV_PART_MAIN);
    lv_obj_set_y(label_app_title, -10);

    for (uint8_t i = 0; i < 4; i++)
    {
        lv_obj_t* line = lv_line_create(screen_home);
        lv_line_set_points(line, sel_points[i], 3);
    }

    arrow_left = lv_line_create(screen_home);
    lv_line_set_points(arrow_left, left_arrow_points, 5);

    arrow_right = lv_line_create(screen_home);
    lv_line_set_points(arrow_right, right_arrow_points, 5);

    lv_group_set_focus_cb(group, focus_change_cb);
    lv_group_focus_obj(lv_obj_get_child(panel, 0));

    lv_indev_add_event_cb(indev, key_esc_cb, LV_EVENT_KEY, NULL);

    lv_obj_t* status_bar = lv_obj_create(screen_home);
    status_left = lv_obj_create(status_bar);
    status_right = lv_obj_create(status_bar);
    lv_obj_remove_style_all(status_bar);
    lv_obj_remove_style_all(status_left);
    lv_obj_remove_style_all(status_right);
    lv_obj_add_style(status_bar, &style_status_bar, LV_PART_MAIN);
    lv_obj_add_style(status_left, &style_status_left, LV_PART_MAIN);
    lv_obj_add_style(status_right, &style_status_right, LV_PART_MAIN);

    lv_obj_remove_flag(status_bar, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(status_left, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(status_right, LV_OBJ_FLAG_SCROLLABLE);

    label_time = lv_label_create(status_bar);
    lv_obj_add_style(label_time, &style_label_time, LV_PART_MAIN);

    label_battery = lv_label_create(status_bar);
    lv_obj_add_style(label_battery, &style_symbol_battery, LV_PART_MAIN);

    label_battery_percentage = lv_label_create(status_right);

    lv_obj_t* label_loop = lv_label_create(status_right);
    lv_label_set_text(label_loop, LV_SYMBOL_LOOP);
    lv_obj_add_flag(label_loop, LV_OBJ_FLAG_HIDDEN);

    lv_obj_t* label_net_config = lv_label_create(status_right);
    lv_label_set_text(label_net_config, FONT_SYMBOL_16_24_NET_CONFIG);
    lv_obj_add_flag(label_net_config, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_style(label_net_config, &style_symbol_16, LV_PART_MAIN);

    lv_obj_t* label_wifi = lv_label_create(status_left);
    lv_label_set_text(label_wifi, LV_SYMBOL_WIFI);
    lv_obj_add_flag(label_wifi, LV_OBJ_FLAG_HIDDEN);

    lv_anim_init(&delay_scroll_anim);
    lv_anim_set_delay(&delay_scroll_anim, 1000);
    lv_anim_set_repeat_count(&delay_scroll_anim, LV_ANIM_REPEAT_INFINITE);
}

void status_wifi_show(bool is_show)
{
    lv_obj_t* label_wifi = lv_obj_get_child(status_left, 0);
    if (is_show)
        lv_obj_remove_flag(label_wifi, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(label_wifi, LV_OBJ_FLAG_HIDDEN);
}

void status_net_config_show(bool is_show)
{
    lv_obj_t* label_net_config = lv_obj_get_child(status_right, 2);
    if (is_show)
        lv_obj_remove_flag(label_net_config, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(label_net_config, LV_OBJ_FLAG_HIDDEN);
}

void status_loop_invert()
{
    lv_obj_t* label_loop = lv_obj_get_child(status_right, 1);
    if (lv_obj_has_flag(label_loop, LV_OBJ_FLAG_HIDDEN))
        lv_obj_remove_flag(label_loop, LV_OBJ_FLAG_HIDDEN);
    else
        lv_obj_add_flag(label_loop, LV_OBJ_FLAG_HIDDEN);
}

extern void chat_set_label_message(const char* message);
extern void chat_set_label_status(const char* text, bool is_error);
extern void chat_set_gif_emotion(const char* emotion);

static time_t last_status_update_time;

static bool is_status_time_out = true;
static bool is_status_err = false;

static void update_label_time(const char* content)
{
    if (content)
    {
        lv_label_set_text(label_time, content);
    }
    else
    {
        time_t now;
        struct tm timeinfo;
        time(&now);
        localtime_r(&now, &timeinfo);
        lv_label_set_text_fmt(label_time, "%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
    }
}

void display_time_update()
{
    lvgl_port_lock(0);

    if (!is_status_time_out)
    {
        time_t now;
        time(&now);
        if (now - last_status_update_time >= 10)
        {
            if (is_status_err || strcmp(lv_label_get_text(label_time), "待命") == 0)
            {
                is_status_time_out = true;
                lv_obj_remove_style(label_time, &style_text_color_red, LV_PART_MAIN);
                update_label_time(NULL);
                chat_set_label_status("", false);
            }
        }
    }
    else
    {
        update_label_time(NULL);
    }
    lvgl_port_unlock();
}

void display_notify(NotificationType type, const char* content)
{
    lvgl_port_lock(0);
    switch (type)
    {
    case kNTMessage:
        chat_set_label_message(content);
        break;
    case kNTStatus:
        time(&last_status_update_time);
        update_label_time(content);
        chat_set_label_status(content, false);
        is_status_time_out = false;
        break;
    case kNTEmotion:
        chat_set_gif_emotion(content);
        break;
    case kNTError:
        time(&last_status_update_time);
        lv_obj_add_style(label_time, &style_text_color_red, LV_PART_MAIN);
        update_label_time(content);
        chat_set_label_status(content, true);
        is_status_time_out = false;
        is_status_err = true;
        break;
    }
    lvgl_port_unlock();
}

void ui_label_battery_update(uint8_t percentage, bool is_charging)
{
    lvgl_port_lock(0);
    lv_label_set_text_fmt(label_battery_percentage, "%d", percentage);
    if (is_charging)
    {
        lv_label_set_text(label_battery, FONT_SYMBOL_18_BATTERY_BOLT);
        lv_obj_remove_style(label_battery, &style_text_color_red, LV_PART_MAIN);
        lv_obj_add_style(label_battery, &style_text_color_green, LV_PART_MAIN);
    }

    else
    {
        lv_obj_remove_style(label_battery, &style_text_color_green, LV_PART_MAIN);
        if (percentage > 80)
        {
            lv_label_set_text(label_battery, FONT_SYMBOL_18_BATTERY_FULL);
        }
        else if (percentage > 60)
        {
            lv_label_set_text(label_battery, FONT_SYMBOL_18_BATTERY_3);
        }
        else if (percentage > 40)
        {
            lv_label_set_text(label_battery, FONT_SYMBOL_18_BATTERY_2);
        }
        else if (percentage > 20)
        {
            lv_label_set_text(label_battery, FONT_SYMBOL_18_BATTERY_1);
        }
        else
        {
            lv_label_set_text(label_battery, FONT_SYMBOL_18_BATTERY_EMPTY);
            lv_obj_add_style(label_battery, &style_text_color_red, LV_PART_MAIN);
        }
    }
    lvgl_port_unlock();
}
