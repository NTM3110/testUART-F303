#ifndef UART_DMA_H
#define UART_DMA_H
#include "main.h"

typedef struct 
{
  volatile uint8_t* buffer;
  uint16_t size;
  volatile uint8_t* tailPtr;
  DMA_HandleTypeDef* dmaHandle;
} RingBufferDmaU8_TypeDef;

#define READLOG_BLOCK_BUFFER_LENGHT  2048

#endif