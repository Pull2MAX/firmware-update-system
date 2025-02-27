#include "gd32f10x.h"
#include "delay.h"

/*
    @brief Initialize the delay function
    @param None
    @return None
*/
void Deley_Init(void)
{
    systick_clksource_set(SYSTICK_CLKSOURCE_HCLK);//配置为最大的108mhz
}

/*
    @brief Delay for a specified number of microseconds
    @param us Number of microseconds to delay
    @return None
*/
void Delay_Us(uint16_t us)
{
    SysTick->LOAD = us * 108;
    SysTick->VAL = 0x00;
    SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
    while(!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk));
    SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

/*
    @brief Delay for a specified number of milliseconds
    @param ms Number of milliseconds to delay
    @return None
*/
void Delay_Ms(uint16_t ms)
{
    while(ms--)
    {
        Delay_Us(1000);
    }
}
