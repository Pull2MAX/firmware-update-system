#ifndef _BOOT_H
#define _BOOT_H
#include "stdint.h" 

uint8_t BootLoader_Enter(uint8_t wait_second_time);

void Bootloader_Branch_Judge(void);

void Print_BootLoader_Control_Info(void);

void BootLoaderCommandChoose(uint8_t *command, uint16_t command_len);

typedef void (*load_a)(void);

void Bootloader_Branch_Judge(void);

__ASM void MSR_SP(uint32_t addr);

void LOAD_App(uint32_t addr);

void BootLoader_Clear(void);  

uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen);

#endif

