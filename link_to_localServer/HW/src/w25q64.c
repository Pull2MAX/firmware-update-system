#include "gd32f10x.h"
#include "usart.h"
#include "spi.h"
#include "w25q64.h"

void W25Q64_Init()
{
    rcu_periph_clock_enable(RCU_GPIOA);
    gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
    CS_DISENABLE; // First disable chip select
    SPI0_Init();
}

void W25Q64_WaitBusy(void) // This function checks if W25Q64 is busy before performing any operations, waits if busy
{
    uint8_t res;
    do{
        CS_ENABLE;
        SPI0_TransmitAndReceive(0x05); // Try to read SPI status
        res = SPI0_TransmitAndReceive(0xff); // Send any content to get status to see if it is busy
        CS_DISENABLE;
    }while((res & 0x01) == 0x01);
}


void W25Q64_Enable(void)
{
    W25Q64_WaitBusy();
    CS_ENABLE;
    SPI0_TransmitAndReceive(0x06);// Enable W25Q64
    CS_DISENABLE;
}


void W25Q64_Erase64k(uint8_t block_num)
{
    uint8_t write_data[4];
    
    write_data[0] = 0xD8;
    write_data[1] = (block_num * 64 * 1024) >> 16; // Multiply by 64 because one block is 64kb
    write_data[2] = (block_num * 64 * 1024) >> 8;
    write_data[3] = (block_num * 64 * 1024) >> 0;
    
    W25Q64_WaitBusy();
    W25Q64_Enable();
    CS_ENABLE;
    SPI0_Write(write_data,4);
    CS_DISENABLE;
    W25Q64_WaitBusy();// Erasing is very slow, so wait a bit more here
}


/**
 * @brief Write data to the specified page page_num of the W25Q64 Flash chip, the data address starts from data_buffer, default write 256 bytes at a time (1 page = 256 bytes) and then finish writing
 * 
 * @param data_buffer Pointer to the start address of the data array to be written
 * @param page_number_from_start Start writing from which page
 */
void W25Q64_PageWrite(uint8_t *data_buffer, uint16_t page_number_from_start)
{
    uint8_t write_data[4];
    
    write_data[0] = 0x02;
    write_data[1] = (page_number_from_start * 256) >> 16; // Multiply by 64 because one block is 64kb
    write_data[2] = (page_number_from_start * 256) >> 8;
    write_data[3] = (page_number_from_start * 256) >> 0;
    
    W25Q64_WaitBusy();
    W25Q64_Enable();
    CS_ENABLE;
    SPI0_Write(write_data, 4);
    SPI0_Write(data_buffer, 256);
    CS_DISENABLE;
}
    

/*
 * Read data from the W25Q64 Flash chip
 * @param recv: Pointer to the buffer for receiving data
 * @param read_addr: The Flash address to read
 * @param datalen: The length of data to read
 */
void W25Q64_ReadData(uint8_t *recv, uint32_t read_addr, uint32_t datalen)
{
    uint8_t write_data[4];    // Array for storing read command and address
    
    write_data[0] = 0x03;     // Standard read command (Read Data)
    write_data[1] = (read_addr) >> 16;  // High 8 bits of address
    write_data[2] = (read_addr) >> 8;   // Middle 8 bits of address
    write_data[3] = (read_addr) >> 0;   // Low 8 bits of address
    
    W25Q64_WaitBusy();        // Wait for Flash to be idle (not busy)
    CS_ENABLE;                // Enable chip select, start SPI communication
    SPI0_Write(write_data, 4);// Send read command and 24-bit address
    SPI0_Read(recv, datalen); // Read the specified length of data
    CS_DISENABLE;             // Disable chip select, end SPI communication
}
   


