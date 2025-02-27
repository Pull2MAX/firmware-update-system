#ifndef _USART_H
#define _USART_H

#include "stdint.h" // To allow this file to recognize uint8_t
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define U0_TX_BUF_SIZE      256
#define U0_RX_MAX           256 // Maximum amount received at one time

#define U2_RX_BUF_SIZE      1024*16
#define U2_TX_BUF_SIZE      256
#define U2_RX_MAX           1024*15 // Maximum amount received at one time

#define NUM                 10

typedef struct{
    uint8_t *start; // start and end point to an area of variable length data
    uint8_t *end;
}UCB_URxBuffPtr; // This structure marks the start and end positions of the receive buffer U0_RxBuff

typedef struct{
    uint16_t U_RxCounter; // This is the cumulative value of the byte count
    UCB_URxBuffPtr U_RxDataPtr[NUM]; // The maximum number of start and end structures that can exist
    UCB_URxBuffPtr *U_RxDataIN; // Pointer to the next writable area
    UCB_URxBuffPtr *U_RxDataOUT; // Pointer to the data that can be dequeued
    UCB_URxBuffPtr *U_RxDataEND; // Specifically mark the end for easy circular data writing
}UCB; // This structure integrates the start and end pointers into the transmit and receive array control

extern uint8_t U0_TxBuff[U0_TX_BUF_SIZE]; // Dedicated for serial port 0 transmission
extern UCB U0_ControlBlock;

extern uint8_t U2_RxBuff[U2_RX_BUF_SIZE];
extern uint8_t U2_TxBuff[U2_TX_BUF_SIZE]; // Dedicated for serial port 2 transmission
extern UCB U2_ControlBlock;

void Usart0_Init(uint32_t baudrate);

void Usart2_Init(uint32_t baudrate);

void DMA_Init(void);

void U2_RxPtrInit(void); // Initialize the content of the super large array used for receiving via serial and DMA

void U0_MyPrintf(char *format,...); // Implement printf for serial port separately

void U2_MyPrintf(char *format,...); // Implement printf for serial port separately

#endif
