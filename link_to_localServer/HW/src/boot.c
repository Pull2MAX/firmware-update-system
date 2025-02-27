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
    @brief Select the command in command line mode. This function is placed at the serial port reception of the main function.
    @param command_number Command in command line mode
    @param datalen Command length in command line mode to prevent it from being our command
*/
void BootLoaderCommandChoose(uint8_t *data, uint16_t datalen)
{  
    if(BootStatusFlag == 0)
    {
        if((datalen == 1) && (data[0] == '1'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to erase all APP area firmware (remember to select -7 to restart later)\r\n", data[0]);
            GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM); // Erase a area
        }
        else if((datalen == 1) && (data[0] == '2'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], preparing to update app firmware via xmodem protocol, please use bin file\r\n",data[0]);
            GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM); // Erase a area
            BootStatusFlag |= (XMODEM_SEND_C_FLAG | XMODEM_DATA_FLAG); // Set the flag to indicate that C character and data need to be sent
            updataA.Xmodem_Timer = 0;
            updataA.Xmodem_Number = 0; 
        }
        else if((datalen == 1) && (data[0] == '3'))
        {
            U0_MyPrintf("\r\nYou have selected [%c], please set the version number for this update\r\n", data[0]);            
            BootStatusFlag |= (SET_VERSION_FLAG); // Set the flag to indicate that C character and data need to be sent
        }
        else if((datalen == 1) && (data[0] == '4'))
        {
            M24C02_ReadOTAInfo();
            U0_MyPrintf("\r\nYou have selected [%c], the queried version number is:%s\r\n", data[0], ota_info.OTA_VERSION);  
            U0_MyPrintf("\r\nPlease select other command to execute\r\n");   
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
        if((datalen == (128+5)) && (data[0] == 0x01)) //The data packet comes through the serial port, 0x01 needs to ensure that the starting frame is 0x01
        {
            BootStatusFlag &= ~XMODEM_SEND_C_FLAG; //Clear the flag bit, indicating that there is no need to send the C character
            updataA.Xmodem_CRC_Result = Xmodem_CRC16(&data[3], 128);//The handwritten function crc only calculates crc from the real data starting from the subscript of 3
            
            if(updataA.Xmodem_CRC_Result == data[131] * 256 + data[132]) //If the crc check is correct, send ack
            {
                //Start writing 128 bytes of data into the buffer dedicated to updating the firmware area (updataA.updata)
                updataA.Xmodem_Number++; //Indicates the sequence number of the received packet. 8 packets make up an updata_buffer and write to the internal flash once. 8*128=1024 bytes
                memcpy(&updataA.updata_buffer[((updataA.Xmodem_Number - 1) % (GD32_ONE_PAGE_BYTESIZE / 128)) * 128], &data[3], 128); //It means that 128 after the address with subscript 3 received from the serial port is written to the corresponding position of updata_buffer, and 128 bytes are moved each time
                if((updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) == 0) //Note that the branch where the code is located does not have any loop structure, so after 8 packets are collected, you need to manually write to the internal flash
                {
                    
                    if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG) //If the flag bit is stored in w25q64, then write to w25q64
                    {
                        //w25q64 is a page of 256 bytes, and the previous if is entered after collecting 1024 bytes
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            W25Q64_PageWrite(&updataA.updata_buffer[i * 256], ((updataA.W25Q64_BLOCK_NUMBER * 64 * 1024 / 256) + (updataA.Xmodem_Number/8 - 1) * 4 + i)); // Block of external flash + offset within the block
                        }                       
                    }

                    //If the flag bit is not stored in w25q64, then write to the internal flash
                    else GD32_WriteFlash(GD32_A_AREA_START_ADDR + ((updataA.Xmodem_Number / (GD32_ONE_PAGE_BYTESIZE / 128)) - 1) * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer, GD32_ONE_PAGE_BYTESIZE); //When writing to the internal flash, the underlying function writes 4 bytes at a time                    
                } 
                U0_MyPrintf("\x06");               
            }
            else//crc check is incorrect, send nack
            {
                U0_MyPrintf("\x15");
            }
        }
        if((datalen == 1) && (data[0] == 0x04)) // Indicates that the end frame (0x04) has been received
        {
            U0_MyPrintf("\x06");
            
            if((updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) != 0)//If there are remaining packets, continue to write to flash. For example, if there are ten packets, the first 8 have already been written to flash above, and there are 2 packets left. This branch is responsible for writing the remaining packets.
            {
                if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG)//Not finished. Continue sending.
                {
                    for(uint8_t i = 0; i < 4; i++) // It will write a little more to the external flash, but it doesn't matter
                    {
                        W25Q64_PageWrite(&updataA.updata_buffer[i * 256], ((updataA.W25Q64_BLOCK_NUMBER * 64 * 1024 / 256) + (updataA.Xmodem_Number/8) * 4 + i));
                    }                   
                }

                else GD32_WriteFlash(GD32_A_AREA_START_ADDR + ((updataA.Xmodem_Number / (GD32_ONE_PAGE_BYTESIZE / 128))) * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer, (updataA.Xmodem_Number % (GD32_ONE_PAGE_BYTESIZE / 128)) * 128); //When writing to the internal flash, the underlying function writes 4 bytes at a time
            }
            BootStatusFlag &= ~XMODEM_DATA_FLAG;//The operation is unified, and the flag bit is cleared immediately after the task is completed

            if(BootStatusFlag & XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG)
            {
                BootStatusFlag &= ~XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG;
                ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] = updataA.Xmodem_Number * 128; //You need to record the number of bytes in the file length member
                M24C02_WriteOTAInfo(); // Write to 24c02
                Delay_Ms(1000);
                Print_BootLoader_Control_Info();
            } 
            else 
            {
                Delay_Ms(100);
                NVIC_SystemReset();//Restart
            }

        }
    }
    
    
    else if(BootStatusFlag & SET_VERSION_FLAG)
    {
        if(datalen == 26)//Here, directly determine the length of the version. Although 32 bytes are reserved, only 26 bytes are used.
        {
            int temp_for_version;
            if(sscanf((char *)data,"VER-%d.%d.%d-%d/%d/%d-%d:%d",&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version,&temp_for_version) == 8)
            {
                memset(ota_info.OTA_VERSION, 0, 32); //First clear the version number. A total of 32 bytes are given to store the version information.
                memcpy(ota_info.OTA_VERSION, data, 26); //Although there are 32 bytes to store version information, only 26 bytes are used.
                M24C02_WriteOTAInfo();
                U0_MyPrintf("\r\nVersion number set successfully![Please select other command to execute!]\r\n");
                BootStatusFlag &= ~SET_VERSION_FLAG; // Clear the flag bit, indicating that there is no need to set the version number
                Print_BootLoader_Control_Info();
            }
            else U0_MyPrintf("\r\nVersion number format error, please re-enter\r\n");
        }
        else U0_MyPrintf("\r\nVersion number length error, should be 26 characters, please re-enter\r\n");
    }

    else if(BootStatusFlag & STORE_APP_TO_W25Q64_FLAG)
    {
        if(datalen == 1)
        {
            if(data[0] >= 0x31 && data[0] <= 0x39)
            {
                updataA.W25Q64_BLOCK_NUMBER = data[0] - 0x30;
                BootStatusFlag |= (XMODEM_SEND_C_FLAG | XMODEM_DATA_FLAG | XMODEM_FOR_STORE_APP_TO_W25Q64_FLAG); //Setting these three flag bits is because if we need to send the bin file to w25q64 through the xmodem protocol, we need to send c first, and then send data.
                updataA.Xmodem_Timer = 0;
                updataA.Xmodem_Number = 0;//Here, it is updated to the external flash, here is just borrowing the members in the structure of updating the internal flash.
                ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] = 0;
                W25Q64_Erase64k(updataA.W25Q64_BLOCK_NUMBER); // Because it is to be written to the external flash, you must manually clear the specified block here.
                U0_MyPrintf("\r\nYou have selected the block number of w25q64:%d\r\n\r\nBlock has been cleared, please immediately select the bin file to send through xmodem protocol to w25q64\r\n", updataA.W25Q64_BLOCK_NUMBER);
                BootStatusFlag &= ~STORE_APP_TO_W25Q64_FLAG;
            }
            else U0_MyPrintf("Please enter 1--9!\r\n");
        }
        else U0_MyPrintf("Data length input error! Need to enter 1--9!\r\n");
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
        else U0_MyPrintf("Data length input error! Need to enter 1--9!\r\n");
    }

    else if(BootStatusFlag & SET_SERVER_INFO_FLAG)
    {
        U2_MyPrintf("AT+SOCKA=%s\r\n", data);//This means directly putting the content received by serial port 2 after the at command, and then handing it over to the 4g module for execution.
        Delay_Ms(30);
        U2_MyPrintf("AT+SOCKAEN=ON\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKBEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKCEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+SOCKDEN=OFF\r\n");
		Delay_Ms(30);
		U2_MyPrintf("AT+HEART=ON,NET,USER,60,C000\r\n");//In order to link to the Ali cloud service later, it means sending a heartbeat packet every 60 seconds.
		Delay_Ms(30);
        U0_MyPrintf("---AT command set successfully, will restart---\r\n");
		U2_MyPrintf("AT+S\r\n");//Save the at command and restart.
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
    if(BootLoader_Enter_Notify(6) == 0) //If the time has passed and w has not been pressed, enter the normal mode.
    {
        if(ota_info.ota_flag == OTA_UPDATE_FLAG) // If you do not enter the command line mode, check whether to update the app area, using the state machine.
        {
            U0_MyPrintf("New program coming, preparing ota update\r\n");
            BootStatusFlag |= UPDATA_A_FLAG; //Set the flag bit to indicate that the app area needs to be updated.
            updataA.W25Q64_BLOCK_NUMBER = 0; //The new app code for updating the a area comes from the beginning of w25q64.
        }
        else//If you don't want to update the app, go to the a partition to execute.
        {
            U0_MyPrintf("\r\n[You didn't make a choice], preparing to jump to app to execute program\r\n");
            LOAD_App(GD32_A_AREA_START_ADDR);
        }
    }
    //If you normally jump to the load_app function to execute the code in the app area, the next two lines will not be executed.
    U0_MyPrintf("\r\n!!!bootloader command line mode!!!\r\n");
    Print_BootLoader_Control_Info();   
}



/*
    The main function of this program is to set a new main stack pointer (MSP) in the bootloader to prepare for jumping to the application.
*/
__ASM void MSR_SP(uint32_t addr) // The addr parameter will be automatically stored in the r0 register.
{
    MSR MSP, r0  // Write the value in r0 (that is, addr) to MSP (main stack pointer)
    BX r14      // Return to the call site to continue execution
}



/*
    @brief Jump the initial_sp of 0x08000000 to the beginning of the app area. The position pointed to by the sp pointer is the stack of ram, so sp has a range.
    The function of this function is to set the new position of the sp pointer and the position of the pc pointer.
*/
void LOAD_App(uint32_t addr)
{
    if((*(uint32_t *)addr >= 0x20000000) && (*(uint32_t *)addr <= 0x20004fff) )//For safety, check whether the address of the app is within the range of ram.
    {
        MSR_SP(*(uint32_t *)addr); // addr points to the starting position of the APP vector table
        my_app = (load_a)*(uint32_t *)(addr+4);
        BootLoader_Clear();// bl is an independent program, so you need to clear the configuration in the main function, do not interfere with the app
        my_app();//This step is mainly to set the position of the pc pointer.
    }else{
        U0_MyPrintf("\r\nJump App partition failed, will enter command line mode to re-select\r\n");
    }
}



void BootLoader_Clear(void)
{
    usart_deinit(USART0);
    gpio_deinit(GPIOA);
    gpio_deinit(GPIOB);
}    



/*
    @brief Manually calculate the CRC16 checksum of Xmodem
    @param data The first address of the data, the data is read byte by byte
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

