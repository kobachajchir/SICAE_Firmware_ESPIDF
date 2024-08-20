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
#include "myByte.h"
#include "esp_system.h"
#include "esp_chip_info.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_http_client.h"
#include "driver/gpio.h"

#define FIRMWARE "0.0.3"

/* Pin declarations */
#define PIN_BTN_ENTER 39
#define PIN_BTN_UP 35
#define PIN_BTN_DOWN 34
#define PIN_ACS 36
#define PIN_IR_RECEIVER 19
#define PIN_IR 18
#define PIN_RELE 23

/* Timer defines */
#define MINMSTIME 10            // Time in ms for entering the loop
#define BTN_PRESS_TIME 30       // Time short press 30ms
#define BTN_LONGPRESS_TIME 1500 // Time long press 1.5s.

/* Buttons flags*/
extern myByte btnFlag;
extern myByte btnFlag2;
extern myByte btnFlag3;
extern uint32_t lastTime;
extern uint32_t btnEnterDuration;
extern uint32_t btnUpDuration;
extern uint32_t btnDownDuration;

extern const uint8_t servercert_pem_start[] asm("_binary_servercert_pem_start");
extern const uint8_t servercert_pem_end[] asm("_binary_servercert_pem_end");
extern const uint8_t serverkey_pem_start[] asm("_binary_serverkey_pem_start");
extern const uint8_t serverkey_pem_end[] asm("_binary_serverkey_pem_end");
extern const uint8_t clientcert_pem_start[] asm("_binary_clientcert_pem_start");
extern const uint8_t clientcert_pem_end[] asm("_binary_clientcert_pem_end");

/* Buttons flags defines*/
#define BTN_UP_PRESSED btnFlag.bits.bit0                // Button Up pressed
#define BTN_UP_RELEASED btnFlag.bits.bit1               // Button Up released
#define BTN_DOWN_PRESSED btnFlag.bits.bit2              // Button Down pressed
#define BTN_DOWN_RELEASED btnFlag.bits.bit3             // Button Down released
#define BTN_ENTER_PRESSED btnFlag.bits.bit4             // Button OK/Back pressed
#define BTN_ENTER_RELEASED btnFlag.bits.bit5            // Button OK/Back released
#define BTN_UP_RELEASED_LONGPRESS btnFlag.bits.bit6     // Button Up released long press
#define BTN_DOWN_RELEASED_LONGPRESS btnFlag.bits.bit7   // Button Down released long press
#define BTN_ENTER_RELEASED_LONGPRESS btnFlag2.bits.bit0 // Button Enter released long press
#define INITIALIZING btnFlag2.bits.bit1
#define FETCHNEWINFODATA btnFlag2.bits.bit2
#define FETCHNEWDEVICESDATA btnFlag2.bits.bit3
#define POSTNONEWDATA btnFlag2.bits.bit4
#define PAUSENEWDATAFETCH btnFlag2.bits.bit5
#define FETCHNEWALIVE btnFlag2.bits.bit6
#define UPDATECURRENTINFO btnFlag2.bits.bit7
#define FETCHNEWFIRMWAREDATA btnFlag3.bits.bit0 // Flag for fetching new firmware data
#define FETCHNEWDATETIMEDATA btnFlag3.bits.bit1 // Flag for fetching new datetime data
#define SETNEWINFODATA btnFlag3.bits.bit2       // Flag for setting new info data
#define SETNEWDEVICEDATA btnFlag3.bits.bit3     // Flag for setting new device data
#define SETNEWDATETIME btnFlag3.bits.bit4       // Flag for setting new datetime
#define FETCHNEWDEVICEDATA btnFlag3.bits.bit5
#define SETCONSUMEDCURRENT btnFlag3.bits.bit6

/* i2c Configuration */
#define I2C_MASTER_SCL_IO GPIO_NUM_22 /*!< GPIO number for I2C master clock */
#define I2C_MASTER_SDA_IO GPIO_NUM_21 /*!< GPIO number for I2C master data  */
#define I2C_MASTER_NUM I2C_NUM_0      /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000     /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0   /*!< I2C master doesn't need buffer */
#define SLAVE_ADDRESS_LCD 0x27        /*!< I2C address of the PCF8574T chip */

#define WIFI_SSID_MAX_LEN 32
#define WIFI_PASS_MAX_LEN 64
#define URL_MAX_LEN 256
#define SERVER_URL_MAX_LEN 156

/* WiFi Configuration */
#define CONFIG_ESP_MAXIMUM_RETRY 5

#define CONFIG_SOFTAP_SSID "ESP32AP"
#define CONFIG_SOFTAP_PASSWORD "ap123456"
#define CONFIG_SOFTAP_CHANNEL 1
#define CONFIG_MAX_STA_CONN 4

#define RELAY_CHECK_INTERVAL pdMS_TO_TICKS(500) // 500 ms

extern gpio_num_t device_pins[4]; // Assuming device 0 is connected to GPIO 23
extern float device_currents[4];

/* FreeRTOS event group to signal when we are connected */
extern EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

extern char linea[16];

extern char device_id[18];
extern uint8_t mac[6];
extern char url[URL_MAX_LEN];
extern char disp_url[SERVER_URL_MAX_LEN];
extern char events_url[SERVER_URL_MAX_LEN];
extern char urlSection[80];
extern char wifiSsid[WIFI_SSID_MAX_LEN];
extern char wifiPassword[WIFI_PASS_MAX_LEN];
extern char rssi_str[10];
extern char *response_data;
extern int response_data_len;
extern esp_netif_ip_info_t system_ip;
extern bool use_ssl; // Flag to control SSL usage
extern char firmwareVersion[16];
extern time_t now;
extern struct tm timeinfo;

#endif // GLOBALS_H
