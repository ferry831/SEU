#include <tareas.h>
#include <main.h>
#include "FreeRTOS.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdlib.h>

#include "semphr.h"
#include <string.h>

#include <task.h>
#include <math.h>


#include "task_CONSOLE.h"
#include "task_TIME.h"
#include "task_COMM.h"
#include "task_EJER3.h"
#include "task_HW.h"

	//g_mode (Entregable 2)
	volatile int g_mode = 0;  /* 0=Normal, 1=Clone, 2=Test */

void CONFIGURACION_INICIAL(void){



	BaseType_t res_task;

 	IObuff=bufferCreat(128);

 	if (!IObuff) return;

	res_task=xTaskCreate(Task_CONSOLE,"CONSOLA",2048,NULL,	NORMAL_PRIORITY,NULL);
 		if( res_task != pdPASS ){
 				printf("PANIC: Error al crear Tarea Visualizador\r\n");
 				fflush(NULL);
 				while(1);
 		}

 	Task_EJER3_init();
 	Task_TIME_init();
 	Task_COMM_init();
 	Task_HW_init();

}


