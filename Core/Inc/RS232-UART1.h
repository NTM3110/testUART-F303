#ifndef RS232_UART1_H
#define RS232_UART1_H

#include "main.h"


// typedef struct {
// 	int hour;
// 	int min;
// 	int sec;
// }TIME;

// typedef struct {
// 	float latitude;
// 	char NS;
// 	float longitude;
// 	char EW;
// }LOCATION;

// typedef struct {
// 	int Day;
// 	int Mon;
// 	int Yr;
// }DATE;

// typedef struct {
// 	TIME tim;
// 	DATE date;
// 	float speed;
// 	float course;
// 	int isValid;
// 	LOCATION lcation;
// }RMCSTRUCT;

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
//int GPS_Decode();
#endif