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
#include "cJSON.h"

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

        /* Publicar en modo Normal */
        if (global_mode == 0) {

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
                "\"value\":\"%d.%d,%ld.%ld,%ld.%ld,%lu.%lu\"},"
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

        /* Recibir datos en modo clon */
        else if (global_mode == 1) {           
            // 1. Obtener a quién queremos leer (00 a 26)
            uint8_t target_id = g_target_clone_id;
            
            /* Silenciar alarma remota */
            if (g_send_alarm_off == 1) {
                g_send_alarm_off = 0; // Limpiamos el flag
                g_alarma_src_seq++;   // Incrementamos secuencia

                snprintf(body_buf, sizeof(body_buf),
                    "{"
                    "\"Alarma_src\":{"
                    "\"type\":\"string\","
                    "\"value\":\"%s_%02lu\"}"
                    "}",
                    IoT_NAME, g_alarma_src_seq
                );

                signal = 1;
                do {
                    if (xSemaphoreTake(COMM_xSem, 20000 / portTICK_RATE_MS) != pdTRUE) {
                        bprintf("ORION CLON: timeout semaforo\r\n");
                        HAL_NVIC_SystemReset();
                    }
                    if (COMM_request.command == 0) {
                        COMM_request.command  = 1;
                        COMM_request.result   = 0;
                        COMM_request.dst_port = ORION_PORT;
                        strncpy((char *)COMM_request.dst_address, ORION_HOST, sizeof(COMM_request.dst_address) - 1);

                        snprintf((char *)COMM_request.HTTP_request, sizeof(COMM_request.HTTP_request),
                            "PATCH /v2/entities/SensorSEU_%02d/attrs HTTP/1.1\r\n"
                            "Host: %s\r\n"
                            "Content-Type: application/json\r\n"
                            "Accept: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "\r\n"
                            "%s",
                            target_id, ORION_HOST, (int)strlen(body_buf), body_buf
                        );

                        bprintf(">> [CLON] Silenciando alarma de SensorSEU_%02d...\r\n", target_id);
                        signal = 0;
                        xSemaphoreGive(COMM_xSem);
                    } else {
                        xSemaphoreGive(COMM_xSem);
                        vTaskDelay((1 + (rand() % 100)) / portTICK_RATE_MS);
                    }
                } while (signal);

                while (COMM_request.result != 1) vTaskDelay(10 / portTICK_RATE_MS);

                if (strstr((char *)COMM_request.HTTP_response, "204") != NULL ||
                    strstr((char *)COMM_request.HTTP_response, "200") != NULL) {
                    bprintf(">> [CLON] Alarma silenciada con exito.\r\n");
                } else {
                    bprintf(">> [CLON] Error silenciando alarma.\r\n");
                }

                COMM_request.result  = 0;
                COMM_request.command = 0;
                vTaskDelay(2000 / portTICK_RATE_MS); 
            }
            // --- B) COMPORTAMIENTO NORMAL: LECTURA (GET) ---
            else {
                signal = 1;
                do {
                    if (xSemaphoreTake(COMM_xSem, 20000 / portTICK_RATE_MS) != pdTRUE) {
                        bprintf("ORION CLON: timeout semaforo\r\n");
                        HAL_NVIC_SystemReset();
                    }
                    
                    if (COMM_request.command == 0) {
                        COMM_request.command  = 1;
                        COMM_request.result   = 0;
                        COMM_request.dst_port = ORION_PORT;

                        strncpy((char *)COMM_request.dst_address, ORION_HOST, sizeof(COMM_request.dst_address) - 1);

                        // Petición GET al nodo clonado
                        snprintf((char *)COMM_request.HTTP_request, sizeof(COMM_request.HTTP_request),
                            "GET /v2/entities/SensorSEU_%02d HTTP/1.1\r\n"
                            "Host: %s\r\n"
                            "Accept: application/json\r\n"
                            "Connection: close\r\n\r\n",
                            target_id, ORION_HOST
                        );

                        bprintf(">> [CLON] Pidiendo datos a SensorSEU_%02d...\r\n", target_id);

                        signal = 0;
                        xSemaphoreGive(COMM_xSem);
                    } else {
                        xSemaphoreGive(COMM_xSem);
                        vTaskDelay((1 + (rand() % 100)) / portTICK_RATE_MS);
                    }
                } while (signal);

                /* Esperar respuesta */
                while (COMM_request.result != 1) vTaskDelay(10 / portTICK_RATE_MS);

                /* DEBUG: Ver respuesta del nodo clonado */
                bprintf("ORION CLON resp cruda: %.200s\r\n", COMM_request.HTTP_response);

                // 2. Parsear el JSON
                if (strlen((char *)COMM_request.HTTP_response) > 0) {
                    cJSON *json = cJSON_Parse((char *)COMM_request.HTTP_response);
                    
                    if (json != NULL) {
                        
                        // Extraer Alarma
                        cJSON *alarma_obj = cJSON_GetObjectItem(json, "Alarma");
                        if (alarma_obj != NULL) {
                            cJSON *val = cJSON_GetObjectItem(alarma_obj, "value");
                            if (val != NULL && val->valuestring != NULL) {
                                g_clone_alarm_active = (strcmp(val->valuestring, "T") == 0) ? 1 : 0;
                            }
                        }

                        // Extraer Temperatura ("Actual,Max,Min,Umbral")
                        cJSON *temp_obj = cJSON_GetObjectItem(json, "Temperatura");
                        if (temp_obj != NULL) {
                            cJSON *val = cJSON_GetObjectItem(temp_obj, "value");
                            if (val != NULL && val->valuestring != NULL) {
                                float t_act, t_max, t_min, t_thr;
                                if (sscanf(val->valuestring, "%f,%f,%f,%f", &t_act, &t_max, &t_min, &t_thr) == 4) {
                                    g_clone_temp_current = t_act;
                                    g_clone_temp_max = t_max;
                                    g_clone_temp_min = t_min;
                                    g_clone_temp_thr = t_thr;
                                }
                            }
                        }

                        // Extraer Luz ("Actual,Max,Min,Umbral")
                        cJSON *luz_obj = cJSON_GetObjectItem(json, "IntensidadLuz");
                        if (luz_obj != NULL) {
                            cJSON *val = cJSON_GetObjectItem(luz_obj, "value");
                            if (val != NULL && val->valuestring != NULL) {
                                float l_act, l_max, l_min, l_thr;
                                if (sscanf(val->valuestring, "%f,%f,%f,%f", &l_act, &l_max, &l_min, &l_thr) == 4) {
                                    g_clone_ldr_current = l_act;
                                    g_clone_ldr_max = l_max;
                                    g_clone_ldr_min = l_min;
                                    g_clone_ldr_thr = l_thr;
                                }
                            }
                        }

                        cJSON_Delete(json);
                        bprintf(">> [CLON] Datos de SensorSEU_%02d parseados y guardados.\r\n", target_id);
                    } else {
                        bprintf(">> [CLON] Error parseando JSON de respuesta.\r\n");
                    }
                } 
                
                COMM_request.result  = 0;
                COMM_request.command = 0;
                vTaskDelay(1000 / portTICK_RATE_MS); // Polling cada segundo en modo clon
            }
        } 
        
        /* Mode test (ignorar WiFi)*/
        else {
            vTaskDelay(10000 / portTICK_RATE_MS);
            continue;
        }
    }
}