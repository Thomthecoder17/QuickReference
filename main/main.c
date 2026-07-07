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

#include "lcd_init.h"

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
    // 1. Initialize lcd
    lcd_init();
    // Is this a real thing? - nope
    lv_demo_benchmark();   
    // 4. Clean up app_main to prevent the Watchdog crash
    // Do NOT run a tight loop here. Delete this task so the core can rest.
    ESP_LOGI("MAIN", "UI Initialized. Deleting app_main task.");
    vTaskDelete(NULL); 
}
