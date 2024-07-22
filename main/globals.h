#ifndef GLOBALS_H
#define GLOBALS_H

#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"

/* WiFi Configuration */
#define CONFIG_ESP_WIFI_SSID "Koba"
#define CONFIG_ESP_WIFI_PASSWORD "koba1254"
#define CONFIG_ESP_MAXIMUM_RETRY 5

#define CONFIG_SOFTAP_SSID "ESP32AP"
#define CONFIG_SOFTAP_PASSWORD "ap123456"
#define CONFIG_SOFTAP_CHANNEL 1
#define CONFIG_MAX_STA_CONN 4

/* FreeRTOS event group to signal when we are connected */
extern EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

#endif // GLOBALS_H
