/*
 * tareas.h
 *
 *  Created on: Apr 29, 2021
 *      Author: pperez
 */

#ifndef __TAREAS_H__
#define __TAREAS_H__

#include <stdint.h>
#include "main.h"
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


//operativa
#define FREERTOS_5SEG		(5000/portTICK_PERIOD_MS)

// Orion Context IoT name
#define IoT_NAME   	 	  "SensorSEU_20"
#define IoT_NAME_CLONE    "SensorSEU_ZL"
#define SSID      "routerSEU"
#define SSID_PASS "00000000"
// seminario 6

#define HIGH_PRIORITY (( configMAX_PRIORITIES - 1 )>>1)+1
#define NORMAL_PRIORITY (( configMAX_PRIORITIES - 1 )>>1)
#define LOW_PRIORITY (( configMAX_PRIORITIES - 1 )>>1)-1

void CONFIGURACION_INICIAL(void);

// Variables modo clon (Entregable 3)
extern volatile uint8_t  g_target_clone_id;
extern volatile uint8_t  g_clone_alarm_active;
extern volatile uint8_t g_send_alarm_off;

extern volatile float    g_clone_temp_current;
extern volatile float    g_clone_temp_max;
extern volatile float    g_clone_temp_min;
extern volatile float    g_clone_temp_thr;

extern volatile float    g_clone_ldr_current;
extern volatile float    g_clone_ldr_max;
extern volatile float    g_clone_ldr_min;
extern volatile float    g_clone_ldr_thr;

//global_mode (Entregable 2)
extern volatile int global_mode;
//Variables Sensores (PR6)
extern volatile uint32_t g_ldr_pct;
extern volatile int32_t  g_temp_x10;
extern volatile uint32_t g_pot_pct;

/* Sensores: Máximos y mínimos */
extern volatile int32_t  g_temp_max;
extern volatile int32_t  g_temp_min;
extern volatile uint32_t g_ldr_max;
extern volatile uint32_t g_ldr_min;
/* Alarma_src: nodo clon silenciar */
extern volatile char     g_alarma_src[32];
extern volatile uint32_t g_alarma_src_seq;


#endif
