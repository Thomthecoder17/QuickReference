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

esp_err_t draw_ui(void) {
    ESP_LOGD("LVGL", "Drawing UI...");
    if (lvgl_port_lock(0)) {
        // ADD DRAWING SH!T HERE
        lvgl_port_unlock();
    } else {
        ESP_LOGE("LVGL", "Failed to lock LVGL mutex for drawing UI");
        return ESP_FAIL;
    }
    return ESP_OK;
}

