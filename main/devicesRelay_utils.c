#include <stdio.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "globals.h"

const char *TAG = "devicesRelay";

void setPower(bool isPowered, uint8_t gpioPort) {
    if (isPowered) {
        gpio_set_level(gpioPort, 0); // Establece el GPIO en bajo para encender el relé
    } else {
        gpio_set_level(gpioPort, 1); // Establece el GPIO en alto para apagar el relé
    }
    ESP_LOGI(TAG, "Set GPIO: %d, as %s", gpioPort, isPowered ? "ON" : "OFF");
}
