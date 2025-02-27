#include "gd32f10x.h"
#include "main.h"
#include "usart.h"
#include "delay.h"
#include "m24c02.h"
#include "iic.h"
#include "spi.h"
#include "w25q64.h"
#include "fmc.h"
#include "4g.h"

//uint32_t write_arr[1024];//fmc底层的函数要求一次需要先写四个字节因此是uint32，一个页的大小是1kb = 1024byte，本次操作的是4个page 

OTA_CONTROL_INFO ota_info;
UPDATA_A_CONTROL_INFO updataA;
uint32_t BootStatusFlag;

int main(void)
{  
    uint8_t i = 0;
    Deley_Init();
    Usart2_Init(115200);
    Usart0_Init(921600);
    IIC_Init(); // To read the structure information inside 24c02, initialization is necessary
    W25Q64_Init();
    M24C02_ReadOTAInfo();
    GM_4G_Init(); // When powered on, gd32 resets, this function synchronously resets the 4G module, this reset requires some time inside the 4G module
    U0_MyPrintf("version:6.0.0\r\n");
    while(1)
    {
        /*-------------------------------------- */
        /*  Process the content in the receive buffer of Serial Port 2         */
        /*-------------------------------------- */
        if(U2_ControlBlock.U_RxDataOUT != U2_ControlBlock.U_RxDataIN) // Check what command the user selected and execute
        {   	
            U0_MyPrintf("\r\nSerial Port 2 received %d bytes of data:\r\n", U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1);
//            for(uint8_t i = 0;i < U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1;i++)
//                U0_MyPrintf("%c",U2_ControlBlock.U_RxDataOUT->start[i]);

            U2_Event(U2_ControlBlock.U_RxDataOUT->start,U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1); // This function is called to process the data from Serial Port 2

			U2_ControlBlock.U_RxDataOUT++;
		    if(U2_ControlBlock.U_RxDataOUT == U2_ControlBlock.U_RxDataEND)
            {
			    U2_ControlBlock.U_RxDataOUT = &U2_ControlBlock.U_RxDataPtr[0]; // Previously written as &U2_ControlBlock.U_RxDataOUT[0] was incorrect
			}
		}
   
    }
}
