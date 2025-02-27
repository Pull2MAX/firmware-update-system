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
    fmc_unlock();//Erase must unlock flash
    for(uint16_t i = 0; i < erase_num; ++i)
    {
        fmc_page_erase((0x08000000 + (startPage + i) * 1024));//0x08000000 is the starting address of flash, plus the page offset is the real address to erase
    }
    fmc_lock();//Erase must lock flash
}

/**  
 * @brief       When writing to the internal Flash memory, it can only be written in 4-byte chunks at a time.
 *  
 * @param[in]   start_addr      Flash start address, must be 4-byte aligned (e.g.: 0x08000000)
 * @param[in]   write_content   Pointer to the data array to be written, each element is a 32-bit word
 * @param[in]   write_length    Number of bytes to write, must be a multiple of 4 (e.g.: writing 3 words is 12 bytes)
 *  
 * @note        Calling conventions:
 *              1. Ensure that the target Flash area has been erased (Flash can only write from 1 to 0, all 1s after erasing)
 *              2. start_addr and write_length must be 4-byte aligned
 *              3. The function has handled the unlocking and locking of the Flash controller (FMC) internally, no external operation is required
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
        fmc_word_program(start_addr, *write_content); // Rely on this function to write four bytes of data at a time
        write_length-=4;
        start_addr+=4;
        write_content++;
    }
    fmc_lock();
}



