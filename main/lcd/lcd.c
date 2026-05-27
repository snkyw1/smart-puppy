#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_lcd_panel_st7789.h"
#include "esp_log.h"
#include "key/key.h"
#include "ui/ui.h"
#include <dirent.h>
#include "esp_lvgl_port.h"
// #include "mmap_generate_image.h"

#define LCD_PIXEL_CLK_HZ    (26 * 1000 * 1000)
#define LVGL_DRAW_BUF_LINES 20
#define LCD_SPI_HOST        SPI2_HOST

#define BLK_DUTY_RESOLUTION LEDC_TIMER_10_BIT
#define BLK_LEDC_CLK        LEDC_USE_APB_CLK
#define BLK_LEDC_TIMER      LEDC_TIMER_0
#define BLK_LEDC_CHANNEL    LEDC_CHANNEL_5
#define BLK_FREQ_HZ         3000

static esp_lcd_panel_handle_t lcd_panel;
static esp_lcd_panel_io_handle_t lcd_io;
static lv_display_t* disp_handle;
// static mmap_assets_handle_t asset_eaf;


extern void han_sans_cn_medium_16_load(const char* partition_label);

static const char* TAG = "LCD";

static void lcd_blk_ledc_init(void)
{
    // Prepare and then apply the LEDC PWM timer configuration
    ledc_timer_config_t ledc_timer = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .duty_resolution = BLK_DUTY_RESOLUTION,
        .timer_num = BLK_LEDC_TIMER,
        .freq_hz = BLK_FREQ_HZ,
        .clk_cfg = BLK_LEDC_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // Prepare and then apply the LEDC PWM channel configuration
    ledc_channel_config_t ledc_channel = {
        .speed_mode = LEDC_LOW_SPEED_MODE,
        .channel = BLK_LEDC_CHANNEL,
        .timer_sel = BLK_LEDC_TIMER,
        .intr_type = LEDC_INTR_DISABLE,
        .gpio_num = CONFIG_LCD_GPIO_BLK,
        .duty = 0,
        .hpoint = 0
    };

    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

void lcd_set_brightness(uint8_t brightness)
{
    uint32_t duty = brightness * (1 << BLK_DUTY_RESOLUTION) / 100;
    ledc_set_duty(LEDC_LOW_SPEED_MODE, BLK_LEDC_CHANNEL, duty);
    ledc_update_duty(LEDC_LOW_SPEED_MODE, BLK_LEDC_CHANNEL);
}

uint8_t lcd_get_brightness()
{
    uint32_t duty = ledc_get_duty(LEDC_LOW_SPEED_MODE, BLK_LEDC_CHANNEL);
    return duty * 100 / (1 << BLK_DUTY_RESOLUTION);
}

static esp_err_t lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    /* LCD initialization */
    ESP_LOGD(TAG, "Initialize SPI bus");
    const spi_bus_config_t buscfg = {
        .sclk_io_num = CONFIG_LCD_GPIO_SCK,
        .mosi_io_num = CONFIG_LCD_GPIO_SDA,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = CONFIG_LCD_WIDTH * LVGL_DRAW_BUF_LINES * sizeof(uint16_t),
    };

    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_SPI_HOST, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    ESP_LOGD(TAG, "Install panel IO");
    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = CONFIG_LCD_GPIO_DC,
        .cs_gpio_num = CONFIG_LCD_GPIO_CS,
        .pclk_hz = LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 4,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi(LCD_SPI_HOST, &io_config, &lcd_io), err, TAG, "New panel IO failed");

    ESP_LOGD(TAG, "Install LCD driver");
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = CONFIG_LCD_GPIO_RST,
#if ESP_IDF_VERSION < ESP_IDF_VERSION_VAL(6, 0, 0)
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
#else
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_RGB,
#endif
        .bits_per_pixel = 16,
    };

    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel), err, TAG, "New panel failed");

    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);

    esp_lcd_panel_swap_xy(lcd_panel, true);
    esp_lcd_panel_mirror(lcd_panel, true, false);
    esp_lcd_panel_invert_color(lcd_panel, true);

    esp_lcd_panel_disp_on_off(lcd_panel, true);

    /* LCD backlight on */
    // gpio_config_t blk_io_cfg = {
    //     .mode = GPIO_MODE_OUTPUT,
    //     .pin_bit_mask = 1ULL << LCD_GPIO_BLK
    // };
    // gpio_config(&blk_io_cfg);
    // gpio_set_level(LCD_GPIO_BLK, 1);
    lcd_blk_ledc_init();
    lcd_set_brightness(100);

    return ret;

err:
    if (lcd_panel)
    {
        esp_lcd_panel_del(lcd_panel);
    }
    if (lcd_io)
    {
        esp_lcd_panel_io_del(lcd_io);
    }
    spi_bus_free(LCD_SPI_HOST);
    return ret;
}

static esp_err_t lvgl_init(void)
{
    lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    lvgl_cfg.task_priority = 2;
    lvgl_cfg.task_affinity = 1;
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    /* Add LCD screen */
    ESP_LOGD(TAG, "Add LCD screen");
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = CONFIG_LCD_WIDTH * LVGL_DRAW_BUF_LINES,
        .double_buffer = true,
        .hres = CONFIG_LCD_WIDTH,
        .vres = CONFIG_LCD_HEIGHT,
        .monochrome = false,
#if LVGL_VERSION_MAJOR >= 9
        .color_format = LV_COLOR_FORMAT_RGB565,
#endif
        .rotation = {
            .swap_xy = true,
            .mirror_x = true,
            .mirror_y = false,
        },
        .flags = {
            .buff_dma = true,
    // .buff_spiram = true,
#if LVGL_VERSION_MAJOR >= 9
            .swap_bytes = true,
#endif
        }
    };
    disp_handle = lvgl_port_add_disp(&disp_cfg);

    han_sans_cn_medium_16_load("font");
    return ESP_OK;
}

// static void eaf_mmap_init()
// {
//     const mmap_assets_config_t config = {
//         .partition_label = "image",
//         .max_files = MMAP_IMAGE_FILES,
//         .checksum = MMAP_IMAGE_CHECKSUM,
//         .flags = {
//             .mmap_enable = true,
//             .use_fs = false,
//             .app_bin_check = true,
//         },
//     };

//     mmap_assets_new(&config, &asset_eaf);
//     ESP_LOGI("Image eaf", "stored_files:%d", mmap_assets_get_stored_files(asset_eaf));
// }

// void lcd_get_eaf_dsc(lv_image_dsc_t* eaf_dsc, int index)
// {
//     eaf_dsc->data_size = mmap_assets_get_size(asset_eaf, index);
//     eaf_dsc->data = mmap_assets_get_mem(asset_eaf, index);
// }

void lcd_show()
{
    /* LCD HW initialization */
    ESP_ERROR_CHECK(lcd_init());

    /* LVGL initialization */
    ESP_ERROR_CHECK(lvgl_init());

    lvgl_port_key_init();

    // eaf_mmap_init();

    lvgl_port_lock(0);
    ui_home();
    lvgl_port_unlock();

}


