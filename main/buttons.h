#ifndef BUTTONS_H
#define BUTTONS_H

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif
    bool debounce(int pin);
    void check_buttons();
#ifdef __cplusplus
}
#endif
#endif