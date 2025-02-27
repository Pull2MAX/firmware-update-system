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

// uint32_t write_arr[1024]; // The FMC low-level function requires writing four bytes at a time, hence uint32_t. The size of one page is 1KB = 1024 bytes, this operation is for 4 pages.

OTA_CONTROL_INFO ota_info;
UPDATA_A_CONTROL_INFO updataA;
uint32_t BootStatusFlag;

int main(void)
{  
    Delay_Init();
    Usart2_Init(115200);
    Usart0_Init(921600);
    IIC_Init(); // Must initialize to read the structure information inside 24C02
    W25Q64_Init();
    M24C02_ReadOTAInfo();
    GM_4G_Init(); // When powered on, GD32 resets, this function synchronously resets the 4G module, this reset requires some time inside the 4G module
    U0_MyPrintf("version:6.0.0\r\n");
    while(1)
    {
        /*-------------------------------------- */
        /*  Process the content in the receive buffer of UART2         */
        /*-------------------------------------- */
        if(U2_ControlBlock.U_RxDataOUT != U2_ControlBlock.U_RxDataIN) // Check what command the user selected and execute it
        {   	
            U0_MyPrintf("\r\nUART2 received %d bytes of data:\r\n", U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1);
            for(uint8_t i = 0; i < U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1; i++)
                U0_MyPrintf("%x", U2_ControlBlock.U_RxDataOUT->start[i]);
            U0_MyPrintf("\r\n");

            U2_Event(U2_ControlBlock.U_RxDataOUT->start, U2_ControlBlock.U_RxDataOUT->end - U2_ControlBlock.U_RxDataOUT->start + 1); // Call this function to process UART2 data

            U2_ControlBlock.U_RxDataOUT++;
            if(U2_ControlBlock.U_RxDataOUT == U2_ControlBlock.U_RxDataEND)
            {
                U2_ControlBlock.U_RxDataOUT = &U2_ControlBlock.U_RxDataPtr[0]; // Previously written as &U2_ControlBlock.U_RxDataOUT[0], which is incorrect
            }
        }

        /*-------------------------------------- */
        /*  Process the content in the send buffer of UART2         */
        /*-------------------------------------- */
        if((U2_ControlBlock.U_TxDataOUT != U2_ControlBlock.U_TxDataIN) && (!U2_ControlBlock.U_TxStatus)) // Check what command the user selected and execute it
        {   	
            U0_MyPrintf("\r\nUART2 has %d bytes of data to send:\r\n", U2_ControlBlock.U_TxDataOUT->end - U2_ControlBlock.U_TxDataOUT->start + 1);
            for(uint8_t i = 0; i < U2_ControlBlock.U_TxDataOUT->end - U2_ControlBlock.U_TxDataOUT->start + 1; i++)
                U0_MyPrintf("%02x", U2_ControlBlock.U_TxDataOUT->start[i]);
            U0_MyPrintf("\r\n");
            // Prepare to start sending with DMA, but memory_addr and num need to be modified during sending
            U2_ControlBlock.U_TxStatus = 1; // Set to 1, indicating sending is in progress
            dma_memory_address_config(DMA0, DMA_CH1, (uint32_t)U2_ControlBlock.U_TxDataOUT->start); // Each time receiving starts from the start position
            dma_transfer_number_config(DMA0, DMA_CH1, U2_ControlBlock.U_TxDataOUT->end - U2_ControlBlock.U_TxDataOUT->start + 1);
            dma_channel_enable(DMA0, DMA_CH1); // Only enable DMA and channel here
            
            U2_ControlBlock.U_TxDataOUT++;
            if(U2_ControlBlock.U_TxDataOUT == U2_ControlBlock.U_TxDataEND)
            {
                U2_ControlBlock.U_TxDataOUT = &U2_ControlBlock.U_TxDataPtr[0]; // Previously written as &U2_ControlBlock.U_TxDataOUT[0], which is incorrect
            }
        }
    }
}
