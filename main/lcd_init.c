#include "lcd_init.h"

// ESP and communication Stuff
#include "driver/spi_master.h"
#include "driver/gpio.h"

// LCD Stuff
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_st7735.h"

// LVGL port
#include "esp_lvgl_port.h"

// ERROR STUFF! (totally not having those)
#include "esp_check.h"
#include "esp_log.h"

#define SPI_HOST SPI2_HOST
#define TAG "LCD_INIT"


static esp_err_t lcd_spi_init(void)
{
    const spi_bus_config_t buscfg = {
        .sclk_io_num = CONFIG_LCD_SCLK_PIN,
        .mosi_io_num = CONFIG_LCD_MOSI_PIN,
        .miso_io_num = CONFIG_LCD_MISO_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = CONFIG_LCD_H_RES * CONFIG_LCD_V_RES * sizeof(uint16_t), // Maximum transfer size in bytes
    };
    return spi_bus_initialize(SPI_HOST, &buscfg, SPI_DMA_CH_AUTO);
}

static esp_err_t create_lcd_panel_io(esp_lcd_panel_io_handle_t *lcd_io)
{
    const esp_lcd_panel_io_spi_config_t io_cfg = {
        .cs_gpio_num = CONFIG_LCD_CS_PIN,
        .dc_gpio_num = CONFIG_LCD_DC_PIN,
        .spi_mode = 0,
        .pclk_hz = CONFIG_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .trans_queue_depth = 10,
    };
    return esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI_HOST, &io_cfg, lcd_io);
}

static esp_err_t create_lcd_panel(esp_lcd_panel_io_handle_t lcd_io, esp_lcd_panel_handle_t *lcd_panel)
{
    const esp_lcd_panel_dev_config_t panel_cfg = {
        .reset_gpio_num = CONFIG_LCD_RST_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = CONFIG_LCD_COLOR_DEPTH,
    };
    return esp_lcd_new_panel_st7735(lcd_io, &panel_cfg, lcd_panel);
}

esp_err_t lcd_init(void)
{
    esp_err_t ret = ESP_OK;
    esp_lcd_panel_io_handle_t lcd_io = NULL;
    esp_lcd_panel_handle_t lcd_panel = NULL;

    ESP_LOGD(TAG, "Initializing SPI bus...");
    ESP_GOTO_ON_ERROR(lcd_spi_init(), err, TAG, "Failed to initialize SPI bus");

    ESP_LOGD(TAG, "Creating LCD panel IO...");
    ESP_GOTO_ON_ERROR(create_lcd_panel_io(&lcd_io), err, TAG, "Failed to create LCD panel IO");

    ESP_LOGD(TAG, "Initializing LCD panel...");
    ESP_GOTO_ON_ERROR(create_lcd_panel(lcd_io, &lcd_panel), err, TAG, "Failed to initialize LCD panel");

    ESP_LOGD(TAG, "Resetting LCD panel...");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_reset(lcd_panel), err, TAG, "Failed to reset LCD panel");

    ESP_LOGD(TAG, "Finalizing LCD panel initialization...");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_init(lcd_panel), err, TAG, "Failed to initialize LCD panel");

    ESP_LOGD(TAG, "Turning on LCD panel...");
    ESP_GOTO_ON_ERROR(esp_lcd_panel_disp_on_off(lcd_panel, true), err, TAG, "Failed to turn on LCD panel");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = CONFIG_LCD_H_RES * 20,
        .double_buffer = false,
        .hres = CONFIG_LCD_H_RES,
        .vres = CONFIG_LCD_V_RES,
        .flags = {
            .buff_dma = true,
            .swap_bytes = true, // THIS LINE IS ABSOLUTELY CRITICAL, AND TOOK ME FOREVER AND MANY AI SESSIONS TO FIGURE OUT. ONE LINE. I MIGHT BE LOSING IT.
        },
    };

    if (lvgl_port_add_disp(&disp_cfg) == NULL) {
        ret = ESP_FAIL;
        goto err;
    }

    ESP_LOGD(TAG, "LCD panel initialized successfully.");
    return ret;

err:
    spi_bus_free(SPI_HOST);
    ESP_LOGE(TAG, "Failed to initialize LCD panel: %s", esp_err_to_name(ret));
    return ret;
}