#include "spi_flash.h"
#include <stdio.h>
#include "string.h"
#include "cmsis_os.h"
#include "RS232-UART1.h"
#include "system_management.h"
#include "gps.h"

extern UART_HandleTypeDef huart1;
uint32_t address = 0x1000;
uint32_t current_addr;
int is_erased = 0;
uint8_t taxBufferDemo[128];

int W25_ChipErase(void)
{
  int retval;
  uint8_t cmd = {W25_CMD_CHIP_ERASE};
  //printf("+%s()\r\n",__func__);
  W25_WriteEnable(); // Make sure we can write...
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  retval = HAL_SPI_Transmit(&hspi1, &cmd , sizeof(cmd ), TIMEOUT); // Send Chip Erase command
  W25_CS_DISABLE();
  //printf("%s: retval %d, ",__func__, retval);
  W25_DelayWhileBusy(CHIP_ERASE_TIMEOUT);
  return retval;
} // W25_ChipErase()

uint8_t W25_ReadStatusReg1(void) {
  uint8_t cmd = W25_CMD_READ_STATUS_REG_1;
  uint8_t status_reg1;
  int retval;
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  //retval = HAL_SPI_Transmit(&hspi1, &cmd , sizeof(cmd), TIMEOUT); // Send Read Status Reg 1 command
  if(retval == HAL_OK)
    retval = HAL_SPI_Receive(&hspi1, &status_reg1, sizeof(status_reg1), TIMEOUT);
  W25_CS_DISABLE();
 // printf("%s: 0x%02X, ",__func__, status_reg1);
  return retval == HAL_OK ? status_reg1:0xFF; // return 0xFF if error
} // W25_ReadStatusReg1()

int W25_Busy(void)
{
  return (W25_ReadStatusReg1() & W25_STATUS1_BUSY);
}

int W25_DelayWhileBusy(uint32_t msTimeout)
{
  uint32_t initial_count = HAL_GetTick();
  int busy;
  uint32_t deltaticks;
  uint32_t count = 0;
  do {
    busy = W25_Busy();
    deltaticks = HAL_GetTick() - initial_count;
    count++;
  } while(busy && deltaticks < msTimeout);
  int retval = busy ? HAL_TIMEOUT:HAL_OK;
  //printf("%s() time(ms): %u, busy: %u\r\n",__func__,deltaticks,busy);
  return retval;
}
void W25_Reset(){
	W25_CS_ENABLE();
	W25_CS_DISABLE();
	W25_CS_ENABLE();
	W25_CS_DISABLE();
	W25_CS_ENABLE();
	W25_CS_DISABLE();
}

int W25_ReadJedecID() {
  int retval;
  uint8_t idcmd = W25_CMD_READ_JEDEC_ID;
 // idcmd[0] = W25_CMD_READ_JEDEC_ID;
  uint8_t jdec_id[4];
  char result[11];
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  //retval = HAL_SPI_Transmit(&hspi1, &idcmd , sizeof(idcmd), TIMEOUT); // Send the ID command
 // if(retval == HAL_OK)
  //  retval = HAL_SPI_Receive(&hspi1, jdec_id, W25_JEDEC_ID_BUF_SIZE, TIMEOUT);
  HAL_SPI_TransmitReceive(&hspi1, &idcmd, jdec_id, 4, 4000);
  W25_CS_DISABLE();
  char spi_flash_intro[] = "Flash ID received: ";
  HAL_UART_Transmit(&huart1, (uint8_t*) spi_flash_intro, strlen(spi_flash_intro), 1000);
  
  sprintf(result, "%02X, %02X, %02X", jdec_id[1], jdec_id[2], jdec_id[3]); 
  HAL_UART_Transmit(&huart1, (uint8_t*) result, 11, 1000);
  HAL_UART_Transmit(&huart1, (uint8_t*)"\r", 1, 1000);
  //printf("%s: retval %d, %02X, %02X, %02X\r\n",__func__, retval, buf[0],buf[1],buf[2]);
  return retval;
} // W25_ReadJEDECID()

int W25_ReadMD(uint8_t *buf, int bufSize) {
  int retval;
  uint8_t cmddata[4] = {0x90,0,0,0};
  if(bufSize < W25_UNIQUE_ID_BUF_SIZE)
    return HAL_ERROR; // buffer too small
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  retval = HAL_SPI_Transmit(&hspi1, cmddata , sizeof(cmddata ), TIMEOUT); // Send the ID command
  if(retval == HAL_OK)
    retval = HAL_SPI_Receive(&hspi1, buf, W25_UNIQUE_ID_BUF_SIZE, TIMEOUT);
  W25_CS_DISABLE();
  //printf("%s: retval %d, ",__func__, retval);
  //hexDump(buf,bufSize);
  //printf("\r\n");
  return retval;
} // W25_ReadUniqueID()

int W25_ReadUniqueID(uint8_t *buf, int bufSize) {
  int retval;
  uint8_t cmddata[5] = {W25_CMD_READ_UNIQUE_ID,0,0,0,0};
  if(bufSize < W25_UNIQUE_ID_BUF_SIZE)
    return HAL_ERROR; // buffer too small
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  retval = HAL_SPI_Transmit(&hspi1, cmddata , sizeof(cmddata ), TIMEOUT); // Send the ID command
  if(retval == HAL_OK)
    retval = HAL_SPI_Receive(&hspi1, buf, W25_UNIQUE_ID_BUF_SIZE, TIMEOUT);
  W25_CS_DISABLE();
  //printf("%s: retval %d, ",__func__, retval);
  //hexDump(buf,bufSize);
  //printf("\r\n");
  return retval;
} // W25_ReadUniqueID()


int W25_WriteEnable(void) {
  uint8_t cmd = W25_CMD_WRITE_ENABLE;
  //printf("+%s()\r\n",__func__);
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  int retval = HAL_SPI_Transmit(&hspi1, &cmd , sizeof(cmd), TIMEOUT); // Send Write Enable command
  W25_CS_DISABLE();
  return retval;
} // W25_WriteEnable()

int W25_SectorErase(uint32_t address)
{
  int retval;
  uint8_t cmdaddr[4] = {W25_CMD_SECTOR_ERASE,address>>16,address>>8,address};
  W25_WriteEnable(); // Make sure we can write...
  //printf("+%s(Addr 0x%06X)\r\n",__func__,address);
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  retval = HAL_SPI_Transmit(&hspi1, cmdaddr , sizeof(cmdaddr ), TIMEOUT); // Send Sector Erase command with address
  W25_CS_DISABLE();
  //printf("%s: retval %d, ",__func__, retval);
  W25_DelayWhileBusy(SECTOR_ERASE_TIMEOUT);
  return retval;
} // W25_SectorErase()

int W25_PageProgram(uint32_t address, uint8_t *buf, uint32_t count)
{
  int retval;
  //printf("+%s(Addr 0x%06X, Len 0x%04X)\r\n",__func__,address,count);
  W25_WriteEnable(); // Make sure we can write...
  while(count) {
    uint8_t cmdaddr[4] = {W25_CMD_PAGE_PROGRAM,address>>16,address>>8,address};
    uint32_t space_left_in_page = 0x100 - (address & 0xFF);
    uint32_t count_this_pass = count <= space_left_in_page? count:space_left_in_page;
    W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
    retval = HAL_SPI_Transmit(&hspi1, cmdaddr , sizeof(cmdaddr ), TIMEOUT); // Send Page Program command with address
    if(retval == HAL_OK)
      retval = HAL_SPI_Transmit(&hspi1, buf, count_this_pass, TIMEOUT); // Write page data
    W25_CS_DISABLE();
    count -= count_this_pass;
    address += count_this_pass;
    buf += count_this_pass;
    W25_DelayWhileBusy(PAGE_PROGRAM_TIMEOUT);
  } // while(count)
  //printf("%s: retval %d, ",__func__, retval);
  //hexDump(buf,count);
  //printf("\r\n");
  return retval;
} 

// Winbond 8.2.6 Read Data (03h)
// The only limit for quantity of data is memory / device size
int W25_ReadData(uint32_t address, uint8_t *buf, int bufSize)
{
  int retval;
  uint8_t cmdaddr[4] = {W25_CMD_READ_DATA,address>>16,address>>8,address};
  //printf("+%s(Addr 0x%06X, buf 0x%08X, Len 0x%04X)\r\n",__func__,address,buf,bufSize);
  W25_CS_ENABLE(); // Drive Winbond chip select, /CS low
  retval = HAL_SPI_Transmit(&hspi1, cmdaddr , sizeof(cmdaddr), 500); // Send Read Data command with address
  if(retval != HAL_OK) {
   // printf("%s: HAL_SPI_Transmit() returned %d\r\n",__func__,retval);
    return retval;
  }
  //memset(buf,0,bufSize); // Buffer is transmitted during receive   
  retval = HAL_SPI_Receive(&hspi1, buf, bufSize, 2000); // need longer time-outs when using slow SPI clock
  if(retval != HAL_OK)
  //  printf("%s: HAL_SPI_Receive() returned %d\r\n",__func__, retval);

  W25_CS_DISABLE();
  //hexDump(buf,bufSize);
  return retval;
} // W25_ReadData()


void receiveTaxData(void) {
//	uint8_t output_buffer[200];
	int k = 0;
    osEvent evt = osMailGet(tax_MailQId, 2000); // Wait for mail
    if (evt.status == osEventMail) {
        TAX_MAIL_STRUCT *receivedData = (TAX_MAIL_STRUCT *)evt.value.p;
		uart_transmit_string(&huart1, (uint8_t*)"Received  TAX Data: \n");
        // Process received data (e.g., display, log, or store data)
		uart_transmit_string(&huart1, receivedData->data);
		for(size_t i = 0; i < 128; i++){
			taxBufferDemo[i] = receivedData->data[i];
			if(receivedData->data[i] != 0x00 && receivedData->data[i+1] == 0x00) k = i;
		}
		osMailFree(tax_MailQId, receivedData); // Free memory after use
		uint8_t addr_idx[3] = {address>>16,address>>8,address};
		char addr_out[10];
		sprintf(addr_out, "%08x", address);
		HAL_UART_Transmit(&huart1, (uint8_t*) addr_out, 8, 1000);
		HAL_UART_Transmit(&huart1, (uint8_t*)"\r\n", 1, 1000);
		k++;
		taxBufferDemo[k] = ';';
		for(size_t idx = 6; idx > 0 ; idx--){
			k++;
			taxBufferDemo[k] = addr_out[8 - idx];
		}
		
		for (j=0;j<110-k-1;j++)
		{
			taxBufferDemo[j+k+1]=0x00;
		}
		char tax_buffer_intro[] = "Tax Buffer SAVED SPI FLASH: ";
		HAL_UART_Transmit(&huart1, (uint8_t*) tax_buffer_intro, strlen(tax_buffer_intro), 1000);
		HAL_UART_Transmit(&huart1, taxBufferDemo, sizeof(taxBufferDemo), 100);
		HAL_UART_Transmit(&huart1, (uint8_t*)"\n", 1, 100);

		W25_Reset();
		if (is_erased == 0){
			W25_SectorErase(address);
			is_erased = 1;
		}
		W25_Reset();
		W25_PageProgram(address, taxBufferDemo, 128);
		current_addr = address;
		address+=128;
		HAL_Delay(1000);
		memset(flashBufferReceived, 0x00,128);
    }
}

void receiveRMCData(void) {
	uint8_t output_buffer[70];
	uart_transmit_string(&huart1, (uint8_t*)"Inside Receiving RMC Data SPI FLASH\n");
    osEvent evt = osMailGet(RMC_MailQId, osWaitForever); // Wait for mail
	uart_transmit_string(&huart1, (uint8_t*)"Status: ");
	uart_transmit_string(&huart1,(uint8_t*) evt.status);
	uart_transmit_string(&huart1,(uint8_t*) "\n");
	
//    if (evt.status == osEventMail) {
//		uart_transmit_string(&huart1, (uint8_t*)"Received  RMC Data SPI FLASH\n");
//        RMCSTRUCT *receivedData = (RMCSTRUCT *)evt.value.p;
//        // Process received data (e.g., display, log, or store data)
//        snprintf((char *)output_buffer, sizeof(output_buffer), "Time Received FLASH: %d:%d:%d\n", receivedData->tim.hour, receivedData->tim.min, receivedData->tim.sec);
//		uart_transmit_string(&huart1, output_buffer);
//		
//        snprintf((char *)output_buffer, sizeof(output_buffer), "Date Received FLASH : %d/%d/%d\n", receivedData->date.Day, receivedData->date.Mon, receivedData->date.Yr);
//		uart_transmit_string(&huart1, output_buffer);
//		
//        snprintf((char *)output_buffer, sizeof(output_buffer), "Location Received FLASH: %.4f %c, %.4f %c\n", receivedData->lcation.latitude, receivedData->lcation.NS, receivedData->lcation.longitude, receivedData->lcation.EW);
//		uart_transmit_string(&huart1, output_buffer);
//        snprintf((char *)output_buffer, sizeof(output_buffer),"Speed FLASH: %.2f, Course: %.2f, Valid: %d\n", receivedData->speed, receivedData->course, receivedData->isValid);
//		uart_transmit_string(&huart1, output_buffer);

//        osMailFree(RMC_MailQId, receivedData); // Free memory after use
//    }
}

void StartSpiFlash(void const * argument)
{
  /* USER CODE BEGIN StartSpiFlash */
  /* Infinite loop */
	current_addr = address;
	for(;;){
		osDelay(1000);
		uart_transmit_string(&huart1, (uint8_t*) "INSIDE SPI FLASH\n");
		W25_Reset();
		W25_ReadJedecID();
		W25_Reset();
		//W25_ReadData(current_addr, flashBufferReceived, 128);
		//char spi_flash_data_intro[] = "Flash DATA received: ";
		//HAL_UART_Transmit(&huart1, (uint8_t*) spi_flash_data_intro, strlen(spi_flash_data_intro), 1000);
		//HAL_UART_Transmit(&huart1, flashBufferReceived, sizeof(flashBufferReceived), 1000);
		//receiveTaxData();
		receiveRMCData();
		osDelay(1000);
  }
  /* USER CODE END StartSpiFlash */
}

