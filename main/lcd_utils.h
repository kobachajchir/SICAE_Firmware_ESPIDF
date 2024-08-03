#ifndef LCD_UTILS_H
#define LCD_UTILS_H

#include <esp_err.h>

// Function declarations
#ifdef __cplusplus
extern "C" {
#endif
    void lcd_init(void);
    esp_err_t lcd_send_cmd(char cmd);
    esp_err_t lcd_send_data(char data);
    void lcd_send_string(char *str);
    void lcd_clear(void);
    void lcd_clear_line(int row);
    void lcd_put_cur(int row, int col);
#ifdef __cplusplus
}
#endif
#endif