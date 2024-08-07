#ifndef NTP_H
#define NTP_H

#include "esp_sntp.h"
#include "globals.h"

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif
    void initialize_sntp(void);
    void obtain_time(void);
#ifdef __cplusplus
}
#endif
#endif