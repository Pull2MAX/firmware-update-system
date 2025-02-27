#include "gd32f10x.h"
#include "usart.h"
#include "main.h"



uint8_t U0_TxBuff[U0_TX_BUF_SIZE];// UART0 transmission buffer
UCB U0_ControlBlock;

uint8_t U2_RxBuff[U2_RX_BUF_SIZE];// UART2 reception buffer
uint8_t U2_TxBuff[U2_TX_BUF_SIZE];// UART2 transmission buffer
UCB U2_ControlBlock;



void Usart0_Init(uint32_t baudrate)
{
    // Enable USART and GPIO clock
    
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_GPIOA);
    
    // IO port initialization
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);// PA9 in the board is used for transmission
    
    
    // USART configuration
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
    // Enable USART and GPIO clock
    
    rcu_periph_clock_enable(RCU_USART2);
    rcu_periph_clock_enable(RCU_GPIOB);
    
    // IO port initialization
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);// PB10 in the board is used for transmission
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_11);// PB11 is for reception
    
    // USART configuration
    usart_deinit(USART2);
    usart_baudrate_set(USART2, baudrate);
    usart_parity_config(USART2, USART_PM_NONE);
    usart_word_length_set(USART2, USART_WL_8BIT);
    usart_stop_bit_set(USART2, USART_STB_1BIT);
    
    usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART2, USART_RECEIVE_ENABLE);
    
    usart_dma_receive_config(USART2, USART_RECEIVE_DMA_ENABLE);// The program mainly relies on the serial port for receiving a large amount of data, so only the serial port reception DMA needs to be enabled
                                                               // For transmission data, the content to be transmitted is very small, so there is no need to enable DMA, and printf will be enabled later
    // Enable the interrupt for receiving data from the serial port
    
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable(USART2_IRQn, 0, 0);
    
    // Enable peripherals
    usart_interrupt_enable(USART2, USART_INT_IDLE);// Enable interrupt, here there are many interrupts, only need to enable idle interrupt: if there is nothing on the data line for a long time, it indicates that the data is likely to be transmitted completely
    
    U2_RxPtrInit();
    DMA_Init();
    usart_enable(USART2);
}  



void DMA_Init(void)// USART0 corresponds to DMA0 channel 4, USART2 corresponds to DMA0 channel 2, used for data transfer
{
    dma_parameter_struct dma_init_struct;
    
    //------------------- USART2's DMA0 channel 2 -------------------

    // Step 2: Reset
    dma_deinit(DMA0, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)U2_RxBuff;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = U2_RX_MAX + 1;// Not relying on DMA interrupt but on USART idle interrupt, so ignore DMA interrupt
    dma_init_struct.periph_addr = USART2 + 4;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    // Step 3: Configure registers
    dma_init(DMA0, DMA_CH2, &dma_init_struct);
    
    dma_circulation_disable(DMA0, DMA_CH2);
    // Step 4: Enable peripherals
    dma_channel_enable(DMA0, DMA_CH2);
}   



void U2_RxPtrInit(void)// Initialize the content of the super large array used for receiving via serial port and DMA
{
    U2_ControlBlock.U_RxCounter = 0;
    U2_ControlBlock.U_RxDataIN = &U2_ControlBlock.U_RxDataPtr[0];
    U2_ControlBlock.U_RxDataOUT = &U2_ControlBlock.U_RxDataPtr[0];
    U2_ControlBlock.U_RxDataEND = &U2_ControlBlock.U_RxDataPtr[NUM - 1];
    U2_ControlBlock.U_RxDataIN->start = U2_RxBuff;
}



void U0_MyPrintf(char *format,...)// Implement printf for UART
{
    uint16_t i;
    
    va_list listdata; // Requires header file stdarg.h
    va_start(listdata, format);
    vsprintf((char *)U0_TxBuff, format, listdata); // Requires header file stdio.h
    va_end(listdata);
     
    for(i = 0;i < strlen((const char*)U0_TxBuff); ++i)// Before sending data, check if the transmission buffer has any data ==¡· judge by flag
    {                
        while(usart_flag_get(USART0, USART_FLAG_TBE) != 1);// The USART's dedicated register for sending data is not equal to 1 indicates it is not empty, at this time it has not been sent completely, using while to wait.

        usart_data_transmit(USART0, U0_TxBuff[i]); // After waiting in while, it indicates that it can be transmitted normally.
    }
    
    while(usart_flag_get(USART0, USART_FLAG_TC) != 1);// Give some time to let the last character finish sending
}



void U2_MyPrintf(char *format,...)// Implement printf for UART
{
    uint16_t i;
    
    va_list listdata; // Requires header file stdarg.h
    va_start(listdata, format);
    vsprintf((char *)U2_TxBuff, format, listdata); // Requires header file stdio.h
    va_end(listdata);
     
    for(i = 0;i < strlen((const char*)U2_TxBuff); ++i)// Before sending data, check if the transmission buffer has any data ==¡· judge by flag
    {                
        while(usart_flag_get(USART2, USART_FLAG_TBE) != 1);// The USART's dedicated register for sending data is not equal to 1 indicates it is not empty, at this time it has not been sent completely, using while to wait.

        usart_data_transmit(USART2, U2_TxBuff[i]); // After waiting in while, it indicates that it can be transmitted normally.
    }
    
    while(usart_flag_get(USART2, USART_FLAG_TC) != 1);// Give some time to let the last character finish sending
}








