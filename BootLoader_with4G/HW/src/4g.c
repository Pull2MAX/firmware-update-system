#include "gd32f10x.h"
#include "main.h"
#include "4g.h"
#include "delay.h"
#include "usart.h"
#include "boot.h"

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
    // Enable GPIO clock
    rcu_periph_clock_enable(RCU_GPIOB);
    
    // Initialize IO ports
    gpio_init(GPIOB, GPIO_MODE_IPD, GPIO_OSPEED_50MHZ, GPIO_PIN_0); // PB0 is used to detect its level to see if there is a connection to the server, so it needs to be configured as input mode
    gpio_init(GPIOB, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2); // PB2 is used to control the reset of the 4G module (high level), so it needs to be configured as output mode

    U0_MyPrintf("The 4G module is resetting, please wait...\r\n");
    gpio_bit_set(GPIOB, GPIO_PIN_2); // Reset the 4G module
    Delay_Ms(500);
    gpio_bit_reset(GPIOB, GPIO_PIN_2); 
}



void U2_Event(uint8_t *data, uint16_t datalen)
{
    if((datalen == 6)&&(memcmp(data, "chaozi", 6) == 0)) // Check if the specific data received by UART2 is "chaozi" and if the received length is 6
    {
        U0_MyPrintf("\r\n4G module reset successful! The module will switch from transparent mode to AT mode...\r\n");
        // The 4G module defaults to transparent mode, so it needs to switch to AT mode
        U2_MyPrintf("+++");// Step 1: Enter AT mode from transparent mode by sending "+++"
    }
    if((datalen == 1)&&(memcmp(data, "a", 1) == 0)) // Step 2: Wait for the module to reply 'a'
    {
        U2_MyPrintf("a"); // Step 3: Reply 'a'
    }
    if((datalen == 5)&&(memcmp(data, "+ok\r\n", 5) == 0))// Step 4: After the module receives 'a', it replies '+ok\r\n', indicating it has entered command mode
    {
        U0_MyPrintf("\r\nThe 4G module has exited transparent mode, now in AT mode!\r\n");
        Print_BootLoader_Control_Info();
    }
}
 
