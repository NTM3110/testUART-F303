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
#include "string.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
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

/* USER CODE BEGIN PV */
uint8_t message1[] = "Hello from Task 1\n";
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_USART1_UART_Init(void);
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
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  RingBufferDmaU8_initUSARTRx(&rs232Ext2RxDMARing, &huart1, gsvSentence, READLOG_BLOCK_BUFFER_LENGHT);
  while (1)
  {
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
		for (j=0;j<110-k-1;j++)
		{
			taxBuffer[j+k+1]=0x00;
		}
		
		HAL_UART_Transmit(&huart1, taxBuffer, sizeof(taxBuffer), 100);
		HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, 100);
		check = 0;
		j = 1;
		k=0;
		cnt=0;
		memset(taxBuffer, 0x00, 128);
		memset(gsvSentence, 0x00, 2048);
		rs232Ext2_InitializeRxDMA();
	}
    /* USER CODE END WHILE */
	//HAL_UART_Transmit(&huart1, message1, sizeof(message1), 100);
		HAL_Delay(1000);
    /* USER CODE BEGIN 3 */
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
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

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
