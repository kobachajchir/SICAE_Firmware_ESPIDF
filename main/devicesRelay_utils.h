#ifndef DEVICES_RELAY_UTILS_H
#define DEVICES_RELAY_UTILS_H

#ifdef __cplusplus
extern "C"
{
#endif
    void setPower(bool isPowered, uint8_t gpioPort);
    int get_gpio_output_gpio_level(gpio_num_t pin);
#ifdef __cplusplus
}
#endif
#endif
