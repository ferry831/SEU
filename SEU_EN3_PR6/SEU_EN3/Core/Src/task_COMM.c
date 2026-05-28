#include <stdint.h>
#include "task_COMM.h"
#include "FreeRTOS.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdlib.h>
#include "semphr.h"
#include "tareas.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdlib.h>

#include "semphr.h"
#include <string.h>

#include <task.h>

#include "task_CONSOLE.h"
#include "task_WIFI.h"
#include "task_HW.h"


scomm_request_t COMM_request;
SemaphoreHandle_t COMM_xSem = NULL;
SemaphoreHandle_t COMM_WAIT_xSem = NULL;
uint32_t global_comm_it;

uint8_t buff_env[2048];
uint8_t buff_env_aux[2048];
uint8_t buff_recv[2048];



void Task_COMM_init(void){
	BaseType_t res_task;

	global_comm_it=0;

	COMM_xSem=xSemaphoreCreateMutex();

	if( COMM_xSem == NULL ){
		printf("PANIC: Error al crear el Semáforo ORION\r\n");
		fflush(NULL);
		while(1);
	}

	COMM_WAIT_xSem= xSemaphoreCreateBinary();

		if( COMM_WAIT_xSem == NULL ){
			printf("PANIC: Error al crear el Semáforo ORION 2\r\n");
			fflush(NULL);
			while(1);
		}


 	res_task=xTaskCreate(Task_COMM,"COMMUNICATION",2048,NULL,	NORMAL_PRIORITY,NULL);
 	if( res_task != pdPASS ){
 	 				printf("PANIC: Error al crear Tarea Comunicaciones\r\n");
 	 				fflush(NULL);
 	 				while(1);
 	}
}

void Task_COMM( void *pvParameters ){

	int signal;




	while (global_wifi_ready==0) osDelay(1);

	while (1) {

		if (global_mode==2) {
			osDelay(10);
			continue;
		}

		signal=1;
		do {
			if (xSemaphoreTake(COMM_xSem, 20000/portTICK_RATE_MS  ) != pdTRUE ){// si en 20 segundos no he continuado entrado en orion mmm mal rollito harakiri
				bprintf("\r\n\n\nHARAKIRI!!\n\n\n");
		   		HAL_NVIC_SystemReset();
		}

		// aquí tengo la exclusión mutua asegurada.
		if (COMM_request.command==1) //any thread has a request
			signal=0;
		else{
			xSemaphoreGive(COMM_xSem); // i’m going out critical section
			osDelay(10);
		}

		}while(signal);



		COMM_request.command=2; // busy, block until any thread gets this response.

		xSemaphoreGive(COMM_xSem); // i’m going out critical section

		// Here is safe access to COMM_request because other thread has put COMM_request.command to 1 so before write any other thread must read 0 in this item.


		//COMM_request.HTTP_response=ESP_Send_Request(COMM_request.dst_address,COMM_request.dst_port,COMM_request.HTTP_request);

		ESP_Send_Request(&COMM_request,10000);

		// remove headers and +IPD messages from ESP32 as a result you have json pure string IF it is completely recover from server in a unique +IPD message

//bprintf("\r\n\n\n**++ %s ++** \n\n\n",COMM_request.HTTP_response);



		// if ESP does not receive response or there are any errors result must be different to 1
		COMM_request.result=1;
		global_comm_it++;
		}
}





void cleanResponse(uint8_t * data,int maxlen)
{
	int t,i;




	uint8_t * j,*from, *pc;


	 i=0;
	 while ((j=(uint8_t *)strstr((char *)data,"+IPD"))){

		 from=(uint8_t *)strstr((char *)j,":");
		 from++;

		 	 for (pc=from;pc<(data+2048);pc++)
			 	*(j++)=*(from++);
	 };
	 	 do
			                  {
			                  }while(data[i++]!='{');
			                  i--;
	for (t=0;t<(2048-i);t++)
		data[t]=data[t+i];

	i=0;

    for (t=0;t<2048;t++)
    	if (data[t]=='}')
    		i=t;
    data[i+1]=0;




}




void cleanResponseTextPlain(uint8_t * data,int maxlen)
{
	int t,i;

	uint8_t * j,*from, *pc;

	 i=0;
	 while ((j=(uint8_t *)strstr((char *)data,"+IPD"))){

		 from=(uint8_t *)strstr((char *)j,":");
		 from++;

		 	 for (pc=from;pc<(data+2048);pc++)
			 	*(j++)=*(from++);
	 };

	do{}while(data[i++]!='"');

	for (t=0;t<(2048-i);t++)
		data[t]=data[t+i];

	i=0;
    for (t=0;t<2048;t++)
    	if (data[t]=='"')
    		i=t;
    data[i+1]=0;

}
