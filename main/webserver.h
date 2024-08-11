#ifndef WEBSERVER_H
#define WEBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif
    httpd_handle_t start_webserver(void);
    esp_err_t server_get_handler(httpd_req_t *req);
    esp_err_t server_post_handler(httpd_req_t *req);
    esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt);
    void client_post_rest_function(void);
#ifdef __cplusplus
}
#endif
#endif
