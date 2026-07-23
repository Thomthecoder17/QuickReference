#pragma once

#include "esp_err.h"

extern double temp;

esp_err_t init_weather_api(void);
esp_err_t fetch_weather_data(void);