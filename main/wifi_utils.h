#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif
    void wifi_init_softap(void);
    void wifi_init_sta(void);
    void printCurrentIP();
    void get_wifi_signal_strength();
    bool is_ap_mode();
#ifdef __cplusplus
}
#endif
#endif
