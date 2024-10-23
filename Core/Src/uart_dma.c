#include "uart_dma.h"

extern UART_HandleTypeDef huart1;
extern RingBufferDmaU8_TypeDef rs232Ext2RxDMARing;
extern uint8_t gsvSentence[2048];
void RingBufferDmaU8_initUSARTRx(RingBufferDmaU8_TypeDef* ring, UART_HandleTypeDef* husart, uint8_t* buffer, uint16_t size) // cai dat dma
{
  ring->buffer = buffer;
  ring->size = size;
  ring->tailPtr = buffer;
  ring->dmaHandle = husart->hdmarx;
  HAL_UART_Receive_DMA(husart, buffer, size);
}


#ifdef USING_CCMRAM
__attribute__((section("ccmram")))
#endif
uint16_t RingBufferDmaU8_available(RingBufferDmaU8_TypeDef* ring) // kiem tra trang thai su dung dma
{
#ifdef __HAL_DMA_GET_COUNTER
  uint32_t leftToTransfer = __HAL_DMA_GET_COUNTER(ring->dmaHandle);
#else
  uint32_t leftToTransfer = ring->dmaHandle->Instance->CNDTR;
#endif
  volatile uint8_t const* head = ring->buffer + ring->size - leftToTransfer;
  volatile uint8_t const* tail = ring->tailPtr;
  if (head >= tail) {
    return head - tail;
  } else {
    return head - tail + ring->size;
  }
}

void rs232Ext2_InitializeRxDMA(void)// ham khoi tao lai DMA
{
	HAL_StatusTypeDef ret = HAL_UART_Abort(&huart1);
	if(ret != HAL_OK)
	{
		Error_Handler();			
	}		
	HAL_Delay(50);	//	50 is OK
	//memset(gnssDmaRingBufferMemory, 0x20, sizeof(gnssDmaRingBufferMemory));	// insert buffer with space character	
	RingBufferDmaU8_initUSARTRx(&rs232Ext2RxDMARing, &huart1, gsvSentence, READLOG_BLOCK_BUFFER_LENGHT);
}