#include "esp_err.h"
#include "button_gpio.h"
#include "esp_lvgl_port.h"
#include "indev/lv_indev.h"
#include "ui/ui.h"
#include "low_power/low_power.h"


static lv_indev_t* button_indev;
static button_event_t button_event = BUTTON_NONE_PRESS;
static lv_key_t single_click_key = LV_KEY_NEXT;
// static TaskHandle_t key_task_handle = NULL;

void key_edit_mode_invert()
{
    switch (single_click_key)
    {
    case LV_KEY_NEXT:
        single_click_key = LV_KEY_RIGHT;
        break;
    case LV_KEY_PREV:
        single_click_key = LV_KEY_LEFT;
        break;
    case LV_KEY_RIGHT:
        single_click_key = LV_KEY_NEXT;
        break;
    case LV_KEY_LEFT:
        single_click_key = LV_KEY_PREV;
        break;
    default:
        break;
    }
}

static void key_invert()
{
    switch (single_click_key)
    {
    case LV_KEY_NEXT:
        single_click_key = LV_KEY_PREV;
        break;
    case LV_KEY_PREV:
        single_click_key = LV_KEY_NEXT;
        break;
    case LV_KEY_RIGHT:
        single_click_key = LV_KEY_LEFT;
        break;
    case LV_KEY_LEFT:
        single_click_key = LV_KEY_RIGHT;
        break;
    default:
        break;
    }
}

static void button_event_cb(void* arg, void* data)
{
    if (low_power_is_sleeping()) {
        button_event = BUTTON_NONE_PRESS;
        return;
    }

    button_event = iot_button_get_event(arg);

    // xTaskNotifyGive(key_task_handle);

    lvgl_port_lock(0);
    if (button_event == BUTTON_MULTIPLE_CLICK)
    {
        // printf("button event: MULTIPLE_CLICK 3\n");
        key_invert();
        status_loop_invert();
        button_event = BUTTON_NONE_PRESS;
        lvgl_port_unlock();
        return;
    }

    lv_indev_read(button_indev);
    lv_indev_read(button_indev);

    lvgl_port_unlock();
}

static void key_read(lv_indev_t* indev, lv_indev_data_t* data)
{
    static uint32_t last_key = 0;

    if (button_event == BUTTON_NONE_PRESS)
    {
        data->state = LV_INDEV_STATE_RELEASED;
        data->key = last_key;
        return;
    }

    data->state = LV_INDEV_STATE_PRESSED;
    switch (button_event)
    {
    case BUTTON_SINGLE_CLICK:
        last_key = single_click_key;
        // printf("single click\n");
        break;
    case BUTTON_DOUBLE_CLICK:
        last_key = LV_KEY_ENTER;
        // printf("double click\n");
        break;
    case BUTTON_LONG_PRESS_UP:
        last_key = LV_KEY_ESC;
        // printf("long press\n");
        break;
    default:
        break;
    }
    data->key = last_key;
    button_event = BUTTON_NONE_PRESS;
}

// static void key_task()
// {
//     while (1)
//     {
//         if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY))
//         {
//             lvgl_port_lock(0);
//             lv_indev_read(button_indev);
//             lv_indev_read(button_indev);
//             lvgl_port_unlock();
//         }
//     }
// }

void lvgl_port_key_init(void)
{
    const button_gpio_config_t bsp_button_config = {
        .gpio_num = CONFIG_TOUCH_KEY_GPIO,
        .active_level = 1,
    };
    const button_config_t btn_cfg = {
        .long_press_time = 700,
        .short_press_time = 200
    };
    button_handle_t btn_handle = NULL;
    ESP_ERROR_CHECK(iot_button_new_gpio_device(&btn_cfg, &bsp_button_config, &btn_handle));

    button_event_args_t args = {
        .multiple_clicks.clicks = 3,
    };
    iot_button_register_cb(btn_handle, BUTTON_SINGLE_CLICK, NULL, button_event_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_DOUBLE_CLICK, NULL, button_event_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_LONG_PRESS_UP, NULL, button_event_cb, NULL);
    iot_button_register_cb(btn_handle, BUTTON_MULTIPLE_CLICK, &args, button_event_cb, NULL);

    lvgl_port_lock(0);

    button_indev = lv_indev_create();
    lv_indev_set_type(button_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_mode(button_indev, LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(button_indev, key_read);
    lv_indev_set_display(button_indev, lv_display_get_default());

    lvgl_port_unlock();
}
