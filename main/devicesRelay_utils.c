#include <stdio.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "globals.h"

void setPower(bool isPowered, uint8_t gpioPort) {
    // Check if the pin is configured as an output
    gpio_config_t io_conf;
    io_conf.pin_bit_mask = (1ULL << gpioPort);  // Pin mask for the given GPIO port
    gpio_get_level(gpioPort); // get the current level to ensure the pin is initialized

    // Get the current configuration of the pin
    if (gpio_get_level(gpioPort) != -1) {
        gpio_set_direction(gpioPort, GPIO_MODE_OUTPUT);  // Set as output if not already set
    }

    // Set the GPIO level based on the isPowered parameter
    if (isPowered) {
        gpio_set_level(gpioPort, 1);  // Set GPIO pin high
    } else {
        gpio_set_level(gpioPort, 0);  // Set GPIO pin low
    }
}