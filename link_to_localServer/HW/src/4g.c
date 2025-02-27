#include "gd32f10x.h"
#include "main.h"
#include "4g.h"
#include "delay.h"
#include "usart.h"
#include "w25q64.h"
#include "m24c02.h"

/*
    @brief Initialize the 4G module, including GPIO configuration and reset operation of the 4G module
    @param None
    @return None
    @details Configure PB0 as input pull-down mode to detect server connection status
             Configure PB2 as push-pull output mode to control the reset of the 4G module
             Execute reset timing: PB2 outputs high for 500ms and then pulls low
*/
void GM_4G_Init(void)
{
    //Enable GPIO clock
    rcu_periph_clock_enable(RCU_GPIOB);
    
    //Initialize IO port
    gpio_init(GPIOB, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, GPIO_PIN_0); //pb0 is used to detect its level and see if it is connected to the server, so it needs to be configured as input mode
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2); //pb2 is used to control the 4g module (high level) reset, so it needs to be configured as output mode

    U0_MyPrintf("4G module is resetting, please wait...\r\n");
    gpio_bit_set(GPIOB, GPIO_PIN_2); //Reset 4g module
    Delay_Ms(500);
    gpio_bit_reset(GPIOB, GPIO_PIN_2); 
}



void U2_Event(uint8_t *data, uint16_t datalen)
{
    if((datalen == 6)&&(memcmp(data, "chaozi", 6) == 0)) // Check whether the specific data received by the receiving function of serial port 2 is "chaozi" and see if the received length is 6
    {
        U0_MyPrintf("\r\n4g module reset success! Waiting for connection to server (PB0=1)...\r\n");
        
        //Enable external interrupt of pb0
        rcu_periph_clock_enable(RCU_AF);//If you want to use the external interrupt function, you need to enable the clock
        exti_deinit();
        exti_init(EXTI_0,EXTI_INTERRUPT,EXTI_TRIG_BOTH);
        gpio_exti_source_select(GPIO_PORT_SOURCE_GPIOB, GPIO_PIN_SOURCE_0);
        exti_interrupt_enable(EXTI_0);
        nvic_irq_enable(EXTI0_IRQn,0,0);
    }
    
    if((datalen >= 1024 * 10)) // If the length exceeds 10k, it means that the received is the new firmware, then the new firmware must be stored in w25q64
    {
        W25Q64_Erase64k(0);
        for(uint8_t i = 0; i < ((datalen/256)+1); ++i) 
        {
            W25Q64_PageWrite(&data[i * 256], i);// w25q64 writes 256 bytes at a time
        }
        ota_info.ota_flag = OTA_UPDATE_FLAG;
        ota_info.FireLen[0] = datalen;
        M24C02_WriteOTAInfo();
        U0_MyPrintf("\r\nSerial port 2 successfully received the bin file! Prepare to restart and update!\r\n");
        NVIC_SystemReset();
    }
}
