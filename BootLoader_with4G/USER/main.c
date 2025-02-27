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
#include "4g.h"

OTA_CONTROL_INFO ota_info;
UPDATA_A_CONTROL_INFO updataA;
uint32_t BootStatusFlag;

int main(void)
{  
    uint8_t i = 0;
    Deley_Init();
    Usart2_Init(115200);
    Usart0_Init(921600);
    IIC_Init();// Must initialize to read the structure information inside 24c02
    W25Q64_Init();
    M24C02_ReadOTAInfo();
    Bootloader_Branch_Judge();
    GM_4G_Init();// When powered on, gd32 resets, this function synchronizes the reset of the 4G module, and this reset takes some time inside the 4G module
    
    
    
    
    while(1){
        /*----------------------------------------  */
        /*  Delay, used for sending 'C' during xmodem protocol to count intervals    */
        /*----------------------------------------  */
        Delay_Ms(10);


        /*-------------------------------------- */
        /*  Process the content in UART0 receive buffer         */
        /*-------------------------------------- */
        if(U0_ControlBlock.U_RxDataOUT != U0_ControlBlock.U_RxDataIN) // Check what command the user selected and execute it
        {   
			BootLoaderCommandChoose(U0_ControlBlock.U_RxDataOUT->start,U0_ControlBlock.U_RxDataOUT->end - U0_ControlBlock.U_RxDataOUT->start + 1);			
			U0_ControlBlock.U_RxDataOUT++;
		    if(U0_ControlBlock.U_RxDataOUT == U0_ControlBlock.U_RxDataEND)
            {
			    U0_ControlBlock.U_RxDataOUT = &U0_ControlBlock.U_RxDataPtr[0];// Previously written as &U0_ControlBlock.U_RxDataOUT[0], which is incorrect
			}
		}
        /*-------------------------------------- */
        /*  Process the content in UART2 receive buffer         */
        /*-------------------------------------- */
        if(U2_ControlBlock.U_RxDataOUT != U2_ControlBlock.U_RxDataIN) 
        {   	
            U0_MyPrintf("\r\nUART2 received %d bytes of data:\r\n",U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1);
            for(uint8_t i = 0;i < U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1;i++)
                U0_MyPrintf("%c",U2_ControlBlock.U_RxDataOUT->start[i]);

            U2_Event(U2_ControlBlock.U_RxDataOUT->start,U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1); // Call this function to process UART2 data separately

			U2_ControlBlock.U_RxDataOUT++;
		    if(U2_ControlBlock.U_RxDataOUT == U2_ControlBlock.U_RxDataEND)
            {
			    U2_ControlBlock.U_RxDataOUT = &U2_ControlBlock.U_RxDataPtr[0];// Previously written as &U2_ControlBlock.U_RxDataOUT[0], which is incorrect
			}
		}





        if(BootStatusFlag & XMODEM_SEND_C_FLAG) // This XMODEM_SEND_C_FLAG is set in the BootLoaderCommandChoose function
        {
            // Send 'C' character to prepare for xmodem protocol
            if(updataA.Xmodem_Timer >= 100)
            {
                U0_MyPrintf("C");
                updataA.Xmodem_Timer = 0;
            }
            updataA.Xmodem_Timer++;
        }



       if(BootStatusFlag & UPDATA_A_FLAG) // This UPDATA_A_FLAG is set in the Bootloader_Branch_Judge function
       {
           // Start updating the app area
           U0_MyPrintf("The size of the firmware being updated is: %d byte\r\n",ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER]);
           
           if(ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] % 4 == 0) // Internal flash can only be written in multiples of 4 bytes
           {
               GD32_EraseFalash(GD32_A_AREA_START_PAGE, GD32_PAGE_A_AREA_NUM);// Must erase the app area first, otherwise writing will fail
               for(i = 0;i < ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] / GD32_ONE_PAGE_BYTESIZE; ++i)
               {
                   W25Q64_ReadData(updataA.updata_buffer, i * 1024 + updataA.W25Q64_BLOCK_NUMBER * 64 * 1024, GD32_ONE_PAGE_BYTESIZE);// Writing to flash must be done in RAM first, can write from external flash to internal flash at once
                   GD32_WriteFlash(GD32_A_AREA_START_ADDR + i * GD32_ONE_PAGE_BYTESIZE, (uint32_t *)updataA.updata_buffer,GD32_ONE_PAGE_BYTESIZE); // When writing to internal flash, the underlying function writes 4 bytes at a time
               }
               if(ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] % 1024 != 0) // Write the remaining space to the app area
               {
                   W25Q64_ReadData(updataA.updata_buffer, i * 1024 + updataA.W25Q64_BLOCK_NUMBER * 64 * 1024, ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] % 1024);
                   GD32_WriteFlash(GD32_A_AREA_START_ADDR + i * GD32_ONE_PAGE_BYTESIZE,(uint32_t *)updataA.updata_buffer ,ota_info.FireLen[updataA.W25Q64_BLOCK_NUMBER] % 1024); 
               }
               if(updataA.W25Q64_BLOCK_NUMBER == 0)
               {
                   ota_info.ota_flag = 0; // If it is the starting area of external flash, it indicates that this is the code we want, set the flag to 0 directly
                   M24C02_WriteOTAInfo();// Write the new ota_info to 24c02
               }
               U0_MyPrintf("\r\nFirmware update completed, system will restart\r\n");
               NVIC_SystemReset();// Restart
           }
           else
           {
               U0_MyPrintf("\r\nThe length of this block is not a multiple of 4, cannot be written to the app area!\r\n");
               BootStatusFlag &= ~UPDATA_A_FLAG;// After checking, clear the flag to prevent re-entry               
           }
       }
            
    }
}
