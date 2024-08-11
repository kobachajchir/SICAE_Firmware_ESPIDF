// nec_utils.c
#include "nec_utils.h"
#include "esp_log.h"
#include "driver/rmt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define TAG "NEC_UTILS"

nec_code_t nec_codes[NEC_CODES_MAX] = {0};
volatile int transmitFlag = 0;
volatile int selectIndex = 0;

void init_nec_codes(void) {
    nec_codes[0] = (nec_code_t){ .address = 0x0440, .command = 0x3003 };
    nec_codes[1] = (nec_code_t){ .address = 0x0450, .command = 0x3103 };
    nec_codes[2] = (nec_code_t){ .address = 0x0460, .command = 0x3203 };
    nec_codes[3] = (nec_code_t){ .address = 0x0470, .command = 0x3303 };
    nec_codes[4] = (nec_code_t){ .address = 0x0480, .command = 0x3403 };
}

void nec_encoder_init(rmt_channel_t channel) {
    rmt_config_t rmt_tx = {
        .rmt_mode = RMT_MODE_TX,
        .channel = channel,
        .clk_div = 80,
        .gpio_num = EXAMPLE_IR_TX_GPIO_NUM,
        .mem_block_num = 1,
        .tx_config = {
            .carrier_freq_hz = 38000,
            .carrier_duty_percent = 33,
            .carrier_level = RMT_CARRIER_LEVEL_HIGH,
            .idle_level = RMT_IDLE_LEVEL_LOW,
            .carrier_en = true,
        }
    };
    rmt_config(&rmt_tx);
    rmt_driver_install(rmt_tx.channel, 0, 0);
}

void nec_send_frame(rmt_channel_t channel, uint16_t address, uint16_t command) {
    rmt_item32_t items[NEC_DATA_ITEM_NUM];
    int i, j = 0;
    nec_build_item(items + j++, NEC_HEADER_HIGH_US, NEC_HEADER_LOW_US);
    for (i = 0; i < 16; i++) {
        nec_build_item(items + j++, NEC_BIT_ONE_HIGH_US, NEC_BIT_ONE_LOW_US);
    }
    rmt_write_items(channel, items, j, true);
    rmt_wait_tx_done(channel, portMAX_DELAY);
}

void nec_build_item(rmt_item32_t* item, int high_us, int low_us) {
    item->level0 = 1;
    item->duration0 = (high_us) / 10 * RMT_TICK_10_US;
    item->level1 = 0;
    item->duration1 = (low_us) / 10 * RMT_TICK_10_US;
}

void nec_parser_frame(rmt_item32_t* items, size_t length, uint16_t* address, uint16_t* command) {
    *address = 0;
    *command = 0;
    for (int i = 0; i < length; i++) {
        if (items[i].level0 == 1 && items[i].duration0 > NEC_BIT_ONE_HIGH_US) {
            *address |= (1 << i);
        }
        if (items[i].level0 == 1 && items[i].duration0 > NEC_BIT_ONE_LOW_US) {
            *command |= (1 << i);
        }
    }
    ESP_LOGI(TAG, "Parsed NEC frame: Address=%04x, Command=%04x", *address, *command);
}

void append_nec_code(uint16_t address, uint16_t command) {
    for (int i = 0; i < NEC_CODES_MAX; i++) {
        if (nec_codes[i].address == 0 && nec_codes[i].command == 0) {
            nec_codes[i] = (nec_code_t){ .address = address, .command = command };
            ESP_LOGI(TAG, "Appended NEC code: Address=%04x, Command=%04x at index %d", address, command, i);
            break;
        }
    }
}

void print_nec_code(uint16_t address, uint16_t command) {
    ESP_LOGI(TAG, "Received NEC code: Address=%04x, Command=%04x", address, command);
}