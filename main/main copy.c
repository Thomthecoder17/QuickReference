#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lcd_st7735.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_check.h"


#define PIN_NUM_SCLK           36
#define PIN_NUM_MOSI           35
#define PIN_NUM_LCD_DC         37
#define PIN_NUM_LCD_RST        38
#define PIN_NUM_LCD_CS         34

#define PIN_NUM_BK_LIGHT      -1

#define LCD_H_RES 128
#define LCD_V_RES 160

#define LCD_HOST SPI2_HOST

void app_main(void)
{
    // 1. Initialize the SPI Bus
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_SCLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 40 * sizeof(uint16_t), // Buffer chunk size
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    // 2. Configure the LCD IO Interface (Handles SPI Mode 0 and Command/Data pin)
    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_LCD_DC,
        .cs_gpio_num = PIN_NUM_LCD_CS,
        .pclk_hz = 20 * 1000 * 1000, // 20 MHz Clock Speed
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,               // Defaults to SPI Mode 0
        .trans_queue_depth = 10,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));
    // 3. Initialize the Screen Panel (Change st7789 to your panel type, e.g., ili9341)
    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_LCD_RST,
        .bits_per_pixel = 16,
    };
    // Note: If using a custom component vendor driver, use its specific creation function
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7789(io_handle, &panel_config, &panel_handle));
    
    // Reset and awake the physical glass
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));

    // 4. Initialize the LVGL Porting Framework
    lv_init();
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    // 5. Register the screen handle into the background LVGL Task
    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = LCD_H_RES * 20, // 20 lines high to keep ESP32-S2 SRAM safe
        .double_buffer = false,         // Keep false for tight single-core S2 memory constraints
        .hres = LCD_H_RES,
        .vres = LCD_V_RES,
        .flags = {
            .buff_dma = true,          // Allocates canvas memory inside internal DMA-capable RAM
        }
    };
    lv_display_t * disp = lvgl_port_add_disp(&disp_cfg);

    // 2. Lock the LVGL port before interacting with any UI elements!
    // This stops the background port task from rendering while we build the screen.
    if (lvgl_port_lock(0)) {
        
        // Create a simple "Hello World" label on the active screen
        lv_obj_t * label = lv_label_create(lv_scr_act());
        
        if (label != NULL) {
            lv_label_set_text(label, "Hi :3");
            lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
        } else {
            ESP_LOGE("MAIN", "LVGL out of memory! Could not allocate label.");
        }

        // 3. Unlock immediately after UI changes are finished
        lvgl_port_unlock();
    }

    // 4. Clean up app_main to prevent the Watchdog crash
    // Do NOT run a tight loop here. Delete this task so the core can rest.
    ESP_LOGI("MAIN", "UI Initialized. Deleting app_main task.");
    vTaskDelete(NULL); 
}
