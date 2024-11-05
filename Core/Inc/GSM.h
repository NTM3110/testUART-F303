#ifndef GSM_H
#define GSM_H

#include "main.h"
#include "RS232-UART1.h"

#define SIM_RESPONSE_MAX_SIZE				256
#define SIM_GPIO_Port								GPIOC
#define SIM_GPIO_PWR_CTRL_Pin				GPIO_PIN_0

#define APN_NAME										"e-connect"
#define APN_USERNAME								""
#define APN_PASSWD									""
#define APN_AUTHEN									0
#define SERVICE_TYPE 								"TCP"
#define IP_ADDRESS									"46.101.24.212"
#define REMOTE_PORT									5015
#define CHECK_RESPONSE							"OK"


#define SIM_ENABLE()   HAL_GPIO_WritePin(SIM_GPIO_Port, SIM_GPIO_PWR_CTRL_Pin, GPIO_PIN_SET)
#define SIM_DISABLE()  HAL_GPIO_WritePin(SIM_GPIO_Port, SIM_GPIO_PWR_CTRL_Pin	, GPIO_PIN_RESET)


extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart1;

void send_AT_command(const char *command);
void receive_response(char *cmd_str);

void init_SIM_module();

int configure_APN(int context_id);

void activate_context(int context_id);

void check_activate_context();

int open_socket_service(int context_id, int connect_id, char *service_type, char *ip_address, int remote_port, int local_port, int access_mode);

void SIM_UART_ReInitializeRxDMA(void);

void StartGSM(void const * argument);
#endif