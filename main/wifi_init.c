#include "wifi_init.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "esp_wifi.h"

#include "esp_log.h"
#include "esp_check.h"

#define TAG "WIFI_INIT"

esp_err_t wifi_init(void) {
    esp_err_t ret = ESP_OK;

    // Initialize NVS
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_GOTO_ON_ERROR(ret, err, TAG, "Failed to initialize NVS");

    // Initialize the TCP/IP stack
    ESP_GOTO_ON_ERROR(esp_netif_init(), err, TAG, "Failed to initialize TCP/IP stack");

    // Create the default event loop
    ESP_GOTO_ON_ERROR(esp_event_loop_create_default(), err, TAG, "Failed to create default event loop");

    // Initialize Wi-Fi with default configuration
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_GOTO_ON_ERROR(esp_wifi_init((wifi_init_config_t *)&cfg), err, TAG, "Failed to initialize Wi-Fi");

    ESP_GOTO_ON_ERROR(esp_wifi_set_mode(WIFI_MODE_STA), err, TAG, "Failed to set Wi-Fi mode");

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = CONFIG_WIFI_SSID,
            .password = CONFIG_WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };
    ESP_GOTO_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wifi_config), err, TAG, "Failed to set Wi-Fi configuration");
    ESP_GOTO_ON_ERROR(esp_wifi_start(), err, TAG, "Failed to start Wi-Fi");
    ESP_GOTO_ON_ERROR(esp_wifi_connect(), err, TAG, "Failed to connect to Wi-Fi");

    return ret;
err:
    ESP_LOGE(TAG, "Wi-Fi initialization failed: %s", esp_err_to_name(ret));
    return ret;
}