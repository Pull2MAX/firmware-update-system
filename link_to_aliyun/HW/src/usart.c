#include "gd32f10x.h"
#include "usart.h"
#include "main.h"

uint8_t U0_TxBuff[U0_TX_BUF_SIZE]; // UART0 transmit buffer
UCB U0_ControlBlock;

uint8_t U2_RxBuff[U2_RX_BUF_SIZE]; // UART2 receive buffer
uint8_t U2_TxBuff[U2_TX_BUF_SIZE]; // UART2 transmit buffer
UCB U2_ControlBlock;

void Usart0_Init(uint32_t baudrate)
{
    // Enable UART and GPIO clock
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_GPIOA);
    
    // Initialize IO ports
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9); // PA9 is used for transmission
    
    // USART settings
    usart_deinit(USART0);
    usart_baudrate_set(USART0, baudrate);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_enable(USART0);
}   

void Usart2_Init(uint32_t baudrate)
{
    // Enable UART and GPIO clock
    rcu_periph_clock_enable(RCU_USART2);
    rcu_periph_clock_enable(RCU_GPIOB);
    
    // Initialize IO ports
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10); // PB10 is used for transmission
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_11); // PB11 is for reception
    
    // USART settings
    usart_deinit(USART2);
    usart_baudrate_set(USART2, baudrate);
    usart_parity_config(USART2, USART_PM_NONE);
    usart_word_length_set(USART2, USART_WL_8BIT);
    usart_stop_bit_set(USART2, USART_STB_1BIT);
    
    usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART2, USART_RECEIVE_ENABLE);
    
    usart_dma_receive_config(USART2, USART_RECEIVE_DMA_ENABLE); // The program mainly relies on UART for receiving large amounts of data, so only the DMA receive for UART needs to be enabled.
    usart_dma_transmit_config(USART2, USART_TRANSMIT_DMA_ENABLE);                                                          
    
    // Enable UART receive data interrupt
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable(USART2_IRQn, 0, 0);
    
    // Enable peripherals
    usart_interrupt_enable(USART2, USART_INT_IDLE); // Enable interrupt, here there are many interrupts, only need to enable idle interrupt: if there is nothing on the data line for a long time, it indicates that the data transmission is likely complete.
    
    U2_RxPtrInit();
    U2_TxPtrInit();
    DMA_Init();
    usart_enable(USART2);
}  

void DMA_Init(void) // USART0 corresponds to DMA0 channel 4, USART2 corresponds to DMA0 channel 2, used for data transfer
{
    dma_parameter_struct dma_init_struct;
    
    //------------------- USART2 DMA0 Channel 2 -------------------
    // Step 2: Reset
    dma_deinit(DMA0, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)U2_RxBuff;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = U2_RX_MAX + 1; // Not relying on DMA interrupt but on USART idle interrupt, no need to care about DMA interrupt, here +1 is to avoid generating DMA interrupt
    dma_init_struct.periph_addr = USART2 + 4;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    
    // Step 3: Configure registers
    dma_init(DMA0, DMA_CH2, &dma_init_struct);
    
    dma_circulation_disable(DMA0, DMA_CH2);
    // Step 4: Enable peripherals
    dma_channel_enable(DMA0, DMA_CH2);

    //------------------- USART2 DMA0 Channel 1 -------------------
    // Step 2: Reset
    dma_deinit(DMA0, DMA_CH1);
    dma_init_struct.direction = DMA_MEMORY_TO_PERIPHERAL; // Memory to peripheral, because it is for transmission
    dma_init_struct.memory_addr = (uint32_t)U2_TxBuff;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = 0; // Temporarily set to 0
    dma_init_struct.periph_addr = USART2 + 4;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    
    // Step 3: Configure registers
    dma_init(DMA0, DMA_CH1, &dma_init_struct);
    
    dma_circulation_disable(DMA0, DMA_CH1);
    // Step 4: Enable peripherals
    // dma_channel_disable(DMA0, DMA_CH1);

    dma_interrupt_enable(DMA0, DMA_CH1, DMA_INT_FTF); // Enable completion interrupt
    nvic_irq_enable(DMA0_Channel1_IRQn, 0, 0);
}   

void U2_RxPtrInit(void) // Initialize the content of the super large array used for receiving via UART and DMA
{
    U2_ControlBlock.U_RxCounter = 0;
    U2_ControlBlock.U_RxDataIN = &U2_ControlBlock.U_RxDataPtr[0];
    U2_ControlBlock.U_RxDataOUT = &U2_ControlBlock.U_RxDataPtr[0];
    U2_ControlBlock.U_RxDataEND = &U2_ControlBlock.U_RxDataPtr[NUM - 1];
    U2_ControlBlock.U_RxDataIN->start = U2_RxBuff;
}

void U2_TxPtrInit(void) // Initialize the content of the super large array used for sending via UART and DMA
{
    U2_ControlBlock.U_TxCounter = 0;
    U2_ControlBlock.U_TxDataIN = &U2_ControlBlock.U_TxDataPtr[0];
    U2_ControlBlock.U_TxDataOUT = &U2_ControlBlock.U_TxDataPtr[0];
    U2_ControlBlock.U_TxDataEND = &U2_ControlBlock.U_TxDataPtr[NUM - 1];
    U2_ControlBlock.U_TxDataIN->start = U2_TxBuff;
    U2_ControlBlock.U_TxStatus = 0; // 0 indicates idle, 1 indicates busy
}

void U0_MyPrintf(char *format,...) // Implement UART printf separately
{
    uint16_t i;
    
    va_list listdata; // Requires stdarg.h header file
    va_start(listdata, format);
    vsprintf((char *)U0_TxBuff, format, listdata); // Requires stdio.h header file
    va_end(listdata);
     
    for(i = 0; i < strlen((const char*)U0_TxBuff); ++i) // Before sending data, check if there are numbers in the send buffer ==¡· Use flag to check
    {                
        while(usart_flag_get(USART0, USART_FLAG_TBE) != 1); // The transmit register for sending data is not equal to 1 indicates it is not empty, at this point it has not been sent completely, wait in while loop.

        usart_data_transmit(USART0, U0_TxBuff[i]); // After waiting in while loop, it indicates that it can be transmitted normally.
    }
    
    while(usart_flag_get(USART0, USART_FLAG_TC) != 1); // Give some time to let the last character finish sending
}

/*
    @brief  Put data into the send buffer through UART2, and then in the main function's while loop, send the data out
    @param None
*/
void U2_SendData(uint8_t *data, uint16_t data_len)
{
    if(U2_TX_BUF_SIZE - U2_ControlBlock.U_TxCounter >= data_len) // Check if the send buffer is sufficient
    {
        U2_ControlBlock.U_TxDataIN->start = &U2_TxBuff[U2_ControlBlock.U_TxCounter];
    }
    else    // Not enough space, need to return to the head and start again
    {
        U2_ControlBlock.U_TxCounter = 0;
        U2_ControlBlock.U_TxDataIN->start = U2_TxBuff;
    }  
    memcpy(U2_ControlBlock.U_TxDataIN->start, data, data_len);
    U2_ControlBlock.U_TxCounter += data_len;
    U2_ControlBlock.U_TxDataIN->end = &U2_TxBuff[U2_ControlBlock.U_TxCounter - 1];
    U2_ControlBlock.U_TxDataIN++; // Move the in pointer

    if(U2_ControlBlock.U_TxDataIN == U2_ControlBlock.U_TxDataEND) // If the in pointer points to the end pointer, it means it has reached the end, need to return to the head
    {
        U2_ControlBlock.U_TxDataIN = &U2_ControlBlock.U_TxDataPtr[0];
    }
}





