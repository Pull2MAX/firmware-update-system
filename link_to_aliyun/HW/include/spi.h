#ifndef _SPI_H
#define _SPI_H
#include "stdint.h" 

void SPI0_Init(void);


uint8_t SPI0_TransmitAndReceive(uint8_t transmit_content);

void SPI0_Write(uint8_t *write_arr, uint16_t datalen);

void SPI0_Read(uint8_t *read_store_arr, uint16_t datalen);



#endif
