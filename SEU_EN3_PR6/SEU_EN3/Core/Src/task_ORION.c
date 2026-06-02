#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "semphr.h"
#include "tareas.h"
#include "task_CONSOLE.h"
#include "task_COMM.h"
#include "task_ORION.h"
#include "main.h"

#define ORION_HOST  "pperezs-sec.disca.upv.es"
#define ORION_PORT  1026
#define SENSOR_ID   "SensorSEU_19" //Usar "SensorSEU_00" para pruebas (está siempre activo) | En laboratorio: "SensorSEU_19"

void Task_ORION_init(void) {
    BaseType_t res_task;
    res_task = xTaskCreate(Task_ORION, "ORION", 2048, NULL,
                           NORMAL_PRIORITY, NULL);
    if (res_task != pdPASS) {
        bprintf("PANIC: Error al crear Tarea ORION\r\n");
        fflush(NULL);
        while(1);
    }
}

void Task_ORION(void *pvParameters) {

    char body_buf[512];
    int  signal;

    /* Esperar a que el WiFi esté listo */
    vTaskDelay(15000 / portTICK_RATE_MS);

    while (1) {

        /* Solo publicar en modo Normal (g_mode == 0) */
        if (global_mode != 0) {
            vTaskDelay(10000 / portTICK_RATE_MS);
            continue;
        }

        /* ---- Leer sensores ---- */
        int32_t  temp_x10 = g_temp_x10;
        uint32_t ldr_pct  = g_ldr_pct;
        uint32_t pot_pct  = g_pot_pct;
        int32_t  temp_max = g_temp_max;
        int32_t  temp_min = g_temp_min;
        uint32_t ldr_max  = g_ldr_max;
        uint32_t ldr_min  = g_ldr_min;

        int temp_int = temp_x10 / 10;
        int temp_dec = (temp_x10 < 0 ? -temp_x10 : temp_x10) % 10;

        /* ---- Construir el cuerpo JSON ---- */
        snprintf(body_buf, sizeof(body_buf),
            "{"
            "\"Temperatura\":{"
            "\"type\":\"String\","
            "\"value\":\"%d.%d,%d.%d,%d.%d,%lu.%lu\"},"
            "\"IntensidadLuz\":{"
            "\"type\":\"String\","
            "\"value\":\"%lu.%lu,%lu.%lu,%lu.%lu,%lu.%lu\"},"
            "\"Alarma\":{"
            "\"type\":\"boolean\","
            "\"value\":\"%s\"},"
            "\"Alarma_src\":{"
            "\"type\":\"string\","
            "\"value\":\"\"},"
            "\"modo\":{"
            "\"type\":\"string\","
            "\"value\":\"%s\"}"
            "}",
            /* Temperatura: actual, max, min, nivel_disparo */
            temp_int, temp_dec,
            temp_max/10, (temp_max<0?-temp_max:temp_max)%10,
            temp_min/10, (temp_min<0?-temp_min:temp_min)%10,
            pot_pct/10, pot_pct%10,
            /* IntensidadLuz: actual, max, min, nivel_disparo */
            ldr_pct/10, ldr_pct%10,
            ldr_max/10, ldr_max%10,
            ldr_min/10, ldr_min%10,
            pot_pct/10, pot_pct%10,
            /* Alarma */
            g_alarm_active ? "T" : "F",
            /* modo */
            IoT_NAME
        );


        /* DEBUG: ver el JSON antes de enviarlo */
               bprintf("JSON len: %d\r\n", (int)strlen(body_buf));
               bprintf("JSON: %s\r\n", body_buf);

        /* ---- Construir la petición HTTP POST ---- */
        /* Rellenar la estructura COMM_request directamente */
        signal = 1;
        do {
            if (xSemaphoreTake(COMM_xSem,
                    20000 / portTICK_RATE_MS) != pdTRUE) {
                bprintf("ORION: timeout semaforo\r\n");
                HAL_NVIC_SystemReset();
            }
            if (COMM_request.command == 0) {
                COMM_request.command  = 1;
                COMM_request.result   = 0;
                COMM_request.dst_port = ORION_PORT;

                /* Copiar dirección al buffer fijo */
                strncpy((char *)COMM_request.dst_address,
                        ORION_HOST,
                        sizeof(COMM_request.dst_address) - 1);

                /* Construir petición HTTP directamente en el buffer */
                snprintf((char *)COMM_request.HTTP_request,
                         sizeof(COMM_request.HTTP_request),
					"PATCH /v2/entities/" IoT_NAME "/attrs HTTP/1.1\r\n"
                    "Host: %s\r\n"
                    "Content-Type: application/json\r\n"
                    "Accept: application/json\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s",
                    ORION_HOST,
                    (int)strlen(body_buf),
                    body_buf
                );

                signal = 0;
                xSemaphoreGive(COMM_xSem);
            } else {
                xSemaphoreGive(COMM_xSem);
                vTaskDelay((1 + (rand() % 100)) / portTICK_RATE_MS);
            }
        } while (signal);

        /* Esperar respuesta */
        while (COMM_request.result != 1)
            vTaskDelay(10 / portTICK_RATE_MS);

        bprintf("ORION resp: %.200s\r\n", COMM_request.HTTP_response);

        /* Comprobar si la publicación fue exitosa */
        if (strstr((char *)COMM_request.HTTP_response, "204") != NULL ||
            strstr((char *)COMM_request.HTTP_response, "200") != NULL)
            bprintf("ORION: publicado OK\r\n");
        else
            bprintf("ORION: error al publicar\r\n");

        COMM_request.result  = 0;
        COMM_request.command = 0;

        vTaskDelay(30000 / portTICK_RATE_MS);
    }
}
