#include "gd32f10x.h"
#include "spi.h"

void SPI0_Init(void)
{
    spi_parameter_struct spi_para;
    
    // Enable SPI and GPIO clock
    rcu_periph_clock_enable(RCU_SPI0);
    rcu_periph_clock_enable(RCU_GPIOA);
    
    // Initialize IO ports
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5 | GPIO_PIN_7);
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
    
    // SPI settings
    spi_i2s_deinit(SPI0);
    spi_para.device_mode = SPI_MASTER;   // Set to master mode
    spi_para.trans_mode = SPI_TRANSMODE_FULLDUPLEX;  // Full duplex mode
    spi_para.frame_size = SPI_FRAMESIZE_8BIT;   // 8-bit data frame size
    spi_para.nss = SPI_NSS_SOFT;    // Software control of NSS
    spi_para.endian = SPI_ENDIAN_MSB;  // MSB first transmission
    spi_para.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE; // Clock polarity low level, sample on the first edge
    spi_para.prescale = SPI_PSC_2;  // Prescale of 8

    spi_init(SPI0, &spi_para);
    spi_enable(SPI0);
}


uint8_t SPI0_TransmitAndReceive(uint8_t transmit_content)
{
    while(!spi_i2s_flag_get(SPI0, SPI_FLAG_TBE)); // If the read value is 0, it means there is still data being sent, it will stay in this while loop
    spi_i2s_data_transmit(SPI0,transmit_content);
    
    while(!spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE));
    return spi_i2s_data_receive(SPI0);
}


void SPI0_Write(uint8_t *write_arr, uint16_t datalen)
{
    for(uint16_t i = 0; i < datalen; ++i)
    {
        SPI0_TransmitAndReceive(write_arr[i]); // Do not care about the return value, only care about the written data
    }
}

void SPI0_Read(uint8_t *read_store_arr, uint16_t datalen)
{
    for(uint16_t i = 0; i < datalen; ++i)
    {
        read_store_arr[i] = SPI0_TransmitAndReceive(0xff); // Provide a useless write content, then wait to receive data and store it in read_store_arr
    }
}




