#include "servo.h"
#include "esp_log.h"

int8_t servo_debug_offsets[5] = {0};

static void servo_debug_reset()
{
    servo_write_angle(SERVO_CHANNEL_LF, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    servo_write_angle(SERVO_CHANNEL_RF, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    servo_write_angle(SERVO_CHANNEL_LR, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    servo_write_angle(SERVO_CHANNEL_RR, 0);
    vTaskDelay(pdMS_TO_TICKS(600));
    servo_write_angle(SERVO_CHANNEL_TAIL, 0);

    for (int i=0; i < sizeof(servo_debug_offsets)/sizeof(int8_t); i ++) {
        servo_debug_offsets[i] = 0;
    }
}

static void servo_debug_adjust(servo_channel channel, int8_t step)
{
    if (servo_debug_offsets[channel] + step < 0) {
        return;
    }
    servo_debug_offsets[channel] += step;
    ESP_LOGI("servo_debug", "channel:%d, offsets:%d, step:%d", channel, servo_debug_offsets[channel], step);
    servo_write_angle(channel, servo_debug_offsets[channel]);
}

void servo_debug_action(uint32_t action_id)
{
    if (action_id ==  10) {
        servo_debug_reset();
        return;
    }

    servo_debug_adjust(action_id / 2, action_id % 2 == 0 ? 1 : -1);

}