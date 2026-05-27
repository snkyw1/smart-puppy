#pragma once
#include <stdint.h>

// 舵机角度定义
typedef struct {
    uint8_t lf;     // 左前
    uint8_t lr;     // 左后
    uint8_t rf;     // 右前
    uint8_t rr;     // 右后
    uint8_t tail;   // 尾巴
} ServoPose;

// 动作ID枚举
typedef enum {
    SERVO_ACT_STAND_UP,
    SERVO_ACT_HANDSHAKE,
    SERVO_ACT_TURN_LEFT,
    SERVO_ACT_TURN_RIGHT,
    SERVO_ACT_FORWARD,
    SERVO_ACT_BACKWARD,
    SERVO_ACT_SIT_DOWN,
    SERVO_ACT_DANCE,
    SERVO_ACT_WAG_TAIL,
    SERVO_ACT_STRETCH,
    SERVO_ACT_LIE_DOWN,
    SERVO_ACT_SLEEP
} ActionID;

void servo_move_perform_action(ActionID action);