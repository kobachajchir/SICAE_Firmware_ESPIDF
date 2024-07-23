#include <sys/param.h>
#include "esp_http_client.h"
#include <esp_tls.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include "esp_log.h"
#include "globals.h"

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
        printf("Client HTTP_EVENT_ON_DATA: %.*s\n", evt->data_len, (char *)evt->data);
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