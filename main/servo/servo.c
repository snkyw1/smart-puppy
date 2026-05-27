/*
 * 舵机驱动模块
 *
 * 使用 LEDC 硬件 PWM 驱动 5 路模拟舵机。PWM 频率 50Hz（周期 20ms），
 * 脉宽范围 500us ~ 2500us 对应角度 0° ~ 180°。
 *
 * 引脚分配（SERVO_GPIO_TABLE）：
 *   前左腿（LF）  : GPIO 17
 *   前右腿（RF）  : GPIO 3
 *   后左腿（LR）  : GPIO 18
 *   后右腿（RR）  : GPIO 46
 *   尾巴（Tail）  : GPIO 38
 *
 * LEDC 配置：
 *   定时器      : LEDC_TIMER_1
 *   分辨率      : 13 位（8192 步）
 *   时钟源      : APB_CLK
 *   速度模式    : 低速模式（可休眠时保持输出）
 *
 * 舵机电源来自 MT3608B 升压输出的 5V，每个舵机堵转电流可达 1A，
 * 5 路同时工作时需确保电源提供足够驱动能力（MT3608B 最大 2A）。
 *
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 */

#include "servo.h"
#include "application.h"
#include "driver/ledc.h"

#define SERVO_LEDC_RESOLUTION LEDC_TIMER_13_BIT
#define SERVO_LEDC_TIMER      LEDC_TIMER_1
#define SERVO_LEDC_CLK        LEDC_USE_APB_CLK
#define SERVO_FREQ_HZ         50
#define SERVO_MAX_ANGLE       180
#define SERVO_MIN_WIDTH_US    500
#define SERVO_MAX_WIDTH_US    2500

#define CLAMP_0_180(val)    ((val) < 0 ? 0 : ((val) > 180 ? 180 : (val)))

static uint32_t g_full_duty = 0;
static TaskHandle_t servo_task_handle;

static int8_t servo_offsets[SERVO_CHANNEL_NUM] = { 0,-5,-7,0,0 };
static int16_t servo_angle[SERVO_CHANNEL_NUM] = { 0 };

static uint32_t calculate_duty(uint8_t angle)
{
    // 微秒数 = 最小脉宽 + 角度占比 * (最大脉宽 - 最小脉宽)
    uint16_t angle_us = SERVO_MIN_WIDTH_US + (angle * (SERVO_MAX_WIDTH_US - SERVO_MIN_WIDTH_US)) / SERVO_MAX_ANGLE;
    // 占空比 = (g_full_duty * angle_us * SERVO_FREQ_HZ) / 1000000
    uint32_t duty = (g_full_duty * angle_us * SERVO_FREQ_HZ) / 1000000;

    return duty;
}

static uint8_t calculate_angle(uint32_t duty)
{
    // angle_us = (duty * 1000000) / (g_full_duty * SERVO_FREQ_HZ)
    uint16_t angle_us = (duty * 1000000) / (g_full_duty * SERVO_FREQ_HZ);

    if (angle_us < SERVO_MIN_WIDTH_US)
    {
        angle_us = SERVO_MIN_WIDTH_US;
    }

    // 角度 = (angle_us - 最小脉宽) * 最大角度 / (最大脉宽 - 最小脉宽)
    uint8_t angle = ((angle_us - SERVO_MIN_WIDTH_US) * SERVO_MAX_ANGLE) / (SERVO_MAX_WIDTH_US - SERVO_MIN_WIDTH_US);

    return angle;
}

void servo_write_angle(servo_channel channel, uint8_t angle)
{
    servo_angle[channel] = angle + servo_offsets[channel];
    angle = CLAMP_0_180(servo_angle[channel]);
    if (channel == SERVO_CHANNEL_RF || channel == SERVO_CHANNEL_RR) {
        angle = 180 - angle;
    }
    
    uint32_t duty = calculate_duty(angle);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel));
}

uint8_t servo_read_angle(servo_channel channel)
{
    // uint32_t duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, (ledc_channel_t)channel);
    // uint8_t angle = calculate_angle(duty);

    uint8_t angle = servo_angle[channel] - servo_offsets[channel];

    return angle;
}

extern void servo_ctrl_end_cb();
#if CONFIG_SERVO_DEBUG
void servo_debug_action(uint32_t action_id);
#endif

static void servo_ctrl_task()
{

    uint32_t notify_value;
    while (true)
    {
        if (xTaskNotifyWait(0, 0x0FFF, &notify_value, portMAX_DELAY))
        {
            uint32_t action_id = __builtin_ctz(notify_value);

#if CONFIG_SERVO_DEBUG
            servo_debug_action(action_id);
#else
            servo_move_perform_action(action_id);
#endif
        }
        servo_ctrl_end_cb();
    }
}

void servo_move(int action_id)
{
    xTaskNotify(servo_task_handle, 1 << action_id, eSetBits);
}

void servo_init()
{
    uint32_t servo_pins[] = {
        CONFIG_SERVO_GPIO_LF,
        CONFIG_SERVO_GPIO_LR,
        CONFIG_SERVO_GPIO_RF,
        CONFIG_SERVO_GPIO_RR,
        CONFIG_SERVO_GPIO_TAIL
    };

    ledc_timer_config_t ledc_timer = {
        .clk_cfg = SERVO_LEDC_CLK,
        .duty_resolution = SERVO_LEDC_RESOLUTION, // resolution of PWM duty
        .freq_hz = SERVO_FREQ_HZ,                 // frequency of PWM signal
        .speed_mode = LEDC_LOW_SPEED_MODE,        // timer mode
        .timer_num = SERVO_LEDC_TIMER             // timer index
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    ledc_channel_config_t ledc_ch = {
        .intr_type = LEDC_INTR_DISABLE,
        .duty = 0,
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .timer_sel = SERVO_LEDC_TIMER,
        .hpoint = 0
    };

    for (size_t i = 0; i < sizeof(servo_pins)/sizeof(uint32_t); i++)
    {
        ledc_ch.channel = i;
        ledc_ch.gpio_num = servo_pins[i];
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_ch));
    }
    g_full_duty = 1 << SERVO_LEDC_RESOLUTION;

    AddIntMcpTool("self.dog.control",
        "机器小狗的动作。机器小狗可以做以下动作：\n"
        "起立(0)\n"
        "握手(1)\n"
        "左转(2)\n"
        "右转(3)\n"
        "前进(4)\n"
        "后退(5)\n"
        "坐下(6)\n"
        "跳舞(7)\n"
        "摇尾巴(8)\n"
        "伸懒腰(9)\n"
        "趴下(10)\n"
        "睡觉(11)",
        "action",
        0, 11, servo_move);

    xTaskCreatePinnedToCore(servo_ctrl_task, "servo_ctrl_task", 2048,
        NULL, 1, &servo_task_handle, 1);
}

void servo_deinit()
{
    for (size_t i = 0; i < SERVO_CHANNEL_NUM; i++)
    {
        ledc_stop(LEDC_LOW_SPEED_MODE, i, 0);
    }
    ledc_timer_rst(LEDC_LOW_SPEED_MODE, SERVO_LEDC_TIMER);
    g_full_duty = 0;
}
