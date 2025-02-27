#include "gd32f10x.h"
#include "usart.h"
#include "main.h"


uint8_t U0_RxBuff[U0_RX_BUF_SIZE];// UART0 receive buffer
uint8_t U0_TxBuff[U0_TX_BUF_SIZE];// UART0 transmit buffer
UCB U0_ControlBlock;

uint8_t U2_RxBuff[U2_RX_BUF_SIZE];// UART2 receive buffer
uint8_t U2_TxBuff[U2_TX_BUF_SIZE];// UART2 transmit buffer
UCB U2_ControlBlock;



void Usart0_Init(uint32_t baudrate)
{
    // Enable clock for UART and GPIO
    
    rcu_periph_clock_enable(RCU_USART0);
    rcu_periph_clock_enable(RCU_GPIOA);
    
    // Initialize IO ports
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_9);// PA9 is used for transmission
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_10);// PA10 is used for reception
    
    // USART configuration
    usart_deinit(USART0);
    usart_baudrate_set(USART0, baudrate);
    usart_parity_config(USART0, USART_PM_NONE);
    usart_word_length_set(USART0, USART_WL_8BIT);
    usart_stop_bit_set(USART0, USART_STB_1BIT);
    
    usart_transmit_config(USART0, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART0, USART_RECEIVE_ENABLE);
    
    usart_dma_receive_config(USART0, USART_RECEIVE_DMA_ENABLE);// The program mainly relies on UART for receiving large amounts of data, so only DMA for UART receiving needs to be enabled.
                                                               // For sending data, the amount is very small, so there is no need to enable DMA, printf will be enabled later.
    
    // Enable UART receive data interrupt
    nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable(USART0_IRQn, 0, 0);
    
    // Enable peripherals
    usart_interrupt_enable(USART0, USART_INT_IDLE);// Enable interrupt, there are many interrupts here, only the idle interrupt needs to be enabled: if there is nothing on the data line for a long time, it indicates that the data transmission is likely complete.
    
    U0_RxPtrInit();
    DMA_Init();
    usart_enable(USART0);
}   



void Usart2_Init(uint32_t baudrate)
{
    // Enable clock for UART and GPIO
    
    rcu_periph_clock_enable(RCU_USART2);
    rcu_periph_clock_enable(RCU_GPIOB);
    
    // Initialize IO ports
    gpio_init(GPIOB, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_10);// PB10 is used for transmission
    gpio_init(GPIOB, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_11);// PB11 is used for reception
    
    // USART configuration
    usart_deinit(USART2);
    usart_baudrate_set(USART2, baudrate);
    usart_parity_config(USART2, USART_PM_NONE);
    usart_word_length_set(USART2, USART_WL_8BIT);
    usart_stop_bit_set(USART2, USART_STB_1BIT);
    
    usart_transmit_config(USART2, USART_TRANSMIT_ENABLE);
    usart_receive_config(USART2, USART_RECEIVE_ENABLE);
    
    usart_dma_receive_config(USART2, USART_RECEIVE_DMA_ENABLE);// The program mainly relies on UART for receiving large amounts of data, so only DMA for UART receiving needs to be enabled.
                                                               // For sending data, the amount is very small, so there is no need to enable DMA, printf will be enabled later.
    // Enable UART receive data interrupt
    //nvic_priority_group_set(NVIC_PRIGROUP_PRE2_SUB2);
    nvic_irq_enable(USART2_IRQn, 0, 0);
    
    // Enable peripherals
    usart_interrupt_enable(USART2, USART_INT_IDLE);// Enable interrupt, there are many interrupts here, only the idle interrupt needs to be enabled: if there is nothing on the data line for a long time, it indicates that the data transmission is likely complete.
    
    U2_RxPtrInit();
    //DMA_Init();
    usart_enable(USART2);
}  



void DMA_Init(void)// USART0 corresponds to DMA0 channel 4, USART2 corresponds to DMA0 channel 2, used for data transfer
{
    dma_parameter_struct dma_init_struct;
    
    //------------------- USART0 DMA0 Channel 4 -------------------
    // Step 1: Enable clock
    rcu_periph_clock_enable(RCU_DMA0);
    // Step 2: Reset
    dma_deinit(DMA0, DMA_CH4);
    
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)U0_RxBuff;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = U0_RX_MAX + 1;// Not relying on DMA interrupt but on USART idle interrupt, no need to care about DMA interrupt
    dma_init_struct.periph_addr = USART0 + 4;
    dma_init_struct.periph_inc = DMA_PERIPH_INCREASE_DISABLE;
    dma_init_struct.periph_width = DMA_PERIPHERAL_WIDTH_8BIT;
    dma_init_struct.priority = DMA_PRIORITY_HIGH;
    // Step 3: Configure registers
    dma_init(DMA0, DMA_CH4, &dma_init_struct);
    
    dma_circulation_disable(DMA0, DMA_CH4);
    // Step 4: Enable peripherals
    dma_channel_enable(DMA0, DMA_CH4);


    //------------------- USART2 DMA0 Channel 2 -------------------

    // Step 2: Reset
    dma_deinit(DMA0, DMA_CH2);
    dma_init_struct.direction = DMA_PERIPHERAL_TO_MEMORY;
    dma_init_struct.memory_addr = (uint32_t)U2_RxBuff;
    dma_init_struct.memory_inc = DMA_MEMORY_INCREASE_ENABLE;
    dma_init_struct.memory_width = DMA_MEMORY_WIDTH_8BIT;
    dma_init_struct.number = U2_RX_MAX + 1;// Not relying on DMA interrupt but on USART idle interrupt, no need to care about DMA interrupt
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

 


void U0_RxPtrInit(void)// Initialize the content of the large array used for receiving via UART and DMA
{
    U0_ControlBlock.U_RxCounter = 0;
    U0_ControlBlock.U_RxDataIN = &U0_ControlBlock.U_RxDataPtr[0];
    U0_ControlBlock.U_RxDataOUT = &U0_ControlBlock.U_RxDataPtr[0];
    U0_ControlBlock.U_RxDataEND = &U0_ControlBlock.U_RxDataPtr[NUM - 1];
    U0_ControlBlock.U_RxDataIN->start = U0_RxBuff;
}



void U2_RxPtrInit(void)// Initialize the content of the large array used for receiving via UART and DMA
{
    U2_ControlBlock.U_RxCounter = 0;
    U2_ControlBlock.U_RxDataIN = &U2_ControlBlock.U_RxDataPtr[0];
    U2_ControlBlock.U_RxDataOUT = &U2_ControlBlock.U_RxDataPtr[0];
    U2_ControlBlock.U_RxDataEND = &U2_ControlBlock.U_RxDataPtr[NUM - 1];
    U2_ControlBlock.U_RxDataIN->start = U2_RxBuff;
}



void U0_MyPrintf(char *format,...)// Implement UART printf separately
{
    uint16_t i;
    
    va_list listdata; // Requires header file stdarg.h
    va_start(listdata, format);
    vsprintf((char *)U0_TxBuff, format, listdata); // Requires header file stdio.h
    va_end(listdata);
     
    for(i = 0;i < strlen((const char*)U0_TxBuff); ++i)// Before sending data, check if there are any numbers in the send buffer ==¡· judge by flag
    {                
        while(usart_flag_get(USART0, USART_FLAG_TBE) != 1);// The UART's dedicated register for sending data is not equal to 1, indicating it is not empty, waiting here until it is.

        usart_data_transmit(USART0, U0_TxBuff[i]); // After waiting, it indicates that it can be transmitted normally.
    }
    
    while(usart_flag_get(USART0, USART_FLAG_TC) != 1);// Give some time to let the last character finish sending
}



void U2_MyPrintf(char *format,...)// Implement UART printf separately
{
    uint16_t i;
    
    va_list listdata; // Requires header file stdarg.h
    va_start(listdata, format);
    vsprintf((char *)U2_TxBuff, format, listdata); // Requires header file stdio.h
    va_end(listdata);
     
    for(i = 0;i < strlen((const char*)U2_TxBuff); ++i)// Before sending data, check if there are any numbers in the send buffer ==¡· judge by flag
    {                
        while(usart_flag_get(USART2, USART_FLAG_TBE) != 1);// The UART's dedicated register for sending data is not equal to 1, indicating it is not empty, waiting here until it is.

        usart_data_transmit(USART2, U2_TxBuff[i]); // After waiting, it indicates that it can be transmitted normally.
    }
    
    while(usart_flag_get(USART2, USART_FLAG_TC) != 1);// Give some time to let the last character finish sending
}








