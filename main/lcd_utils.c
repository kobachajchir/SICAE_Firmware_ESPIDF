#include <globals.h>
#include <esp_err.h>
#include "driver/i2c.h"
#include "esp_log.h"
#include <unistd.h>
#include "lcd_utils.h"

static const char *TAG = "lcd";

esp_err_t lcd_send_cmd(char cmd) {
    esp_err_t err;
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (cmd & 0xf0);
    data_l = ((cmd << 4) & 0xf0);
    data_t[0] = data_u | 0x0C; // en=1, rs=0
    data_t[1] = data_u | 0x08; // en=0, rs=0
    data_t[2] = data_l | 0x0C; // en=1, rs=0
    data_t[3] = data_l | 0x08; // en=0, rs=0

    err = i2c_master_write_to_device(I2C_MASTER_NUM, SLAVE_ADDRESS_LCD, data_t, 4, 1000);
    if (err != ESP_OK) ESP_LOGI(TAG, "Error no. %d in command", err);
    return err;
}

esp_err_t lcd_send_data(char data) {
    esp_err_t err;
    char data_u, data_l;
    uint8_t data_t[4];
    data_u = (data & 0xf0);
    data_l = ((data << 4) & 0xf0);
    data_t[0] = data_u | 0x0D; // en=1, rs=1
    data_t[1] = data_u | 0x09; // en=0, rs=1
    data_t[2] = data_l | 0x0D; // en=1, rs=1
    data_t[3] = data_l | 0x09; // en=0, rs=1

    err = i2c_master_write_to_device(I2C_MASTER_NUM, SLAVE_ADDRESS_LCD, data_t, 4, 1000);
    if (err != ESP_OK) ESP_LOGI(TAG, "Error no. %d in command", err);
    return err;
}

void lcd_init(void) {
   usleep(50000);
    lcd_send_cmd(0x30);
   usleep(5000);
    lcd_send_cmd(0x30);
   usleep(200);
    lcd_send_cmd(0x30);
   usleep(10000);
    lcd_send_cmd(0x20); // 4-bit mode
   usleep(10000);

    // display initialization
    lcd_send_cmd(0x28); // Function set: 4-bit mode, 2 lines, 5x8 dots
   usleep(1000);
    lcd_send_cmd(0x08); // Display off
   usleep(1000);
    lcd_send_cmd(0x01); // Clear display
   usleep(1000);
   usleep(1000);
    lcd_send_cmd(0x06); // Entry mode: increment cursor, no shift
   usleep(1000);
    lcd_send_cmd(0x0C); // Display on, cursor off, blink off
   usleep(1000);
}

void lcd_send_string(char *str) {
    while (*str) lcd_send_data(*str++);
}

void lcd_clear(void){
    lcd_send_cmd(0x80);
    for (int i = 0; i < 70; i++){
        lcd_send_data(' ');
    }
}

void lcd_put_cur(int row, int col)
{
    switch (row)
    {
        case 0:
            col |= 0x80;
            break;
        case 1:
            col |= 0xC0;
            break;
    }

    lcd_send_cmd (col);
}
