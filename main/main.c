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
#include "display.h"
#include "demos/widgets/lv_demo_widgets.h" //IT'S RIGHT THERE!!!!!!!!!!!!!!!

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
    // 1. Initialize LVGL
    ESP_ERROR_CHECK(lvgl_init());

    // 2. Initialize lcd and register the display with LVGL
    ESP_ERROR_CHECK(lcd_init());

    // 3. Build the demo UI while holding the LVGL mutex
    if (lvgl_port_lock(0)) {
        lv_demo_widgets();
        lvgl_port_unlock();
    }

    // Keep app_main alive but idle so the system watchdog stays happy.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
