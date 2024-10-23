#include "main.h"

#define PAGE_PROGRAM 		0x02
#define WRITE_ENABLE 		0x06
#define CHIP_ERASE			0xC7
#define READ_DATA			0x03

#define FLASH_GPIO 			GPIOB
#define FLASH_GPIO_NSS		GPIOA
#define FLASH_NSS_PIN		GPIO_PIN_15
#define FLASH_SCK_PIN		GPIO_PIN_3
#define FLASH_MISO_PIN		GPIO_PIN_4
#define FLASH_MOSI_PIN		GPIO_PIN_5

void Write_Enable(SPI_HandleTypeDef* hspi);
void Chip_Erase(SPI_HandleTypeDef *hspi );
void Page_Program(SPI_HandleTypeDef* hspi,  uint8_t *addr, uint8_t* data, uint16_t length);
void Read_Data(SPI_HandleTypeDef* hspi, uint8_t *addr, uint8_t *dataReceived, uint16_t length);