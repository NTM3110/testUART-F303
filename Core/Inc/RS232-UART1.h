#ifndef RS232_UART1_H
#define RS232_UART1_H

#include "main.h"



typedef struct 
{
  volatile uint8_t* buffer;
  uint16_t size;
  volatile uint8_t* tailPtr;
  DMA_HandleTypeDef* dmaHandle;
} RingBufferDmaU8_TypeDef;


extern UART_HandleTypeDef huart1;
extern uint16_t j,k,cnt,check;
#define READLOG_BLOCK_BUFFER_LENGHT  2048


void RingBufferDmaU8_initUSARTRx(RingBufferDmaU8_TypeDef* ring, UART_HandleTypeDef* husart, uint8_t* buffer, uint16_t size);

uint16_t RingBufferDmaU8_available(RingBufferDmaU8_TypeDef* ring); // kiem tra trang thai su dung dma

void rs232Ext2_InitializeRxDMA(void);// ham khoi tao lai DMA

void Bill_Decode();
#endif