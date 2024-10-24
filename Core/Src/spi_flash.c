#include "spi_flash.h"
#include <stdio.h>
#include "string.h"


extern UART_HandleTypeDef huart1;



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
	uint8_t cmd[2] = {0x66, 0x99};
	W25_CS_ENABLE();
	HAL_SPI_Transmit(&hspi1, cmd, 2, 1000);
	W25_CS_DISABLE();
	W25_DelayWhileBusy(1000);
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

