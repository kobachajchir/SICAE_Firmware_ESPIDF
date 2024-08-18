#include <sys/param.h>
#include "esp_http_client.h"
#include <esp_tls.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include "esp_log.h"
#include "globals.h"
#include "json/cJSON/cJSON.h"
#include "json/cJSON/cJSON_Utils.h"
#include "lcd_utils.h" 
#include "globals.h"

#define FILE_PATH_MAX (256)
#define FULL_PATH_MAX (FILE_PATH_MAX + 8) // Adding 8 to accommodate "/spiffs/"

static const char *TAG = "webserver";

esp_err_t server_post_handler(httpd_req_t *req) {
    char content[100];
    size_t recv_size = MIN(req->content_len, sizeof(content));
    int ret = httpd_req_recv(req, content, recv_size);

    printf("\nServer POST content: %s\n", content);

    if (ret <= 0) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
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

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt) {
    switch (evt->event_id) {
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
            ESP_LOGI(TAG, "HTTP GET Response (on data): %s", response_data);
        }
        esp_err_t url_err = esp_http_client_get_url(evt->client, url, sizeof(url));
        if (url_err == ESP_OK) {
            ESP_LOGI(TAG, "Request URL: %s", url);
        } else {
            ESP_LOGE(TAG, "Failed to get request URL: %s", esp_err_to_name(url_err));
        }
        break;

    case HTTP_EVENT_HEADER_SENT:
        ESP_LOGI(TAG, "HTTP_EVENT_HEADER_SENT");
        break;

    case HTTP_EVENT_ON_HEADER:
        ESP_LOGI(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key, evt->header_value);
        break;

    case HTTP_EVENT_ON_FINISH:
        if (response_data) {
            ESP_LOGI(TAG, "HTTP GET Response: %s", response_data);
            // Parse JSON response using cJSON
            cJSON *json = cJSON_Parse(response_data);
            if (json) {
                cJSON *status = cJSON_GetObjectItem(json, "status");
                if (cJSON_IsBool(status) && cJSON_IsTrue(status)) {
                    lcd_put_cur(0, 0); 
                    lcd_send_string("DATOS NUEVOS");
                    lcd_put_cur(1, 0); 
                    lcd_send_string("DETECTADOS");
                    
                    // Retrieve the "section" field from the JSON response
                    cJSON *section = cJSON_GetObjectItem(json, "section");
                    if (cJSON_IsString(section)) {
                        const char *section_value = section->valuestring;

                        // Store section_value in urlSection for later use
                        strncpy(urlSection, section_value, sizeof(urlSection) - 1);
                        urlSection[sizeof(urlSection) - 1] = '\0'; // Ensure null termination

                        // Determine if the URL is for setting or getting data
                        if (strncmp(section_value, "set/", 4) == 0) {
                            // This is a "set" request
                            ESP_LOGI(TAG, "Set request for section: %s", section_value);
                            // Handle specific set operations (e.g., setting devices, info, time, etc.)
                            if (strncmp(section_value + 4, "devices/", 8) == 0) {
                                SETNEWDEVICEDATA = 1;
                                ESP_LOGI(TAG, "Setting device data"); // Log for setting device data
                            } else if (strcmp(section_value + 4, "info") == 0) {
                                SETNEWINFODATA = 1;
                                ESP_LOGI(TAG, "Setting info data"); // Log for setting info data
                            } else if (strncmp(section_value + 4, "time/", 5) == 0) {
                                SETNEWDATETIME = 1;
                                ESP_LOGI(TAG, "Setting time"); // Log for setting time
                            } else {
                                ESP_LOGE(TAG, "Unknown set request: %s", section_value);
                            }
                        } else if (strncmp(section_value, "get/", 4) == 0) {
                            // This is a "get" request
                            ESP_LOGI(TAG, "Get request for section: %s", section_value);
                            // Handle specific get operations (e.g., getting devices, info, etc.)
                            if (strcmp(section_value + 4, "info") == 0) {
                                FETCHNEWINFODATA = 1;
                                ESP_LOGI(TAG, "Fetching info data"); // Log for fetching info data
                            } else if (strcmp(section_value + 4, "devices") == 0) {
                                FETCHNEWDEVICESDATA = 1;
                                ESP_LOGI(TAG, "Fetching devices data"); // Log for fetching devices data
                            } else if (strcmp(section_value + 4, "firmware") == 0) {
                                FETCHNEWFIRMWAREDATA = 1;
                                ESP_LOGI(TAG, "Fetching firmware data"); // Log for fetching firmware data
                            } else if (strcmp(section_value + 4, "datetime") == 0) {
                                FETCHNEWDATETIMEDATA = 1;
                                ESP_LOGI(TAG, "Fetching datetime"); // Log for fetching datetime
                            } else if (strcmp(section_value + 4, "alive") == 0) {
                                FETCHNEWALIVE = 1;
                                ESP_LOGI(TAG, "Fetching alive status"); // Log for fetching alive status
                            } else {
                                ESP_LOGE(TAG, "Unknown get request: %s", section_value);
                            }
                        } else {
                            ESP_LOGE(TAG, "Unknown request type in section: %s", section_value);
                        }

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
            if (FETCHNEWINFODATA || FETCHNEWDEVICESDATA || FETCHNEWFIRMWAREDATA || FETCHNEWDATETIMEDATA || SETNEWINFODATA || SETNEWDEVICEDATA || SETNEWDATETIME) {
                POSTNONEWDATA = 1;
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

static esp_err_t file_get_handler(httpd_req_t *req) {
    char filepath[FULL_PATH_MAX];
    char file[FILE_PATH_MAX];

    ESP_LOGI(TAG, "Request URI: %s", req->uri);

    // Get the query string from the URI
    char query[128];
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        ESP_LOGI(TAG, "Found URL query: %s", query);

        // Get the 'file' parameter from the query string
        if (httpd_query_key_value(query, "file", file, sizeof(file)) == ESP_OK) {
            ESP_LOGI(TAG, "Found URL query parameter => file=%s", file);

            // Ensure the filepath does not exceed the buffer size
            if (strlen(file) + strlen("/spiffs/") + 1 > sizeof(filepath)) {
                ESP_LOGE(TAG, "File path is too long");
                httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "File path too long");
                return ESP_FAIL;
            }
            lcd_clear();
            lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
            lcd_send_string("ACCESO WEB");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            strlcpy(linea, "FILE: ", sizeof(linea));
            strlcat(linea, file, sizeof(linea));
            lcd_send_string(linea);
            strlcpy(filepath, "/spiffs/", sizeof(filepath));
            strlcat(filepath, file, sizeof(filepath));
        } else {
            ESP_LOGE(TAG, "File query parameter not found");
            httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "File query parameter not found");
            return ESP_FAIL;
        }
    } else {
        ESP_LOGE(TAG, "No query found");
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "No query found");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Requested file path: %s", filepath);

    FILE *file_ptr = fopen(filepath, "r");
    if (!file_ptr) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    char *chunk = malloc(1024);
    size_t chunksize;

    // Set the MIME type based on the file extension
    const char *ext = strrchr(filepath, '.');
    if (ext) {
        if (strcmp(ext, ".html") == 0) {
            httpd_resp_set_type(req, "text/html");
        } else if (strcmp(ext, ".js") == 0) {
            httpd_resp_set_type(req, "application/javascript");
        } else if (strcmp(ext, ".css") == 0) {
            httpd_resp_set_type(req, "text/css");
        } else if (strcmp(ext, ".png") == 0) {
            httpd_resp_set_type(req, "image/png");
        } else if (strcmp(ext, ".jpg") == 0) {
            httpd_resp_set_type(req, "image/jpeg");
        } else if (strcmp(ext, ".ico") == 0) {
            httpd_resp_set_type(req, "image/x-icon");
        } else {
            httpd_resp_set_type(req, "text/plain");
        }
    } else {
        httpd_resp_set_type(req, "text/plain");
    }

    do {
        chunksize = fread(chunk, 1, 1024, file_ptr);
        if (chunksize > 0) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
    } while (chunksize != 0);

    fclose(file_ptr);
    free(chunk);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t api_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "API Request URI: %s", req->uri);
    lcd_clear();
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string("ACCESO WEB");
    lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
    lcd_send_string("API: CONN INFO");
    // Handle specific API endpoints
    if (strcmp(req->uri, "/connectionInfo") == 0) {
        const char *resp_str = "{\"status\": \"connected\"}";
        httpd_resp_set_type(req, "application/json");
        httpd_resp_send(req, resp_str, strlen(resp_str));
        return ESP_OK;
    }
    
    // Default response for unknown API endpoints
    httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "API endpoint not found");
    return ESP_FAIL;
}

static esp_err_t index_get_handler(httpd_req_t *req) {
    const char *filepath = "/spiffs/index.html";
    lcd_clear();
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string("ACCESO WEB");
    lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
    lcd_send_string("CARGANDO UI");
    ESP_LOGI(TAG, "Requested file path: %s", filepath);

    FILE *file = fopen(filepath, "r");
    if (!file) {
        ESP_LOGE(TAG, "Failed to open file : %s", filepath);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, "File not found");
        return ESP_FAIL;
    }

    char *chunk = malloc(1024);
    size_t chunksize;

    httpd_resp_set_type(req, "text/html");

    do {
        chunksize = fread(chunk, 1, 1024, file);
        if (chunksize > 0) {
            httpd_resp_send_chunk(req, chunk, chunksize);
        }
    } while (chunksize != 0);

    fclose(file);
    free(chunk);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

const httpd_uri_t file_query = {
    .uri = "/file",
    .method = HTTP_GET,
    .handler = file_get_handler,
    .user_ctx = NULL
};

const httpd_uri_t api_get = {
    .uri = "/*",
    .method = HTTP_GET,
    .handler = api_get_handler,
    .user_ctx = NULL
};

const httpd_uri_t index_get = {
    .uri = "/",
    .method = HTTP_GET,
    .handler = index_get_handler,
    .user_ctx = NULL
};

const httpd_uri_t server_uri_post = {
    .uri = "/",
    .method = HTTP_POST,
    .handler = server_post_handler
};

void client_post_rest_function(void)
{
    esp_http_client_config_t config_post = {
        .url = "https://sicaewebapp-default-rtdb.firebaseio.com/",
        .method = HTTP_METHOD_POST,
        .cert_pem = (const char *)clientcert_pem_start,
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

    httpd_handle_t server = NULL;

    if (use_ssl) {
        httpd_ssl_config_t config = HTTPD_SSL_CONFIG_DEFAULT();

        config.servercert = (const uint8_t *)servercert_pem_start;
        config.servercert_len = servercert_pem_end - servercert_pem_start;
        config.prvtkey_pem = (const uint8_t *)serverkey_pem_start;
        config.prvtkey_len = serverkey_pem_end - serverkey_pem_start;

        ESP_LOGI(TAG, "Server Cert Length: %d", config.servercert_len);
        ESP_LOGI(TAG, "Server Key Length: %d", config.prvtkey_len);

        esp_err_t ret = httpd_ssl_start(&server, &config);
        if (ESP_OK != ret)
        {
            ESP_LOGI(TAG, "Error starting server!");
            return NULL;
        }
    } else {
        httpd_config_t config = HTTPD_DEFAULT_CONFIG();
        config.uri_match_fn = httpd_uri_match_wildcard;
        esp_err_t ret = httpd_start(&server, &config);
        if (ESP_OK != ret)
        {
            ESP_LOGI(TAG, "Error starting server!");
            return NULL;
        }
    }

    ESP_LOGI(TAG, "Registering URI handlers");
    httpd_register_uri_handler(server, &index_get);
    httpd_register_uri_handler(server, &file_query);
    httpd_register_uri_handler(server, &api_get);
    httpd_register_uri_handler(server, &server_uri_post);
    return server;
}
