#include "gd32f10x.h"
#include "main.h"
#include "usart.h"
#include "delay.h"
#include "m24c02.h"
#include "iic.h"
#include "spi.h"
#include "w25q64.h"
#include "fmc.h"
#include "boot.h"
#include "main.h"

load_a my_app; 

uint8_t BootLoader_Enter_Notify(uint8_t wait_second_time)
{
    U0_MyPrintf("\r\n-----%d seconds to input [lowercase w] will immediately enter bootloader command line, otherwise will immediately execute App-----\r\n",wait_second_time);
    while(wait_second_time)
    {
        Delay_Ms(1000);
        wait_second_time--;
        if(U0_RxBuff[0]=='w')   return 1;
    }
    return 0;
}

/*
    @brief Choose the command in command line mode, this function is placed in the serial reception of the main function
    @param command_number Command in command line mode
    @param datalen Length of the command in command line mode, to prevent it from being an invalid command
*/
void BootLoaderCommandChoose(uint8_t *data, uint16_t datalen)
{  
    if(BootStatusFlag == 0)
    {
        if((datalen == 1) && (data[0] == '1'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to erase all APP area firmware (remember to select -7 to restart afterwards)\r\n", data[0]);
            GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM); // Erase area A
        }
        else if((datalen == 1) && (data[0] == '2'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to update app firmware via xmodem protocol, please use bin file\r\n",data[0]);
            GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM); // Erase area A
            BootStatusFlag |= (XMODEM_SEND_C_FLAG | XMODEM_DATA_FLAG); // Set flags indicating need to send C character and data
            updataA.Xmodem_Timer = 0;
            updataA.Xmodem_Number = 0; 
        }
        else if((datalen == 1) && (data[0] == '3'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], please set the version number for this update\r\n", data[0]);            
            BootStatusFlag |= (SET_VERSION_FLAG); // Set flag indicating need to send C character and data
        }
        else if((datalen == 1) && (data[0] == '4'))
        {
            M24C02_ReadOTAInfo();
            U0_MyPrintf("\r\nYou have selected [%c]\r\nThe queried version number is:%s\r\n", data[0], ota_info.OTA_VERSION);  
            U0_MyPrintf("\r\nPlease select other commands to execute\r\n");   
            Print_BootLoader_Control_Info();       
        }
        else if((datalen == 1) && (data[0] == '5'))
        {
            U0_MyPrintf("This function can store the sent program into W25Q64, please enter the block number to overwrite (1--9)\r\n");
            BootStatusFlag |= STORE_APP_TO_W25Q64_FLAG;
        }
        else if((datalen == 1) && (data[0] == '6'))
        {
            U0_MyPrintf("This function will use the app firmware from external flash, please select the block number of external flash (1--9)\r\n");
            BootStatusFlag |= USE_APP_IN_W25Q64_FLAG;   
        }
        else if((datalen == 1) && (data[0] == '7'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to restart\r\n", data[0]);
            Delay_Ms(1000);
            NVIC_SystemReset();
        }
        else if((datalen == 1) && (data[0] == '8'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], please set the server connection information\r\n", data[0]);
            BootStatusFlag |= SET_SERVER_INFO_FLAG;
        }
    }


    else if(BootStatusFlag & XMODEM_DATA_FLAG)
    {
        if((datalen == (128+5)) && (data[0] == 0x01)) // Data packet received via serial, 0x01 ensures the start frame is 0x01
        {
            BootStatusFlag &= ~XMODEM_SEND_C_FLAG; // Clear flag indicating no need to send C character
            updataA.Xmodem_CRC_Result = Xmodem_CRC16(&data[3], 128);// Manually written function crc only calculates crc starting from index 3
            
            if(updataA.Xmodem_CRC_Result == data[131] * 256 + data[132]) // If crc check is correct, send ack
            {
                // Start writing 128 bytes into the dedicated buffer for updating firmware (updataA.updata)
                updataA.Xmodem_Number++; // Indicates the sequence number of received packets, 8 packets fill one updata_buffer to write once to internal flash, 8*128=1024 bytes
                memcpy(&updataA.updata_buffer[((updataA.Xmodem_Number - 1) % (GD32_ONE_PAGE_BYTESIZE / 128)) * 128], &data[3], 128); // Means writing the 128 bytes received from serial starting from index 3 to the corresponding position in updata_buffer, each time moving 128 bytes
                if((updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) == 0) // Note that the code in this branch has no loop structure, so after 8 packets are filled, it needs to manually write to internal flash
                {
                    
                    if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG) // If the flag is to store to w25q64, then write to w25q64
                    {
                        // W25Q64 is 256 bytes per page, the previous if is to fill 1024 bytes
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            W25Q64_PageWrite(&updataA.updata_buffer[i * 256], ((updataA.W25Q64_BLOCK_NUMBER * 64 * 1024 / 256) + (updataA.Xmodem_Number/8 - 1) * 4 + i)); // External flash block + block internal offset
                        }                       
                    }

                    // If the flag is not to store to w25q64, then write to internal flash
                    else GD32_WriteFlash(GD32_A_AREA_START_ADDR + ((updataA.Xmodem_Number / (GD32_ONE_PAGE_BYTESIZE / 128)) - 1) * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer, GD32_ONE_PAGE_BYTESIZE); // When writing to internal flash, the underlying function writes 4 bytes at a time                    
                } 
                U0_MyPrintf("\x06");               
            }
            else // crc check failed, send nack
            {
                U0_MyPrintf("\x15");
            }
        }
        if((datalen == 1) && (data[0] == 0x04)) // Indicates the end frame (0x04) has been received
        {
            U0_MyPrintf("\x06");
            
            if((updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) != 0) // If there are remaining packets, continue writing to flash, for example, if there are ten packets, the first 8 have been written to flash, and the remaining 2 packets are handled here
            {
                if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG) // Not finished. Continue sending.
                {
                    for(uint8_t i = 0; i < 4; i++) // It will write a little more to external flash but it doesn't matter 
                    {
                        W25Q64_PageWrite(&updataA.updata_buffer[i * 256], ((updataA.W25Q64_BLOCK_NUMBER * 64 * 1024 / 256) + (updataA.Xmodem_Number/8) * 4 + i));
                    }                   
                }

                else GD32_WriteFlash(GD32_A_AREA_START_ADDR + ((updataA.Xmodem_Number / (GD32_ONE_PAGE_BYTESIZE / 128))) * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer, (updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) * 128); // When writing to internal flash, the underlying function writes 4 bytes at a time
            }
            BootStatusFlag &= ~XMODEM_DATA_FLAG; // Unified operation, clear flag immediately after the task is done

            if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG)
            {
                BootStatusFlag &= ~XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG;
                ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] = updataA.Xmodem_Number * 128; // Record the byte count in the file length member
                M24C02_WriteOTAInfo(); // Write to 24c02
                Delay_Ms(1000);
                Print_BootLoader_Control_Info();
            } 
            else 
            {
                Delay_Ms(100);
                NVIC_SystemReset(); // Restart  
            }

        }
    }
    
    
    else if(BootStatusFlag & SET_VERSION_FLAG)
    {
        if(datalen == 26) // Directly check the length of the version, although 32 bytes are reserved, only 26 bytes are used
        {
            int temp_for_version;
            if(sscanf((char *)data,"VER-%d.%d.%d-%d-%d-%d-%d.%d",&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version) == 8)
            {
                memset(ota_info.OTA_VERSION, 0, 32); // First clear the version number, a total of 32 bytes are given to store version information
                memcpy(ota_info.OTA_VERSION, data, 26); // Although 32 bytes are given to store version information, only 26 bytes are used
                M24C02_WriteOTAInfo();
                U0_MyPrintf("\r\nVersion number set successfully! [Please select other commands to execute!]\r\n");
                BootStatusFlag &= ~SET_VERSION_FLAG; // Clear flag indicating no need to set version number
                Print_BootLoader_Control_Info();
            }
            else U0_MyPrintf("\r\nVersion number format error, please re-enter\r\n");
        }
        else U0_MyPrintf("\r\nLength of version number error, should be 26 characters, please re-enter\r\n");
    }

    else if(BootStatusFlag & STORE_APP_TO_W25Q64_FLAG)
    {
        if(datalen == 1)
        {
            if(data[0] >= 0x31 && data[0] <= 0x39)
            {
                updataA.W25Q64_BLOCK_NUMBER = data[0] - 0x30;
                BootStatusFlag |= (XMODEM_SEND_C_FLAG | XMODEM_DATA_FLAG | XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG); // Set these three flags because we need to send the bin file via xmodem protocol to w25q64, first send C, then send data
                updataA.Xmodem_Timer = 0;
                updataA.Xmodem_Number = 0; // Here is updating to external flash, borrowing the members in the structure for updating internal flash
                ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] = 0;
                W25Q64_Erase64k(updataA.W25Q64_BLOCK_NUMBER); // Because it is to be written to external flash, it must be manually cleared first
                U0_MyPrintf("\r\nThe block number you selected for w25q64 is:%d\r\n\r\nThe block has been cleared, please immediately select the bin file to be stored and send it to w25q64 via xmodem protocol\r\n", updataA.W25Q64_BLOCK_NUMBER);
                BootStatusFlag &= ~STORE_APP_TO_W25Q64_FLAG;
            }
            else U0_MyPrintf("Please enter a number between 1 and 9!\r\n");
        }
        else U0_MyPrintf("Data length input error! Needs to input 1--9!\r\n");
    }

    else if(BootStatusFlag & USE_APP_IN_W25Q64_FLAG)
    {
        if(datalen == 1)
        {
            if(data[0] >= 0x31 && data[0] <= 0x39)
            {
                updataA.W25Q64_BLOCK_NUMBER = data[0] - 0x30;
                BootStatusFlag |= UPDATA_A_FLAG;
                BootStatusFlag &= ~USE_APP_IN_W25Q64_FLAG;
            }
            else U0_MyPrintf("Please enter a number between 1 and 9!\r\n");
        }
        else U0_MyPrintf("Data length input error! Needs to input 1--9!\r\n");
    }

    else if(BootStatusFlag & SET_SERVER_INFO_FLAG)
    {
        U2_MyPrintf("AT+SOCKA=%s\r\n", data); // This means directly appending the content received from serial 2 to the AT command, then handing it over to the 4G module for execution
        Delay_Ms(30);
        U2_MyPrintf("AT+SOCKAEN=ON\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKBEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKCEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKDEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+HEART=ON,NET,USER,60,C000\r\n"); // To connect to Alibaba Cloud service later, meaning sending a heartbeat packet every 60 seconds
		Delay_Ms(30);
        U0_MyPrintf("---AT command settings successful, restarting soon---\r\n");
		U2_MyPrintf("AT+S\r\n"); // Save AT command, then restart
		Delay_Ms(30);
        BootStatusFlag &= ~SET_SERVER_INFO_FLAG;
    }
    
}

void Print_BootLoader_Control_Info(void)
{
	U0_MyPrintf("\r\n");
	U0_MyPrintf("[1] Erase all APP area firmware\r\n");
	U0_MyPrintf("[2] Serial IAP download A area program\r\n");
	U0_MyPrintf("[3] Set OTA version number\r\n");
	U0_MyPrintf("[4] Query OTA version number\r\n");
	U0_MyPrintf("[5] Download program to external Flash\r\n");
	U0_MyPrintf("[6] Use program in external Flash\r\n");
	U0_MyPrintf("[7] Restart\r\n");
    U0_MyPrintf("[8] Set server connection information\r\n");
}

void Bootloader_Branch_Judge(void)
{
    if(BootLoader_Enter_Notify(6) == 0) // If time is up and 'w' is not pressed, enter normal mode
    {
        if(ota_info.ota_flag == OTA_UPDATE_FLAG) // If not entering command line mode, check whether to update app area, using state machine
        {
            U0_MyPrintf("New program arrived, preparing for OTA update\r\n");
            BootStatusFlag |= UPDATA_A_FLAG; // Set flag indicating need to update app area
            updataA.W25Q64_BLOCK_NUMBER = 0; // The new app code for updating area A comes from the beginning of w25q64
        }
        else // If not updating app, jump to execute in area A
        {
            U0_MyPrintf("\r\n[You did not make a choice], preparing to jump to app to execute the program\r\n");
            LOAD_App(GD32_A_AREA_START_ADDR);
        }
    }
    // If normally jumping to load_app function to execute app area code, the next two lines will not be executed
    U0_MyPrintf("\r\n!!! Bootloader command line mode!!!\r\n");
    Print_BootLoader_Control_Info();   
}



/*
    The main function of this program is to set a new main stack pointer (MSP) in the bootloader, preparing to jump to the application program.
*/
__ASM void MSR_SP(uint32_t addr) // addr parameter will be automatically stored in r0 register
{
    MSR MSP, r0  // Write the value in r0 (which is addr) into MSP (main stack pointer)
    BX r14      // Return to the calling place to continue execution
}



/*
    @brief Jump from 0x08000000 initial_sp to the very beginning of the app area, the sp pointer points to the position of ram stack, so sp has a range
    This function sets the new position of the sp pointer and the pc pointer.
*/
void LOAD_App(uint32_t addr)
{
    if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004fff)) // For safety, check if the app address is within the range of ram
    {
        MSR_SP(*(uint32_t *)addr); // addr points to the start position of the APP vector table
        my_app = (load_a)*(uint32_t *)(addr+4);
        BootLoader_Clear(); // bl is an independent program, so it needs to clear the configuration in the main function to avoid interfering with the app
        my_app(); // This step mainly sets the position of the pc pointer
    } else {
        U0_MyPrintf("\r\nJump to App area failed, entering command line mode to reselect\r\n");
    }
}



void BootLoader_Clear(void)
{
    usart_deinit(USART0);
    gpio_deinit(GPIOA);
    gpio_deinit(GPIOB);
}    



/*
    @brief Manually calculate the Xmodem CRC16 checksum
    @param data The starting address of the data, data is read byte by byte    
    @param len Data length
    @return Returns the calculated CRC16 checksum
*/
uint16_t Xmodem_CRC16(uint8_t *data, uint16_t datalen)
{
    uint8_t i;
    uint16_t cirInit = 0x0000;
    uint16_t crcPoly = 0x1021;
    while(datalen--)
    {
        cirInit = (*data << 8) ^ cirInit;
        for (i = 0; i < 8; i++)
        {
            if (cirInit & 0x8000)
            {
                cirInit = (cirInit << 1) ^ crcPoly;
            }
            else
            {
                cirInit = cirInit << 1;
            }
        }
        data++;
    }
    return cirInit;
}

