#include "gd32f10x.h"
#include "spi.h"
#include "delay.h"

/*
    @brief Initialize SPI0
    @param None
    @return None
*/
void SPI_Init(void)
{
    // Enable GPIO clock
    rcu_periph_clock_enable(RCU_GPIOA);
    rcu_periph_clock_enable(RCU_SPI0);
    
    // Configure SPI pins
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5); // SCK
    gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6); // MISO
    gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7); // MOSI

    // Configure SPI
    spi_parameter_struct spi_init_struct;
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAME_SIZE_8BIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_LOW_PH_1EDGE;
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.baudrate_prescaler = SPI_BAUDRATE_PRESCALER_32;
    spi_init_struct.first_bit = SPI_FIRST_MSB;
    spi_init(SPI0, &spi_init_struct);

    // Enable SPI
    spi_enable(SPI0);
}


/*
    @brief Send a byte of data and receive a byte of data
    @param transmit_content The data to send
    @return The received data
*/
uint8_t SPI_SendByte(uint8_t byte)
{
    // Send a byte
    spi_i2s_data_transmit(SPI0, byte);
    while(spi_i2s_flag_get(SPI0, SPI_FLAG_TBE) == RESET); // Wait until transmit buffer is empty
    while(spi_i2s_flag_get(SPI0, SPI_FLAG_RBNE) == RESET); // Wait until receive buffer is not empty
    return spi_i2s_data_receive(SPI0); // Return received byte
}


/*
    @brief Write data to SPI0
    @param write_arr The data array to write
    @param datalen The length of data to write
    @return None
*/
void SPI0_Write(uint8_t *write_arr, uint16_t datalen)
{
    for(uint16_t i = 0; i < datalen; ++i)
    {
        SPI_SendByte(write_arr[i]); // Do not care about the return value, only care about the written data
    }
}


/*
    @brief Read data from SPI0
    @param read_store_arr The array to store the read data
    @param datalen The length of data to read
    @return None
*/
void SPI0_Read(uint8_t *read_store_arr, uint16_t datalen)
{
    for(uint16_t i = 0; i < datalen; ++i)
    {
        read_store_arr[i] = SPI_SendByte(0xff); // Send a dummy byte and wait to receive data into read_store_arr
    }
}




