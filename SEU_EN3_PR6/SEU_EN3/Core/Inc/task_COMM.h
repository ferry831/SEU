/*
 * Task_COMM.h
 *
 *  Created on: 24 may. 2023
 *      Author: pperez
 */

#ifndef INC_TASK_COMM_H_
#define INC_TASK_COMM_H_


#include "FreeRTOS.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdlib.h>

#include "semphr.h"

// internal

void cleanResponse(uint8_t * data,int maxlen);

typedef struct REQUEST_DUMMY {
								int32_t  command;
								int32_t  result;
								int32_t  dst_port;
								uint8_t * 	 dst_address;
								uint8_t * 	 HTTP_request;
								uint8_t *   HTTP_response;
} scomm_request_t;



extern scomm_request_t COMM_request;
extern SemaphoreHandle_t COMM_xSem;
extern SemaphoreHandle_t COMM_WAIT_xSem;
extern uint32_t global_wifi_ready;

extern uint8_t buff_json[2048];
extern uint8_t buff_request[2048];

#include "main.h"
#include <stdint.h>
#include "FreeRTOS.h"



extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;

extern uint8_t buff_recv[2048];


// WIFI
#define SSID			  "routerSEU"
#define SSID_PASS		  "00000000"

void WIFI_RESET(void);
void WIFI_Boot(void);
void WIFI_Boot_TEST(void);

uint8_t * ESP_Send_Request(uint8_t * dst_address, uint32_t dst_port, uint8_t * request);

extern uint32_t global_comm_it;
void Task_COMM_init(void);
void Task_COMM( void *pvParameters );


#endif /* INC_TASK_COMM_H_ */
