#include "gd32f10x.h"
#include "iic.h"
#include "delay.h"

void IIC_Init(void)
{
    // Enable GPIO clock
    rcu_periph_clock_enable(RCU_GPIOB);
    
    // IO port initialization
    gpio_init(GPIOB, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    gpio_init(GPIOB, GPIO_MODE_OUT_OD, GPIO_OSPEED_50MHZ, GPIO_PIN_7);
    IIC_SCL_H;
    IIC_SDA_H;
}



void IIC_Start(void)
{
    IIC_SCL_H;
    IIC_SDA_H;
    Delay_Us(2);
    IIC_SDA_L;
    // The following operations are to prevent erroneous operations
    Delay_Us(2);
    IIC_SCL_L;
}



void IIC_Stop(void)
{
    IIC_SCL_H;
    IIC_SDA_L;   
    Delay_Us(2);
    IIC_SDA_H;
}


void IIC_Send_Byte(uint8_t content)
{
    int8_t i;
    for(i = 7; i >= 0; --i)
    {
        IIC_SCL_L;
        if(content & BIT(i))    IIC_SDA_H;
        else IIC_SDA_L;
        Delay_Us(2);
        IIC_SCL_H;
        Delay_Us(2);
    }
    IIC_SCL_L;
    IIC_SDA_H; // Release control of the data line and wait for the slave to pull it low to indicate receipt of information
}


uint8_t IIC_Wait_Ack(int16_t timeout) // The SDA has been released during the sending above, this function waits for SDA to be pulled low
{
    do {
        timeout--;
        Delay_Us(2);
    } while((IIC_Read_SDA) && (timeout >= 0)); // If SDA is not pulled low, it indicates no ACK has been received, waiting until timeout or SDA is low
    if(timeout < 0) return 1; // Check again to prevent exiting due to timeout, returning 1 indicates abnormal exit
    
    IIC_SCL_H;
    Delay_Us(2);
    if(IIC_Read_SDA != 0) return 2; // The IIC protocol requires that during the high level of the clock, SDA must be stable and read as low (0), if not, it is incorrect
    
    
    IIC_SCL_L;
    Delay_Us(2);
    return 0; // Returning 0 indicates normal status, as it received a response
}


uint8_t IIC_Read_Byte(uint8_t ack) // The master starts reading, this ack parameter indicates whether the master wants to send an acknowledgment signal to the slave, 1 means it needs to send
{
    int8_t i;
    uint8_t recv = 0;
    for(i = 7; i >= 0; i--)
    {
        IIC_SCL_L; // Set low level to prepare for the data on SDA
        Delay_Us(2);
        IIC_SCL_H;
        if(IIC_Read_SDA) recv |= BIT(i);
        Delay_Us(2);
    }
    
    IIC_SCL_L;
    Delay_Us(2);
    
    if(ack) // If an acknowledgment signal is to be sent, it indicates that communication is not yet finished  
    {
        IIC_SDA_L; // Actively pull low
        
        IIC_SCL_H;
        Delay_Us(2);
        IIC_SCL_L;  

        IIC_SDA_H;    
        Delay_Us(2);
    } else { // If no acknowledgment signal is sent, it indicates that communication has ended
        IIC_SDA_H;
        IIC_SCL_H;
        Delay_Us(2);
        IIC_SCL_L;  
    }
    
    return recv;
}
