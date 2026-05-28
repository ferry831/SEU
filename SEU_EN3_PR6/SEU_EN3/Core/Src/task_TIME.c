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
#include "task_TIME.h"
#include "cJSON.h"


#include <time.h>

uint32_t global_time_it;
int time_available = 0;
time_t tm_offset;
time_t ticks_since_start_ts;


void Task_TIME_init(void){

	BaseType_t res_task;
	global_time_it=0;
	global_wifi_ready=0;
	res_task=xTaskCreate( Task_TIME,"TIME",2048,NULL,	NORMAL_PRIORITY,NULL);
	if( res_task != pdPASS ){
		bprintf("PANIC: Error al crear Tarea TIME\r\n");
		fflush(NULL);
		while(1);
	}

}

void Task_TIME( void *pvParameters ){

	int signal;
	CJSON_PUBLIC(cJSON *) jsons1;

 	cJSON *name = NULL;


	while (1) {

		signal=1;
		do {
			if (xSemaphoreTake(COMM_xSem, 20000/portTICK_RATE_MS  ) != pdTRUE ){// si en 20 segundos no he continuado entrado en orion mmm mal rollito harakiri
				bprintf("\r\n\n\nHARAKIRI!!\n\n\n");
		   		HAL_NVIC_SystemReset();
			}

			// aquí tengo la exclusión mutua asegurada.
			if (COMM_request.command==0){ //nada quiere nada
				COMM_request.command=1;
				COMM_request.result=0;
				// now structure access is secure, nobody can rewrite it if flag command is 1
				// EJERCICIO 5
				COMM_request.dst_port=5000; //rellenar
				COMM_request.dst_address=(uint8_t *)"pperez2.disca.upv.es"; // url
				COMM_request.HTTP_request=(uint8_t *)"GET /tiempo HTTP/1.1\r\n\r\n"; // campo

				signal=0;
				xSemaphoreGive(COMM_xSem); // i’m going out critical section
			}
			else {
			      xSemaphoreGive(COMM_xSem); // i’m going out critical section
			      vTaskDelay((1+(rand()%100))/portTICK_RATE_MS);
			}

		}while(signal);

  		// Here you must parse json response in COMM_request.response item
		while (COMM_request.result!=1) vTaskDelay(10/portTICK_RATE_MS );

////
	jsons1 = cJSON_Parse((const char *)COMM_request.HTTP_response);
	if (jsons1) {
				name = cJSON_GetObjectItem(jsons1, "tiempo_actual");
			    struct tm tm_date = {0};
			    int year, month, day, hour, minute, second;
			    sscanf(name->valuestring, "%d-%d-%d %d:%d:%d", &year, &month, &day, &hour, &minute, &second);
			    tm_date.tm_year = year - 1900; // Ajustar año
			    tm_date.tm_mon = month - 1;    // Ajustar mes
			    tm_date.tm_mday = day;
			    tm_date.tm_hour = hour;
			    tm_date.tm_min = minute;
			    tm_date.tm_sec = second;
			    tm_date.tm_isdst = -1;         // Dejar que la biblioteca determine el DST

			    // Obtener la fecha de inicio en time_t
			    tm_offset = mktime(&tm_date);
			    // Convertir ticks a segundos
			    ticks_since_start_ts = xTaskGetTickCount();

			    cJSON_Delete(jsons1);
				time_available=1;
	}
	else
		bprintf("TIME: Response error \r\n");

////
	    COMM_request.result=0;
		COMM_request.command=0; // this thread liberates requesting structure for others threads
		xSemaphoreGive(COMM_xSem); // i’m going out critical section

		global_time_it++;
		vTaskDelay(100000/portTICK_RATE_MS); // cada 100 segundos pide el time, podria ser cada más tiempo, en función de la deriva o cada menos para hacer pruebas.

	}
}

int task_TIME_timeAvailable(void){
	return time_available;
}

time_t task_TIME_getTime(void){

	return tm_offset + (xTaskGetTickCount() - ticks_since_start_ts)/portTICK_RATE_MS/1000;
}

