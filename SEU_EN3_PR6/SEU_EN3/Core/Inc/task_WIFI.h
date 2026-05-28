/*
 * task_WIFI.h
 *
 *  Created on: 24 may. 2023
 *      Author: pperez
 */

#ifndef INC_TASK_WIFI_H_
#define INC_TASK_WIFI_H_
#include "main.h"
#include <stdint.h>
#include "FreeRTOS.h"
#include "task_WIFI.h"
#include "task_COMM.h"


extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;

//extern uint8_t buff_recv[2048];


 

extern uint32_t global_wifi_ready;

void Task_WIFI_init(void);
void Task_WIFI( void *pvParameters );
void WIFI_RESET(void);
void WIFI_Boot(void);
void WIFI_Boot_TEST(void);


uint8_t * ESP_Send_Request(scomm_request_t *request, uint32_t timeout);
//uint8_t * ESP_Send_Request(uint8_t * dst_address, uint32_t dst_port, uint8_t * request);

#endif /* INC_TASK_WIFI_H_ */
