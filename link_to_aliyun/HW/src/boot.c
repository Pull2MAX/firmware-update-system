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

/*
    @brief Enter the bootloader command line immediately by entering [lowercase w] within %d seconds, otherwise execute App immediately
    @param wait_second_time Time to wait in seconds
    @return 1 if entered, 0 if not
*/
uint8_t BootLoader_Enter_Notify(uint8_t wait_second_time)
{
    U0_MyPrintf("\r\n-----%d seconds to enter [lowercase w] to immediately enter bootloader command line, otherwise execute App immediately-----\r\n",wait_second_time);
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
    @param datalen Command length in command line mode, to prevent it from being an invalid command
*/
void BootLoaderCommandChoose(uint8_t *data, uint16_t datalen)
{  
    if(BootStatusFlag == 0)
    {
        if((datalen == 1) && (data[0] == '1'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to erase all APP area firmware (remember to select -7 to restart later)\r\n", data[0]);
            GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM); // Erase area A
        }
        else if((datalen == 1) && (data[0] == '2'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to update app firmware via xmodem protocol, please use bin file\r\n",data[0]);
            GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM); // Erase area A
            BootStatusFlag |= (XMODEM_SEND_C_FLAG | XMODEM_DATA_FLAG); // Set flags for sending C character and data
            updataA.Xmodem_Timer = 0;
            updataA.Xmodem_Number = 0; 
        }
        else if((datalen == 1) && (data[0] == '3'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], please set the version number for this update\r\n", data[0]);            
            BootStatusFlag |= (SET_VERSION_FLAG); // Set flag, indicating need to send C character and data
        }
        else if((datalen == 1) && (data[0] == '4'))
        {
            M24C02_ReadOTAInfo();
            U0_MyPrintf("\r\nYou have selected [%c]\r\nThe queried version number is: %s\r\n", data[0], ota_info.OTA_VERSION);  
            U0_MyPrintf("\r\nPlease choose other commands to execute\r\n");   
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
        if((datalen == (128+5)) && (data[0] == 0x01)) // Data packet received via serial port, 0x01 ensures start frame is 0x01
        {
            BootStatusFlag &= ~XMODEM_SEND_C_FLAG; // Clear flag, indicating no need to send C character
            updataA.Xmodem_CRC_Result = Xmodem_CRC16(&data[3], 128);// Custom CRC function only calculates CRC for real data starting from index 3
            
            if(updataA.Xmodem_CRC_Result == data[131] * 256 + data[132]) // If CRC check is correct, send ACK
            {
                // Start writing 128 bytes to the dedicated update buffer (updataA.updata)
                updataA.Xmodem_Number++; // Indicates packet sequence number, 8 packets fill up one updata_buffer to write to internal flash, 8*128=1024 bytes
                memcpy(&updataA.updata_buffer[((updataA.Xmodem_Number - 1) % (GD32_ONE_PAGE_BYTESIZE / 128)) * 128], &data[3], 128); // Copy 128 bytes from index 3 of received data to corresponding position in updata_buffer
                if((updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) == 0) // Note this branch has no loop structure, so after 8 packets are filled, need to manually write to internal flash
                {
                    
                    if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG) // If flag is for storing to w25q64, then write to w25q64
                    {
                        // w25q64 is 256 bytes per page, previous if condition fills 1024 bytes
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            W25Q64_PageWrite(&updataA.updata_buffer[i * 256], ((updataA.W25Q64_BLOCK_NUMBER * 64 * 1024 / 256) + (updataA.Xmodem_Number/8 - 1) * 4 + i)); // External flash block + block offset
                        }                       
                    }

                    // If flag is not for storing to w25q64, then write to internal flash
                    else GD32_WriteFlash(GD32_A_AREA_START_ADDR + ((updataA.Xmodem_Number / (GD32_ONE_PAGE_BYTESIZE / 128)) - 1) * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer, GD32_ONE_PAGE_BYTESIZE); // When writing internal flash, underlying function writes 4 bytes at a time                    
                } 
                U0_MyPrintf("\x06");               
            }
            else// If CRC check fails, send NACK
            {
                U0_MyPrintf("\x15");
            }
        }
        if((datalen == 1) && (data[0] == 0x04)) // Indicates end frame (0x04) received
        {
            U0_MyPrintf("\x06");
            
            if((updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) != 0)// If there are remaining packets, continue writing to flash, e.g. 10 packets, first 8 already written above, remaining 2 handled here
            {
                if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG)// Not finished yet. Continue sending.
                {
                    for(uint8_t i = 0; i < 4; i++) // May write a bit more to external flash but it's fine 
                    {
                        W25Q64_PageWrite(&updataA.updata_buffer[i * 256], ((updataA.W25Q64_BLOCK_NUMBER * 64 * 1024 / 256) + (updataA.Xmodem_Number/8) * 4 + i));
                    }                   
                }

                else GD32_WriteFlash(GD32_A_AREA_START_ADDR + ((updataA.Xmodem_Number / (GD32_ONE_PAGE_BYTESIZE / 128))) * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer, (updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) * 128); // When writing internal flash, underlying function writes 4 bytes at a time
            }
            BootStatusFlag &= ~XMODEM_DATA_FLAG;// Unified operation, clear flag immediately after task is done

            if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG)
            {
                BootStatusFlag &= ~XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG;
                ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] = updataA.Xmodem_Number * 128; // Need to record byte count in file length member
                M24C02_WriteOTAInfo(); // Write to 24c02
                Delay_Ms(1000);
                Print_BootLoader_Control_Info();
            } 
            else 
            {
                Delay_Ms(100);
                NVIC_SystemReset();// Restart  
            }

        }
    }
    
    
    else if(BootStatusFlag & SET_VERSION_FLAG)
    {
        if(datalen == 26) // Directly check the length of the version, although 32 bytes are reserved, only 26 bytes are used
        {
            int temp_for_version;
            if(sscanf((char *)data,"VER-%d.%d.%d-%d/%d/%d-%d:%d",&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version) == 8)
            {
                memset(ota_info.OTA_VERSION, 0, 32); // First clear the version number, a total of 32 bytes are allocated to store version information
                memcpy(ota_info.OTA_VERSION, data, 26); // Although 32 bytes are allocated to store version information, only 26 bytes are used
                M24C02_WriteOTAInfo();
                U0_MyPrintf("\r\nVersion number set successfully![Please choose other commands to execute!]\r\n");
                BootStatusFlag &= ~SET_VERSION_FLAG; // Clear the flag, indicating that there is no need to set the version number
                Print_BootLoader_Control_Info();
            }
            else U0_MyPrintf("\r\nVersion number format error, please re-enter\r\n");
        }
        else U0_MyPrintf("\r\nVersion number length error, it should be 26 characters, please re-enter\r\n");
    }

    else if(BootStatusFlag & STORE_APP_TO_W25Q64_FLAG)
    {
        if(datalen == 1)
        {
            if(data[0] >= 0x31 && data[0] <= 0x39)
            {
                updataA.W25Q64_BLOCK_NUMBER = data[0] - 0x30;
                BootStatusFlag |= (XMODEM_SEND_C_FLAG | XMODEM_DATA_FLAG | XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG); // Set these three flags because we need to send the bin file to w25q64 via xmodem protocol, first send c, then send data
                updataA.Xmodem_Timer = 0;
                updataA.Xmodem_Number = 0; // Here is updating to external flash, borrowing the members in the internal flash update structure
                ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] = 0;
                W25Q64_Erase64k(updataA.W25Q64_BLOCK_NUMBER); // Since it is to be written to external flash, the specified block must be manually cleared first
                U0_MyPrintf("\r\nYou selected the block number of w25q64: %d\r\n\r\nBlock has been cleared, please immediately choose the bin file to be stored through xmodem protocol\r\n", updataA.W25Q64_BLOCK_NUMBER);
                BootStatusFlag &= ~STORE_APP_TO_W25Q64_FLAG;
            }
            else U0_MyPrintf("Please enter 1--9!\r\n");
        }
        else U0_MyPrintf("Data length input error! It needs to be 1--9!\r\n");
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
            else U0_MyPrintf("Please enter 1--9!\r\n");
        }
        else U0_MyPrintf("Data length input error! It needs to be 1--9!\r\n");
    }

    else if(BootStatusFlag & SET_SERVER_INFO_FLAG)
    {
        U2_MyPrintf("AT+SOCKA=%s\r\n", data); // This means directly appending the content received from serial port 2 to the AT command, and then handing it over to the 4G module for execution
        Delay_Ms(30);
        U2_MyPrintf("AT+SOCKAEN=ON\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKBEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKCEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKDEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+HEART=ON,NET,USER,60,C000\r\n"); // For connecting to Alibaba Cloud service later, meaning sending a heartbeat packet every 60 seconds
		Delay_Ms(30);
        U0_MyPrintf("---AT command set successfully, about to restart---\r\n");
		U2_MyPrintf("AT+S\r\n"); // Save the AT command, then restart
		Delay_Ms(30);
        BootStatusFlag &= ~SET_SERVER_INFO_FLAG;
    }
    
}

void Print_BootLoader_Control_Info(void)
{
	U0_MyPrintf("\r\n");
	U0_MyPrintf("[1]Erase all APP area firmware\r\n");
	U0_MyPrintf("[2]Serial IAP download A area program\r\n");
	U0_MyPrintf("[3]Set OTA version number\r\n");
	U0_MyPrintf("[4]Query OTA version number\r\n");
	U0_MyPrintf("[5]Download program to external Flash\r\n");
	U0_MyPrintf("[6]Use external Flash program\r\n");
	U0_MyPrintf("[7]Restart\r\n");
    U0_MyPrintf("[8]Set server connection information\r\n");
}

void Bootloader_Branch_Judge(void)
{
    if(BootLoader_Enter_Notify(6) == 0) // If the time has passed and w has not been pressed, enter normal mode
    {
        if(ota_info.ota_flag == OTA_UPDATE_FLAG) // If not entering command line mode, check whether to update the app area, using a state machine
        {
            U0_MyPrintf("New program coming, preparing ota update\r\n");
            BootStatusFlag |= UPDATA_A_FLAG; // Set the flag to indicate that the app area needs to be updated
            updataA.W25Q64_BLOCK_NUMBER = 0; // The new app code for updating area A comes from the beginning of w25q64
        }
        else // If not updating the app, jump to execute in area A
        {
            U0_MyPrintf("\r\n[You did not make a choice], preparing to jump to app and execute the program\r\n");
            LOAD_App(GD32_A_AREA_START_ADDR);
        }
    }
    // If normally jumping to the load_app function to execute the app area code, the next two lines will not be executed
    U0_MyPrintf("\r\n!!! Bootloader command line mode !!!\r\n");
    Print_BootLoader_Control_Info();   



/*
    The main function of this program is to set the new Main Stack Pointer (MSP) in the bootloader, preparing for jumping to the application.
*/
__ASM void MSR_SP(uint32_t addr) // addr parameter will be automatically stored in r0 register
{
    MSR MSP, r0  // Write the value in r0 (which is addr) to MSP (Main Stack Pointer)
    BX r14      // Return to the caller to continue execution
}



/*
    @brief Jump from 0x08000000's initial_sp to the start position of the app area. The sp pointer points to the RAM stack, so sp has a range
    This function sets the new position of the sp pointer and the position of the pc pointer
*/
void LOAD_App(uint32_t addr)
{
    if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004fff)) // For safety, check if the app address is within RAM range
    {
        MSR_SP(*(uint32_t *)addr); // addr points to the start of the APP vector table
        my_app = (load_a)*(uint32_t *)(addr+4);
        BootLoader_Clear(); // bl is an independent program, so we need to clear the configuration in the main function to avoid interfering with the app
        my_app(); // This step mainly sets the position of the pc pointer
    }else{
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
    @param data Start address of the data, data is read byte by byte    
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

