#include "GSM.h"
#include "string.h"
#include "cmsis_os.h"
#include <stdio.h>

uint8_t response[256];
RingBufferDmaU8_TypeDef SIMRxDMARing;
int is_activated = 0;

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

void deactivate_context(int context_id){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIDEACT=%d\r\n", context_id);
	send_AT_command((char*)command);
	receive_response("DEACTIVATE CONTEXT");
	osDelay(100);
}

void receive_response(char *cmd_str) {
		uint8_t output_buffer[128];
		snprintf((char *)output_buffer, 128, "Response \n at command: %s", cmd_str);
		uart_transmit_string(&huart1, output_buffer);
		//while(response[1] == '\0'){}
		HAL_UART_Transmit(&huart1, response, SIM_RESPONSE_MAX_SIZE, 1000);
		osDelay(1000);
	
	/// IF RESPOND IS FOR CHECKING SIM STATUS
//		if(strstr(cmd_str, "ACTIVATE") != NULL){
//			// If activate PDP context error
//			if(strstr((char *) response, "ERROR") != NULL){
//				deactivate_context(1);
//				
//			}
//			else is_activated = 1;
//		}
		//memset(response, 0x00, 128);
		//SIM_UART_ReInitializeRxDMA();
}

void init_SIM_module() {
    
    // Check if module responds
	SIM_ENABLE();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	osDelay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
}

int first_check_SIM(){
		const char *substring = "READY";
		int receive_OK = 0;
		if(strstr((char *) response, substring) != NULL)
		{
			send_AT_command("AT\r\n");
			receive_response("First check SIM MODULE\n");
			osDelay(100);
			memset(response, 0x00, 128);
			SIM_UART_ReInitializeRxDMA();
			receive_OK = 1;
		}
		return receive_OK;
}

int check_SIM_ready(){
		const int TIME_LIMIT = 20;
	 // Check if SIM is ready
		send_AT_command("AT+CPIN?\r\n");
		osDelay(100);
		while(strstr((char *) response, "PB DONE") == NULL){
			receive_response("Check SIM\n");
		}
		receive_response("Check SIM\n");
		osDelay(100);
		memset(response, 0x00, 128);
		SIM_UART_ReInitializeRxDMA();
		return 1;
}

void check_configure_APN(){
		send_AT_command("AT+QICSGP=?\r\n");
		osDelay(150);
		receive_response("Check Configuring APN\n");
}

int configure_APN(int context_id){
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QICSGP=%d,%d,\"%s\",\"%s\",\"%s\",%d,0\r\n", context_id, 1, APN_NAME, APN_USERNAME, APN_PASSWD, APN_AUTHEN);
	send_AT_command((char*)command);
	osDelay(150);
	receive_response("CONFIGURE APN\n");
	char *first_pointer = NULL;
	char *second_pointer = NULL; 	
	while (first_pointer == NULL || second_pointer == NULL){
		check_configure_APN();
		osDelay(300);
		receive_response("Check Configuring APN\n");
		first_pointer = strstr((char*)response, CHECK_RESPONSE);
		if(first_pointer != NULL){
					second_pointer = strstr(first_pointer+1, CHECK_RESPONSE);
		}
	}
	return 1;
}

void check_activate_context(){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIACT?\r\n");
	send_AT_command((char*)command);
	receive_response("CHECK Activate CONTEXT");
}

void activate_context(int context_id){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIACT=%d\r\n", context_id);
	send_AT_command((char*)command);
	receive_response("Activate Context\r\n");
	char *first_pointer = NULL;
	char *second_pointer = NULL; 	
	while (first_pointer == NULL || second_pointer == NULL){
		check_activate_context();
		osDelay(300);
		receive_response("Check Activate Context\r\n");
		first_pointer = strstr((char*)response, CHECK_RESPONSE);
		if(first_pointer != NULL){
					second_pointer = strstr(first_pointer+1, CHECK_RESPONSE);
		}
	}
}



void open_socket_service(int context_id, int connect_id, char *service_type, char *ip_address, int remote_port, int local_port, int access_mode){
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QIOPEN=%d, %d,\"%s\",\"%s\",%d,%d,%d",context_id, connect_id, service_type, ip_address, remote_port, local_port, access_mode);
	send_AT_command((char *) command);
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
	int isReady = 0;
	int first_check  = 0;
	int first_check_received = 0;
	int process = 0;
//HAL_UART_Receive_DMA(&huart1, rx_buffer, 128);
  for(;;)
  {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
		osDelay(500);
		switch(process){
			//Wait for SIM module to start
			case 0: 
					isReady = first_check_SIM();
					if(isReady) process++;
					break;
			case 1:
				// Check status of SIM. Wait until SIM is ready 
					osDelay(100);
					check_SIM_ready();
					osDelay(150);
					process++;
			case 2: 
				// Configure the PDP context
					configure_APN(1);
					process++;
					memset(response, 0x00, 128);
					SIM_UART_ReInitializeRxDMA();
			case 3: 
				//Activate the PDP context. 
					uart_transmit_string(&huart1, (uint8_t *)"Inside process 3");
					activate_context(1);
					osDelay(200);
					process++;
					break;
			case 4: 
					uart_transmit_string(&huart1, (uint8_t *)"Inside process 4");
					break;
		}
//		if(first_check == 0){
//			first_check_received = first_check_SIM();
//		}
//		if(first_check_received == 1){
//			HAL_UART_Transmit(&huart1, (uint8_t*) "SIM ready\n", strlen("SIM ready\n"), 1000);
//			osDelay(100);
//			//configure_APN(1);
//			first_check = 1;
//			first_check_received = 0;
//			
//			check_SIM_ready();
//			osDelay(150);
//			isReady = 1;
//		}
//		if (isReady){
//				configure_APN(1);
//				send_AT_command("AT+QICSGP=?");
//				osDelay(150);
//				receive_response("Check Configuring APN\n");
//		}
		HAL_UART_Transmit(&huart1, (uint8_t*) "Hello from GSM\n", strlen("Hello from GSM\n"), 1000);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
		osDelay(500);
  }
  /* USER CODE END StartGSM */
}