#include "esp_lvgl_port.h"
#include "key/key.h"
#include "lvgl.h"
#include "lvgl_private.h"
#include "servo.h"
#include "style/my_style.h"
#include "ui/ui.h"

#if CONFIG_SERVO_DEBUG
extern int8_t servo_debug_offsets[];

static lv_obj_t* label_offsets;
static const char* servo_ctrl_map[] = { "左前+1", "左前-1", "左后+1", "左后-1", "\n",
    "右前+1", "右前-1", "右后+1", "右后-1", "\n",
    "尾巴+1", "尾巴-1", "归零",  "" };

#else
static const char* servo_ctrl_map[] = { "起立", "握手", "左转", "右转", "\n",
    "前进", "后退", "坐下", "跳舞", "\n",
    "摇尾巴", "伸懒腰", "趴下", "睡眠", "" };

#endif

static lv_obj_t* matrix_servo_ctrl;

static void servo_ctrl_cb(lv_event_t* e)
{
    lv_obj_t* obj = lv_event_get_target(e);
    uint32_t id = lv_buttonmatrix_get_selected_button(obj);
    lv_obj_add_state(obj, LV_STATE_DISABLED);

    servo_move(id);
}

void servo_ctrl_end_cb()
{

    if (matrix_servo_ctrl)
    {
        lvgl_port_lock(0);
        lv_obj_remove_state(matrix_servo_ctrl, LV_STATE_DISABLED);
#if CONFIG_SERVO_DEBUG
        lv_label_set_text_fmt(label_offsets, "lf: %d, lr: %d, rf: %d, rr: %d, tail: %d",
            servo_debug_offsets[0], servo_debug_offsets[1], servo_debug_offsets[2], servo_debug_offsets[3], servo_debug_offsets[4]);
#endif
        lvgl_port_unlock();
    }
}

void servo_ctrl_exit_cb()
{
    matrix_servo_ctrl = NULL;
    key_edit_mode_invert();
}

void app_init_servo_ctrl(void)
{
    lv_obj_t* screen = my_empty_screen_create(servo_ctrl_exit_cb);
    matrix_servo_ctrl = lv_buttonmatrix_create(screen);
    lv_buttonmatrix_set_map(matrix_servo_ctrl, servo_ctrl_map);
    // lv_buttonmatrix_set_one_checked(matrix_servo_ctrl, true);
    lv_obj_add_event_cb(matrix_servo_ctrl, servo_ctrl_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_style(matrix_servo_ctrl, &style_matrix_servo_ctrl, LV_PART_MAIN);
    lv_obj_add_style(matrix_servo_ctrl, &style_matrix_servo_ctrl_item, LV_PART_ITEMS);
    lv_obj_add_style(matrix_servo_ctrl, &style_matrix_item_on_focus, LV_PART_ITEMS | LV_STATE_FOCUS_KEY);

#if CONFIG_SERVO_DEBUG
    label_offsets = lv_label_create(screen);
    lv_obj_set_width(label_offsets, CONFIG_LCD_WIDTH - 20);
    lv_obj_align(label_offsets, LV_ALIGN_TOP_MID, 0, 5);
    lv_label_set_text_fmt(label_offsets, "lf: %d, lr: %d, rf: %d, rr: %d, tail: %d",
        servo_debug_offsets[0], servo_debug_offsets[1], servo_debug_offsets[2], servo_debug_offsets[3], servo_debug_offsets[4]);
    lv_buttonmatrix_set_button_width(matrix_servo_ctrl, 10, 2);
#endif
    add_path(screen);
    key_edit_mode_invert();
}