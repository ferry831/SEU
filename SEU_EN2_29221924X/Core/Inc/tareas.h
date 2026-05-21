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
#define IoT_NAME   	 	  "SensorSEU_PPB00"
#define IoT_NAME_CLONE    "SensorSEU_ZL"

// seminario 6

#define HIGH_PRIORITY (( configMAX_PRIORITIES - 1 )>>1)+1
#define NORMAL_PRIORITY (( configMAX_PRIORITIES - 1 )>>1)
#define LOW_PRIORITY (( configMAX_PRIORITIES - 1 )>>1)-1

void CONFIGURACION_INICIAL(void);

typedef struct {
    float current;
    float min;
    float max;
    float threshold;
} SensorData_t;

typedef struct {
    SensorData_t temperature;
    SensorData_t light;

    bool local_alarm;
    bool remote_alarm;

    uint8_t cloned_node_id;
    uint8_t local_node_id;
} SystemState_t;

//g_mode (Entregable 2)
extern volatile int g_mode;
extern SystemState_t g_system_state;

extern osMutexId mutex;

#endif
