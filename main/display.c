#include "display.h"

// LVGL Stuff
#include "lvgl.h" 
#include "esp_lvgl_port.h"

// ERROR STUFF! (totally not having those)
#include "esp_check.h"
#include "esp_log.h"

static lv_obj_t * screen = NULL;
static lv_obj_t * weather_label = NULL;

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

esp_err_t init_ui(void) {
    if (screen == NULL) {
        screen = lv_screen_active();
    }

    if (screen == NULL) {
        ESP_LOGE("LVGL", "Screen is NULL, cannot initialize UI");
        return ESP_FAIL;
    }

    ESP_LOGD("LVGL", "Initializing UI...");

    if (lvgl_port_lock(0)) {
        lv_obj_clean(screen);
        weather_label = lv_label_create(screen);
        lv_label_set_long_mode(weather_label, LV_LABEL_LONG_WRAP);
        lv_obj_set_style_text_font(weather_label, &lv_font_montserrat_26, 0);
        lv_obj_set_align(weather_label, LV_ALIGN_CENTER);
        lvgl_port_unlock();
    } else {
        ESP_LOGE("LVGL", "Failed to lock LVGL mutex for initializing UI");
        return ESP_FAIL;
    }
    return ESP_OK;
}


esp_err_t set_weather_text(const char *text) {
    if (weather_label == NULL) {
        ESP_LOGE("LVGL", "Weather label is NULL, cannot set text");
        return ESP_FAIL;
    }

    if (lvgl_port_lock(0)) {
        lv_label_set_text(weather_label, text);
        lvgl_port_unlock();
    } else {
        ESP_LOGE("LVGL", "Failed to lock LVGL mutex for setting weather text");
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t clean_screen(void) {
    if (screen == NULL) {
        screen = lv_screen_active();
    }

    if (screen == NULL) {
        ESP_LOGE("LVGL", "Screen is NULL, cannot clean screen");
        return ESP_FAIL;
    }

    if (lvgl_port_lock(0)) {
        lv_obj_clean(screen);
        lvgl_port_unlock();
    } else {
        ESP_LOGE("LVGL", "Failed to lock LVGL mutex for cleaning screen");
        return ESP_FAIL;
    }
    return ESP_OK;
}
