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

SystemState_t g_system_state = {
    // current, min, max, threshold
    .temperature = {0.0f, 100.0f, -100.0f, 0.0f}, // TODO: ajustar valores min/max según el sensor
    .light = {0.0f, 100.0f, -100.0f, 0.0f}, // TODO: ajustar valores min/max según el sensor

    .local_alarm = false,
    .remote_alarm = false,

    .cloned_node_id = 0,
    .local_node_id = 19 // 19 por el número del puesto en el lab
};

osMutexId mutex;

void CONFIGURACION_INICIAL(void){

	BaseType_t res_task;

	osMutexDef(mutexDef);
	mutex = osMutexCreate(osMutex(mutexDef));

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


