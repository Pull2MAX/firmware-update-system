#ifndef _USART_H
#define _USART_H

#include "stdint.h" // To allow this file to recognize uint8_t
#include <stdarg.h>
#include <stdio.h>
#include <string.h>


#define U0_TX_BUF_SIZE      256
#define U0_RX_MAX           256 // Maximum amount received at one time

#define U2_RX_BUF_SIZE      1024*5
#define U2_TX_BUF_SIZE      1024*5
#define U2_RX_MAX           512 // Maximum amount received at one time

#define NUM                 10


typedef struct{
    uint8_t *start; // Start and end point to the variable length data area
    uint8_t *end;
}UCB_URxBuffPtr; // This structure marks the start and end position of the receive buffer U0_RxBuff



typedef struct{
    //接收专用
    uint16_t U_RxCounter; // Here is the accumulated value of the number of bytes
    UCB_URxBuffPtr U_RxDataPtr[NUM]; // The start and end structure can exist up to NUM
    UCB_URxBuffPtr *U_RxDataIN; // Point to the next area to be written
    UCB_URxBuffPtr *U_RxDataOUT; // Point to the data to be dequeued
    UCB_URxBuffPtr *U_RxDataEND; // Special mark for end, convenient for loop writing data 

    //发送专用
    uint16_t U_TxCounter; // Control the accumulated value of the sent bytes
    uint16_t U_TxStatus; // Control the send status of dma 0; is idle 1: is busy
    UCB_URxBuffPtr U_TxDataPtr[NUM]; // The start and end structure can exist up to NUM
    UCB_URxBuffPtr *U_TxDataIN; // Point to the next area to be written
    UCB_URxBuffPtr *U_TxDataOUT; // Point to the data to be dequeued
    UCB_URxBuffPtr *U_TxDataEND; // Special mark for end, convenient for loop writing data 
}UCB; // This structure integrates start&end pointers into the receive and transmit array to control

extern uint8_t U0_TxBuff[U0_TX_BUF_SIZE]; // Serial port 0 send exclusive
extern UCB U0_ControlBlock;

extern uint8_t U2_RxBuff[U2_RX_BUF_SIZE];
extern uint8_t U2_TxBuff[U2_TX_BUF_SIZE]; // Serial port 2 send exclusive
extern UCB U2_ControlBlock;






void Usart0_Init(uint32_t baudrate);

void Usart2_Init(uint32_t baudrate);

void DMA_Init(void);

void U2_RxPtrInit(void); // Initialize the content of the large array area using serial port and dma reception

void U2_TxPtrInit(void);

void U0_MyPrintf(char *format,...); // Implement serial port printf separately

void U2_SendData(uint8_t *data, uint16_t data_len);


#endif
