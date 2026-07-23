#include "esp_err.h"

extern const char* FIRMWARE_VERSION;
extern const char* APP_NAME;
extern bool is_sntp_initialized;

esp_err_t init_sntp(void);