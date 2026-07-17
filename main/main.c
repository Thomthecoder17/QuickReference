#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_check.h"

#include "lcd_init.h"
#include "display.h"
#include "wifi_init.h"

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
    esp_log_level_set("*", ESP_LOG_DEBUG); 
    
    ESP_LOGI("MAIN", "App started up successfully.");
    // Set up WiFi
    ESP_ERROR_CHECK(wifi_init());

    //Initialize LVGL
    ESP_ERROR_CHECK(lvgl_init());

    //Initialize lcd and register the display with LVGL
    ESP_ERROR_CHECK(lcd_init());

    //Build the UI after the display is registered
    ESP_ERROR_CHECK(init_ui());

    //Update the UI text
    ESP_ERROR_CHECK(set_weather_text("25°C"));

    //Keep app_main alive but idle so the system watchdog stays happy.
    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
