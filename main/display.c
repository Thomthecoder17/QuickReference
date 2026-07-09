#include "display.h"

// LVGL Stuff
#include "lvgl.h" 
#include "esp_lvgl_port.h"

// ERROR STUFF! (totally not having those)
#include "esp_check.h"
#include "esp_log.h"

esp_err_t lvgl_init(void) {
    esp_err_t ret = ESP_OK;

    ESP_LOGD("LVGL", "Initializing LVGL...");
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_GOTO_ON_ERROR(lvgl_port_init(&lvgl_cfg), err, "LVGL", "Failed to initialize LVGL");

    return ret;
err:
    ESP_LOGE("LVGL", "Failed to initialize LVGL: %s", esp_err_to_name(ret));
    return ret;
}

