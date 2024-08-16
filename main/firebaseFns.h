#ifndef FIREBASEFNS_H
#define FIREBASEFNS_H

#ifdef __cplusplus
extern "C"
{
#endif
    void firebase_get_dispositivo_info();
    void firebase_get_dispositivo_devices();
    void firebase_get_dispositivo_device();
    void firebase_get_new_data(void);
    void clear_new_data_section(void);
    void firebase_set_dispositivo_info();
    void firebase_get_firmware_info();
    void send_alive_package();
#ifdef __cplusplus
}
#endif
#endif
