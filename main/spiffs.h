#ifndef SPIFFS_H
#define SPIFFS_H

#include "esp_log.h"
#include <string.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif
    void init_spiffs();
#ifdef __cplusplus
}
#endif
#endif