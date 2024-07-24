#include <sys/param.h>
#include "esp_http_client.h"
#include <esp_tls.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include "esp_log.h"
#include "globals.h"
#include "json/cJSON/cJSON.h"
#include "json/cJSON/cJSON_Utils.h"

static const char *TAG = "webserver";

static esp_err_t server_get_handler(httpd_req_t *req)
{
    const char resp[] = "Server GET Response .................";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t server_post_handler(httpd_req_t *req)
{
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);

    printf("\nServer POST content: %s\n", content);

    if (ret <= 0)
    {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT)
        {
            httpd_resp_send_408(req);
        }
        return ESP_FAIL;
    }

    const char resp[] = "Server POST Response .................";
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);

    return ESP_OK;
}

const httpd_uri_t server_uri_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = server_get_handler
};

const httpd_uri_t server_uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = server_post_handler
};

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
    case HTTP_EVENT_ON_DATA:
        if (!esp_http_client_is_chunked_response(evt->client)) {
            if (response_data == NULL) {
                response_data = malloc(evt->data_len + 1);
                response_data_len = evt->data_len;
            } else {
                response_data = realloc(response_data, response_data_len + evt->data_len + 1);
                response_data_len += evt->data_len;
            }
            memcpy(response_data + response_data_len - evt->data_len, evt->data, evt->data_len);
            response_data[response_data_len] = '\0';
        }
        break;
    case HTTP_EVENT_ON_FINISH:
        if (response_data) {
            ESP_LOGI(TAG, "HTTP GET Response: %s", response_data);

            // Parse JSON response using cJSON
            cJSON *json = cJSON_Parse(response_data);
            if (json) {
                cJSON *status = cJSON_GetObjectItem(json, "status");
                if (cJSON_IsBool(status) && cJSON_IsTrue(status)) {
                    cJSON *section = cJSON_GetObjectItem(json, "section");
                    if (cJSON_IsString(section)) {
                        const char *section_value = section->valuestring;
                        
                        // Store section_value in urlSection
                        strncpy(urlSection, section_value, sizeof(urlSection) - 1);
                        urlSection[sizeof(urlSection) - 1] = '\0'; // Ensure null termination

                        // Check section
                        if (strcmp(section_value, "/info") == 0) {
                            FETCHNEWINFODATA = 1;
                        } else if (strncmp(section_value, "/devices/", 9) == 0) {
                            FETCHNEWDEVICESDATA = 1;
                        }
                        ESP_LOGI(TAG, "Parsed section_value: %s", section_value);
                    } else {
                        ESP_LOGE(TAG, "section is not a string or missing");
                    }
                } else {
                    ESP_LOGE(TAG, "status is not a boolean or false");
                }
                cJSON_Delete(json);
            } else {
                const char *error_ptr = cJSON_GetErrorPtr();
                if (error_ptr != NULL) {
                    ESP_LOGE(TAG, "Failed to parse JSON response at %s", error_ptr);
                } else {
                    ESP_LOGE(TAG, "Failed to parse JSON response");
                }
            }
            free(response_data);
            response_data = NULL;
            response_data_len = 0;
        }
        break;
    default:
        break;
    }
    return ESP_OK;
}

void client_post_rest_function(void)
{
    esp_http_client_config_t config_post = {
        .url = "https://sicaewebapp-default-rtdb.firebaseio.com/",
        .method = HTTP_METHOD_POST,
        .cert_pem = (const char *)_binary_clientcert_pem_start,
        .event_handler = client_event_get_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_post);

    char *post_data = "{\"key\": \"value\"}"; // Example JSON payload
    esp_http_client_set_post_field(client, post_data, strlen(post_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP POST Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP POST request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

httpd_handle_t start_webserver(void)
{
    ESP_LOGI(TAG, "Starting server");

    httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();
    httpd_handle_t server = NULL;

    config.cacert_pem = (const uint8_t *)_binary_servercert_pem_start;
    config.cacert_len = _binary_servercert_pem_end - _binary_servercert_pem_start;
    config.prvtkey_pem = (const uint8_t *)_binary_serverkey_pem_start;
    config.prvtkey_len = _binary_serverkey_pem_end - _binary_serverkey_pem_start;

    // Logging the lengths of the certificates
    ESP_LOGI(TAG, "Server Cert Length: %d", config.cacert_len);
    ESP_LOGI(TAG, "Server Key Length: %d", config.prvtkey_len);

    esp_err_t ret = httpd_ssl_start(&server, &config);
    if (ESP_OK != ret)
    {
        ESP_LOGI(TAG, "Error starting server!");
        return NULL;
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &server_uri_get);
    httpd_register_uri_handler(server, &server_uri_post);
    return server;
}