#include <iostream>
#include <Arduino.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include <stdio.h>
#include <stdint.h>
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
#include "spiffs.h"
#include "nvs_utils.h"
#include "devicesRelay_utils.h"
#include "ntp.h"
#include "IR_utils.h"
#include "driver/adc.h"

using namespace ESPFirebase;

TaskHandle_t buttonTaskHandler;
TaskHandle_t dataFetchHandler;
TaskHandle_t dataProcessHandler;
TaskHandle_t aliveHandler;
TaskHandle_t aCSReadHandler;
char wifiSsid[WIFI_SSID_MAX_LEN] = {0};
char wifiPassword[WIFI_PASS_MAX_LEN] = {0};
char url[URL_MAX_LEN] = {0};
char disp_url[SERVER_URL_MAX_LEN] = {0};
char events_url[SERVER_URL_MAX_LEN] = {0};
time_t now = 0;
struct tm timeinfo = {0};
float zeroOffset = 0.0;

// Manually define the missing type
typedef esp_err_t (*esp_tls_handshake_callback_t)(esp_tls_t *tls, void *arg);

extern "C"
{
    void app_main();
    static void firebase_get_dispositivos();
    static esp_err_t i2c_master_init();
    void get_current_time(char *buffer, size_t max_len);
    void button_task(void *arg);
    void data_processing_task(void *pvParameters);
    void check_new_data_task(void *pvParameter);
    void alive_package_task(void *pvParameters);
    void init_gpio();
    void configureSingleADCChannel(adc1_channel_t adc_channel);
    void calibrate_acs712();
    void read_acs712_task(void *param);
}

static const char *TAG = "main";
static int s_retry_num = 0;
gpio_num_t device_pins[8] = {GPIO_NUM_23, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};

myByte btnFlag = {0};
myByte btnFlag2 = {0};
myByte btnFlag3 = {0};

uint32_t lastTime = 0;
uint32_t btnEnterDuration = 0;
uint32_t btnUpDuration = 0;
uint32_t btnDownDuration = 0;

char linea[16];
char device_id[18];
uint8_t mac[6];
char urlSection[80];
char *response_data = NULL;
int response_data_len = 0;
bool use_ssl = false;
esp_netif_ip_info_t system_ip;
char firmwareVersion[16] = {0};
char rssi_str[10];

void init_gpio()
{
    gpio_config_t io_conf;

    // Configurar los pines como entradas
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;

    // Pines de entrada
    io_conf.pin_bit_mask = (1ULL << PIN_BTN_UP) | (1ULL << PIN_BTN_DOWN) | (1ULL << PIN_BTN_ENTER) | (1ULL << GPIO_NUM_36); // Añadir GPIO36 para ADC
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;                                                                           // Desactivar resistencias internas pull-down
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;                                                                               // Desactivar resistencias internas pull-up

    // Aplicar la configuración de GPIO para entradas
    gpio_config(&io_conf);

    // Configurar el pin del relé como salida
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;

    // Solo el pin del relé como salida
    io_conf.pin_bit_mask = (1ULL << PIN_RELE);

    // Aplicar la configuración de GPIO para salidas
    gpio_config(&io_conf);
}

void configureSingleADCChannel(adc1_channel_t adc_channel)
{
    // Set ADC width to 12 bits (the default)
    adc1_config_width(ADC_WIDTH_BIT_12);

    // Configure only the specific ADC channel (ADC1_CHANNEL_0 -> GPIO36)
    esp_err_t ret = adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11); // 11dB attenuation for full 0-3.3V range

    if (ret == ESP_OK)
    {
        ESP_LOGI("ADC", "ADC channel %d configured successfully", adc_channel);
    }
    else
    {
        ESP_LOGE("ADC", "Failed to configure ADC channel %d", adc_channel);
    }
}

extern "C" void get_current_time(char *buffer, size_t max_len)
{
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);
    strftime(buffer, max_len, "%Y-%m-%dT%H:%M:%S.000Z", &timeinfo);
}

void button_task(void *arg)
{
    while (1)
    {
        check_buttons();
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust delay for main loop to 100 ms
    }
}

void check_new_data_task(void *pvParameter)
{
    while (1)
    {
        // Check if WiFi is connected
        EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdFALSE, portMAX_DELAY);
        if (bits & WIFI_CONNECTED_BIT && !(FETCHNEWINFODATA || FETCHNEWDEVICESDATA))
        {
            firebase_get_new_data();
        }
        else
        {
            // If WiFi is not connected, exit the task
            ESP_LOGE(TAG, "WiFi disconnected, exiting check_new_data_task");
            vTaskDelete(dataProcessHandler);
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static esp_err_t i2c_master_init()
{
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    conf.clk_flags = 0;
    esp_err_t err = i2c_param_config(I2C_MASTER_NUM, &conf);
    if (err != ESP_OK)
    {
        return err;
    }
    return i2c_driver_install(I2C_MASTER_NUM, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

void data_processing_task(void *pvParameters)
{
    while (1)
    {
        if (FETCHNEWINFODATA)
        {
            FETCHNEWINFODATA = 0;
            ESP_LOGI(TAG, "New Info data"); // No new data enters here
            lcd_put_cur(0, 0);              // Move cursor to the beginning of the first line
            lcd_send_string("ACTUALIZANDO");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            lcd_send_string("OBTENIENDO INFO");
            vTaskSuspend(dataFetchHandler);
            firebase_get_dispositivo_info();
        }

        if (FETCHNEWDEVICESDATA)
        {
            FETCHNEWDEVICESDATA = 0;
            ESP_LOGI(TAG, "New Devices data"); // No new data enters here
            lcd_put_cur(0, 0);                 // Move cursor to the beginning of the first line
            lcd_send_string("ACTUALIZANDO");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            lcd_send_string("OBTENIENDO DISP");
            vTaskSuspend(dataFetchHandler);
            vTaskSuspend(aliveHandler);
            firebase_get_dispositivo_devices();
        }
        if (SETNEWDEVICEDATA)
        {
            SETNEWDEVICEDATA = 0; // Reset the flag after handling

            // Example urlSection: "set/devices/0/status/true" or "set/devices/0/status/false"
            ESP_LOGI(TAG, "Processing new device data: %s", urlSection);

            // Assuming urlSection starts with "set/devices/"
            if (strncmp(urlSection, "set/devices/", 12) == 0)
            {
                char *device_str = urlSection + 12; // Get the part after "set/devices/"
                char *status_str = strstr(device_str, "/status/");

                if (status_str != NULL)
                {
                    // Null-terminate the device number string
                    *status_str = '\0';

                    // Move past "/status/"
                    status_str += 8;

                    // Convert the device number to an integer
                    int device_number = atoi(device_str);

                    // Determine the desired status (true/false)
                    bool desired_status = (strcmp(status_str, "true") == 0);

                    ESP_LOGI(TAG, "Device number: %d, Desired status: %s", device_number, desired_status ? "true" : "false");

                    // Assuming you have a mapping of device numbers to GPIO pins
                    // For example, device_number 0 corresponds to GPIO_PIN_X
                    uint8_t gpio_pin;
                    esp_err_t err = read_device_pin_from_nvs(device_number, &gpio_pin);
                    if (err == ESP_OK)
                    {
                        ESP_LOGI("NVS", "Device 0 is mapped to GPIO %d", gpio_pin);
                    }
                    else
                    {
                        ESP_LOGE("NVS", "Failed to read GPIO pin for device 0.");
                    }

                    // Toggle the GPIO pin based on the desired status
                    setPower(desired_status, gpio_pin);
                    firebase_update_dispositivo_device_status(device_number, desired_status);
                }
                else
                {
                    ESP_LOGE(TAG, "Invalid urlSection format: %s", urlSection);
                }
            }
            else
            {
                ESP_LOGE(TAG, "Invalid urlSection start: %s", urlSection);
            }
            POSTNONEWDATA = 1;
        }

        if (POSTNONEWDATA)
        {
            POSTNONEWDATA = 0;
            ESP_LOGI(TAG, "Clear new data"); // No new data enters here
            clear_new_data_section();
            vTaskResume(dataFetchHandler);
            vTaskResume(aliveHandler);
            printCurrentIP();
        }

        // Sleep for a short time to yield control to other tasks
        vTaskDelay(pdMS_TO_TICKS(100)); // Adjust the delay as needed
    }
}

void alive_package_task(void *pvParameters)
{
    while (1)
    {
        // Call the function to send the alive package
        send_alive_package();

        // Wait for 1 minute (60000 milliseconds)
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}

void calibrate_acs712()
{
    const int calibrationSamples = 100; // Number of samples to average
    const float Vref = 3.3;             // ADC reference voltage
    const int ADC_MAX = 4095;           // Maximum ADC value for a 12-bit ADC

    int adcSum = 0;

    for (int i = 0; i < calibrationSamples; i++)
    {
        int adcValue = adc1_get_raw(ADC1_CHANNEL_0); // Read ADC value
        adcSum += adcValue;

        // Small delay between samples
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Calculate the average ADC value
    int adcAverage = adcSum / calibrationSamples;

    // Convert ADC value to voltage
    zeroOffset = (adcAverage / (float)ADC_MAX) * Vref;

    ESP_LOGI(TAG, "Calibration complete. Zero offset voltage: %.2f V", zeroOffset);
}

void read_acs712_task(void *param)
{
    const float sensitivity = 0.066; // Sensitivity in V/A for the ACS712-30A model
    const float Vref = 3.3;          // ADC reference voltage
    const int ADC_MAX = 4095;        // Maximum ADC value for a 12-bit ADC

    while (true)
    {
        int gpio_level = gpio_get_level(GPIO_NUM_23);
        ESP_LOGI(TAG, "GPIO level for device 0: %d", gpio_level);

        if (gpio_level != 0)
        { // Check if the relay is off
            ESP_LOGI(TAG, "Relay is off, skipping current measurement.");
            vTaskDelay(pdMS_TO_TICKS(500)); // Delay for 500 ms before checking again
            continue;
        }

        float total_current = 0;
        for (int i = 0; i < 500; i++)
        {
            // Perform current measurement
            int adc_reading = adc1_get_raw(ADC1_CHANNEL_0);
            float voltage = ((float)adc_reading / ADC_MAX) * Vref;
            float current = (voltage - zeroOffset) / sensitivity; // Convert voltage to current
            total_current += current;

            // Short delay between readings
            vTaskDelay(pdMS_TO_TICKS(1)); // 1 ms delay between readings
        }

        // Calculate the average current
        float average_current = total_current / 500;
        ESP_LOGI(TAG, "Average measured current: %.2f A", average_current);
        total_current = 0;
        // Delay for 500 ms before starting the next set of readings
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void app_main(void)
{
    esp_err_t ret;
    nvs_init(); // init nvs
    INITIALIZING = 1;
    esp_efuse_mac_get_default(mac);
    snprintf(device_id, sizeof(device_id), "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    ESP_LOGI(TAG, "Device ID %s", device_id);
    ESP_LOGI(TAG, "Inicializando");
    initArduino();
    ESP_LOGI(TAG, "Init Arduino Framework");
    initialize_nvs_with_default_data(device_id);
    read_firmware();
    ESP_LOGI(TAG, "Firmware version: %s", firmwareVersion);
    ESP_LOGI(TAG, "Initializing GPIO...");
    init_gpio();
    // Immediately set the relay GPIO to a known state to prevent toggling
    gpio_set_level(GPIO_NUM_23, 1);
    configureSingleADCChannel(ADC1_CHANNEL_0);
    ESP_LOGI(TAG, "Calibrating ACS712 sensor...");
    calibrate_acs712(); // Call the calibration function here
    // Keep the relay off after calibration
    ESP_LOGI(TAG, "GPIO Initialized with external pull-down resistors");

    ret = i2c_master_init();
    ESP_ERROR_CHECK(ret);

    lcd_init();
    ESP_LOGI(TAG, "LCD initialized successfully");
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string("INICIANDO");
    lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
    lcd_send_string("FILESYSTEM");
    init_spiffs();
    lcd_send_string("IR RX");
    // Configura el receptor IR en el pin adecuado
    // Initialize SPIFFS
    lcd_clear_line(1);
    lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
    lcd_send_string("WIFI");
    lcd_clear();
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string("OBTENIENDO");
    lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
    lcd_send_string("CRED. WIFI");
    read_wifi_credentials(wifiSsid, sizeof(wifiSsid), wifiPassword, sizeof(wifiPassword));
    lcd_clear();
    lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
    lcd_send_string("INICIANDO");
    lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
    lcd_send_string("WIFI");
    wifi_init_sta();

    // Wait for WiFi to connect
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "WIFI connected, starting server...");
        start_webserver();

        ESP_LOGI(TAG, "Starting client...");
        if (INITIALIZING)
        {
            // Config and Authentication
            ESP_LOGI(TAG, "Obteniendo NTP");
            lcd_clear();
            lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
            lcd_send_string("OBTENIENDO");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            lcd_send_string("TIEMPO NTP");
            obtain_time();
            user_account_t account = {USER_EMAIL, USER_PASSWORD};
            FirebaseApp app = FirebaseApp(API_KEY);
            app.loginUserAccount(account);
            RTDB db = RTDB(&app, DATABASE_URL);
            lcd_clear();
            lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
            lcd_send_string("LOGUEANDO EN");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            lcd_send_string("FIREBASE");
            read_server_credentials(disp_url, sizeof(disp_url), events_url, sizeof(events_url));
            firebase_set_dispositivo_info();
            xTaskCreate(check_new_data_task, "check_new_data_task", 4096, NULL, 10, &dataFetchHandler); // Aumentamos el tamaño de la pila aquí
            xTaskCreate(alive_package_task, "alive_package_task", 4096, NULL, 5, &aliveHandler);
            xTaskCreate(data_processing_task, "data_processing_task", 4096, NULL, 5, &dataProcessHandler);
            ESP_LOGI(TAG, "Data processing tasks created");
        }
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 wifiSsid, wifiPassword);
        // Start SoftAP
        ESP_LOGI(TAG, "Starting SoftAP");
        lcd_put_cur(0, 0); // Move cursor to the beginning of the first line
        lcd_send_string("CREANDO AP");
        wifi_init_softap();
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
    if (INITIALIZING)
    {
        INITIALIZING = 0;
        firebase_read_device_status();
        xTaskCreate(button_task, "button_task", 2048, NULL, 10, &buttonTaskHandler);
        ESP_LOGI(TAG, "Buttons tasks created");
        printCurrentIP();
        xTaskCreate(read_acs712_task, "read_acs712_task", 2048, NULL, 5, &aCSReadHandler);
    }
}
