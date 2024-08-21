
#ifndef DEVICETYPE_H
#define DEVICETYPE_H

#include "driver/gpio.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // Define the Device struct
    typedef struct Device
    {
        const char *name; // Device name
        int id;           // Device ID
        gpio_num_t pin;   // GPIO pin associated with the device
        bool state;       // Device state (ON/OFF)
        const char *type; // Device type (e.g., "relay", "ir")
    } Device;

#ifdef __cplusplus
}
#endif

#endif // DEVICETYPE_H
