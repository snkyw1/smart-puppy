#ifndef SMART_PUPPY_LCD_UI_H
#define SMART_PUPPY_LCD_UI_H

#include "src/misc/lv_types.h"

#include "application.h"


#define ANIM_DURATION 300
#ifdef __cplusplus
extern "C" {
#endif
typedef struct
{
    const char* title;
    const char* icon;
    void (*app_init_cb)();
} App;

typedef struct Path
{
    lv_obj_t* obj;
    struct Path* pre;
} Path;

extern lv_anim_t delay_scroll_anim;

typedef void(*func_screen_cb)();


void ui_home();
void add_path(lv_obj_t* obj);
void delete_path();
lv_obj_t* my_msgbox_create(bool has_group);
lv_obj_t* my_screen_create(const char* title, func_screen_cb screen_exit_cb);
lv_obj_t* my_empty_screen_create(func_screen_cb screen_exit_cb);

lv_group_t* my_screen_get_group(lv_obj_t* screen);
lv_obj_t* my_screen_get_content(lv_obj_t* screen);
lv_obj_t* my_screen_get_title(lv_obj_t* screen);
lv_obj_t* my_screen_get_header(lv_obj_t* screen);


void status_wifi_show(bool is_show);
void status_net_config_show(bool is_show);
void status_loop_invert();

void display_notify(NotificationType type, const char * content);
void display_time_update();
void ui_label_battery_update(uint8_t percentage, bool is_charging);

#ifdef __cplusplus
}
#endif
#endif
