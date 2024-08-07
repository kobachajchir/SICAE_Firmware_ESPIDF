#ifndef HTTPMETHODS_H
#define HTTPMETHODS_H

#ifdef __cplusplus
extern "C" {
#endif
    void perform_http_get(const char *url);
    void perform_http_post(const char *url, const char *json_data);
    void perform_http_patch(const char *url, const char *json_data);
    void perform_http_put(const char *url, const char *json_data);
#ifdef __cplusplus
}
#endif
#endif