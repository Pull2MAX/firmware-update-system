/*!
    \file    gd32f10x_it.c
    \brief   interrupt service routines

    \version 2024-01-05, V2.3.0, firmware for GD32F10x
*/

/*
    Copyright (c) 2024, GigaDevice Semiconductor Inc.

    Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    1. Redistributions of source code must retain the above copyright notice, this 
       list of conditions and the following disclaimer.
    2. Redistributions in binary form must reproduce the above copyright notice, 
       this list of conditions and the following disclaimer in the documentation 
       and/or other materials provided with the distribution.
    3. Neither the name of the copyright holder nor the names of its contributors 
       may be used to endorse or promote products derived from this software without 
       specific prior written permission.

    THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT 
NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY 
OF SUCH DAMAGE.
*/
#include "gd32f10x_it.h"




#include "usart.h"
void USART2_IRQHandler(void)
{
    if(usart_interrupt_flag_get(USART2, USART_INT_FLAG_IDLE) != 0)//Regular operation, first check if it is an idle interrupt, then clear it!
    {
        usart_flag_get(USART2, USART_FLAG_IDLEF);//According to the data manual, the first step is to read this STAT register, and the second step is to read the data register to clear the flag. As long as there is a reading process, it is fine.
        usart_data_receive(USART2);//If the DATA register is also read, the idle flag is cleared.
        
        U2_ControlBlock.U_RxCounter += (U2_RX_MAX + 1) - dma_transfer_number_get(DMA0, DMA_CH2);//Because U_RxCounter expects to get the total number of bytes received,
                                                            //but dma_transfer_number_get returns the number of bytes that have not been filled, so a subtraction is needed to calculate the actual number of bytes received.
        U2_ControlBlock.U_RxDataIN->end = &U2_RxBuff[U2_ControlBlock.U_RxCounter - 1];
        
        U2_ControlBlock.U_RxDataIN++;//Point to the next area.
        
        if(U2_ControlBlock.U_RxDataIN == U2_ControlBlock.U_RxDataEND)//If it moves to the end of the array, return to the head.
        {
            U2_ControlBlock.U_RxDataIN = &U2_ControlBlock.U_RxDataPtr[0];
        }
        
        if(U2_RX_BUF_SIZE - U2_ControlBlock.U_RxCounter >= U2_RX_MAX)//Check if there is enough remaining space. If there is enough, allocate it; if not, return to the head.
        {
            U2_ControlBlock.U_RxDataIN->start = &U2_RxBuff[U2_ControlBlock.U_RxCounter];
        }else{
            U2_ControlBlock.U_RxDataIN->start = U2_RxBuff;
            U2_ControlBlock.U_RxCounter = 0;
        }
        //Reconfigure some settings of dma
        dma_channel_disable(DMA0, DMA_CH2);
        dma_transfer_number_config(DMA0, DMA_CH2, U2_RX_MAX + 1);
        dma_memory_address_config(DMA0, DMA_CH2, (uint32_t)U2_ControlBlock.U_RxDataIN->start);
        dma_channel_enable(DMA0, DMA_CH2);
    }    
}


void EXTI0_IRQHandler(void)
{
    if(exti_interrupt_flag_get(EXTI_0))//Check the flag bit to confirm that it is an external interrupt before operating.
    {
        exti_interrupt_flag_clear(EXTI_0);//First clear the flag bit.
        if(gpio_input_bit_get(GPIOB, GPIO_PIN_0)) // If it enters the interrupt and finds a high level, it means that the connection to the server is successful.
        {
            U0_MyPrintf("\r\nPB0 is high, server connection successful!\r\n");
        }
        else
        {
            U0_MyPrintf("\r\nPB0 is low, server has disconnected!\r\n");
        }
    }
    
}










/*!
    \brief      this function handles NMI exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void NMI_Handler(void)
{
    /* if NMI exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles HardFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void HardFault_Handler(void)
{
    /* if Hard Fault exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles MemManage exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void MemManage_Handler(void)
{
    /* if Memory Manage exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles BusFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void BusFault_Handler(void)
{
    /* if Bus Fault exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles UsageFault exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void UsageFault_Handler(void)
{
    /* if Usage Fault exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles SVC exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SVC_Handler(void)
{
    /* if SVC exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles DebugMon exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void DebugMon_Handler(void)
{
    /* if DebugMon exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles PendSV exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void PendSV_Handler(void)
{
    /* if PendSV exception occurs, go to infinite loop */
    while(1){
    }
}

/*!
    \brief      this function handles SysTick exception
    \param[in]  none
    \param[out] none
    \retval     none
*/
void SysTick_Handler(void)
{

}
