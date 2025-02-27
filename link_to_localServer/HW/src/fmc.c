#include "gd32f10x.h"
#include "fmc.h"
#include "spi.h"
#include "w25q64.h"

/*
    @brief Erase flash
    @param startPage The starting page number to erase
    @param erase_num The number of pages to erase
*/
void GD32_EraseFalash(uint16_t startPage, uint16_t erase_num) 
{
    fmc_unlock();//Flash must be unlocked before erasing
    for(uint16_t i = 0; i < erase_num; ++i)
    {
        fmc_page_erase((0x08000000 + (startPage + i) * 1024));//0x08000000 is the starting address of flash, plus the page offset is the actual address to be erased
    }
    fmc_lock();//Flash must be locked after erasing
}
}

/**  
 * @brief       Write to internal Flash memory, can only write 4 bytes at a time
 *  
 * @param[in]   start_addr      The starting address of Flash, must be 4-byte aligned (e.g., 0x08000000)  
 * @param[in]   write_content   Pointer to the data array to be written, each element is 32 bits  
 * @param[in]   write_length    The number of bytes to write, must be a multiple of 4 (e.g., writing 3 words is 12 bytes)  
 *  
 * @note        Calling specifications:  
 *              1. Ensure the target Flash area has been erased (Flash can only write 1 to 0, all 1 after erasure)  
 *              2. start_addr and write_length must meet 4-byte alignment  
 *              3. The function internally handles the unlocking and locking of the Flash controller (FMC), no external operation is required  
 *  
 * @example     Example:  
 *              uint32_t data[] = {0x11223344, 0xAABBCCDD, 0x55667788};  
 *              GD32_WriteFlash(0x08000000, data, sizeof(data)); // Write 3 words to Flash  
 */  
void GD32_WriteFlash(uint32_t start_addr, uint32_t *write_content, uint32_t write_length)
{
    fmc_unlock();
    while(write_length)
    {
        fmc_word_program(start_addr, *write_content); // Write four bytes of data at a time using this function
        write_length-=4;
        start_addr+=4;
        write_content++;
    }
    fmc_lock();
}



