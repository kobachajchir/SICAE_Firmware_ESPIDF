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

// Manually define the missing type
typedef esp_err_t (*esp_tls_handshake_callback_t)(esp_tls_t *tls, void *arg);

static void firebase_get_dispositivos(void);
static void get_current_time(char *buffer, size_t max_len);

static const char *TAG = "main";
static int s_retry_num = 0;

myByte btnFlag = {0};
myByte btnFlag2 = {0};

uint32_t lastTime = 0;
uint32_t btnEnterDuration = 0;
uint32_t btnUpDuration = 0;
uint32_t btnDownDuration = 0;

char linea[16];

void init_gpio() {
    gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << PIN_BTN_UP) | (1ULL << PIN_BTN_DOWN) | (1ULL << PIN_BTN_ENTER);
    io_conf.pull_down_en = 0; // Disable internal pull-down resistors
    io_conf.pull_up_en = 0;   // Disable internal pull-up resistors
    gpio_config(&io_conf);
}

static void get_current_time(char *buffer, size_t max_len)
{
    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, max_len, "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
}

static void firebase_get_dispositivos(void)
{
    // Obtener la hora actual
    char time_str[64];
    get_current_time(time_str, sizeof(time_str));

    // Construir la cadena JSON
    char json_data[256];
    snprintf(json_data, sizeof(json_data), 
             "{\"auth\": null, \"resource\": {\"key\": \"value\"}, \"path\": \"/dispositivos\", \"method\": \"get\", \"time\": \"%s\"}", 
             time_str);

    // Crear la URL completa
    char url[512];
    snprintf(url, sizeof(url), "https://sicaewebapp-default-rtdb.firebaseio.com/dispositivos.json");

    // Configurar la solicitud HTTP
    esp_http_client_config_t config_get = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .cert_pem = (const char *)_binary_clientcert_pem_start,
        .event_handler = client_event_get_handler
    };

    esp_http_client_handle_t client = esp_http_client_init(&config_get);

    // Añadir la cadena JSON a la solicitud
    esp_http_client_set_post_field(client, json_data, strlen(json_data));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    // Ejecutar la solicitud HTTP
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI(TAG, "HTTP GET Status = %d, content_length = %d",
                 esp_http_client_get_status_code(client),
                 (int)esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    }

    esp_http_client_cleanup(client);
}

void button_task(void *arg) {
    while (1) {
        check_buttons();
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust delay for main loop to 100 ms
    }
}


static esp_err_t i2c_master_init(void) {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode,
                              I2C_MASTER_RX_BUF_DISABLE,
                              I2C_MASTER_TX_BUF_DISABLE, 0);
}

void app_main(void){
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    INITIALIZING = 1;
    ESP_LOGI(TAG, "Inicializando");
    ESP_LOGI(TAG, "Initializing GPIO...");
    init_gpio();
    ESP_LOGI(TAG, "GPIO Initialized with external pull-down resistors");

    ESP_ERROR_CHECK(ret);
    ESP_ERROR_CHECK(i2c_master_init());
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
        client_post_rest_function();
        ESP_LOGI(TAG, "Fetching dispositivos data from Firebase...");
        firebase_get_dispositivos();
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
}
