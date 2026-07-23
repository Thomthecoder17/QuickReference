#include "weather_api.h"

#include <string.h>

#include "esp_log.h"
#include "esp_check.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"

#include "config.h"

#define TAG "WEATHER_API"
#define MAX_HTTP_OUTPUT_BUFFER 4096

double temp = 0; // Global variable to store the temperature

typedef struct {
    char buffer[MAX_HTTP_OUTPUT_BUFFER + 1];
    int buffer_idx;
} http_response_t;

static esp_http_client_handle_t client = NULL;
static http_response_t local_response = {0};

static esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    switch (evt->event_id) {
        case HTTP_EVENT_ERROR:
            ESP_LOGE(TAG, "HTTP_EVENT_ERROR");
            break;
        case HTTP_EVENT_ON_CONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
            break;
        case HTTP_EVENT_HEADER_SENT:
            ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
            break;
        case HTTP_EVENT_ON_HEADER:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
            break;
        case HTTP_EVENT_ON_DATA:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
            http_response_t *output = (http_response_t *)evt->user_data;

            if(output && (output->buffer_idx + evt->data_len < sizeof(output->buffer) - 1)) {
                memcpy(output->buffer + output->buffer_idx, evt->data, evt->data_len);
                output->buffer_idx += evt->data_len;
                output->buffer[output->buffer_idx] = '\0';
            } else {
                ESP_LOGE(TAG, "Output buffer is too small or NULL");
                return ESP_FAIL;
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
            break;
        case HTTP_EVENT_DISCONNECTED:
            ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
            break;
        default:
            ESP_LOGD(TAG, "Unhandled HTTP event: %d", evt->event_id);
            break;
    }
    return ESP_OK;
}

esp_err_t init_weather_api(void) {
    esp_err_t ret = ESP_OK;

    ret = init_sntp(); // Initialize SNTP to ensure time is set for HTTPS requests
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP");
        return ret;
    }

    char user_agent[64] = {0};
    snprintf(user_agent, sizeof(user_agent), "%s/%s", APP_NAME, FIRMWARE_VERSION);

    char url[256] = {0};
    char* require_qc = CONFIG_WEATHER_REQUIRE_QC ? "true" : "false";
    snprintf(url, sizeof(url), "%s/stations/%s/observations/latest?require_qc=%s", CONFIG_WEATHER_API_URL, CONFIG_WEATHER_STATION_ID, require_qc);
    ESP_LOGD(TAG, "Weather API URL: %s", url); 

    esp_http_client_config_t config = {
        .url = url,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .user_agent = user_agent,
        .user_data = &local_response,
        .event_handler = _http_event_handler,
        .method = HTTP_METHOD_GET,
        .timeout_ms = 5000,
    };

    client = esp_http_client_init(&config);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize HTTP client");
        return ESP_FAIL;
    }

    return ret;
}

esp_err_t fetch_weather_data(void) {
    esp_err_t ret = ESP_OK;

    memset(local_response.buffer, 0, sizeof(local_response.buffer));
    local_response.buffer_idx = 0;

    if (client == NULL) {
        ESP_LOGE(TAG, "HTTP client is not initialized");
        return ESP_FAIL;
    }

    ret = esp_http_client_perform(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(ret));
        return ESP_FAIL;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Received weather data: %s", local_response.buffer);

    cJSON *json = cJSON_Parse(local_response.buffer);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return ESP_FAIL;
    }

    // Process the JSON data as needed
    // For example, extract specific fields and store them in the weather_data parameter
    cJSON *properties = cJSON_GetObjectItem(json, "properties");

    if (properties) {
        cJSON *temperature = cJSON_GetObjectItem(properties, "temperature");
        if (temperature) {
            cJSON *value = cJSON_GetObjectItem(temperature, "value");
            cJSON *unit = cJSON_GetObjectItem(temperature, "unitCode");
            if (value && unit) {
                double temp_value = value->valuedouble;
                const char *unit_str = unit->valuestring;
                if (strcmp(unit_str, "wmoUnit:degC") == 0) {
                    temp = (int)(temp_value * 9.0 / 5.0 + 32.0); // Convert Celsius to Fahrenheit
                } else if (strcmp(unit_str, "wmoUnit:degF") == 0) {
                    temp = (int)temp_value; // Already in Fahrenheit
                } else {
                    ESP_LOGE(TAG, "Unknown temperature unit: %s", unit_str);
                    return ESP_FAIL;
                }
            }
        }
    }
    cJSON_Delete(json);

    return ret;
}