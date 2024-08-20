
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
#include "lcd_utils.h" // Include the header for LCD functions
#include "wifi_utils.h"
#include "httpMethods.h"
#include "ntp.h"
#include "nvs_utils.h"
#include "devicesRelay_utils.h"

static const char *TAG = "firebaseFns";

void firebase_get_new_data(void)
{
    // Construct the URL based on the device ID
    snprintf(url, sizeof(url), "%snewData.json", events_url);

    // Configure the HTTP client
    esp_http_client_config_t config_get = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = (const char *)clientcert_pem_start,
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

void clear_new_data_section(void){
    ESP_LOGI(TAG, "clear new data"); //No new data enters here
    // Construct the URL based on the device ID
    snprintf(url, sizeof(url), "%snewData.json", events_url);

    // Create JSON payload to clear the newData section and set status to false
    cJSON *json = cJSON_CreateObject();
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }
    cJSON_AddStringToObject(json, "section", "");
    cJSON_AddBoolToObject(json, "status", false);

    char *post_data = cJSON_PrintUnformatted(json);
    if (post_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON data");
        cJSON_Delete(json);
        return;
    }

    // Configure the HTTP client
    esp_http_client_config_t config_post = {
        .url = url,
        .method = HTTP_METHOD_PUT,  // Use PUT method to update the data
        .cert_pem = (const char *)clientcert_pem_start,
        .event_handler = client_event_get_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 (int)esp_http_client_get_content_length(client));

        if (esp_http_client_get_status_code(client) == 200) {
            POSTNONEWDATA = 0;
            lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
            lcd_send_string("NUBE ACTUALIZADA");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            lcd_send_string("");
            FETCHNEWINFODATA = 0;
            FETCHNEWDEVICESDATA = 0;
            FETCHNEWDATETIMEDATA = 0;
            FETCHNEWFIRMWAREDATA = 0;
            FETCHNEWALIVE = 0;
        }
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
    cJSON_Delete(json);
    free(post_data);
}

void firebase_set_dispositivo_info() {
    // Set up the JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }
    get_wifi_signal_strength();
    bool apMode = is_ap_mode();
    cJSON_AddStringToObject(root, "salaName", "Sala Piloto");
    cJSON_AddStringToObject(root, "firmware", firmwareVersion);
    cJSON_AddStringToObject(root, "ssid", wifiSsid);
    cJSON_AddBoolToObject(root, "status", true);
    cJSON_AddStringToObject(root, "connDBI", rssi_str);
    cJSON_AddBoolToObject(root, "apMode", apMode);

    // Format and add the IP address
    char ip_str[16];
    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&system_ip.ip));
    cJSON_AddStringToObject(root, "ip", ip_str);

    // Add lastConnectionTime field
    time(&now);
    localtime_r(&now, &timeinfo);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &timeinfo);  // Remove 'Z'
    cJSON_AddStringToObject(root, "lastConnectionTime", time_str);

    // Convert JSON object to string
    char *put_data = cJSON_Print(root);
    if (put_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON object");
        cJSON_Delete(root);
        return;
    }

    snprintf(url, sizeof(url), "%sinfo.json", disp_url);

    // Perform the HTTP PUT
    perform_http_put(url, put_data);

    // Clean up
    cJSON_free(put_data);
    cJSON_Delete(root);
}

void send_alive_package() {
    // Set up the JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }
    get_wifi_signal_strength();
    // Only update the status to true
    cJSON_AddBoolToObject(root, "status", true);

    // Add lastConnectionTime field
    time(&now);
    localtime_r(&now, &timeinfo);
    char time_str[64];
    strftime(time_str, sizeof(time_str), "%Y-%m-%dT%H:%M:%S", &timeinfo);  // Remove 'Z'
    cJSON_AddStringToObject(root, "lastConnectionTime", time_str);
    cJSON_AddStringToObject(root, "connDBI", rssi_str);
    // Convert JSON object to string
    char *patch_data = cJSON_Print(root);
    if (patch_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON object");
        cJSON_Delete(root);
        return;
    }

    // Assuming `disp_url` is a global variable or accessible in this scope
    snprintf(url, sizeof(url), "%sinfo.json", disp_url);

    // Perform the HTTP PATCH
    perform_http_patch(url, patch_data);

    // Clean up
    cJSON_free(patch_data);
    cJSON_Delete(root);
}

void firebase_get_dispositivo_info() {
    snprintf(url, sizeof(url), "%sinfo.json", disp_url);
    perform_http_get(url);
}

void firebase_get_dispositivo_device() {
    snprintf(url, sizeof(url), "%s%s.json", disp_url, urlSection);
    perform_http_get(url);
}

void firebase_get_dispositivo_devices() {
    snprintf(url, sizeof(url), "%sdevices.json", disp_url);
    perform_http_get(url);
}

void firebase_update_dispositivo_device_status(int device_number, bool status) {
    // Create a JSON root object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }

    // Add the "status" field to the JSON object
    cJSON_AddBoolToObject(root, "status", status);

    // Convert the JSON object to a string
    char *patch_data = cJSON_Print(root);
    if (patch_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON object");
        cJSON_Delete(root);
        return;
    }

    // Format the URL to point directly to the device's status field, ensuring it ends with .json
    char url[256];
    snprintf(url, sizeof(url), "%sdevices/%d.json", disp_url, device_number);

    // Log the URL and payload for debugging
    ESP_LOGI(TAG, "Updating Firebase URL: %s", url);
    ESP_LOGI(TAG, "Payload: %s", patch_data);

    // Perform the HTTP PATCH request to update the device status
    perform_http_patch(url, patch_data);

    // Clean up
    cJSON_free(patch_data);
    cJSON_Delete(root);
}

void send_devices_pins() {
    nvs_handle_t my_handle;
    esp_err_t err;

    // Open NVS handle
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle!", esp_err_to_name(err));
        return;
    }

    // Create a JSON root object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create JSON object");
        nvs_close(my_handle);
        return;
    }

    // Iterate over NVS items
    nvs_iterator_t it = NULL;
    err = nvs_entry_find("nvs", "storage", NVS_TYPE_ANY, &it);
    while (it != NULL) {
        nvs_entry_info_t info;
        nvs_entry_info(it, &info);

        // Check if the key matches the pattern "device-n"
        if (strstr(info.key, "device-") != NULL) {
            // Read the pin value associated with the device key
            uint8_t pin;
            size_t required_size = sizeof(pin);
            err = nvs_get_u8(my_handle, info.key, &pin);
            if (err == ESP_OK) {
                // Add to JSON object
                cJSON_AddNumberToObject(root, info.key, pin);
            } else {
                ESP_LOGE(TAG, "Error reading value for key %s", info.key);
            }
        }

        // Move to the next item
        it = nvs_entry_next(it);
    }

    // Add other fields to the JSON object
    cJSON_AddStringToObject(root, "section", "devices_pins");
    cJSON_AddStringToObject(root, "feedback", "200");
    cJSON_AddBoolToObject(root, "status", true);

    // Convert JSON object to string
    char *json_data = cJSON_Print(root);
    if (json_data == NULL) {
        ESP_LOGE(TAG, "Failed to print JSON object");
        cJSON_Delete(root);
        nvs_close(my_handle);
        return;
    }

    // Construct the full URL for the request
    snprintf(url, sizeof(url), "%sretrievedData.json", events_url);

    // Log the URL and JSON data for debugging
    ESP_LOGI(TAG, "Updating Firebase URL: %s", url);
    ESP_LOGI(TAG, "JSON Payload: %s", json_data);

    // Perform the HTTP PUT request to overwrite the data in Firebase
    perform_http_patch(url, json_data);  // Assuming perform_http_patch exists

    // Clean up
    cJSON_free(json_data);
    cJSON_Delete(root);
    nvs_close(my_handle);
}

void firebase_read_device_status()
{
    // Create a JSON object
    cJSON *root = cJSON_CreateObject();
    if (root == NULL)
    {
        ESP_LOGE(TAG, "Failed to create JSON object");
        return;
    }

    // Add section and status to the JSON object
    cJSON_AddStringToObject(root, "section", "get/devices");
    cJSON_AddBoolToObject(root, "status", true);

    // Convert JSON object to a string
    char *json_data = cJSON_Print(root);
    if (json_data == NULL)
    {
        ESP_LOGE(TAG, "Failed to print JSON object");
        cJSON_Delete(root);
        return;
    }

    // Prepare the Firebase URL for updating newData
    snprintf(url, sizeof(url), "%snewData.json", events_url);

    // Log the URL and JSON data for debugging
    ESP_LOGI(TAG, "Updating Firebase newData with URL: %s", url);
    ESP_LOGI(TAG, "Payload: %s", json_data);

    // Perform the HTTP PATCH request to update the newData
    perform_http_put(url, json_data);

    // Clean up
    cJSON_free(json_data);
    cJSON_Delete(root);
}

