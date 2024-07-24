#ifndef FIREBASEFNS_H
#define FIREBASEFNS_H

#ifdef __cplusplus
extern "C" {
#endif
    void firebase_get_dispositivo_info();
    void firebase_get_dispositivo_devices();
    void firebase_get_dispositivo_device();
    void firebase_get_new_data(void);
    void perform_http_get(const char *url);

#ifdef __cplusplus
}
#endif
#endif
