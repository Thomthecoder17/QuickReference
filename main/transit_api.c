#include "transit_api.h"

#include <string.h>
#include <time.h>

#include "esp_log.h"
#include "esp_check.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"

#include "cJSON.h"

#include "config.h"

#define TAG "TRANSIT_API"
#define MAX_HTTP_OUTPUT_BUFFER 8192

int predictions_by_dir[2] = {0, 0}; // Global variable to store predictions for two directions
bool handled_dirs[2] = {false, false}; // Track which directions have been handled

char *dir1_headsign;
char *dir2_headsign;

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

static esp_err_t init_transit_api(char *url) {
    esp_err_t ret = ESP_OK;

    ret = init_sntp(); // Initialize SNTP to ensure time is set for HTTPS requests
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize SNTP");
        return ret;
    }

    char user_agent[64] = {0};
    snprintf(user_agent, sizeof(user_agent), "%s/%s", APP_NAME, FIRMWARE_VERSION);

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

static time_t convert_to_epoch(const char *time_str) {
    struct tm tm_time = {0};
    if (strptime(time_str, "%Y-%m-%dT%H:%M:%S", &tm_time) == NULL) {
        ESP_LOGE(TAG, "Failed to parse time string: %s", time_str);
        return -1;
    }
    return mktime(&tm_time);
}

esp_err_t fetch_transit_predictions(bool *handled_dirs) {
    esp_err_t ret = ESP_OK;

    memset(&local_response.buffer, 0, sizeof(local_response.buffer));
    local_response.buffer_idx = 0;

    // If neither are handled, or both are handled, and you want to overwrite with schedule data, don't filter by direction. 
    // Otherwise, filter by the direction that hasn't been handled yet.
    char *dir_filter = "";
    if (!handled_dirs[0] && handled_dirs[1]) {
        dir_filter = "&filter[direction_id]=0";
    }
    if (!handled_dirs[1] && handled_dirs[0]) {
        dir_filter = "&filter[direction_id]=1";
    }

    char url[256] = {0};
    snprintf(url, sizeof(url), "%spredictions?filter[stop]=%s&filter[route]=%s%s&sort=arrival_time&page[limit]=4", CONFIG_TRANSIT_API_URL, CONFIG_TRANSIT_STOP_ID, CONFIG_TRANSIT_ROUTE_ID, dir_filter);
    ESP_LOGD(TAG, "Transit API URL: %s", url);

    if (client == NULL) {
        ret = init_transit_api(url);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize transit API client");
            return ret;
        }
    }
    else {
        esp_http_client_set_url(client, url);
    }

    ret = esp_http_client_perform(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(ret));
        return ret;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Transit API response: %s", local_response.buffer);

    cJSON *json = cJSON_Parse(local_response.buffer);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON parse error before: %s", error_ptr);
        } else {
            ESP_LOGE(TAG, "Unknown JSON parse error");
        }
        return ESP_FAIL;
    }

    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (data) {
        if (cJSON_IsArray(data)) {
            ESP_LOGD(TAG, "Processing data array");
            // Parses through the predictions array as long as a direction isn't handled
            int idx = 0;
            while (idx < cJSON_GetArraySize(data) && (!handled_dirs[0] || !handled_dirs[1])) {
                cJSON *prediction = cJSON_GetArrayItem(data, idx);

                // Parsing through JSON data for attributes
                if (prediction) {
                    cJSON *attributes = cJSON_GetObjectItem(prediction, "attributes");
                    ESP_LOGD(TAG, "Processing prediction attributes");
                    if (attributes) {
                        cJSON *dir = cJSON_GetObjectItem(attributes, "direction_id");
                        ESP_LOGD(TAG, "Processing direction_id");

                        // Validate direction_id
                        if (dir && cJSON_IsNumber(dir)) {
                            // Get the direction_id value 
                            int direction_id = dir->valueint;

                            // Check if the direction_id hasn't been handled yet
                            if (handled_dirs[direction_id] == false) {

                                // Get the arrival_time from the attributes
                                cJSON *arrival_time = cJSON_GetObjectItem(attributes, "arrival_time");
                                ESP_LOGD(TAG, "Processing arrival_time for direction_id %d", direction_id);

                                // Validate arrival_time
                                if (arrival_time && cJSON_IsString(arrival_time)) {

                                    // Convert arrival time to epoch, validate, and calculate time until arrival
                                    time_t epoch_arrival_time = convert_to_epoch(arrival_time->valuestring);
                                    if (epoch_arrival_time != -1) {
                                        int time_until_arrival = (int)(epoch_arrival_time - time(NULL));

                                        if (time_until_arrival >= 0) {
                                            predictions_by_dir[direction_id] = time_until_arrival;
                                            handled_dirs[direction_id] = true; // Mark this direction as handled
                                        } 
                                    }
                                }
                            }
                        }
                    }
                }
                idx++;
            }
        }
    }

    cJSON_Delete(json);

    return ret;
}

esp_err_t fetch_transit_schedule(bool *handled_dirs) {
    esp_err_t ret = ESP_OK;

    memset(&local_response.buffer, 0, sizeof(local_response.buffer));
    local_response.buffer_idx = 0;

    // If neither are handled, or both are handled, and you want to overwrite with schedule data, don't filter by direction. 
    // Otherwise, filter by the direction that hasn't been handled yet.
    char *dir_filter = "";
    if (!handled_dirs[0] && handled_dirs[1]) {
        dir_filter = "&filter[direction_id]=0";
    }
    if (!handled_dirs[1] && handled_dirs[0]) {
        dir_filter = "&filter[direction_id]=1";
    }

    char url[256] = {0};
    snprintf(url, sizeof(url), "%sschedules?filter[stop]=%s&filter[route]=%s%s&sort=arrival_time&page[limit]=4", CONFIG_TRANSIT_API_URL, CONFIG_TRANSIT_STOP_ID, CONFIG_TRANSIT_ROUTE_ID, dir_filter);
    ESP_LOGD(TAG, "Transit API URL: %s", url);

    if (client == NULL) {
        ret = init_transit_api(url);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "Failed to initialize transit API client");
            return ret;
        }
    }
    else {
        esp_http_client_set_url(client, url);
    }

    ret = esp_http_client_perform(client);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "HTTP request failed: %s", esp_err_to_name(ret));
        return ret;
    }

    int status_code = esp_http_client_get_status_code(client);
    if (status_code != 200) {
        ESP_LOGE(TAG, "HTTP request failed with status code: %d", status_code);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "Transit API response: %s", local_response.buffer);

    cJSON *json = cJSON_Parse(local_response.buffer);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        const char *error_ptr = cJSON_GetErrorPtr();
        if (error_ptr != NULL) {
            ESP_LOGE(TAG, "JSON parse error before: %s", error_ptr);
        } else {
            ESP_LOGE(TAG, "Unknown JSON parse error");
        }
        return ESP_FAIL;
    }

    cJSON *data = cJSON_GetObjectItem(json, "data");
    if (data) {
        if (cJSON_IsArray(data)) {
            // Parses through the schedule array as long as a direction isn't handled
            int idx = 0;
            while (idx < cJSON_GetArraySize(data) && (!handled_dirs[0] || !handled_dirs[1])) {
                cJSON *schedule_entry = cJSON_GetArrayItem(data, idx);

                // Parsing through JSON data for attributes
                if (schedule_entry) {
                    cJSON *attributes = cJSON_GetObjectItem(schedule_entry, "attributes");
                    if (attributes) {
                        cJSON *dir = cJSON_GetObjectItem(attributes, "direction_id");

                        // Validate direction_id
                        if (dir && cJSON_IsNumber(dir)) {
                            // Get the direction_id value 
                            int direction_id = dir->valueint;

                            // Check if the direction_id hasn't been handled yet
                            if (handled_dirs[direction_id] == false) {

                                handled_dirs[direction_id] = true;

                                // Get the departure_time from the attributes
                                cJSON *departure_time = cJSON_GetObjectItem(attributes, "departure_time");
                                // Validate departure_time
                                if (departure_time && cJSON_IsString(departure_time)) {
                                    // Convert departure time to epoch, validate, and calculate time until departure
                                    time_t epoch_departure_time = convert_to_epoch(departure_time->valuestring);
                                    if (epoch_departure_time != -1) {
                                        int time_until_departure = (int)(epoch_departure_time - time(NULL));

                                        if (time_until_departure >= 0) {
                                            predictions_by_dir[direction_id] = time_until_departure;
                                        } 
                                    }
                                }
                            }
                        }
                    }
                }
                idx++;
            }
        }
    }

    cJSON_Delete(json);

    return ret;
}

esp_err_t fetch_transit_data(void) {
    esp_err_t ret = ESP_OK;

    // Reset the handled directions
    handled_dirs[0] = false;
    handled_dirs[1] = false;

    ret = fetch_transit_predictions(handled_dirs);
    if (ret != ESP_OK || (!handled_dirs[0] && !handled_dirs[1])) {
        fetch_transit_schedule(handled_dirs);
    }

    // Fetch transit data from the API
    // Parse the response and update prediction_dir1, prediction_dir2, dir1_headsign, dir2_headsign
    return ret;
}