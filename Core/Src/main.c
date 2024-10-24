/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "string.h"
#include "spi_flash.h"
#include <stdio.h>

typedef struct 
{
  volatile uint8_t* buffer;
  uint16_t size;
  volatile uint8_t* tailPtr;
  DMA_HandleTypeDef* dmaHandle;
} RingBufferDmaU8_TypeDef;


RingBufferDmaU8_TypeDef rs232Ext2RxDMARing;
UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;
uint32_t avlMaxDMABufferUsage = 0;

uint8_t gsvSentence[2048];
#define READLOG_BLOCK_BUFFER_LENGHT  2048
uint8_t taxBuffer[128];
uint8_t flashBufferReceived[128];

////////////////////////

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
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_rx;

/* USER CODE BEGIN PV */
uint8_t message1[] = "Hello from Task 1\n";
uint8_t addr_spi_flash[3] = {0x00,0x00,0x00}; 
uint8_t unique_id[8];
uint8_t md_id[2];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
	uint16_t j = 1,k=0,cnt=0,check=0;	
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
 // HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_9); 
  //W25_CS_ENABLE();
  //HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_15);
  //HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
 // HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);
  RingBufferDmaU8_initUSARTRx(&rs232Ext2RxDMARing, &huart1, gsvSentence, READLOG_BLOCK_BUFFER_LENGHT);
 // printf("Hello \n\r");
 taxBuffer[0] = '$';
 int is_written = 0;
 uint32_t address = 0x12B0;
 uint32_t current_address = address;
 int count_save = 0;
  while (1)
  {
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
	HAL_Delay(1000);
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
		
		
		HAL_UART_Transmit(&huart1, taxBuffer, sizeof(taxBuffer), 100);
		HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, 100);
		
		uint8_t addr_idx[3] = {address>>16,address>>8,address};
		char addr_out[10];
		sprintf(addr_out, "%08x", address);
		HAL_UART_Transmit(&huart1, (uint8_t*) addr_out, 8, 1000);
		HAL_UART_Transmit(&huart1, (uint8_t*)"\r", 1, 1000);
		k++;
		taxBuffer[k] = ';';
		for(size_t idx = 6; idx > 0 ; idx--){
			k++;
			taxBuffer[k] = addr_out[8 - idx];
		}
		
		for (j=0;j<110-k-1;j++)
		{
			taxBuffer[j+k+1]=0x00;
		}
		W25_ReadJedecID();
		if (count_save == 0)
			W25_SectorErase(address);
		W25_PageProgram(address, taxBuffer, 128);
		//memset(flashBufferReceived, 0x00,128);
		is_written = 1;
		count_save++;
		current_address = address;
		address+= 128;
		check = 0;
		j = 1;
		k=0;
		cnt=0;
		memset(taxBuffer, 0x00, 128);
		memset(gsvSentence, 0x00, 2048);
		rs232Ext2_InitializeRxDMA();
	}
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	//if(is_written == 1){
		//W25_ReadData(address, flashBufferReceived, 128);
		//char spi_flash_data_intro[] = "Flash DATA received: ";
		//HAL_UART_Transmit(&huart1, (uint8_t*) spi_flash_data_intro, strlen(spi_flash_data_intro), 1000);
		//HAL_UART_Transmit(&huart1, flashBufferReceived, sizeof(flashBufferReceived), 1000);
		//HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, 1000);
		//is_written = 0;
		//memset(flashBufferReceived, 0x00,128);
		
	//}
	W25_CS_ENABLE();
	W25_CS_DISABLE();
	W25_CS_ENABLE();
	W25_CS_DISABLE();
	W25_ReadJedecID();
	W25_CS_ENABLE();
	W25_CS_DISABLE();
	W25_CS_ENABLE();
	W25_CS_DISABLE();
	HAL_Delay(1000);
	W25_ReadData(current_address, flashBufferReceived, 128);
	char spi_flash_data_intro[] = "Flash DATA received: ";
	HAL_UART_Transmit(&huart1, (uint8_t*) spi_flash_data_intro, strlen(spi_flash_data_intro), 1000);
	HAL_UART_Transmit(&huart1, flashBufferReceived, sizeof(flashBufferReceived), 1000);
	HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, 1000);
	HAL_UART_Transmit(&huart1, message1, sizeof(message1), 100);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
	HAL_Delay(1000);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART1;
  PeriphClkInit.Usart1ClockSelection = RCC_USART1CLKSOURCE_PCLK2;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_8;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 7;
  hspi1.Init.CRCLength = SPI_CRC_LENGTH_DATASIZE;
  hspi1.Init.NSSPMode = SPI_NSS_PULSE_ENABLE;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel5_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA1_Channel5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA1_Channel5_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_15, GPIO_PIN_SET);

  /*Configure GPIO pin : PC9 */
  GPIO_InitStruct.Pin = GPIO_PIN_9;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pin : PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
#if defined(__GNUC__)
int _write(int fd, char * ptr, int len)
{
  HAL_UART_Transmit(&huart1, (uint8_t *) ptr, len, HAL_MAX_DELAY);
  return len;
}
#elif defined (__ICCARM__)
#include "LowLevelIOInterface.h"
size_t __write(int handle, const unsigned char * buffer, size_t size)
{
  HAL_UART_Transmit(&huart1, (uint8_t *) buffer, size, HAL_MAX_DELAY);
  return size;
}
#elif defined (__CC_ARM)
int fputc(int ch, FILE *f)
{
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, HAL_MAX_DELAY);
    return ch;
}
#endif
