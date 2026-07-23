#include <time.h>
#include "config.h"
#include "esp_netif_sntp.h"
#include "esp_log.h"

#define TAG "CONFIG"

const char* FIRMWARE_VERSION = "1.0.0";
const char* APP_NAME = "QuickReference";

bool is_sntp_initialized = false;

esp_err_t init_sntp(void) {
    // 1. Thread-safe guard check
    
    if (is_sntp_initialized) {
        ESP_LOGW(TAG, "SNTP already initialized. Skipping.");
        return ESP_OK; 
    }

    // 2. Configure and initialize SNTP (Modern API)
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG("pool.ntp.org");
    esp_err_t err = esp_netif_sntp_init(&config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init SNTP framework");
        return err;
    }

    // 3. Set timezone to UTC
    setenv("TZ", "UTC0", 1);
    tzset();

    // 4. Wait for SNTP to synchronize
    int retry = 0;
    const int retry_count = 10;
    while (esp_netif_sntp_sync_wait(pdMS_TO_TICKS(1000)) != ESP_OK && ++retry < retry_count) {
        ESP_LOGI(TAG, "Waiting for system time to be set... (%d/%d)", retry, retry_count);
    }

    if (retry >= retry_count) {
        ESP_LOGE(TAG, "Failed to set system time");
        return ESP_FAIL;
    }

    // 5. Mark as initialized only after a successful sync
    is_sntp_initialized = true;
    ESP_LOGI(TAG, "Time successfully synchronized to UTC!");
    return ESP_OK;
}