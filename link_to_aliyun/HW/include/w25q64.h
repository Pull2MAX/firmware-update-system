#ifndef _w25q64_H
#define _w25q64_H

#include "stdint.h" // To allow this file to recognize uint8_t


#define CS_ENABLE       gpio_bit_reset(GPIOA, GPIO_PIN_4)
#define CS_DISENABLE    gpio_bit_set(GPIOA, GPIO_PIN_4)


void W25Q64_Init(void);
void W25Q64_WaitBusy(void);
void W25Q64_Enable(void);
void W25Q64_Erase64k(uint8_t block_num);
void W25Q64_PageWrite(uint8_t *data_buffer, uint16_t page_number_from_start);
void W25Q64_ReadData(uint8_t *recv, uint32_t read_addr, uint32_t datalen);

#endif

