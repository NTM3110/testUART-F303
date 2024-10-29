#include "RS232-UART1.h"
#include "cmsis_os.h"
#include "string.h"
#include "stdint.h"
#include <stdio.h>
#include "stdlib.h"
#include "math.h"

RingBufferDmaU8_TypeDef rs232Ext2RxDMARing;

uint8_t taxBuffer[128];
uint8_t gsvSentence[2048];

// RMCSTRUCT rmc;
// #define GMT 		000

// int isRMCExist = 0;
// int inx = 0;
// int hr=0,min=0,day=0,mon=0,yr=0;
// int daychange = 0;

uint8_t message1[] = "Hello from DSS\n";

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

void Bill_Decode(){
	for (uint16_t i = 0; i < READLOG_BLOCK_BUFFER_LENGHT; i++) 
	{
		if (gsvSentence[i] == ')' & gsvSentence[i+1]==':' )
		{
			check++;
		}	
	}
	if (check > 7)
	{	
		check=0;
		for (uint16_t i = 0; i < READLOG_BLOCK_BUFFER_LENGHT; i++) 
		{	
			if (gsvSentence[i] == ':' & gsvSentence[i-1] == 'E')
			{
					for (j=0; j < 10; j++)
					{
							if(gsvSentence[j+i+1] > 47 & gsvSentence[j+i+1] < 123)
							{	
								k++;
								taxBuffer[k]=gsvSentence[j+i+1];
								cnt++;
							}	
					}	
							k++;
							taxBuffer[k]=';';	
							i=i+cnt;				
			}
			if (gsvSentence[i] == ':' & gsvSentence[i-1] == 'i')
			{
					for (j=0; j < 5; j++)
					{
							if((gsvSentence[j+i+1] > 47 & gsvSentence[j+i+1] < 58 ))
							{	
								k++;
								taxBuffer[k]=gsvSentence[j+i+1];
								cnt++;
							}	
					}	
							k++;
							taxBuffer[k]=';';	
							i=READLOG_BLOCK_BUFFER_LENGHT;
			}		
		}
		
		for (uint16_t i = 0; i < READLOG_BLOCK_BUFFER_LENGHT; i++) 
		{
			if (gsvSentence[i] == ')' & gsvSentence[i+1]==':' )
			{
				check++;
				if (check<=8)
				{	
					for (j=0; j < 30; j++)
					{
						if(gsvSentence[j+i+2]!=0x0A)	
						{	
							if(  (gsvSentence[j+i+2] > 47 & gsvSentence[j+i+2] < 58) ||(gsvSentence[j+i+2] == 44 ) )
							{	
								k++;
								taxBuffer[k]=gsvSentence[j+i+2];
								cnt++;
							}	

						}	
						else
						{
							j=READLOG_BLOCK_BUFFER_LENGHT;
							i=i+cnt;
							cnt=0;
							k++;
							taxBuffer[k]=';';
						}
					 }	
				}	
			}
		}
		k++;
		taxBuffer[k]='#';
		
		
	}
}

// void checkDecode(RMCSTRUCT* rmc){
// 	char long_str[10];
// 	sprintf(long_str, "%.4f", rmc->lcation.longitude);
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "Longtitude: ", strlen("Longtitude: "), 1000);
// 	HAL_UART_Transmit(&huart1, (uint8_t*)long_str, 10,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "\n", strlen("\n"), 1000);
// 	char lat_str[10];
// 	sprintf(lat_str, "%.4f", rmc->lcation.latitude);
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "Latitude: ", strlen("Latitude: "), 1000);
// 	HAL_UART_Transmit(&huart1, (uint8_t*)lat_str, 10,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "\n", strlen("\n"), 1000);
// 	char speed_str[10];
// 	sprintf(speed_str, "%.2f", rmc->speed);
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "Speed: ", strlen("Speed: "), 1000);
// 	HAL_UART_Transmit(&huart1, (uint8_t*)speed_str, 10,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "\n", strlen("\n"), 1000);
// 	char course_str[10];
// 	sprintf(course_str, "%.2f", rmc->course);
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "Course: ", strlen("Course: "), 1000);
// 	HAL_UART_Transmit(&huart1, (uint8_t*)course_str, 6,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "\n", strlen("\n"), 1000);
// 	char year_str[2];
// 	sprintf(year_str, "%d", rmc->date.Yr);
// 	char month_str[2];
// 	sprintf(month_str, "%d", rmc->date.Mon);
// 	char day_str[2];
// 	sprintf(day_str, "%d", rmc->date.Day);
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "DATE: ", strlen("DATE: "), 1000);
// 	HAL_UART_Transmit(&huart1, (uint8_t*)day_str, 2,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*)month_str, 2,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*)year_str, 2,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "\n", strlen("\n"), 1000);\
// 	char hour_str[2];
// 	sprintf(hour_str, "%d", rmc->tim.hour);
// 	char minute_str[2];
// 	sprintf(minute_str, "%d", rmc->tim.min);
// 	char second_str[2];
// 	sprintf(second_str, "%d", rmc->tim.sec);
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "TIME: ", strlen("TIME: "), 1000);
// 	HAL_UART_Transmit(&huart1, (uint8_t*)hour_str, 2,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*)minute_str, 2,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*)second_str, 2,1000); 
// 	HAL_UART_Transmit(&huart1, (uint8_t*) "\n", strlen("\n"), 1000);
// }

// int GPS_Decode(RMCSTRUCT* rmc ){
// 	for(size_t i = 0; i < 2048; i++){
// 		if (gsvSentence[i] == '$' && gsvSentence[i+1] == 'G' && gsvSentence[i+2] == 'N' && gsvSentence[i+3] == 'R' && gsvSentence[i+4] == 'M' && gsvSentence[i+5] == 'C') isRMCExist = 1;
// 	}
// 	if (isRMCExist == 1){
// 		HAL_UART_Transmit(&huart1, (uint8_t*) "RMC EXIST\n", strlen("RMC EXIST\n"), 1000);
// 		inx = 0;
// 		char buffer[12];
// 		int i = 0;
// 		while (gsvSentence[inx] != ',') inx++;  // 1st ,
// 		inx++;
// 		while (gsvSentence[inx] != ',')
// 		{
// 		 	buffer[i] = gsvSentence[inx];
// 		 	i++;
// 		 	inx++;
// 		 }
		
// 		 hr = (atoi(buffer)/10000) + GMT/100;   // get the hours from the 6 digit number

// 		 min = ((atoi(buffer)/100)%100) + GMT%100;  // get the minutes from the 6 digit number
		
		 
		
// 		 // Store the time in the RMC structure
// 		 rmc->tim.hour = hr;
// 		 rmc->tim.min = min;
// 		 rmc->tim.sec = atoi(buffer)%100;
// 		 // After time ,
// 		inx++;
// 		if (gsvSentence[inx] == 'A')  // Here 'A' Indicates the data is valid, and 'V' indicates invalid data
// 		{
// 			rmc->isValid = 1;
// 		}
// 		else
// 		{
// 			rmc->isValid = 0;
// 			return 1;
// 		}
// 		//Get latitude
// 		inx++;
// 		inx++;
// 		memset(buffer, '\0', 12);
// 		i = 0; 
// 		while (gsvSentence[inx] != ','){
// 			buffer[i] = gsvSentence[inx];
// 			i++;
// 			inx++;
// 		}
// 		if (strlen(buffer) < 6) return 2;  // If the buffer length is not appropriate, return error
// 		int16_t num = (atoi(buffer));   // change the buffer to the number. It will only convert upto decimal
// 		int j = 0;
// 		while (buffer[j] != '.') j++;   // Figure out how many digits before the decimal
// 		j++;
// 		int declen = (strlen(buffer))-j;  // calculate the number of digit after decimal
// 		int dec = atoi ((char *) buffer+j);  // conver the decimal part a a separate number
// 		float lat = (num/100.0) + (dec/pow(10, (declen+2)));  // 1234.56789 = 12.3456789
// 		rmc->lcation.latitude = lat;  // save the lattitude data into the strucure
// 		inx++; 
// 		rmc->lcation.NS = gsvSentence[inx];  // save the N/S into the structure


// 	/***********************  GET LONGITUDE **********************/
// 		inx++;  // ',' after NS character
// 		inx++;  // Reach the first number in the longitude
// 		memset(buffer, '\0', 12);
// 		i=0;
// 		while (gsvSentence[inx] != ',') 
// 		{
// 			buffer[i] = gsvSentence[inx];
// 			i++;
// 			inx++;
// 		}
// 		num = (atoi(buffer));  // change the buffer to the number. It will only convert upto decimal
// 		j = 0;
// 		while (buffer[j] != '.') j++;  // Figure out how many digits before the decimal
// 		j++;
// 		declen = (strlen(buffer))-j;  // calculate the number of digit after decimal
// 		dec = atoi ((char *) buffer+j);  // conver the decimal part a a separate number
// 		lat = (num/100.0) + (dec/pow(10, (declen+2)));  // 1234.56789 = 12.3456789
// 		rmc->lcation.longitude = lat;  // save the longitude data into the strucure
// 		inx++;
// 		rmc->lcation.EW = gsvSentence[inx];
		
// 		while (gsvSentence[inx] != ',') inx++;  // after EW ,

// 		// Get Speed
// 		inx++;
// 		i=0;
// 		memset(buffer, '\0', 12);
// 		 while (gsvSentence[inx] != ',')
// 		{
// 			buffer[i] = gsvSentence[inx];
// 			i++;
// 		 	inx++;
// 		}

// 		if (strlen (buffer) > 0){          // if the speed have some data
// 			int16_t num = (atoi(buffer));  // convert the data into the number
// 			int j = 0;
// 			while (buffer[j] != '.') j++;   // same as above
// 			j++;
// 			int declen = (strlen(buffer))-j;
// 			int dec = atoi ((char *) buffer+j);
// 			float lat = num + (dec/pow(10, (declen)));
// 			rmc->speed = lat;
// 		 }
// 		else rmc->speed = 0;

// 		//Get Course
// 		inx++;
// 		i=0;
// 		memset(buffer, '\0', 12);
// 		while (gsvSentence[inx] != ',')
// 		{
// 			buffer[i] = gsvSentence[inx];
// 			i++;
// 			inx++;
// 		}

// 		if (strlen (buffer) > 0){  // if the course have some data
// 			int16_t num = (atoi(buffer));   // convert the course data into the number
// 			int j = 0;
// 			while (buffer[j] != '.') j++;   // same as above
// 			j++;
// 			int declen = (strlen(buffer))-j;
// 			int dec = atoi ((char *) buffer+j);
// 			float lat = num + (dec/pow(10, (declen)));
// 			rmc->course = lat;
// 		}
// 		else
// 		{
// 			rmc->course = 0;
// 		}

// 		//Get Date
// 		inx++;
// 		i=0;
// 		memset(buffer, '\0', 12);
// 		while (gsvSentence[inx] != ',')
// 		{
// 			buffer[i] = gsvSentence[inx];
// 			i++;
// 			inx++;
// 		}

// 		// Date in the format 280222
// 		day = atoi(buffer)/10000;  // extract 28
// 		mon = (atoi(buffer)/100)%100;  // extract 02
// 		yr = atoi(buffer)%100;  // extract 22

// 		day = day+daychange;   // correction due to GMT shift

// 		// save the data into the structure
// 		rmc->date.Day = day;
// 		rmc->date.Mon = mon;
// 		rmc->date.Yr = yr;
// 		checkDecode(rmc);
// 		return 0;
// 	}
// 	return 1;
// }

void StartUART1(void const * argument)
{
	HAL_UART_Transmit(&huart1, (uint8_t*) "STARTING UART1", strlen("STARTING UART1"), 1000);
  /* USER CODE BEGIN StartUART1 */
	RingBufferDmaU8_initUSARTRx(&rs232Ext2RxDMARing, &huart1, gsvSentence, READLOG_BLOCK_BUFFER_LENGHT);
  /* Infinite loop */
  for(;;)
  {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	osDelay(1000);
	//W25_Reset();
	HAL_UART_Transmit(&huart1, message1, sizeof(message1), 100);
	HAL_UART_Transmit(&huart1, gsvSentence, 100, 1000);
	// if(GPS_Decode(&rmc) == 0){
	// 	memset(gsvSentence, 0x00, 2048);
	// 	rs232Ext2_InitializeRxDMA();
	// 	isRMCExist =0;
	// }
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
	osDelay(1000);
  }
  /* USER CODE END StartUART1 */
}
