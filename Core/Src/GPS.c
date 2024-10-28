#include "main.h"
#include "GPS.h"
#include "string.h"
#include "cmsis_os.h"
#include "NMEA.h"

uint8_t rmc[128]= {0};
RingBufferDmaU8_TypeDef GPSRxDMARing;

uint8_t gpsSentence[GPS_STACK_SIZE];

void getGPS(){
	if(gpsSentence[0] != 0){
		HAL_UART_Transmit(&huart1, (uint8_t *)"GPS Received", strlen("GPS Received"), 1000);
		HAL_UART_Transmit(&huart1, gpsSentence, 10, 1000);
		//memset(gpsSentence, 0x00, 2048);
		HAL_Delay(1000);
	}
}

void getRMC(){
	int idx = 0;
	for(size_t i = 0; i < GPS_STACK_SIZE; i++){
		if (gpsSentence[i] == '$' && gpsSentence[i+1] == 'G' && gpsSentence[i+2] == 'N' && gpsSentence[i+3] == 'R' && gpsSentence[i+4] == 'M' && gpsSentence[i+5] == 'C'){
			
			HAL_UART_Transmit(&huart1, (uint8_t *)"Getting RMC\n", strlen("Getting RMC\n"), 1000);
			while(gpsSentence[i+2] != 0x0A){
				rmc[idx] = gpsSentence[i];
				idx++;
				i++;
			}
			HAL_UART_Transmit(&huart1, rmc, 128,1000);
			memset(rmc, 0x00, 128);
			break;
		}
	 }
	for(size_t i = idx ; i< 128; i++){
		rmc[i] = 0;
	}
}
	

void GPSUART_ReInitializeRxDMA(void)// ham khoi tao lai DMA
{
	HAL_StatusTypeDef ret = HAL_UART_Abort(&huart2);
	if(ret != HAL_OK)
	{
		Error_Handler();			
	}		
	HAL_Delay(50);	//	50 is OK
	//memset(gnssDmaRingBufferMemory, 0x20, sizeof(gnssDmaRingBufferMemory));	// insert buffer with space character	
	RingBufferDmaU8_initUSARTRx(&GPSRxDMARing, &huart2, gpsSentence, GPS_STACK_SIZE);
}

void StartGPS(void const * argument)
{
  HAL_UART_Transmit(&huart1,(uint8_t*) "STARTING GPS", strlen("STARTING GPS"), 1000);
  /* USER CODE BEGIN StartGPS */
  RingBufferDmaU8_initUSARTRx(&GPSRxDMARing, &huart2, gpsSentence, GPS_STACK_SIZE);
  //GPS_ENABLE();
  int countLine = 0;
  int i;
  RMCSTRUCT * gnrmc;
  /* Infinite loop */
   uint32_t avlMaxDMABufferUsage = 0;
	
  memset(gpsSentence, 0x00, GPS_STACK_SIZE);	
  while(1)
  {
	uint32_t avlDmaAvailble = RingBufferDmaU8_available(&GPSRxDMARing);
	uint32_t avlDmaUsagePercent = (avlDmaAvailble * 100) / GPS_STACK_SIZE;
	if(avlMaxDMABufferUsage < avlDmaUsagePercent)
	{
		avlMaxDMABufferUsage = avlDmaUsagePercent;
	}	
	if (avlMaxDMABufferUsage > 60)// khoi tao lai dma sau khi dung het 60% bo nho 
	{
		//memset(gpsSentence, 0x00, GPS_STACK_SIZE);
		GPSUART_ReInitializeRxDMA();
		avlMaxDMABufferUsage=0;
		avlDmaUsagePercent=0;
	}
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	osDelay(500);
	HAL_UART_Transmit(&huart1, (uint8_t *)"GPS Received:\n", strlen("GPS Received:\n"), 1000);
	HAL_UART_Transmit(&huart1, gpsSentence, GPS_STACK_SIZE, 1000);	
	HAL_UART_Transmit(&huart1, (uint8_t *)"\n", strlen("\n"), 1000);
	getRMC();
	//decodeRMC((char*) rmc, gnrmc);
	if(gnrmc->isValid == 1){
		HAL_UART_Transmit(&huart1, (uint8_t *)"GPS DATA RECEIVED VALID\n", strlen("GPS DATA RECEIVED VALID\n"), 1000);
	}
	else{
		HAL_UART_Transmit(&huart1, (uint8_t *)"GPS DATA RECEIVED INVALID\n", strlen("GPS DATA RECEIVED INVALID \n"), 1000);
	}
	HAL_UART_Transmit(&huart1, (uint8_t *)"Getting GPS \n", strlen("Getting GPS \n"), 1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
	osDelay(500);
  }
  /* USER CODE END StartGPS */
}
