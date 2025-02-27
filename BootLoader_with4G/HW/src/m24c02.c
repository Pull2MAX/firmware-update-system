#include "gd32f10x.h"
#include "m24c02.h"
#include "iic.h"
#include "main.h"
#include "string.h"
#include "usart.h"
#include "delay.h"


/*
    @brief Write a byte of data to the 24C02 EEPROM
    @param addr The address to write to
    @param write_content The data to write
    @return 0: Success 1: Failed to send device address 2: Failed to send write address 3: Failed to send data
*/
uint8_t M24C02_WriteByte(uint8_t addr, uint8_t write_content)
{
    IIC_Start(); // Start IIC communication
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(M24C02_WADDR); // Send write address
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(addr); // Send address to write
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(write_content); // Send data to write
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Stop(); // Stop IIC communication
    return 0; // Success
}


/*
    @brief Write a page (16 bytes) of data to the 24C02 EEPROM
    @param addr The address to write to
    @param write_arr The data array to write
    @return 0: Success 1: Failed to send device address 2: Failed to send write address 3: Failed to send data
*/
uint8_t M24C02_WritePage(uint8_t addr, uint8_t *write_arr)
{
    IIC_Start(); // Start IIC communication
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(M24C02_WADDR); // Send write address
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(addr); // Send address to write
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    for(uint8_t i = 0; i < 16; i++) // Write 16 bytes
    {
        IIC_Send_Byte(write_arr[i]); // Send data to write
        if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    }
    IIC_Stop(); // Stop IIC communication
    return 0; // Success
}


/*
    @brief Read data from the 24C02 EEPROM
    @param addr The address to read from
    @param read_length The length of data to read
    @param read_arr The array to store the read data
    @return 0: Success 1: Failed to send device address 2: Failed to send write address 3: Failed to send data
*/
uint8_t M24C02_ReadData(uint8_t addr, uint16_t read_length, uint8_t *read_arr) // The return value of the function represents the error information, parameters need to manually input the starting address to read and store the content read
{
    IIC_Start(); // Start IIC communication
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(M24C02_WADDR); // Send write address
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(addr); // Send address to read
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Start(); // Restart IIC communication
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    IIC_Send_Byte(M24C02_RADDR); // Send read address
    if(IIC_Wait_Ack(1000) != 0) return 1; // Wait for ACK
    for(uint16_t i = 0; i < read_length; i++)
    {
        read_arr[i] = IIC_Read_Byte((i == (read_length - 1)) ? 0 : 1); // Read data
    }
    IIC_Stop(); // Stop IIC communication
    return 0; // Success
}



/*
    @brief Read OTA control information from the 24C02 EEPROM, first clear the ota_info structure, then read data from address 0 into the structure
    @param None
    @return None
*/
void M24C02_ReadOTAInfo(void)
{
    uint8_t ota_version[16] = {0};
    M24C02_ReadData(0x00, 16, ota_version); // Read OTA version info
    memcpy(ota_info.OTA_VERSION, ota_version, 16); // Store the version info
}


/*
    @brief Write OTA control information to the 24C02 EEPROM
    @param None
    @return None
*/
void M24C02_WriteOTAInfo(void)
{
    M24C02_WritePage(0x00, ota_info.OTA_VERSION); // Write OTA version info
}




