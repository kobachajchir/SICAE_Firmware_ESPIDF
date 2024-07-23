#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
    void wifi_init_softap(void);
    void wifi_init_sta(void);
#ifdef __cplusplus
}
#endif
#endif
