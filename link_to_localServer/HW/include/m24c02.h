#ifndef _M24C02_H
#define _M24C02_H
#include "stdint.h"

#define M24C02_WADDR 0xA0 
#define M24C02_RADDR 0xA1

#include "gd32f10x.h"
#include "m24c02.h"
#include "iic.h"



uint8_t M24C02_WriteByte(uint8_t addr,uint8_t write_content);

uint8_t M24C02_WritePage(uint8_t addr,uint8_t *write_arr);

uint8_t M24C02_ReadData(uint8_t addr, uint16_t read_length, uint8_t *read_arr);//函数的返回值代表着错误信息，参数需要手动输入开始读取的地址和读取到的内容存放

void M24C02_ReadOTAInfo(void);

void M24C02_WriteOTAInfo(void);

#endif
