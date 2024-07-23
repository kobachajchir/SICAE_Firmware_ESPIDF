#include <stdio.h>
#include <esp_log.h>
#include "driver/gpio.h"
#include "globals.h"

static const char *TAG = "buttons";

bool debounce(int pin) {
    int count = 0;
    const int sample_count = 10; // Reduced sample count to 10
    const int debounce_delay = 1; // Reduced delay to 10 ms

    for (int i = 0; i < sample_count; i++) {
        int pin_state = gpio_get_level(pin); // Get the raw pin state
        if (pin_state == 1) { // Check for high level (button pressed)
            count++;
        }
        vTaskDelay(pdMS_TO_TICKS(debounce_delay)); // Delay for 10 ms
    }
    bool result = count > (sample_count / 2); // Return true if the pin reads high more than half the time
    //ESP_LOGI(TAG, "Debounce result for pin %d: %d", pin, result);
    return result;
}

void check_buttons() {
    uint32_t currentTime = xTaskGetTickCount() * portTICK_PERIOD_MS;

    // BTN ENTER
    if (debounce(PIN_BTN_ENTER) && !BTN_ENTER_PRESSED) {
        BTN_ENTER_PRESSED = 1;
        btnEnterDuration = currentTime;
        ESP_LOGI(TAG, "BTN_ENTER Pressed");
    } else if (!debounce(PIN_BTN_ENTER) && BTN_ENTER_PRESSED) {
        BTN_ENTER_PRESSED = 0;
        if ((currentTime - btnEnterDuration) >= BTN_PRESS_TIME && (currentTime - btnEnterDuration) < BTN_LONGPRESS_TIME) {
            ESP_LOGI(TAG, "BTN_ENTER shortpress");
            BTN_ENTER_RELEASED = 1;
        }else if ((currentTime - btnEnterDuration) >= BTN_LONGPRESS_TIME) {
            ESP_LOGI(TAG, "BTN_ENTER longpress");
            BTN_ENTER_RELEASED_LONGPRESS = 1;
        }
        btnEnterDuration = 0;
    }

    // BTN UP
    if (debounce(PIN_BTN_UP) && !BTN_UP_PRESSED) {
        BTN_UP_PRESSED = 1;
        btnUpDuration = currentTime;
        ESP_LOGI(TAG, "BTN_UP Pressed");
    } else if (!debounce(PIN_BTN_UP) && BTN_UP_PRESSED) {
        BTN_UP_PRESSED = 0;
        if ((currentTime - btnUpDuration) >= BTN_PRESS_TIME && (currentTime - btnUpDuration) < BTN_LONGPRESS_TIME) {
            ESP_LOGI(TAG, "BTN_UP shortpress");
            BTN_UP_RELEASED = 1;
        }else if ((currentTime - btnUpDuration) >= BTN_LONGPRESS_TIME) {
            ESP_LOGI(TAG, "BTN_UP longpress");
            BTN_UP_RELEASED_LONGPRESS = 1;
        }
        btnUpDuration = 0;
    }

    // BTN DOWN
    if (debounce(PIN_BTN_DOWN) && !BTN_DOWN_PRESSED) {
        BTN_DOWN_PRESSED = 1;
        btnDownDuration = currentTime;
        ESP_LOGI(TAG, "BTN_DOWN Pressed");
    } else if (!debounce(PIN_BTN_DOWN) && BTN_DOWN_PRESSED) {
        BTN_DOWN_PRESSED = 0;
        if ((currentTime - btnDownDuration) >= BTN_PRESS_TIME && (currentTime - btnDownDuration) < BTN_LONGPRESS_TIME) {
            ESP_LOGI(TAG, "BTN_DOWN shortpress");
            BTN_DOWN_RELEASED = 1;
        }else if ((currentTime - btnDownDuration) >= BTN_LONGPRESS_TIME) {
            ESP_LOGI(TAG, "BTN_DOWN longpress");
            BTN_DOWN_RELEASED_LONGPRESS = 1;
        }
        btnDownDuration = 0;
    }
}