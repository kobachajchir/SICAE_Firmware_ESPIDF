#include <globals.h>
#include "esp_log.h"
#include "wifi_utils.h"
#include "lcd_utils.h" // Include the header for LCD functions

static const char *TAG = "wifi_utils";
EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

bool is_ap_mode() {
    wifi_mode_t mode;
    esp_err_t result = esp_wifi_get_mode(&mode);

    if (result == ESP_OK) {
        // Check if the Wi-Fi mode is AP
        if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA) {
            return true; // Device is in AP mode
        } else if (mode == WIFI_MODE_STA) {
            return false; // Device is connected to a Wi-Fi network (Station mode)
        }
    } else {
        ESP_LOGE("WIFI", "Failed to get Wi-Fi mode: %s", esp_err_to_name(result));
    }

    // Default return false if unable to determine mode
    return false;
}

void get_wifi_signal_strength() {
    wifi_ap_record_t ap_info;

    // Get the current AP info
    esp_err_t result = esp_wifi_sta_get_ap_info(&ap_info);

    if (result == ESP_OK) {
        // Print the RSSI (signal strength)
        sprintf(rssi_str, "%d", ap_info.rssi);
        ESP_LOGI("WIFI", "RSSI: %s dBm", rssi_str);
    } else {
        ESP_LOGE("WIFI", "Failed to get Connection info: %s", esp_err_to_name(result));
    }
}

void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < CONFIG_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        system_ip = event->ip_info;
        printCurrentIP("WIFI CONECTADO");
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *event = (wifi_event_ap_staconnected_t *) event_data;
        ESP_LOGI(TAG, "station " MACSTR " joined, AID=%d", MAC2STR(event->mac), event->aid);
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *event = (wifi_event_ap_stadisconnected_t *) event_data;
        ESP_LOGI(TAG, "station " MACSTR " left, AID=%d, reason:%d", MAC2STR(event->mac), event->aid, event->reason);
    }
}
void wifi_init_softap(void) {
    esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = CONFIG_SOFTAP_SSID,
            .ssid_len = strlen(CONFIG_SOFTAP_SSID),
            .password = CONFIG_SOFTAP_PASSWORD,
            .channel = CONFIG_SOFTAP_CHANNEL,
            .max_connection = CONFIG_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA2_PSK
        },
    };

    if (strlen(CONFIG_SOFTAP_PASSWORD) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_softap finished. SSID:%s password:%s",
             CONFIG_SOFTAP_SSID, CONFIG_SOFTAP_PASSWORD);
    esp_netif_ip_info_t ip_info;
    esp_netif_get_ip_info(esp_netif_create_default_wifi_ap(), &ip_info);
    printCurrentIP("AP CREADO");

}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "",
            .password = "",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .pmf_cfg = {
                .capable = true,
                .required = false
            }
        }
    };

    // Copia las credenciales Wi-Fi en las estructuras correspondientes
    strncpy((char *)wifi_config.sta.ssid, wifiSsid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, wifiPassword, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {

    } else if (bits & WIFI_FAIL_BIT) {
        // Start SoftAP
        ESP_LOGI(TAG, "Starting SoftAP");
        lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
        lcd_send_string("CREANDO AP");
        wifi_init_softap();
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void printCurrentIP() {
    wifi_mode_t mode;
    
    esp_wifi_get_mode(&mode);
    lcd_clear();

    if (mode == WIFI_MODE_STA) {
            lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
            lcd_send_string("WIFI CONECTADO");
            char linea[16];
            snprintf(linea, sizeof(linea), IPSTR, IP2STR(&system_ip.ip));
            lcd_put_cur(1, 0); // Move cursor to the beginning of the second line
            lcd_send_string(linea); // Display the IP address
    } else if (mode == WIFI_MODE_AP) {
            lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
            lcd_send_string("AP CREADO");
            char linea[16];
            snprintf(linea, sizeof(linea), IPSTR, IP2STR(&system_ip.ip));
            lcd_put_cur(1, 0); // Move cursor to the beginning of the second line
            lcd_send_string(linea); // Display the IP address
    } else {
        ESP_LOGE("printCurrentIP", "Unknown WiFi mode");
    }
}