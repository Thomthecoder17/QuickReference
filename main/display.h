#pragma once

#include "esp_err.h"

esp_err_t lvgl_init(void);

esp_err_t init_ui(void);

esp_err_t set_weather_text(const char *text);

esp_err_t clean_screen(void);