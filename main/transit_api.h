#pragma once

#include "esp_err.h"

extern int predictions_by_dir[2];
extern bool handled_dirs[2]; // Track which directions have been handled

extern char *dir1_headsign;
extern char *dir2_headsign;

esp_err_t fetch_transit_data(void);