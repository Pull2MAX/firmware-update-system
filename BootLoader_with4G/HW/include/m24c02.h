#ifndef _M24C02_H
#define _M24C02_H
#include "stdint.h"

#define M24C02_WADDR 0xA0 
#define M24C02_RADDR 0xA1

#include "gd32f10x.h"
#include "m24c02.h"
#include "iic.h"

uint8_t M24C02_WriteByte(uint8_t addr, uint8_t write_content);

uint8_t M24C02_WritePage(uint8_t addr, uint8_t *write_arr);

uint8_t M24C02_ReadData(uint8_t addr, uint16_t read_length, uint8_t *read_arr); // The return value of the function represents the error information, parameters need to manually input the starting address to read and store the content read

void M24C02_ReadOTAInfo(void);

void M24C02_WriteOTAInfo(void);

#endif
