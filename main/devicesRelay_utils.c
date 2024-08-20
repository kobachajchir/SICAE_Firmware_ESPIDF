#include <stdio.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "hal/gpio_hal.h"
#include "soc/gpio_struct.h"
#include "soc/gpio_reg.h"
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

int get_gpio_output_gpio_level(gpio_num_t pin)
{
    // Determine if the pin is an output
    if (GPIO.enable & (1ULL << pin))
    {
        // Read the output state from the appropriate register
        if (pin < 32)
        {
            return (GPIO.out >> pin) & 0x1; // GPIO 0-31
        }
        else
        {
            return (GPIO.out1.val >> (pin - 32)) & 0x1; // GPIO 32-39
        }
    }
    else
    {
        // Pin is not configured as an output
        return -1;
    }
}