#include "GSM.h"
#include "string.h"
#include "cmsis_os.h"
#include <stdio.h>
#include "time.h"
#include "GPS.h"
#include <stdint.h>
#include <stdlib.h>


uint8_t TERMINAL_REGISTRATION[128];

uint8_t response[SIM_RESPONSE_MAX_SIZE];
RingBufferDmaU8_TypeDef SIMRxDMARing;
int is_activated = 0;
int is_set_time = 0;
RMCSTRUCT rmc_jt;

uint8_t calculate_checksum(uint8_t *data, size_t length) {
    uint8_t checksum = 0;
    for (size_t i = 1; i < length - 2; i++) {  // Skip start and end markers
        checksum ^= data[i];
    }
    return checksum;
}

// Function to create and return the message array
uint8_t* create_message_array(JT808_TerminalRegistration *reg_msg, size_t *array_length) {
    // Calculate the size of the array
    *array_length = sizeof(JT808_TerminalRegistration);
    
    // Allocate memory for the message array
    uint8_t *message_array = (uint8_t *)malloc(*array_length);
    if (message_array == NULL) {
        return NULL; // Return NULL if allocation fails
    }

    // Calculate checksum and assign it to the struct
    reg_msg->check_sum = calculate_checksum((uint8_t *)reg_msg, sizeof(JT808_TerminalRegistration));

    // Copy struct contents into the message array
    memcpy(message_array, reg_msg, *array_length);

    return message_array;
}


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

int find_length(uint8_t *str){
	int i = 0;
	while(str[i] != '\0'){
		i++;
	}
	return i;
		
}

void receive_response(char *cmd_str) {
		uint8_t output_buffer[128];
		snprintf((char *)output_buffer, 128, "Response at command: %s\n", cmd_str);
		uart_transmit_string(&huart1, output_buffer);
		//while(response[1] == '\0'){}
		
		HAL_UART_Transmit(&huart1, response, sizeof(response), 1000);
		uart_transmit_string(&huart1, (uint8_t*)"\n");
		osDelay(1000);
}

void init_SIM_module() {
    
    // Check if module responds
	SIM_ENABLE();
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	osDelay(1000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
}

void reboot_SIM_module(){
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	osDelay(1500);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
	osDelay(2000);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_RESET);
	osDelay(1500);
	HAL_GPIO_WritePin(GPIOC, GPIO_PIN_1, GPIO_PIN_SET);
}


int first_check_SIM(){
		const char *substring = "READY";
		int receive_OK = 0;
		if(strstr((char *) response, substring) != NULL)
		{
			send_AT_command(FIRST_CHECK);
			receive_response("First check SIM MODULE\n");
			osDelay(100);
			memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
			SIM_UART_ReInitializeRxDMA();
			receive_OK = 1;
		}
		return receive_OK;
}

void set_time (uint8_t hr, uint8_t min, uint8_t sec)
{
	RTC_TimeTypeDef sTime = {0};
	sTime.Hours = hr;
	sTime.Minutes = min;
	sTime.Seconds = sec;
	sTime.DayLightSaving = RTC_DAYLIGHTSAVING_NONE;
	sTime.StoreOperation = RTC_STOREOPERATION_RESET;
	if (HAL_RTC_SetTime(&hrtc, &sTime, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}
}

void set_date (uint8_t year, uint8_t month, uint8_t date)  // monday = 1
{
	RTC_DateTypeDef sDate = {0};
	sDate.Month = month;
	sDate.Date = date;
	sDate.Year = year;
	if (HAL_RTC_SetDate(&hrtc, &sDate, RTC_FORMAT_BIN) != HAL_OK)
	{
		Error_Handler();
	}

	HAL_RTCEx_BKUPWrite(&hrtc, RTC_BKP_DR1, 0x2345);  // backup register
}

void get_RTC_time_date()
{
	uint8_t output_buffer[128];
	char time[10];  // "HH:MM:SS" format, 8 characters + null terminator
	char date[12]; // "YYYY-MM-DD" format, 10 characters + null terminator
  RTC_DateTypeDef gDate;
  RTC_TimeTypeDef gTime;

  /* Get the RTC current Time */
  HAL_RTC_GetTime(&hrtc, &gTime, RTC_FORMAT_BIN);
  /* Get the RTC current Date */
  HAL_RTC_GetDate(&hrtc, &gDate, RTC_FORMAT_BIN);

  /* Display time Format: hh:mm:ss */
  /* Format time as "HH:MM:SS" */
	snprintf(time, sizeof(time), "%02d:%02d:%02d\n", gTime.Hours, gTime.Minutes, gTime.Seconds);
	uart_transmit_string(&huart1,(uint8_t*) time);

	/* Format date as "YYYY-MM-DD" */
	snprintf(date, sizeof(date), "20%02d-%02d-%02d\n", gDate.Year, gDate.Month, gDate.Date);
	uart_transmit_string(&huart1,(uint8_t*) date);
	
	rmc_jt.date.Yr = gDate.Year;
	rmc_jt.date.Mon = gDate.Month;
	rmc_jt.date.Day = gDate.Date;
	rmc_jt.tim.hour = gTime.Hours;
	rmc_jt.tim.min = gTime.Minutes;
	rmc_jt.tim.sec = gTime.Seconds;
	
	snprintf((char*)output_buffer, 128, "Time to GMT+8 saved to RMC: 20%02d/%02d/%02d, %02d:%02d:%02d\n", rmc_jt.date.Yr, rmc_jt.date.Mon, rmc_jt.date.Day, rmc_jt.tim.hour, rmc_jt.tim.min, rmc_jt.tim.sec);
	uart_transmit_string(&huart1, (uint8_t*) output_buffer);
}

void extract_time(uint8_t *message){
    int year, month, day, hour, minute, second;
    uint8_t output_buffer[128];
    // Search for the +CTZE line and extract date and time
    char *tz_line = strstr((char*) message, "+CTZE");
    if (tz_line){
        sscanf(tz_line, "+CTZE: \"+28\",0,\"%d/%d/%d,%d:%d:%d\"", &year, &month, &day, &hour, &minute, &second);
        
        // Adjust for GMT+8 (originally in UTC+8, so add 1 hour)
        hour += 8;
        if (hour >= 24) {
            hour -= 24;
            day += 1;
            // Simplified example: Add code here to handle month/day overflow as needed
        }
        
				rmc_jt.date.Yr = year-2000;
				rmc_jt.date.Mon = month;
				rmc_jt.date.Day = day;
				rmc_jt.tim.hour = hour;
				rmc_jt.tim.min = minute;
				rmc_jt.tim.sec = second;
        snprintf((char*)output_buffer, 128, "Adjusted time to GMT+8: 20%02d/%02d/%02d, %02d:%02d:%02d\n", rmc_jt.date.Yr, rmc_jt.date.Mon, rmc_jt.date.Day, rmc_jt.tim.hour, rmc_jt.tim.min, rmc_jt.tim.sec);
				uart_transmit_string(&huart1, (uint8_t*) "RTC Time: ");
				set_time(hour, minute, second);
				set_date(year-2000, month, day);
				uart_transmit_string(&huart1, (uint8_t*) "\n");
				get_RTC_time_date();
				uart_transmit_string(&huart1, output_buffer);
		} else {
        snprintf((char*)output_buffer, 128, "Time information not found");
				uart_transmit_string(&huart1, output_buffer);
    }
}



int check_SIM_ready(){
		const int TIME_LIMIT = 20;
		int count_check_sim = 0;
	 // Check if SIM is ready
		send_AT_command(CHECK_SIM_READY);
		osDelay(100);
		while(strstr((char *) response, "PB DONE") == NULL){
			if(!is_set_time){
				extract_time(response);
			}
			
			receive_response("Check SIM\n");
			count_check_sim++;
			if (count_check_sim >= TIME_LIMIT){
				return 0;
			}
		}
		receive_response("Check SIM\n");
		
		count_check_sim = 0;
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		osDelay(100);
		
		//GET SIM CCID
		send_AT_command(GET_SIM_CCID);
		while(strstr((char *) response, "+QCCID:") == NULL){
			receive_response("Check SIM CCID\n");
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		
		//GET IMEI
		send_AT_command("AT+CGSN=1\r\n");
		while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			receive_response("Check IMEI-0\n");
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		
		//GET MODEL IDENTIFICATION
		send_AT_command("AT+CGMM\r\n");
		while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			receive_response("Check MODEL IDENTIFICATION\n");
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		
		// Configuring Network Registration Status (CS Service)
		send_AT_command("AT+CREG=1\r\n");
		char *first_pointer = NULL;
		char *second_pointer = NULL; 	
		receive_response("Configuring Network Registration Status (CS Service)");
		while (first_pointer == NULL || second_pointer == NULL){
			send_AT_command("AT+CREG?\r\n");
			osDelay(150);
			receive_response("Check Network Registration Status (CS Service)\n");
			osDelay(300);
			first_pointer = strstr((char*)response, CHECK_RESPONSE);
			if(first_pointer != NULL){
						second_pointer = strstr(first_pointer+1, CHECK_RESPONSE);
			}
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
	
		//Configuring Network Registration Status (PS Service)
		send_AT_command("AT+CGREG=1\r\n");
		first_pointer = NULL;
		second_pointer = NULL;
		receive_response("Configuring Network Registration Status (PS Service)");
		while (first_pointer == NULL || second_pointer == NULL){
			send_AT_command("AT+CGREG?\r\n");
			osDelay(150);
			receive_response("Check Network Registration Status (PS Service)\n");
			osDelay(300);
			first_pointer = strstr((char*)response, CHECK_RESPONSE);
			if(first_pointer != NULL){
						second_pointer = strstr(first_pointer+1, CHECK_RESPONSE);
			}
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		
			//GET IMEI
		send_AT_command("AT+CSQ\r\n");
		while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			receive_response("Check Signal Quality Report\n");
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		
		return 1;
}

void check_configure_APN(){
		send_AT_command("AT+QICSGP=1\r\n");
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
	osDelay(150);
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

int deactivate_context(int context_id){
	uint8_t command[128];
	int count_error = 0;
	osDelay(100);
	while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			snprintf((char *)command, sizeof(command), "AT+QIDEACT=%d\r\n", context_id);
			send_AT_command((char*)command);
			receive_response("DEACTIVATE CONTEXT");
			if (strstr((char *) response, "ERROR") != NULL){
				count_error++;
				memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
				SIM_UART_ReInitializeRxDMA();
			}
			if (count_error >= 5){
				uart_transmit_string(&huart1,(uint8_t*) "Rebooting SIM module");
				reboot_SIM_module();
				memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
				SIM_UART_ReInitializeRxDMA();
				return 0;
			}
	}
	count_error = 0;
	receive_response("DEACTIVATE CONTEXT");
	osDelay(100);
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
	return 1;
}

void check_open_socket_service(){
	uint8_t command[128];
	snprintf((char *)command, sizeof(command), "AT+QIOPEN?\r\n");
	send_AT_command((char*)command);
	receive_response("CHECK Activate CONTEXT");
}

int open_socket_service(int context_id, int connect_id, int local_port, int access_mode){
	const int timeout_seconds = 150; // Receive response each second 
	int elapsed_time_ms = 0;
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QIOPEN=%d,%d,\"%s\",\"%s\",%d,%d,%d\r\n",context_id, connect_id, SERVICE_TYPE, IP_ADDRESS, REMOTE_PORT, local_port, access_mode);
	send_AT_command((char *) command);
	osDelay(100);
	char *first_pointer = NULL;
	//time_t start = time(NULL);
	uart_transmit_string(&huart1, (uint8_t *) "Ini start TIME");
	while(first_pointer == NULL && elapsed_time_ms < timeout_seconds){
		char output_elapsed[128];
		receive_response("Check OPEN socket service: \r\n");
		first_pointer = strstr((char*)response, "+QIOPEN:");
		elapsed_time_ms++;
		snprintf(output_elapsed, 128, "Elapsed Time: %d\n", elapsed_time_ms);
		uart_transmit_string(&huart1, (uint8_t *)output_elapsed);
	}
	receive_response("Check OPEN socket service: \r\n");
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
	
	if(first_pointer != NULL)
	{
		send_AT_command("AT+QISTATE=1,0\r\n");
		while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			receive_response("Check SOCKET CONNECTION\n");
		}
		osDelay(100);
		memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
		SIM_UART_ReInitializeRxDMA();
		return 1;
	}
	else return 0;
}

void check_socket_connection(){
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QPING=1,\"%s\"\r\n", IP_ADDRESS);
	send_AT_command((char*)command);
	char *first_pointer = NULL;
	char *second_pointer = NULL;
	char *third_pointer = NULL;
	char *fourth_pointer = NULL;
	char *fifth_pointer = NULL;
	while(first_pointer == NULL || second_pointer == NULL || third_pointer == NULL || fourth_pointer == NULL || fifth_pointer == NULL){
			receive_response("Check SOCKET CONNECTION\n");
			first_pointer = strstr((char*)response, "+QPING:");
			if(first_pointer != NULL){
				 second_pointer = strstr(first_pointer+1, "+QPING:");
			}
			if(second_pointer != NULL){
				 third_pointer = strstr(second_pointer+1, "+QPING:");
			}
			if(third_pointer != NULL){
				 fourth_pointer = strstr(third_pointer+1, "+QPING:");
			}
			if(fourth_pointer != NULL){
				 fifth_pointer = strstr(fourth_pointer+1, "+QPING:");
			}
	}
	receive_response("Check SOCKET CONNECTION\n");
	osDelay(100);
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
}

void send_data_to_server(int connect_id, uint8_t* message, int message_length){
	uint8_t command[256];
	int count_check = 0;
	check_socket_connection();
//	char message_hex[512];  // Each byte takes 2 hex chars

//	for (int i = 0; i < message_length; i++) {
//    snprintf(&message_hex[i * 2], 3, "%02X", message[i]);
//	}

	//snprintf((char *)command, sizeof(command), "AT+QISENDEX=%d,\"%s\"\r\n", connect_id, message_hex);

	snprintf((char *)command, sizeof(command), "AT+QISENDEX=%d,\"%s\"\r\n", connect_id, message);
	send_AT_command((char*)command);
	
	while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			receive_response("Check sending to server\n");
	}
	receive_response("Check sending to server\n");
	osDelay(100);
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
}

void check_data_sent_to_server(int connect_id){
	uint8_t command[256];
	int count_check = 0;
	snprintf((char *)command, sizeof(command), "AT+QISEND=%d,0\r\n", connect_id);
	send_AT_command((char*)command);
	while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			char output_elapsed[128];
			if(count_check >= 3){
				count_check = 0;
				break;
			}
			count_check++;
			snprintf(output_elapsed, 128, "Elapsed Time: %d\n", count_check);
			uart_transmit_string(&huart1, (uint8_t *)output_elapsed);
			receive_response("Check sending to server\n");
	}
	receive_response("Check sending to server\n");
	osDelay(100);
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
	
	snprintf((char *)command, sizeof(command), "AT+QIRD=%d,1500\r\n", connect_id);
	send_AT_command((char*)command);
	while(strstr((char *) response, "+QIRD") == NULL){
			receive_response("Check received data from server\n");
	}
	osDelay(100);
	uart_transmit_string(&huart1, (uint8_t*) "OUT OF receive data from server");
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
	
}
void close_connection(int connect_id){
	uint8_t command[256];
	snprintf((char *)command, sizeof(command), "AT+QICLOSE=%d\r\n", connect_id);
	send_AT_command((char*)command);
	while(strstr((char *) response, CHECK_RESPONSE) == NULL){
			receive_response("Check CLOSING to server\n");
	}
	receive_response("Check CLOSING to server\n");
	osDelay(100);
	memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
	SIM_UART_ReInitializeRxDMA();
}
void StartGSM(void const * argument)
{
	uart_transmit_string(&huart1, (uint8_t*)"Starting GSM: Pushing data to Server");
  /* USER CODE BEGIN StartGSM */
  /* Infinite loop */   // Buffer for received data
//	uint8_t response[4];   // Buffer for received data
	// Flag to indicate data reception
	RingBufferDmaU8_initUSARTRx(&SIMRxDMARing, &huart3, response, SIM_RESPONSE_MAX_SIZE);
	
	
	JT808_TerminalRegistration reg_msg = {
        .start_mask = 0x7E,
        .message_type = {0x01, 0x00},
        .message_length = {0x00, 0x2D},
        .terminal_phone_number = {0x00, 0x12, 0x34, 0x56, 0x78, 0x91},
        .message_serial_number = {0x00, 0x03},
        .province_ID = {0x00, 0x00},
        .city_ID = {0x00, 0x00},
        .manufacturer_ID = {0x00, 0x00, 0x00, 0x00, 0x00},
        .terminal_type = {0x41, 0x35, 0x4D, 0x00, 0x00, 0x00, 0x00, 0x00},
        .terminal_ID = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
        .plate_color = 0x00,
        .plate_no = {0x00, 0x00, 0x00, 0x00,0x35, 0x36, 0x37, 0x38, 0x39, 0x31, 0x20, 0x02, 0xD4, 0xC1, 0x41, 0x30, 0x30, 0x30, 0x30, 0x30},
        .check_sum = 0x00,  // Placeholder, will be set by the function
        .end_mask = 0x7E
    };

	size_t message_length;
	uint8_t *message_array = create_message_array(&reg_msg, &message_length);
	
	init_SIM_module();
	int isReady = 0;
	int process = 0;
	uint8_t output_location [256];
//HAL_UART_Receive_DMA(&huart1, rx_buffer, 128);
  for(;;)
  {
    HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_SET);
		osDelay(500);
		switch(process){
			//Wait for SIM module to start
			case 0: 
					uart_transmit_string(&huart1, (uint8_t *)"First CHECK\r\n");
					isReady = first_check_SIM();
					if(isReady) process++;
					break;
			case 1:
				// Check status of SIM. Wait until SIM is ready 
					uart_transmit_string(&huart1, (uint8_t *)"Check EVERYTHING READY\r\n");
					osDelay(100);
					int check_SIM = check_SIM_ready();
					osDelay(150);
					if (check_SIM == 0){
						memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
						SIM_UART_ReInitializeRxDMA();
						uart_transmit_string(&huart1,(uint8_t*) "Rebooting SIM module");
						reboot_SIM_module();
						
						process = 0;
					}
					else process++;
					break;
			case 2: 
				// Configure the PDP context
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: Configure PDP context\r\n");
					memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
					SIM_UART_ReInitializeRxDMA();
					configure_APN(1);
					process++;
					memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
					SIM_UART_ReInitializeRxDMA();
					break;
			case 3: 
				//Activate the PDP context. 
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: Activate PDP context\r\n");
					activate_context(1);
					osDelay(200);
					process++;
					memset(response, 0x00, SIM_RESPONSE_MAX_SIZE);
					SIM_UART_ReInitializeRxDMA();
					break;
			case 4: 
					//Open socket service
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: OPEN SOCKET SERVICE\r\n");
					int received_res = open_socket_service(1, 0, 0, 0);
					if(received_res){
						uart_transmit_string(&huart1, (uint8_t*) "Connect to Server successfully\n");
						process++;
					}
					else 
					{
						uart_transmit_string(&huart1, (uint8_t*) "Connect to Server Failed\n");
						int receive_deactivate = deactivate_context(1);
						if (receive_deactivate) process = 1;
						else process = 0;
					}
					break;
			case 5:
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: Register/Login to the server.\r\n");
					send_data_to_server(0,(uint8_t*)TERMINAL_REGISTRATION_DEMO, message_length);
					free(message_array);
					process++;
					break;
			case 6:
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: Check Register/Login\r\n");
					check_data_sent_to_server(0);
					receive_response("Check terminal register\n");
					process++;
					break;
			case 7:
					
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: Send Location\r\n");
					for(size_t idx = 0; idx < 10 ; idx++){	
						get_RTC_time_date();
						osDelay(500);
					}
					snprintf((char *)output_location, 256, "7E02000032001234567891000A000000000000000001406786064E4CC4000000000000%02d%02d%02d%02d%02d%02d0104000000002A020000300113310100FD0403F100000A7E",rmc_jt.date.Yr, rmc_jt.date.Mon, rmc_jt.date.Day, rmc_jt.tim.hour, rmc_jt.tim.min, rmc_jt.tim.sec);
					send_data_to_server(0, output_location,0);
					process++;
					break;
			case 8:
					uart_transmit_string(&huart1, (uint8_t *)"Inside process: Check Sending Location Report\r\n");
					check_data_sent_to_server(0);
					receive_response("Check location report\n");
					process++;
					break;
			case 9:
					close_connection(0);
					process = 4;
					break;
					
		}
		HAL_UART_Transmit(&huart1, (uint8_t*) "Hello from GSM\n", strlen("Hello from GSM\n"), 1000);
		HAL_GPIO_WritePin(GPIOC, GPIO_PIN_9, GPIO_PIN_RESET);
		osDelay(500);
  }
  /* USER CODE END StartGSM */
}