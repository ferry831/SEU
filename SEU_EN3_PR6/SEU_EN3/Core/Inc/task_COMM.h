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
void cleanResponseTextPlain(uint8_t * data,int maxlen);

typedef struct REQUEST_DUMMY {
								int32_t  command;
								int32_t  result;
								int32_t  dst_port;
								uint8_t  dst_address[100];
								uint8_t  AUXILIARY[2048];
								uint8_t  HTTP_request[2048];
								uint8_t  HTTP_response[2048];
} scomm_request_t;



extern scomm_request_t COMM_request;
extern SemaphoreHandle_t COMM_xSem;
extern SemaphoreHandle_t COMM_WAIT_xSem;
extern uint32_t global_wifi_ready;

//extern uint8_t buff_json[2048];
//extern uint8_t buff_request[2048];


extern uint32_t global_comm_it;
void Task_COMM_init(void);
void Task_COMM( void *pvParameters );


#endif /* INC_TASK_COMM_H_ */
