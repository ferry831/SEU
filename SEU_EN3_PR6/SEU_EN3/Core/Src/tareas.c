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
#include "task_WIFI.h"
#include "task_ORION.h"

    //g_mode (Entregable 2)
    volatile int global_mode = 0;  /* 0=Normal, 1=Clone, 2=Test */
    //Variables Sensores (PR6)
    volatile uint32_t g_ldr_pct  = 0;
    volatile int32_t  g_temp_x10 = 0;
    volatile uint32_t g_pot_pct  = 0;
    //Inicialización Max/min Sensores (EN3)
    volatile int32_t  g_temp_max      = -9999;
    volatile int32_t  g_temp_min      =  9999;
    volatile uint32_t g_ldr_max       = 0;
    volatile uint32_t g_ldr_min       = 1000;
    volatile char     g_alarma_src[32] = "";
    volatile uint32_t g_alarma_src_seq = 0;
    // Variables Modo Clon (EN3)
    volatile uint8_t  g_target_clone_id = 0;
    volatile uint8_t  g_clone_alarm_active = 0;
    volatile uint8_t g_send_alarm_off = 0;

    volatile float    g_clone_temp_current = 0.0f;
    volatile float    g_clone_temp_max = 0.0f;
    volatile float    g_clone_temp_min = 0.0f;
    volatile float    g_clone_temp_thr = 0.0f;

    volatile float    g_clone_ldr_current = 0.0f;
    volatile float    g_clone_ldr_max = 0.0f;
    volatile float    g_clone_ldr_min = 0.0f;
    volatile float    g_clone_ldr_thr = 0.0f;

void CONFIGURACION_INICIAL(void){

    BaseType_t res_task;

    IObuff=bufferCreat(128);

    if (!IObuff) return;

    res_task=xTaskCreate(Task_CONSOLE,"CONSOLA",2048,NULL,NORMAL_PRIORITY,NULL);
        if( res_task != pdPASS ){
                printf("PANIC: Error al crear Tarea Visualizador\r\n");
                fflush(NULL);
                while(1);
        }

		/* 	Task_EJER3_init();
 	Task_TIME_init();
 	Task_COMM_init();
 	Task_HW_init();
 	Task_ORION_init();*/
	
      Task_WIFI_init();     // <- primero WiFi
        Task_COMM_init();     // <- luego COMM
        //Task_TIME_init();
        Task_ORION_init();
        Task_HW_init();
        Task_EJER3_init();

}
