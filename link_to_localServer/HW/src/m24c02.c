#include "gd32f10x.h"
#include "m24c02.h"
#include "iic.h"
#include "main.h"
#include "string.h"
#include "usart.h"
#include "delay.h"

uint8_t M24C02_WriteByte(uint8_t addr,uint8_t write_content)
{
    IIC_Start();
    IIC_Send_Byte(M24C02_WADDR); // From the perspective of the computer, it selects the device and writes
    if(IIC_Wait_Ack(100) != 0) return 1;
    
    IIC_Send_Byte(addr);
    if(IIC_Wait_Ack(100) != 0) return 2; // No acknowledgment received, return 2 indicates failure
    
    IIC_Send_Byte(write_content);
    if(IIC_Wait_Ack(100) != 0) return 3;
    
    IIC_Stop();
    return 0;  
}


uint8_t M24C02_WritePage(uint8_t addr,uint8_t *write_arr) // addr is the starting address when writing by page, write_arr is the data to be written (16 bytes)
{
    IIC_Start();
    IIC_Send_Byte(M24C02_WADDR); // From the perspective of the computer, it selects the device and writes
    if(IIC_Wait_Ack(100) != 0) return 1;
    
    IIC_Send_Byte(addr);
    if(IIC_Wait_Ack(100) != 0) return 2; // No acknowledgment received, return 2 indicates failure
    
    for(uint8_t i = 0; i < 16; ++i)
    {
        IIC_Send_Byte(write_arr[i]);
        if(IIC_Wait_Ack(100) != 0) return 3 + i;
    }
    IIC_Stop();
    return 0;  
}


uint8_t M24C02_ReadData(uint8_t addr, uint16_t read_length, uint8_t *read_arr) // The return value of the function represents error information, parameters need to manually input the starting address to read and the content to be read
{
    // The following content strictly follows the timing
    IIC_Start();
    IIC_Send_Byte(M24C02_WADDR); // This is write mode, indicating that the host will write to the slave (i.e., the address of the data)
    if(IIC_Wait_Ack(100) != 0) return 1;
    IIC_Send_Byte(addr);
    if(IIC_Wait_Ack(100) != 0) return 2;
    
    IIC_Start(); // According to the timing diagram, the device address needs to be re-entered and the host is in read mode
    IIC_Send_Byte(M24C02_RADDR); // It is important to note that the host needs to adopt read mode, thus the address passed is for reading
    if(IIC_Wait_Ack(100) != 0) return 3;
    for(uint8_t i = 0; i < read_length - 1; ++i)
    {
        read_arr[i] = IIC_Read_Byte(1);
    }
    read_arr[read_length - 1] = IIC_Read_Byte(0);
    IIC_Stop();
    return 0;
}



/*
    @brief Read OTA control information from 24C02 EEPROM, first clear the ota_info structure, then read data from address 0 into the structure
    @param None
    @return None
*/
void M24C02_ReadOTAInfo(void)
{
    memset(&ota_info, 0, OTA_CONTROL_INFO_SIZE); // This initializes the values of ota_info, then the next line is to actually read into ota_info
    M24C02_ReadData(0, OTA_CONTROL_INFO_SIZE, (uint8_t *)&ota_info); // ota_flag is stored starting from address 0 in m24c02
}



void M24C02_WriteOTAInfo(void)
{
    uint8_t *source_data = (uint8_t *)&ota_info;
    
    // Write in pages of 16 bytes
    for(uint8_t page = 0; page < OTA_CONTROL_INFO_SIZE / 16; ++page)
    {
        uint8_t eeprom_addr = page * 16;        // EEPROM target address
        uint8_t *data_to_write = source_data + page * 16;  // Source data address
        
        M24C02_WritePage(eeprom_addr, data_to_write); // 24c02 strictly writes in pages of 16 bytes
        Delay_Ms(5);  // EEPROM writing requires time
    }
}




