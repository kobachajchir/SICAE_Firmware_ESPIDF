#ifndef NVS_H
#define NVS_H

#include <stdio.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_mac.h"

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif
    void nvs_init();
    void read_server_credentials(char *disp_url, size_t disp_url_len, char *events_url, size_t events_url_len);
    void save_server_credentials(const char *device_id);
    void read_wifi_credentials(char *ssid, size_t ssid_len, char *password, size_t password_len);
    void save_wifi_credentials(const char *ssid, const char *password, const char *url);
    void initialize_nvs_with_default_data(char* device_id);
    void read_ap_credentials(char *ssid, size_t ssid_len, char *password, size_t password_len);
    void save_ap_credentials(const char *ssid, const char *password);
#ifdef __cplusplus
}
#endif
#endif