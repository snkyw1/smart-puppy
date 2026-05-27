/**
 * @file sc7a20h.h
 * @brief SC7A20H 三轴加速度计驱动头文件
 * @note  士兰微电子 ±2G/±4G/±8G/±16G 三轴数字加速度计
 */

#pragma once

#include "driver/gpio.h"
#include "driver/i2c_types.h"
#include "soc/gpio_num.h"
#include <stdbool.h>
#include <stdint.h>

/* I2C 地址 */
#define SC7A20H_I2C_ADDR_SDO_LOW  0x18 // SDO 接地
#define SC7A20H_I2C_ADDR_SDO_HIGH 0x19 // SDO 悬空或接高

// clang-format off
/* 输出数据率 (ODR) */
typedef enum {
    SC7A20H_ODR_POWER_DOWN = 0x00, // 关断模式
    SC7A20H_ODR_1_56HZ     = 0x01, // 1.56 Hz
    SC7A20H_ODR_12_5HZ     = 0x02, // 12.5 Hz
    SC7A20H_ODR_25HZ       = 0x03, // 25 Hz
    SC7A20H_ODR_50HZ       = 0x04, // 50 Hz
    SC7A20H_ODR_100HZ      = 0x05, // 100 Hz
    SC7A20H_ODR_200HZ      = 0x06, // 200 Hz
    SC7A20H_ODR_400HZ      = 0x07, // 400 Hz
    SC7A20H_ODR_800HZ      = 0x08, // 800 Hz
    SC7A20H_ODR_1_48KHZ    = 0x09, // 1.48 kHz (高性能模式)
    SC7A20H_ODR_2_66KHZ    = 0x0A, // 2.66 kHz (高性能模式)
    SC7A20H_ODR_4_434KHZ   = 0x0B, // 4.434 kHz (高性能模式)
} sc7a20h_odr_t;

/* 量程范围 Accelerometer full scale range*/
typedef enum {
    SC7A20H_FSR_2G  = 0b00, // ±2G
    SC7A20H_FSR_4G  = 0b01, // ±4G
    SC7A20H_FSR_8G  = 0b10, // ±8G
    SC7A20H_FSR_16G = 0b11, // ±16G
} sc7a20h_fsr_t;

/* 工作模式 */
typedef enum {
    SC7A20H_MODE_NORMAL    = 0b00, // 正常模式
    SC7A20H_MODE_LOW_POWER = 0b01, // 低功耗模式
    SC7A20H_MODE_HIGH_PERF = 0b10, // 高性能模式
    SC7A20H_MODE_ENHANCED  = 0b11, // 增强模式
} sc7a20h_mode_t;

/* 中断配置 */
typedef enum {
    SC7A20H_INT1 = 0,
    SC7A20H_INT2 = 1,
} sc7a20h_int_pin_t;

/* 中断类型 */
typedef enum {
    SC7A20H_INT_TYPE_CLICK    = 0x80, // 单击/双击
    SC7A20H_INT_TYPE_AOI1     = 0x40, // AOI1 中断
    SC7A20H_INT_TYPE_AOI2     = 0x20, // AOI2 中断
    SC7A20H_INT_TYPE_DRDY     = 0x10, // 数据就绪
    SC7A20H_INT_TYPE_FIFO_WTM = 0x04, // FIFO 阈值
    SC7A20H_INT_TYPE_OVERRUN  = 0x02, // FIFO 溢出
} sc7a20h_int_type_t;

/* 中断类型 */
typedef enum {
    SC7A20H_AOI_AND = 0x80, // bit7: 0, 或中断事件; 1, 与中断事件
    SC7A20H_AOI_6D  = 0x40, // bit6: 0, 3D; 1, 6D
    SC7A20H_AOI_ZH  = 0x20, // bit5: Z 轴高事件中断使能
    SC7A20H_AOI_ZL  = 0x10, // bit4: Z 轴低事件中断使能
    SC7A20H_AOI_YH  = 0x08, // bit3: Y 轴高事件中断使能
    SC7A20H_AOI_YL  = 0x04, // bit2: Y 轴低事件中断使能
    SC7A20H_AOI_XH  = 0x02, // bit1: X 轴高事件中断使能
    SC7A20H_AOI_XL  = 0x01, // bit0: X 轴低事件中断使能
} sc7a20h_int_aoi_cfg_t;
// clang-format on

/* 轴使能 */
typedef struct {
    bool x_en;
    bool y_en;
    bool z_en;
} sc7a20h_axis_en_t;

/* 加速度数据 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} sc7a20h_accel_t;

/* 加速度数据 (转换为 g) */
typedef struct {
    float x_g;
    float y_g;
    float z_g;
} sc7a20h_accel_g_t;

typedef struct {
    gpio_num_t int1_gpio;
    gpio_num_t int2_gpio;
    gpio_isr_t isr;
    bool active_level;
    bool pp_od; // 0:push-pull, 1:open drain
} sc7a20h_int_config_t;

typedef struct {
    uint8_t i2c_addr;
    sc7a20h_odr_t odr;         // 输出数据率
    sc7a20h_fsr_t fsr;         // 量程
    sc7a20h_mode_t mode;       // 工作模式
    sc7a20h_axis_en_t axis_en; // 轴使能
} sc7a20h_config_t;

/**
 * @brief 初始化 SC7A20H
 * @param i2c_bus 总线句柄
 * @param cfg 设备配置
 */
void sc7a20h_init(i2c_master_bus_handle_t i2c_bus, sc7a20h_config_t* cfg);

/**
 * @brief 软件复位
 */
void sc7a20h_soft_reset();

/**
 * @brief 验证设备ID和版本
 * @return ESP_OK 验证成功，ESP_FAIL 验证失败
 */
esp_err_t sc7a20h_verification();

/**
 * @brief 读取原始加速度数据 (12bit)
 * @param accel 输出原始数据
 */
void sc7a20h_read_accel(sc7a20h_accel_t* accel);

/**
 * @brief 读取加速度数据 (转换为 g)
 * @param accel_g 输出 g 值
 */
void sc7a20h_read_accel_g(sc7a20h_accel_g_t* accel_g);

/**
 * @brief 设置输出数据率
 * @param odr 数据率
 */
void sc7a20h_set_odr(sc7a20h_odr_t odr);

/**
 * @brief 设置量程
 * @param fsr 量程
 */
void sc7a20h_set_fsr(sc7a20h_fsr_t fsr);

/**
 * @brief 配置中断
 * @param int_cfg 配置参数
 */
void sc7a20h_config_int(sc7a20h_int_config_t* int_cfg);

/**
 * @brief 重置中断
 */
void sc7a20h_deconfig_int();


void sc7a20h_config_int_aoi1(sc7a20h_int_pin_t int_pin, uint8_t cfg,  uint8_t threshold, uint8_t duration);
void sc7a20h_config_int_aoi2(sc7a20h_int_pin_t int_pin, uint8_t cfg,  uint8_t threshold, uint8_t duration);