#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <sys/param.h>
#include <string.h>
#include "nvs_flash.h" 
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "netif/dhcp_state.h"
#include "esp_http_client.h"
#include <esp_tls.h>
#include <esp_http_server.h>
#include <esp_https_server.h>
#include <unistd.h>
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "lcd_utils.h" 
#include "wifi_utils.h" 
#include "webserver.h" 
#include "globals.h"
#include "buttons.h"
#include "value.h"
#include "json.h"
#include "app.h"
#include "rtdb.h"
#include "firebase_config.h"
#include "firebaseFns.h"

using namespace ESPFirebase;

// Manually define the missing type
typedef esp_err_t (*esp_tls_handshake_callback_t)(esp_tls_t *tls, void *arg);

extern "C" {
    void app_main();
    static void firebase_get_dispositivos();
    static esp_err_t i2c_master_init();
    void get_current_time(char *buffer, size_t max_len);
    void button_task(void *arg);
}

static const char *TAG = "main";
static int s_retry_num = 0;

myByte btnFlag = {0};
myByte btnFlag2 = {0};

uint32_t lastTime = 0;
uint32_t btnEnterDuration = 0;
uint32_t btnUpDuration = 0;
uint32_t btnDownDuration = 0;

char linea[16];
char device_id[18];
uint8_t mac[6];
char url[256];
char urlSection[50];
char *response_data = NULL;
int response_data_len = 0;

extern "C" void init_gpio() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BTN_UP) | (1ULL << PIN_BTN_DOWN) | (1ULL << PIN_BTN_ENTER);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE; // Disable internal pull-down resistors
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;   // Disable internal pull-up resistors
    gpio_config(&io_conf);
}

extern "C" void get_current_time(char *buffer, size_t max_len)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, max_len, "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
}

void button_task(void *arg) {
    while (1) {
        check_buttons();
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust delay for main loop to 100 ms
    }
}


static esp_err_t i2c_master_init() {
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

extern "C" void app_main(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    INITIALIZING = 1;
    esp_efuse_mac_get_default(mac);
    snprintf(device_id, sizeof(device_id), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Device ID %s", device_id);
    ESP_LOGI(TAG, "Inicializando");
    ESP_LOGI(TAG, "Initializing GPIO...");
    init_gpio();
    ESP_LOGI(TAG, "GPIO Initialized with external pull-down resistors");

    ret = i2c_master_init();
    ESP_ERROR_CHECK(ret);
    ESP_LOGI(TAG, "I2C initialized successfully");

    lcd_init();
    ESP_LOGI(TAG, "LCD initialized successfully");
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string("INICIANDO");
    wifi_init_sta();

    // Wait for WiFi to connect
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "WIFI connected, starting server...");
        start_webserver();
        
        ESP_LOGI(TAG, "Starting client...");
        firebase_get_new_data();
        if(INITIALIZING){
            // Config and Authentication
            user_account_t account = {USER_EMAIL, USER_PASSWORD};
            FirebaseApp app = FirebaseApp(API_KEY);
            app.loginUserAccount(account);
            RTDB db = RTDB(&app, DATABASE_URL);
        }
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 CONFIG_ESP_WIFI_SSID, CONFIG_ESP_WIFI_PASSWORD);
        // Start SoftAP
        ESP_LOGI(TAG, "Starting SoftAP");
        lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
        lcd_send_string("CREANDO AP");
        wifi_init_softap();
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    if(INITIALIZING){
        INITIALIZING = 0;
        xTaskCreate(button_task, "button_task", 2048, NULL, 10, NULL);
        ESP_LOGI(TAG, "Buttons tasks created");
    }
    if(FETCHNEWINFODATA){
        FETCHNEWINFODATA = 0;
        firebase_get_dispositivo_info();
    }
    if(FETCHNEWDEVICESDATA){
        FETCHNEWDEVICESDATA = 0;
        firebase_get_dispositivo_device();
    }
}
