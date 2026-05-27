/**
 * @file sc7a20h.c
 * @brief SC7A20H 三轴加速度计驱动实现
 */
#include "sc7a20h.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#define TAG "SC7A20H"

#define I2C_MASTER_FREQ_HZ       100000
/* 设备ID和Version */
#define SC7A20H_CHIP_ID          0x11
#define SC7A20H_VERSION          0x28
/* 寄存器定义 */
#define SC7A20H_REG_WHO_AM_I     0x0F // 设备 ID (0x11)
#define SC7A20H_REG_CTRL_REG0    0x1F // 控制寄存器0
#define SC7A20H_REG_CTRL_REG1    0x20 // 控制寄存器1 (ODR, 使能)
#define SC7A20H_REG_CTRL_REG2    0x21 // 控制寄存器2 (滤波)
#define SC7A20H_REG_CTRL_REG3    0x22 // 控制寄存器3 (中断1)
#define SC7A20H_REG_CTRL_REG4    0x23 // 控制寄存器4 (量程, 自测试)
#define SC7A20H_REG_CTRL_REG5    0x24 // 控制寄存器5 (FIFO, 中断锁存)
#define SC7A20H_REG_CTRL_REG6    0x25 // 控制寄存器6 (中断2)
#define SC7A20H_REG_STATUS       0x27 // 状态寄存器
#define SC7A20H_REG_OUT_X_L      0x28 // X轴低字节
#define SC7A20H_REG_OUT_X_H      0x29 // X轴高字节
#define SC7A20H_REG_OUT_Y_L      0x2A // Y轴低字节
#define SC7A20H_REG_OUT_Y_H      0x2B // Y轴高字节
#define SC7A20H_REG_OUT_Z_L      0x2C // Z轴低字节
#define SC7A20H_REG_OUT_Z_H      0x2D // Z轴高字节
#define SC7A20H_REG_FIFO_CTRL    0x2E // FIFO控制
#define SC7A20H_REG_FIFO_SRC     0x2F // FIFO状态
#define SC7A20H_REG_AOI1_CFG     0x30 // 中断1配置
#define SC7A20H_REG_AOI1_SRC     0x31 // 中断1状态
#define SC7A20H_REG_AOI1_THS     0x32 // 中断1阈值
#define SC7A20H_REG_AOI1_DUR     0x33 // 中断1持续时间
#define SC7A20H_REG_AOI2_CFG     0x34 // 中断2配置
#define SC7A20H_REG_AOI2_SRC     0x35 // 中断2状态
#define SC7A20H_REG_AOI2_THS     0x36 // 中断2阈值
#define SC7A20H_REG_AOI2_DUR     0x37 // 中断2持续时间
#define SC7A20H_REG_CLICK_CFG    0x38 // 单击检测配置
#define SC7A20H_REG_CLICK_SRC    0x39 // 单击检测状态
#define SC7A20H_REG_CLICK_COEFF1 0x3A // 单击系数1
#define SC7A20H_REG_CLICK_COEFF2 0x3B // 单击系数2
#define SC7A20H_REG_CLICK_COEFF3 0x3C // 单击系数3
#define SC7A20H_REG_CLICK_COEFF4 0x3D // 单击系数4
#define SC7A20H_REG_SOFT_RESET   0x68 // 软复位
#define SC7A20H_REG_VERSION      0x70 // 版本

static i2c_master_dev_handle_t sc7a20h_i2c_dev;
static uint8_t fsr_num = 2;
static gpio_num_t int1_pin = -1;
static gpio_num_t int2_pin = -1;

/* I2C 写单个寄存器 */
static void sc7a20h_write_byte(const uint8_t reg_addr, const uint8_t val)
{
    uint8_t buf[2] = { reg_addr, val };
    ESP_ERROR_CHECK(i2c_master_transmit(sc7a20h_i2c_dev, buf, 2, -1));
}

/* I2C 读单个寄存器 */
static void sc7a20h_read_byte(const uint8_t reg_addr, uint8_t* data_buf)
{
    ESP_ERROR_CHECK(i2c_master_transmit_receive(
        sc7a20h_i2c_dev, &reg_addr, 1, data_buf, 1, -1));
}

static void sc7a20h_read(const uint8_t reg_start_addr, uint8_t* data_buf, uint8_t data_len)
{
    uint8_t reg_addr_inc = reg_start_addr | 0x80; // 设置自动地址递增
    ESP_ERROR_CHECK(i2c_master_transmit_receive(
        sc7a20h_i2c_dev, &reg_addr_inc, 1, data_buf, data_len, -1));
}

void sc7a20h_init(i2c_master_bus_handle_t i2c_bus, sc7a20h_config_t* cfg)
{
    ESP_LOGI(TAG, "Initializing ...");

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = cfg->i2c_addr,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    ESP_ERROR_CHECK(i2c_master_bus_add_device(i2c_bus, &dev_config, &sc7a20h_i2c_dev));

    /* 验证芯片ID */
    if (sc7a20h_verification() != ESP_OK) return;

    /* 软件复位 */
    sc7a20h_soft_reset();
    vTaskDelay(pdMS_TO_TICKS(10)); // 等待复位完成

    /* 配置 CTRL_REG1: ODR + 低功耗 + 轴使能 */
    uint8_t ctrl_reg1 = (cfg->odr << 4) | ((cfg->mode & 0x01) << 3); // LPen bit
    if (cfg->axis_en.x_en) ctrl_reg1 |= 0x01;
    if (cfg->axis_en.y_en) ctrl_reg1 |= 0x02;
    if (cfg->axis_en.z_en) ctrl_reg1 |= 0x04;
    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG1, ctrl_reg1);

    /* 配置 CTRL_REG0: 高性能/增强模式 + OSR */
    uint8_t ctrl_reg0 = 0;
    ctrl_reg0 |= (cfg->mode >> 1); // HR bit

    /* OSR 默认 001 (ODR/2) */
    ctrl_reg0 |= 0x10; // OSR[0]
    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG0, ctrl_reg0);

    /* 配置 CTRL_REG4: 量程 */
    uint8_t ctrl_reg4 = (cfg->fsr << 4) | 0x80; // FSR && BDU=1 (数据更新时锁存输出寄存器)
    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG4, ctrl_reg4);
    fsr_num = 2 << cfg->fsr;

    /* 配置 CTRL_REG3: FIFO模式 (默认12bit) */
    // sc7a20h_write_byte_byte(SC7A20H_REG_CTRL_REG3, 0x00);
    ESP_LOGI(TAG, "Init Success");
}

void sc7a20h_soft_reset()
{
    /* 写入 0xA5 复位整个电路 */
    sc7a20h_write_byte(SC7A20H_REG_SOFT_RESET, 0xA5);
}

esp_err_t sc7a20h_verification()
{
    uint8_t chip_id, version;
    sc7a20h_read_byte(SC7A20H_REG_WHO_AM_I, &chip_id);
    if (chip_id != SC7A20H_CHIP_ID)
    {
        ESP_LOGE(TAG, "Invalid chip ID: 0x%02X, expected 0x%02X", chip_id, SC7A20H_CHIP_ID);
        return ESP_FAIL;
    }

    sc7a20h_read_byte(SC7A20H_REG_VERSION, &version);
    if (version != SC7A20H_VERSION)
    {
        ESP_LOGE(TAG, "Unexpected version: 0x%02X, expected 0x%02X", version, SC7A20H_VERSION);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Chip verified, ID: 0x%02X, Version: 0x%02X", chip_id, version);
    return ESP_OK;
}

void sc7a20h_read_accel(sc7a20h_accel_t* accel)
{
    uint8_t buf[6];
    sc7a20h_read(SC7A20H_REG_OUT_X_L, buf, 6);

    /* 组合 16bit 数据 (12bit 有效，左对齐) */
    accel->x = (int16_t)(buf[0] | (buf[1] << 8)) >> 4;
    accel->y = (int16_t)(buf[2] | (buf[3] << 8)) >> 4;
    accel->z = (int16_t)(buf[4] | (buf[5] << 8)) >> 4;
}

void sc7a20h_read_accel_g(sc7a20h_accel_g_t* accel_g)
{
    sc7a20h_accel_t accel;
    sc7a20h_read_accel(&accel);

    accel_g->x_g = (float)accel.x * fsr_num / 2048;
    accel_g->y_g = (float)accel.y * fsr_num / 2048;
    accel_g->z_g = (float)accel.z * fsr_num / 2048;
}

void sc7a20h_set_odr(sc7a20h_odr_t odr)
{
    uint8_t ctrl_reg1;
    sc7a20h_read_byte(SC7A20H_REG_CTRL_REG1, &ctrl_reg1);

    ctrl_reg1 &= 0x0F;       // 清除 ODR 位
    ctrl_reg1 |= (odr << 4); // 设置新 ODR

    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG1, ctrl_reg1);
}

void sc7a20h_set_fsr(sc7a20h_fsr_t fsr)
{
    uint8_t ctrl_reg4;
    sc7a20h_read_byte(SC7A20H_REG_CTRL_REG4, &ctrl_reg4);

    ctrl_reg4 &= ~(0x03 << 4); // 清除 FS 位
    ctrl_reg4 |= (fsr << 4);   // 设置新量程

    fsr = 1 << fsr;

    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG4, ctrl_reg4);
}

void sc7a20h_config_int(sc7a20h_int_config_t* int_cfg)
{
    uint8_t reg6_val = (!int_cfg->active_level) << 1 | int_cfg->pp_od;
    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG6, reg6_val);

    int1_pin = int_cfg->int1_gpio;
    int2_pin = int_cfg->int2_gpio;
    uint64_t pin_bit_mask = 0;
    if (int_cfg->int1_gpio != GPIO_NUM_NC) pin_bit_mask |= (1ULL << int_cfg->int1_gpio);
    if (int_cfg->int2_gpio != GPIO_NUM_NC) pin_bit_mask |= (1ULL << int_cfg->int2_gpio);

    if (pin_bit_mask == 0)
    {
        ESP_LOGW(TAG, "No interrupt GPIO configured");
        return;
    }

    gpio_config_t int_gpio_config = {
        .mode = GPIO_MODE_INPUT,
        .intr_type = (int_cfg->active_level ? GPIO_INTR_POSEDGE : GPIO_INTR_NEGEDGE),
        .pin_bit_mask = pin_bit_mask,
    };
    ESP_ERROR_CHECK(gpio_config(&int_gpio_config));

    gpio_install_isr_service(ESP_INTR_FLAG_EDGE);

    if (int_cfg->int1_gpio != GPIO_NUM_NC)
    {
        gpio_isr_handler_add(int_cfg->int1_gpio, int_cfg->isr, (void*)SC7A20H_INT1);
        gpio_intr_enable(int_cfg->int1_gpio);
        ESP_LOGI(TAG, "int1 configured on GPIO %d", int_cfg->int1_gpio);
    }

    if (int_cfg->int2_gpio != GPIO_NUM_NC)
    {
        gpio_isr_handler_add(int_cfg->int2_gpio, int_cfg->isr, (void*)SC7A20H_INT2);
        gpio_intr_enable(int_cfg->int2_gpio);
        ESP_LOGI(TAG, "int2 configured on GPIO %d", int_cfg->int2_gpio);
    }
}

void sc7a20h_deconfig_int()
{
    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG6, 0x00);
    uint8_t reg_val;
    sc7a20h_read_byte(SC7A20H_REG_CTRL_REG3, &reg_val);
    sc7a20h_write_byte(SC7A20H_REG_CTRL_REG3, reg_val & 0x01);
    gpio_intr_disable(int1_pin);
    gpio_intr_disable(int2_pin);
    gpio_isr_handler_remove(int1_pin);
    gpio_isr_handler_remove(int2_pin);
    // gpio_uninstall_isr_service();
    if (int1_pin != GPIO_NUM_NC) gpio_reset_pin(int1_pin);
    if (int2_pin != GPIO_NUM_NC) gpio_reset_pin(int2_pin);
}

static void sc7a20h_set_int_type(sc7a20h_int_pin_t int_pin, sc7a20h_int_type_t int_type)
{
    if (int_pin == SC7A20H_INT2)
    {
        if (int_type == SC7A20H_INT_TYPE_FIFO_WTM || int_type == SC7A20H_INT_TYPE_OVERRUN)
        {
            ESP_LOGE(TAG, "int2 cannot use FIFO interrupt");
            return;
        }
        else if (int_type == SC7A20H_INT_TYPE_DRDY)
        {
            int_type = SC7A20H_INT_TYPE_DRDY >> 1;
        }
    }

    uint8_t reg_addr = (int_pin == SC7A20H_INT1) ? SC7A20H_REG_CTRL_REG3 : SC7A20H_REG_CTRL_REG6;
    uint8_t reg_val;
    sc7a20h_read_byte(reg_addr, &reg_val);
    reg_val |= int_type;

    sc7a20h_write_byte(reg_addr, reg_val);
}

void sc7a20h_config_int_aoi1(sc7a20h_int_pin_t int_pin, uint8_t cfg,  uint8_t threshold, uint8_t duration)
{
    // 6D 方向位置/运动检测
    // AOI=0(或), 6D=1 => 6D方向运动识别
    // 任意方向变化都可触发
    sc7a20h_write_byte(SC7A20H_REG_AOI1_CFG, cfg);

    // 设置阈值
    sc7a20h_write_byte(SC7A20H_REG_AOI1_THS, threshold);

    // 设置持续时间，num*1/ODR
    sc7a20h_write_byte(SC7A20H_REG_AOI1_DUR, duration | 0x80);

    // AOI1 映射到 INT
    sc7a20h_set_int_type(int_pin, SC7A20H_INT_TYPE_AOI1);
}

void sc7a20h_config_int_aoi2(sc7a20h_int_pin_t int_pin, uint8_t cfg,  uint8_t threshold, uint8_t duration)
{
    sc7a20h_write_byte(SC7A20H_REG_AOI2_CFG, cfg);
    sc7a20h_write_byte(SC7A20H_REG_AOI2_THS, threshold);
    sc7a20h_write_byte(SC7A20H_REG_AOI2_DUR, duration | 0x80);
    sc7a20h_set_int_type(int_pin, SC7A20H_INT_TYPE_AOI2);
}
