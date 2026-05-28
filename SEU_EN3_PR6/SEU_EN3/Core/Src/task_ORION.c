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
#define SENSOR_ID   "SensorSEU_19"

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

        int temp_int = temp_x10 / 10;
        int temp_dec = (temp_x10 < 0 ? -temp_x10 : temp_x10) % 10;

        /* ---- Construir el cuerpo JSON ---- */
        snprintf(body_buf, sizeof(body_buf),
            "{"
            "\"contextElements\":[{"
            "\"type\":\"Sensor\","
            "\"isPattern\":\"false\","
            "\"id\":\"%s\","
            "\"attributes\":["
            "{\"name\":\"Temperatura\",\"type\":\"floatarray\","
             "\"value\":\"%d.%d,%lu.%lu,0.0,50.0\"},"
            "{\"name\":\"IntensidadLuz\",\"type\":\"floatarray\","
             "\"value\":\"%lu.%lu,%lu.%lu,0.0,100.0\"},"
            "{\"name\":\"Alarma\",\"type\":\"boolean\","
             "\"value\":\"%s\"},"
            "{\"name\":\"modo\",\"type\":\"string\","
             "\"value\":\"%s\"}"
            "]}],"
            "\"updateAction\":\"APPEND\""
            "}",
            SENSOR_ID,
            temp_int, temp_dec,
            pot_pct / 10, pot_pct % 10,
            ldr_pct / 10, ldr_pct % 10,
            pot_pct / 10, pot_pct % 10,
            g_alarm_active ? "T" : "F",
            SENSOR_ID
        );

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
                    "POST /v1/updateContext HTTP/1.1\r\n"
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

        /* Comprobar si la publicación fue exitosa */
        if (strstr((char *)COMM_request.HTTP_response, "200") != NULL)
            bprintf("ORION: publicado OK\r\n");
        else
            bprintf("ORION: error al publicar\r\n");

        COMM_request.result  = 0;
        COMM_request.command = 0;

        vTaskDelay(10000 / portTICK_RATE_MS);
    }
}
