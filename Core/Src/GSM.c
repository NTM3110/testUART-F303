#include "GSM.h"
#include "string.h"
#include "cmsis_os.h"
#include <stdio.h>

uint8_t response[128];
RingBufferDmaU8_TypeDef SIMRxDMARing;

void send_AT_command(const char *command) {
    HAL_UART_Transmit(&huart3, (uint8_t *)command, strlen(command), HAL_MAX_DELAY);
}

void SIM_UART_ReInitializeRxDMA(void){
	HAL_StatusTypeDef ret = HAL_UART_Abort(&huart3);
	if(ret != HAL_OK)
	{
		Error_Handler();			
	}		
	HAL_Delay(50);	//	50 is OK
	//memset(gnssDmaRingBufferMemory, 0x20, sizeof(gnssDmaRingBufferMemory));	// insert buffer with space character	
	RingBufferDmaU8_initUSARTRx(&SIMRxDMARing, &huart3, response, SIM_RESPONSE_MAX_SIZE);
}
void receive_response(char *cmd_str) {
		snprintf(
		uart_transmit_string(&huart1, (uint8_t*) "Response for SIM module\n at command: %s", cmd_str);
		//while(response[1] == '\0'){}
		HAL_UART_Transmit(&huart1, response, SIM_RESPONSE_MAX_SIZE, 1000);
		memset(response, 0x00, 128);
		SIM_UART_ReInitializeRxDMA();
}

void init_SIM_module() {
    
    // Check if module responds
	SIM_ENABLE();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	osDelay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	send_AT_command("AT\r\n");
	receive_response();
	HAL_Delay(100);
		
    // Check if SIM is ready
   // send_AT_command("AT+CPIN?\r\n");
   // receive_response(response, sizeof(response));
    //HAL_Delay(100);
		
	//	SIM_DISABLE();
//    // Configure APN (replace with your network provider's APN)
//    send_AT_command("AT+CGDCONT=1,\"IP\",\"your_apn_here\"\r\n");
//    receive_response(response, sizeof(response));
//    HAL_Delay(100);

//    // Attach to the packet domain service
//    send_AT_command("AT+CGATT=1\r\n");
//    receive_response(response, sizeof(response));
//    HAL_Delay(100);
}
void configure_APN(int context_id){
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QICSGP=%d,\"%s\",\"%s\",\"%s\",%d\r\n", context_id, APN_NAME, APN_USERNAME, APN_PASSWD, APN_AUTHEN);
	send_AT_command((char*)command);
	receive_response();
}

void activate_context(int context_id){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIACT=%d\r\n", context_id);
	send_AT_command((char*)command);
	receive_response();
}

void deactivate_context(int context_id){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIDEACT=%d\r\n", context_id);
	send_AT_command((char*)command);
	receive_response();
}
void check_context(){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIACT?\r\n");
	send_AT_command((char*)command);
	receive_response();
}

void open_socket_service(int context_id, int connect_id, char *service_type, char *ip_address, int remote_port, int local_port, int access_mode){
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QIOPEN=%d, %d,\"%s\",\"%s\",%d,%d,%d",context_id, connect_id, service_type, ip_address, remote_port, local_port, access_mode);
	send_AT_command((char *) command);
}

void connect_to_TCP_server(){
	int time = 0;
	while(1){
		configure_APN(1);
		activate_context(1);
		check_context();
	}
	
}

void StartGSM(void const * argument)
{
	uart_transmit_string(&huart1, (uint8_t*)"Starting GSM pushing GPS to Server");
  /* USER CODE BEGIN StartGSM */
  /* Infinite loop */
	uint8_t rx_buffer[128];   // Buffer for received data
//	uint8_t response[4];   // Buffer for received data
	uint8_t data_available = 0; // Flag to indicate data reception
	RingBufferDmaU8_initUSARTRx(&SIMRxDMARing, &huart3, response, SIM_RESPONSE_MAX_SIZE);
	
	init_SIM_module();
//HAL_UART_Receive_DMA(&huart1, rx_buffer, 128);
  for(;;)
  {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
		osDelay(500);
		//while(response[1] == '\0'){}
		receive_response();
//    /* USER CODE END WHILE */
//        // Check if data is available
//		if (rx_buffer[6] != 0x00) {
//					HAL_UART_Transmit(&huart1, rx_buffer, 128, 1000);// Set the flag when data is received
//		}

//	 if (HAL_UART_Receive(&huart1, response, 4, 100) == HAL_OK) {
//					data_available = 1; // Set the flag when data is received
//		 
//		}

//		// If data is available, process it
//		if (data_available) {
//				HAL_UART_Transmit(&huart1, response, 4, 100);
//				data_available = 0; // Clear the flag
//				// Process the received data in rx_buffer
//		}
		HAL_UART_Transmit(&huart1, (uint8_t*) "Hello from GSM\n", strlen("Hello from GSM\n"), 1000);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
		osDelay(500);
  }
  /* USER CODE END StartGSM */
}