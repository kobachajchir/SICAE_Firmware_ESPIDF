
#include <sys/param.h>
#include "esp_http_client.h"
#include <esp_tls.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include "esp_log.h"
#include <string.h>
#include "webserver.h"
#include "globals.h"
#include "json/cJSON/cJSON.h"
#include "json/cJSON/cJSON_Utils.h"

static const char *TAG = "firebaseFns";

void firebase_get_new_data(void)
{
    // Construct the URL based on the device ID
    snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/events/%s/newData.json", device_id);

    // Configure the HTTP client
    esp_http_client_config_t config_get = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = (const char *)_binary_clientcert_pem_start,
        .event_handler = client_event_get_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_get);
    // Log the request URL
    ESP_LOGI(TAG, "Request HTTP GET to URL: %s", config_get.url);
    // Perform the GET request
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                esp_http_client_get_status_code(client),
                (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    // Clean up the HTTP client
    esp_http_client_cleanup(client);
}

void perform_http_get(const char* url) {
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = (const char *)_binary_clientcert_pem_start,
        .event_handler = client_event_get_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    ESP_LOGI(TAG, "Request HTTP GET to URL: %s", config.url);
    
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    esp_http_client_cleanup(client);
}

void firebase_get_dispositivo_info() {
    snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/dispositivos/%s/data/info.json", device_id);
    perform_http_get(url);
}

void firebase_get_dispositivo_device() {
    snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/dispositivos/%s/data/%s.json", device_id, urlSection);
    perform_http_get(url);
}

void firebase_get_dispositivo_devices() {
    snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/dispositivos/%s/data/devices.json", device_id);
    perform_http_get(url);
}