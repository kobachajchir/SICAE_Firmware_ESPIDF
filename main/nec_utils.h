// nec_utils.h
#ifndef NEC_UTILS_H
#define NEC_UTILS_H

#include <stdint.h>
#include "driver/rmt.h"

#define NEC_CODES_MAX 10

typedef struct {
    uint16_t address;
    uint16_t command;
} nec_code_t;

        extern nec_code_t nec_codes[NEC_CODES_MAX];
        extern volatile int transmitFlag;
        extern volatile int selectIndex;

        // Initialize the NEC codes with example values
        void init_nec_codes(void);

        // Initialize the NEC encoder
        void nec_encoder_init(rmt_channel_t channel);

        // Send an NEC frame
        void nec_send_frame(rmt_channel_t channel, uint16_t address, uint16_t command);

        // Parse an NEC frame
        void nec_parser_frame(rmt_item32_t* items, size_t length, uint16_t* address, uint16_t* command);

        // Append an NEC code to the table
        void append_nec_code(uint16_t address, uint16_t command);

        // Print an NEC code
        void print_nec_code(uint16_t address, uint16_t command);

        #endif // NEC_UTILS_H