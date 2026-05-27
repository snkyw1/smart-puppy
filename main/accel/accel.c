#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "sc7a20h/sc7a20h.h"
#include "low_power/low_power.h"

#define I2C_MASTER_NUM I2C_NUM_0

static const char* TAG = "SC7A20H_EXAMPLE";

static i2c_master_bus_handle_t i2c_bus;

static TaskHandle_t sc7a20h_task_handle;

static void IRAM_ATTR sc7a20h_isr_handler(void* arg)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;

    uint32_t sc7a20h_int_pin = (uint32_t)arg;

    if (sc7a20h_task_handle == NULL) {
        return;
    }

    xTaskNotifyFromISR(sc7a20h_task_handle, 1 << sc7a20h_int_pin, eSetBits, &xHigherPriorityTaskWoken);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void sc7a20h_task()
{
    sc7a20h_accel_t accel;
    uint32_t notify_value;

    while (1)
    {
        if (xTaskNotifyWait(0, (1 << SC7A20H_INT1) | (1 << SC7A20H_INT2), &notify_value, portMAX_DELAY))
        {
            if (notify_value & (1 << SC7A20H_INT1))
            {
                ESP_LOGI(TAG, "INT1 is triggered");
            }
            else
            {
                ESP_LOGI(TAG, "INT2 is triggered");
                low_power_wake();
            }

            // sc7a20h_read_byte(SC7A20H_REG_AOI1_SRC, &int_src_aoi1);
            // ESP_LOGI(TAG, "AOI1 Int Src: 0x%02X", int_src_aoi1);
            sc7a20h_read_accel(&accel);
            ESP_LOGI(TAG, "Accel: X=%d, Y=%d, Z=%d", accel.x, accel.y, accel.z);
        }

        /* 读取加速度数据 */
        // sc7a20h_read_accel(&accel);
        // ESP_LOGI(TAG, "Accel: X=%d, Y=%d, Z=%d", accel.x, accel.y, accel.z);
        // vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void accel_init(void)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM,
        .sda_io_num = CONFIG_ACCEL_I2C_SDA_IO,
        .scl_io_num = CONFIG_ACCEL_I2C_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &i2c_bus));

    sc7a20h_config_t cfg = {
        .i2c_addr = SC7A20H_I2C_ADDR_SDO_HIGH, // SDO 接高，0x19
        .odr = SC7A20H_ODR_100HZ,
        .fsr = SC7A20H_FSR_2G, // ±2G 量程，1mg/LSB
        .mode = SC7A20H_MODE_NORMAL,
        .axis_en = {
            .x_en = true,
            .y_en = true,
            .z_en = true,
        },
    };
    sc7a20h_init(i2c_bus, &cfg);

    xTaskCreate(sc7a20h_task, "sc7a20h_task", 2048, NULL, 1, &sc7a20h_task_handle);

    sc7a20h_int_config_t int_cfg = {
        .int1_gpio = CONFIG_ACCEL_INT1_GPIO,
        .int2_gpio = CONFIG_ACCEL_INT2_GPIO,
        .isr = sc7a20h_isr_handler,
        .active_level = 1,
        .pp_od = 0,
    };

    sc7a20h_config_int(&int_cfg);

    sc7a20h_config_int_aoi1(SC7A20H_INT2, SC7A20H_AOI_ZL, 55, 3);
}