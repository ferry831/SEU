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

/* Variables globales leídas desde task_HW */
extern volatile uint8_t  g_alarm_active;
extern uint32_t          g_ldr_pct;    /* intensidad lumínica 0-1000 */
extern int32_t           g_temp_x10;   /* temperatura x10            */
extern uint32_t          g_pot_pct;    /* nivel de alarma 0-1000     */

#define ORION_HOST  "pperez2.disca.upv.es"
#define ORION_PORT  1026
#define SENSOR_ID   "SensorSEU_01"

void Task_ORION_init(void) {
    xTaskCreate(Task_ORION, "ORION", 2048, NULL, NORMAL_PRIORITY, NULL);
}

void Task_ORION(void *pvParameters) {

    static uint8_t http_buf[1024];
    static uint8_t body_buf[512];
    int signal;

    vTaskDelay(15000 / portTICK_RATE_MS);  /* esperar a que WiFi esté listo */

    while (1) {

        /* Solo publicar en modo Normal */
        if (g_mode != 0) {
            vTaskDelay(10000 / portTICK_RATE_MS);
            continue;
        }

        /* ---- Construir el cuerpo JSON ---- */
        /* Temperatura: valor, nivel alarma, min, max */
        float temp_c    = (float)g_temp_x10 / 10.0f;
        float pot_pct_f = (float)g_pot_pct  / 10.0f;
        float ldr_pct_f = (float)g_ldr_pct  / 10.0f;

        snprintf((char *)body_buf, sizeof(body_buf),
            "{"
            "\"contextElements\":[{"
            "\"type\":\"Sensor\","
            "\"isPattern\":\"false\","
            "\"id\":\"%s\","
            "\"attributes\":["
            "{\"name\":\"Temperatura\",\"type\":\"floatarray\","
             "\"value\":\"%.1f,%.1f,0.0,50.0\"},"
            "{\"name\":\"IntensidadLuz\",\"type\":\"floatarray\","
             "\"value\":\"%.1f,%.1f,0.0,100.0\"},"
            "{\"name\":\"Alarma\",\"type\":\"boolean\","
             "\"value\":\"%s\"},"
            "{\"name\":\"modo\",\"type\":\"string\","
             "\"value\":\"%s\"}"
            "]}],"
            "\"updateAction\":\"APPEND\""
            "}",
            SENSOR_ID,
            temp_c,   pot_pct_f,   /* Temperatura: valor, nivel alarma */
            ldr_pct_f, pot_pct_f,  /* IntensidadLuz: valor, nivel alarma */
            g_alarm_active ? "T" : "F",
            SENSOR_ID
        );

        /* ---- Construir la petición HTTP POST ---- */
        snprintf((char *)http_buf, sizeof(http_buf),
            "POST /v1/updateContext HTTP/1.1\r\n"
            "Host: %s\r\n"
            "Content-Type: application/json\r\n"
            "Accept: application/json\r\n"
            "Content-Length: %d\r\n"
            "\r\n"
            "%s",
            ORION_HOST,
            (int)strlen((char *)body_buf),
            body_buf
        );

        /*  Enviar a través de task_COMM  */
        signal = 1;
        do {
            if (xSemaphoreTake(COMM_xSem, 20000/portTICK_RATE_MS) != pdTRUE) {
                bprintf("ORION: timeout semaforo\r\n");
                HAL_NVIC_SystemReset();
            }
            if (COMM_request.command == 0) {
                COMM_request.command      = 1;
                COMM_request.result       = 0;
                COMM_request.dst_port     = ORION_PORT;
                COMM_request.dst_address  = (uint8_t *)ORION_HOST;
                COMM_request.HTTP_request = http_buf;
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

        bprintf("ORION: publicado OK\r\n");

        COMM_request.result  = 0;
        COMM_request.command = 0;

        vTaskDelay(10000 / portTICK_RATE_MS);  /* publicar cada 10 s */
    }
}
