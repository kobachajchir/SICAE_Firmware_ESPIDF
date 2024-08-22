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
#include "menuTypes.h"
#include "deviceType.h"

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
int deviceSelection = 0;

MenuItem *submenu1Items = NULL;

// Define submenu 2 items
MenuItem submenu2Items[] = {
    {"Option 1", nullptr},
    {"Option 2", nullptr},
    {"VOLVER", volver}};

// Define submenu 3 items
MenuItem submenu3Items[] = {
    {"Option 1", nullptr},
    {"Option 2", nullptr},
    {"VOLVER", volver}};

// Define the submenus and main menu
SubMenu submenu1 = {"Dispositivos", nullptr, 0, NULL}; // Initialize without items initially
SubMenu submenu2 = {"SubMenu 2", submenu2Items, 3, &mainMenu};
SubMenu submenu3 = {"SubMenu 3", submenu3Items, 3, &mainMenu};

// Define main menu items
MenuItem mainMenuItems[] = {
    {"Dispositivos", submenu1Fn},
    {"SubMenu 2", submenu2Fn},
    {"SubMenu 3", submenu3Fn},
    {"VOLVER", volver}};

// Define the main menu
SubMenu mainMenu = {"Main Menu", mainMenuItems, 4, NULL};

// Initialize the menu system
MenuSystem menuSystem = {&mainMenu};

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
    void navigate_menu();
    void deviceAction();
    void free_devices_array();
    void populate_device_menu();
    void firebase_update_task(void *param);
}

static const char *TAG = "main";
static int s_retry_num = 0;
gpio_num_t device_pins[4] = {GPIO_NUM_23, GPIO_NUM_NC, GPIO_NUM_NC, GPIO_NUM_NC};
float device_currents[4] = {0.0, 0.0, 0.0, 0.0};

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

Device *devices = NULL;
int deviceCount = 0;
int currentDeviceIndex = 0;

void free_devices_array()
{
    if (devices != NULL)
    {
        for (int i = 0; i < deviceCount; i++)
        {
            free((void *)devices[i].name);
            free((void *)devices[i].type);
        }
        free(devices);
        devices = NULL;
        deviceCount = 0;
    }
}

void populate_device_menu()
{
    ESP_LOGI("Menu", "Starting to populate device menu...");

    int maxItems = deviceCount + 1; // Add one for the "VOLVER" item

    ESP_LOGI("Menu", "Device count: %d", deviceCount);
    ESP_LOGI("Menu", "Max items (including VOLVER): %d", maxItems);

    // Free previously allocated memory for submenu1Items if needed
    if (submenu1Items != NULL)
    {
        ESP_LOGI("Menu", "Freeing previously allocated memory for submenu1Items");
        free(submenu1Items);
    }

    // Allocate memory for submenu1Items based on device count
    submenu1Items = (MenuItem *)malloc(sizeof(MenuItem) * maxItems);
    if (submenu1Items == NULL)
    {
        ESP_LOGE("Menu", "Failed to allocate memory for submenu1Items");
        return;
    }

    ESP_LOGI("Menu", "Allocated memory for %d menu items", maxItems);

    // Populate the submenu with devices
    for (int i = 0; i < deviceCount; i++)
    {
        submenu1Items[i].name = devices[i].name;
        submenu1Items[i].action = deviceAction; // Set the action to a generic function
    }

    // Add the "VOLVER" option
    submenu1Items[deviceCount].name = "VOLVER";
    submenu1Items[deviceCount].action = (MenuFunction)volver;

    // Update the submenu structure
    submenu1.items = submenu1Items;
    submenu1.itemCount = maxItems;

    ESP_LOGI("Menu", "Updated submenu1 with %d items", maxItems);

    // Optionally, refresh the menu display if needed
    if (menuSystem.currentMenu == &submenu1)
    {
        ESP_LOGI("Menu", "Displaying submenu1");
        displayMenu(&menuSystem, 0);
    }

    ESP_LOGI("Menu", "Finished populating device menu");
}

void firebase_update_task(void *param)
{
    // Perform the Firebase update here
    Device *selectedDevice = &devices[currentDeviceIndex]; // Get a pointer to the selected device
    firebase_update_dispositivo_device_status(currentDeviceIndex, !selectedDevice->state);
    vTaskDelete(NULL); // Delete the task when done
}

void deviceAction()
{
    // Use the global variable currentDeviceIndex to get the selected device
    Device *selectedDevice = &devices[currentDeviceIndex]; // Get a pointer to the selected device

    ESP_LOGI(TAG, "Selected device: %s", selectedDevice->name);
    ESP_LOGI(TAG, "Device Type: %s", selectedDevice->type);
    ESP_LOGI(TAG, "Device Pin: %d", selectedDevice->pin);
    ESP_LOGI(TAG, "Device State: %s", selectedDevice->state ? "ON" : "OFF");

    // Add your logic to control the device based on its type, pin, and state
    if (strcmp(selectedDevice->type, "relay") == 0)
    {
        // Toggle the relay state
        selectedDevice->state = !selectedDevice->state;
        gpio_set_level(selectedDevice->pin, selectedDevice->state ? 1 : 0);
        xTaskCreate(firebase_update_task, "Firebase Update Task", 4096, NULL, 5, NULL);
    }
    else if (strcmp(selectedDevice->type, "ir") == 0)
    {
        // Handle IR device action here
        // Example: send IR command based on state
    }

    ESP_LOGI(TAG, "Updated device state: %s", selectedDevice->state ? "ON" : "OFF");
}

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
        if (BTN_UP_RELEASED || BTN_DOWN_RELEASED || BTN_ENTER_RELEASED)
        {
            navigate_menu();
        }
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
            free_devices_array();
            ESP_LOGI(TAG, "New Devices data"); // No new data enters here
            lcd_put_cur(0, 0);                 // Move cursor to the beginning of the first line
            lcd_send_string("ACTUALIZANDO");
            lcd_put_cur(1, 0); // Move cursor to the beginning of the first line
            lcd_send_string("OBTENIENDO DISP");
            vTaskSuspend(dataFetchHandler);
            // vTaskSuspend(aliveHandler);
            firebase_get_dispositivo_devices();
        }
        if (SETCONSUMEDCURRENT)
        {
            SETCONSUMEDCURRENT = 0;
            vTaskSuspend(aCSReadHandler);
            firebase_update_device_current(currentDeviceIndex, device_currents[currentDeviceIndex]);
            vTaskResume(aCSReadHandler);
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
            /*vTaskResume(aCSReadHandler);
            vTaskResume(aliveHandler);*/
            if (POPULATEDEVICES)
            {
                POPULATEDEVICES = 0;
                populate_device_menu();
            }
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
        for (int i = 0; i < deviceCount; i++) // Iterate over all devices
        {
            if (strcmp(devices[i].type, "relay") != 0)
            {
                continue; // Skip non-relay devices
            }

            gpio_num_t pin = devices[i].pin;
            int gpio_level = get_gpio_output_gpio_level(pin);
            ESP_LOGI(TAG, "GPIO level for device %d: %d", i, gpio_level);

            if (gpio_level != 0)
            {                                    // Check if the relay is off
                vTaskDelay(pdMS_TO_TICKS(1000)); // Delay for 1000 ms before checking again
                continue;
            }

            float total_current = 0;
            for (int j = 0; j < 1000; j++)
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
            device_currents[i] = total_current / 1000;
            ESP_LOGI(TAG, "Average measured current for device %d: %.2f A", i, device_currents[i]);

            currentDeviceIndex = i;
            // Set the flag to update the current consumed
            SETCONSUMEDCURRENT = 1;
        }

        // Delay for 1000 ms before starting the next set of readings
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void navigate_menu()
{
    static int currentMenuIndex = 0;

    // Check if the UP button is pressed
    if (BTN_UP_RELEASED)
    {
        BTN_UP_RELEASED = 0;

        if (--currentMenuIndex < 0)
        {
            currentMenuIndex = menuSystem.currentMenu->itemCount - 1;
        }

        displayMenu(&menuSystem, currentMenuIndex);
    }

    // Check if the DOWN button is pressed
    if (BTN_DOWN_RELEASED)
    {
        BTN_DOWN_RELEASED = 0;

        if (++currentMenuIndex >= menuSystem.currentMenu->itemCount)
        {
            currentMenuIndex = 0;
        }

        displayMenu(&menuSystem, currentMenuIndex);
    }

    // Check if the ENTER button is pressed
    if (BTN_ENTER_RELEASED)
    {
        BTN_ENTER_RELEASED = 0;

        MenuItem *selectedItem = &menuSystem.currentMenu->items[currentMenuIndex];

        if (strcmp(selectedItem->name, "VOLVER") == 0)
        {
            volver(); // Navigate back to the parent menu
        }
        else if (selectedItem->action != NULL)
        {
            // Execute the action associated with the menu item
            selectedItem->action();
        }
        else if (selectedItem->submenu != NULL)
        {
            // Navigate into the submenu
            menuSystem.currentMenu = selectedItem->submenu;
            currentMenuIndex = 0; // Reset index for the new menu
            displayMenu(&menuSystem, currentMenuIndex);
        }
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
        submenu1.parent = &mainMenu;
        submenu2.parent = &mainMenu;
        submenu3.parent = &mainMenu;
        // Initialize the menu system
        initMenuSystem(&menuSystem);
        firebase_get_dispositivo_devices();
        xTaskCreate(button_task, "button_task", 2048, NULL, 10, &buttonTaskHandler);
        ESP_LOGI(TAG, "Buttons tasks created");
        printCurrentIP();
        xTaskCreate(read_acs712_task, "read_acs712_task", 2048, NULL, 5, &aCSReadHandler);
    }
}
