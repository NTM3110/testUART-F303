#include "spi_flash.h"

void Set_NSS_Pin(){
	HAL_GPIO_WritePin(FLASH_GPIO_NSS, FLASH_NSS_PIN, GPIO_PIN_SET);
}
void Reset_NSS_Pin(){
	HAL_GPIO_WritePin(FLASH_GPIO_NSS, FLASH_NSS_PIN, GPIO_PIN_RESET);
}

int W25_ReadJedecID(uint8_t *buf, int bufSize) {
  int retval;
  uint8_t idcmd = W25_CMD_READ_JEDEC_ID;
  if(bufSize < W25_JEDEC_ID_BUF_SIZE)
    return HAL_ERROR; // buffer too small
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  retval = HAL_SPI_Transmit(&hspi2, &idcmd , sizeof(idcmd ), TIMEOUT); // Send the ID command
  if(retval == HAL_OK)
    retval = HAL_SPI_Receive(&hspi2, buf, W25_JEDEC_ID_BUF_SIZE, TIMEOUT);
  W25_CS_DISABLE();
  //printf("%s: retval %d, %02X, %02X, %02X\r\n",__func__, retval, buf[0],buf[1],buf[2]);
  return retval;
} // W25_ReadJEDECID()

void Write_Enable(SPI_HandleTypeDef* hspi){
	uint8_t cmd  = WRITE_ENABLE;
	// Set NSS to Low
	Reset_NSS_Pin();
	//SEND 0x06 qua SPI1
	HAL_SPI_Transmit(hspi, &cmd, 1, 100);
	//Set NSS to HIGH
	Set_NSS_Pin();
	
}
void Chip_Erase(SPI_HandleTypeDef *hspi ){
	uint8_t cmd  = CHIP_ERASE;
	// Set NSS to Low
	Reset_NSS_Pin();
	//SEND 0x06 qua SPI1
	HAL_SPI_Transmit(hspi, &cmd, 1, 100);
	//Set NSS to HIGH
	Set_NSS_Pin();
}

void Page_Program(SPI_HandleTypeDef* hspi,  uint8_t *addr, uint8_t* data, uint16_t length){
	uint8_t cmd  = PAGE_PROGRAM;
	//Set NSS to Low 
	Reset_NSS_Pin();
	//Send instruction code: 0x02
	HAL_SPI_Transmit(hspi, &cmd, 1, 100);
	//Send data address 24 bits
	HAL_SPI_Transmit(hspi, addr, 3, 300);
	// Send data
	HAL_SPI_Transmit(hspi, data, length, 1000);
	//SET NSS TO HIGH
	Set_NSS_Pin();
}

void Read_Data(SPI_HandleTypeDef* hspi, uint8_t *addr, uint8_t *dataReceived, uint16_t length){
	uint8_t cmd = READ_DATA;
// Set NSS to Low.
	Reset_NSS_Pin();
	// Send instruction code.
	HAL_SPI_Transmit(hspi, &cmd, 1, 300);
	// Send address 24 bit
	HAL_SPI_Transmit(hspi, addr, 3, 300);
	
	HAL_SPI_Receive(hspi, dataReceived, length, 1000);
	
	Set_NSS_Pin();
}