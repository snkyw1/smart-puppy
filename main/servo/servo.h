#ifndef SMART_PUPPY_SERVO_H
#define SMART_PUPPY_SERVO_H
#include_next <stdint.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "servo_move.h"

typedef enum {
    SERVO_CHANNEL_LF = 0,
    SERVO_CHANNEL_LR,
    SERVO_CHANNEL_RF,
    SERVO_CHANNEL_RR,
    SERVO_CHANNEL_TAIL,
    SERVO_CHANNEL_NUM
} servo_channel;

void servo_init();
uint8_t servo_read_angle(servo_channel channel);
void servo_write_angle(servo_channel channel, uint8_t angle);
void servo_move(int action_id);

#endif // SMART_PUPPY_SERVO_H