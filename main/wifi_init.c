#include "wifi_init.h"

#include "nvs_flash.h"
#include "esp_netif.h"
#include "esp_event.h"

#include "esp_wifi.h"

#include "esp_log.h"
#include "esp_check.h"

#define TAG "WIFI_INIT"

// Define event bits for tracking state
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
#define MAX_FAILURES 5


// Event handler to process network state changes automatically
static void event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < MAX_FAILURES) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retrying connection to AP...");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGE(TAG, "Failed to connect to AP");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Successfully acquired IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_init(void) {
    esp_err_t ret = ESP_OK;


    // 1. CREATE THIS FIRST before any error macros can run
    s_wifi_event_group = xEventGroupCreate();
    if (s_wifi_event_group == NULL) {
        ESP_LOGE(TAG, "Failed to create FreeRTOS event group");
        return ESP_ERR_NO_MEM;
    }

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

    // Create actual netif instance for station mode (Crucial for DNS/DHCP routing!)
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ret = ESP_FAIL;
        ESP_GOTO_ON_ERROR(ret, err, TAG, "Failed to create default station netif");
    }

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        NULL));


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

    /* 
       CRITICAL PATH: Wait here until either connection succeeds (WIFI_CONNECTED_BIT) 
       or fails definitively (WIFI_FAIL_BIT).
    */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID: %s", CONFIG_WIFI_SSID);
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to SSID: %s", CONFIG_WIFI_SSID);
        return ESP_FAIL;
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
        return ESP_FAIL;
    }

    return ret;
err:
    ESP_LOGE(TAG, "Wi-Fi initialization failed: %s", esp_err_to_name(ret));
    return ret;
}