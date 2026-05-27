#include "servo_move.h"
#include "servo.h"

// 硬件配置
#define SERVO_MINIMAL_DELAY_MS       20  // 两个舵机之间的最小延时，否则电池可能会断电
#define SERVO_THIRD_MINIMAL_DELAY_MS 160 // 已有两个舵机运动时，第三个舵机运动的最小延时
#define SERVO_STEP_DELAY_MS          6
#define SERVO_STEP_ANGLE             2 // 单步移动角度(度)，1-3之间

#define SERVO_MINMAL_DELAY()        vTaskDelay(pdMS_TO_TICKS(SERVO_MINIMAL_DELAY_MS))
#define SERVO_THIRD_MINIMAL_DELAY() vTaskDelay(pdMS_TO_TICKS(SERVO_THIRD_MINIMAL_DELAY_MS))

#define SERVO_DONT_MOVE 255

// 预定义姿态
const ServoPose POSE_STAND = { 90, 90, 90, 90, 90 };     // 站立
const ServoPose POSE_SIT = { 90, 10, 90, 10, 90 };       // 坐下
const ServoPose POSE_HAND = { 0, 30, 30, 30, 90 };       // 握手
const ServoPose POSE_DOWN = { 0, 180, 0, 180, 90 };      // 趴下
const ServoPose POSE_SLEEP = { 180, 0, 180, 0, 0 };      // 睡觉
const ServoPose POSE_STRETCH = { 90, 140, 90, 140, 90 }; // 伸懒腰

static void servo_move_one_smooth(servo_channel channel, uint8_t target, uint8_t interval_ms)
{
    uint8_t current = servo_read_angle(channel);
    uint8_t steps = abs(target - current) / SERVO_STEP_ANGLE;
    int8_t step_dir = (target > current) ? SERVO_STEP_ANGLE : -SERVO_STEP_ANGLE;

    for (int step = 0; step < steps; step++)
    {
        if (step < steps)
        {
            current += step_dir;
            servo_write_angle(channel, current);
            vTaskDelay(pdMS_TO_TICKS(interval_ms));
        }
    }
}

// 多舵机同步步进（错峰驱动）
static void servo_move_smooth(ServoPose target_pose)
{
    uint8_t* target = (uint8_t*)&target_pose;
    uint8_t current[SERVO_CHANNEL_NUM];
    uint8_t steps[SERVO_CHANNEL_NUM];
    int8_t step_dir[SERVO_CHANNEL_NUM];
    int8_t max_steps = 0;

    for (int i = 0; i < SERVO_CHANNEL_NUM; i++)
    {
        // 1. 读取当前位置
        current[i] = servo_read_angle(i);
        // 2. 计算各舵机需要移动的步数
        steps[i] = (target[i] == SERVO_DONT_MOVE) ? 0 : abs(target[i] - current[i]) / SERVO_STEP_ANGLE;
        max_steps = (steps[i] > max_steps) ? steps[i] : max_steps;
        // 3. 计算每步方向
        step_dir[i] = (target[i] > current[i]) ? SERVO_STEP_ANGLE : -SERVO_STEP_ANGLE;
    }

    // 4. 逐步移动
    for (int step = 0; step < max_steps; step++)
    {
        // 错峰驱动：每个舵机在不同时间点更新
        for (int i = 0; i < SERVO_CHANNEL_NUM; i++)
        {
            if (step < steps[i])
            {
                current[i] += step_dir[i];
                servo_write_angle(i, current[i]);
                vTaskDelay(pdMS_TO_TICKS(SERVO_STEP_DELAY_MS));
            }
        }
    }
}

static void resume_stand_up(void)
{
    bool is_change = false;
    for (int i = 0; i < 4; i++)
    {
        if (servo_read_angle(i))
        {
            servo_write_angle(i, 90);
            SERVO_MINMAL_DELAY();
            is_change = true;
        }
    }
    if (is_change)
    {
        SERVO_THIRD_MINIMAL_DELAY();
    }
}

// 起立（从坐/趴到站立）
void action_stand_up(void)
{
    // servo_move_smooth(POSE_STAND);
    servo_write_angle(SERVO_CHANNEL_LF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 90);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 90);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_TAIL, 90);
    SERVO_THIRD_MINIMAL_DELAY();
}

// 握手（抬右前脚）
void action_handshake(void)
{
    servo_move_smooth(POSE_HAND);
    ServoPose pose = {0, SERVO_DONT_MOVE ,SERVO_DONT_MOVE,SERVO_DONT_MOVE, 0};
    for (int i = 0; i < 6; i++) {
        pose.lf = 45;
        pose.tail = 150;
        servo_move_smooth(pose);
        // vTaskDelay(pdMS_TO_TICKS(150));
        pose.lf = 0;
        pose.tail = 30;
        servo_move_smooth(pose);
    }
    
    vTaskDelay(pdMS_TO_TICKS(150));
    servo_move_smooth(POSE_STAND);
}

// 左转（左前左后收起，右前右后伸展）
void action_turn_left(void)
{
    resume_stand_up();
    for (int i = 0; i < 5; i++)
    {
        servo_write_angle(SERVO_CHANNEL_RF, 45);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LR, 135);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RR, 135);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LF, 45);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RF, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LR, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RR, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LF, 90);
        SERVO_THIRD_MINIMAL_DELAY();
    }
}

// 右转
void action_turn_right(void)
{
    resume_stand_up();
    for (int i = 0; i < 5; i++)
    {
        servo_write_angle(SERVO_CHANNEL_LF, 45);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RR, 135);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LR, 135);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RF, 45);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LF, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RR, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LR, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RF, 90);
        SERVO_THIRD_MINIMAL_DELAY();
    }
}

// 前进（交替步态）
void action_forward(void)
{
    resume_stand_up();

    for (int i = 0; i < 3; i++)
    {
        servo_write_angle(SERVO_CHANNEL_RR, 45);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LF, 45);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RF, 135);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LR, 135);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RR, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LF, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RF, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LR, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RF, 45);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LR, 45);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RR, 135);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LF, 135);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RF, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LR, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_RR, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_LF, 90);
        SERVO_THIRD_MINIMAL_DELAY();
    }
}

// 后退（与前进相反）
void action_backward(void)
{
    resume_stand_up();
    for (int i = 0; i < 3; i++)
    {
        servo_write_angle(SERVO_CHANNEL_LF, 135);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RR, 135);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LR, 45);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RF, 45);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LF, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RR, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LR, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RF, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LR, 135);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RF, 135);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LF, 45);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RR, 45);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LR, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RF, 90);
        SERVO_THIRD_MINIMAL_DELAY();

        servo_write_angle(SERVO_CHANNEL_LF, 90);
        SERVO_MINMAL_DELAY();
        servo_write_angle(SERVO_CHANNEL_RR, 90);
        SERVO_THIRD_MINIMAL_DELAY();
    }
}

// 坐下
void action_sit_down(void)
{
    servo_move_smooth(POSE_SIT);
}

// 跳舞（简单摇摆）
void action_dance(void)
{
    resume_stand_up();

    servo_write_angle(SERVO_CHANNEL_LF, 135);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 135);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 135);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 135);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 90);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 90);
    SERVO_THIRD_MINIMAL_DELAY();

    servo_write_angle(SERVO_CHANNEL_LR, 135);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 135);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 45);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 45);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 90);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 90);
    SERVO_THIRD_MINIMAL_DELAY();

    servo_write_angle(SERVO_CHANNEL_RF, 45);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 45);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 45);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 45);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 90);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 90);
    SERVO_THIRD_MINIMAL_DELAY();

    servo_write_angle(SERVO_CHANNEL_LR, 45);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 45);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 135);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 135);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_LR, 90);
    SERVO_THIRD_MINIMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RF, 90);
    SERVO_MINMAL_DELAY();
    servo_write_angle(SERVO_CHANNEL_RR, 90);
    SERVO_THIRD_MINIMAL_DELAY();
}

// 摇尾巴
void action_wag_tail(void)
{
    for (int i = 0; i < 4; i++)
    {
        servo_move_one_smooth(SERVO_CHANNEL_TAIL, 150, 6);
        vTaskDelay(pdMS_TO_TICKS(60));
        servo_move_one_smooth(SERVO_CHANNEL_TAIL, 30, 6);
        vTaskDelay(pdMS_TO_TICKS(60));
    }
    servo_move_one_smooth(SERVO_CHANNEL_TAIL, 90, 6);
}

// 伸懒腰（前腿前伸，身体下压）
void action_stretch(void)
{
    servo_move_smooth(POSE_STRETCH);
    vTaskDelay(pdMS_TO_TICKS(150));
    ServoPose pose  = { SERVO_DONT_MOVE, 0, SERVO_DONT_MOVE, 0, 0 };;
    for (int i = 0; i < 6; i++)
    {
        pose.lr = 180;
        pose.rr = 140;
        pose.tail = 150;
        servo_move_smooth(pose);
        vTaskDelay(pdMS_TO_TICKS(100));
        pose.lr = 140;
        pose.rr = 180;
        pose.tail = 30;
        servo_move_smooth(pose);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    servo_move_smooth(POSE_STAND);
}

// 趴下
void action_lie_down(void)
{
    servo_move_smooth(POSE_DOWN);
}

// 睡觉（全部收起来）
void action_sleep(void)
{
    servo_move_smooth(POSE_SLEEP);
}

// 动作执行函数指针数组
void (*action_table[])(void) = {
    action_stand_up,   // ACT_STAND_UP
    action_handshake,  // ACT_HANDSHAKE
    action_turn_left,  // ACT_TURN_LEFT
    action_turn_right, // ACT_TURN_RIGHT
    action_forward,    // ACT_FORWARD
    action_backward,   // ACT_BACKWARD
    action_sit_down,   // ACT_SIT_DOWN
    action_dance,      // ACT_DANCE
    action_wag_tail,   // ACT_WAG_TAIL
    action_stretch,    // ACT_STRETCH
    action_lie_down,   // ACT_LIE_DOWN
    action_sleep       // ACT_SLEEP
};

// 执行动作
void servo_move_perform_action(ActionID action)
{
#if !CONFIG_SERVO_DEBUG
    static bool is_first = true;
    if (is_first)
    {
        for (uint8_t i = 0; i < 5; i++)
        {
            servo_write_angle(i, 90);
            vTaskDelay(pdMS_TO_TICKS(600));
        }
        is_first = false;
    }
#endif
    if (action < sizeof(action_table) / sizeof(action_table[0]))
    {
        action_table[action]();
    }
}