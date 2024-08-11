#pragma once
typedef union {
  struct {
    __UINT8_TYPE__ bit0 : 1;
    __UINT8_TYPE__ bit1 : 1;
    __UINT8_TYPE__ bit2 : 1;
    __UINT8_TYPE__ bit3 : 1;
    __UINT8_TYPE__ bit4 : 1;
    __UINT8_TYPE__ bit5 : 1;
    __UINT8_TYPE__ bit6 : 1;
    __UINT8_TYPE__ bit7 : 1;
  } bits;
  __UINT8_TYPE__ byte;
} myByte;