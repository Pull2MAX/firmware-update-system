#ifndef _FMC_H
#define _FMC_H
#include "stdint.h" 

void GD32_EraseFalash(uint16_t startPage, uint16_t erase_num);
void GD32_WriteFlash(uint32_t start_addr, uint32_t *write_content, uint32_t write_length);

#endif
