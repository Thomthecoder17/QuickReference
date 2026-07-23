#include "stdio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "nvs_flash.h"

#include "esp_log.h"
#include "esp_check.h"

#include "lcd_init.h"
#include "display.h"
#include "wifi_init.h"

#include "weather_api.h"
#include "transit_api.h"

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
    ESP_LOGI("MAIN", "App started up successfully.");
    // Set up WiFi
    ESP_ERROR_CHECK(wifi_init());

    //Initialize LVGL
    ESP_ERROR_CHECK(lvgl_init());

    //Initialize lcd and register the display with LVGL
    ESP_ERROR_CHECK(lcd_init());

    //Build the UI after the display is registered
    ESP_ERROR_CHECK(init_ui());

    ESP_ERROR_CHECK(set_weather_text("..."));

    //Initialize the HTTP client
    ESP_ERROR_CHECK(init_weather_api());    

    //Keep app_main alive but idle so the system watchdog stays happy.
    while (true) {
        // Fetch the weather data
        ESP_ERROR_CHECK(fetch_weather_data());
        //ESP_ERROR_CHECK(fetch_transit_data());

        ESP_LOGD("MAIN", "Weather: %.2f°F, Transit Dir1: %d min, Transit Dir2: %d min", temp, predictions_by_dir[0], predictions_by_dir[1]);

        //Update the UI text
        int temp_int = (int)temp;

        char weather_text[32];
        snprintf(weather_text, sizeof(weather_text), "%d°F", temp_int);

        ESP_LOGI("MAIN", "Setting weather text: %s", weather_text);
        ESP_ERROR_CHECK(set_weather_text(weather_text));

        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}
