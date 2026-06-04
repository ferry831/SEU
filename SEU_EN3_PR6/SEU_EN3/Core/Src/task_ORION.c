#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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

/*
 * SENSOR_ID realmente no se usa para publicar.
 * El identificador real del nodo es IoT_NAME, definido en tareas.h.
 *
 * Placa 19:
 *   #define IoT_NAME "SensorSEU_19"
 *
 * Placa 20:
 *   #define IoT_NAME "SensorSEU_20"
 */
#define SENSOR_ID   IoT_NAME

static char last_alarma_src_processed[40] = "";

/* -----------------------------------------------------------
 * Procesar orden remota Alarma_src.
 * Esto lo usa el nodo conectado, por ejemplo la placa 19,
 * cuando otro clon escribe Alarma_src = SensorSEU_20_01.
 * ----------------------------------------------------------- */
static void procesar_alarma_src_local(const char *src)
{
    if (src == NULL || src[0] == '\0') {
        return;
    }

    if (strcmp(src, last_alarma_src_processed) == 0) {
        return;
    }

    strncpy(last_alarma_src_processed, src, sizeof(last_alarma_src_processed) - 1);
    last_alarma_src_processed[sizeof(last_alarma_src_processed) - 1] = '\0';

    bprintf(">> [NORMAL] Nueva orden Alarma_src recibida: %s\r\n", src);

    /*
     * Igual que pulsar el boton derecho local:
     * apagamos la alarma y la desarmamos temporalmente.
     */
    g_alarm_active   = 0;
    g_alarm_disarmed = 1;
    g_disarm_tick    = xTaskGetTickCount();

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

    bprintf(">> [NORMAL] Alarma local apagada por clon. Reactivacion en 10s\r\n");
}

/* -----------------------------------------------------------
 * Parser sencillo para leer CSV con 4 valores:
 * "25.9,27.5,25,30"
 *
 * Evita sscanf("%f"), porque en STM32 puede no funcionar
 * si no esta habilitado el soporte de float en scanf.
 * ----------------------------------------------------------- */
static const char *parse_num_x10(const char *p, int32_t *out)
{
    int sign = 1;
    int32_t entero = 0;
    int32_t dec = 0;

    while (*p == ' ' || *p == '\t') {
        p++;
    }

    if (*p == '-') {
        sign = -1;
        p++;
    }

    while (*p >= '0' && *p <= '9') {
        entero = entero * 10 + (*p - '0');
        p++;
    }

    if (*p == '.') {
        p++;

        if (*p >= '0' && *p <= '9') {
            dec = (*p - '0');
            p++;

            while (*p >= '0' && *p <= '9') {
                p++;
            }
        }
    }

    *out = sign * (entero * 10 + dec);
    return p;
}

static int parse_csv4_x10(const char *s,
                          int32_t *a,
                          int32_t *b,
                          int32_t *c,
                          int32_t *d)
{
    const char *p = s;

    p = parse_num_x10(p, a);
    if (*p != ',') return 0;
    p++;

    p = parse_num_x10(p, b);
    if (*p != ',') return 0;
    p++;

    p = parse_num_x10(p, c);
    if (*p != ',') return 0;
    p++;

    p = parse_num_x10(p, d);

    return 1;
}

/* -----------------------------------------------------------
 * Enviar una request usando Task_COMM.
 * Devuelve 1 si la request se ha entregado a COMM y hay respuesta,
 * 0 si hay error de semaforo.
 * ----------------------------------------------------------- */
static int orion_send_request(const char *request_text)
{
    int signal = 1;

    do {
        if (xSemaphoreTake(COMM_xSem, 20000 / portTICK_RATE_MS) != pdTRUE) {
            bprintf("ORION: timeout semaforo\r\n");
            return 0;
        }

        if (COMM_request.command == 0) {
            COMM_request.command  = 1;
            COMM_request.result   = 0;
            COMM_request.dst_port = ORION_PORT;

            strncpy((char *)COMM_request.dst_address,
                    ORION_HOST,
                    sizeof(COMM_request.dst_address) - 1);

            COMM_request.dst_address[sizeof(COMM_request.dst_address) - 1] = '\0';

            strncpy((char *)COMM_request.HTTP_request,
                    request_text,
                    sizeof(COMM_request.HTTP_request) - 1);

            COMM_request.HTTP_request[sizeof(COMM_request.HTTP_request) - 1] = '\0';

            signal = 0;
            xSemaphoreGive(COMM_xSem);
        } else {
            xSemaphoreGive(COMM_xSem);
            vTaskDelay((1 + (rand() % 100)) / portTICK_RATE_MS);
        }
    } while (signal);

    while (COMM_request.result != 1) {
        vTaskDelay(10 / portTICK_RATE_MS);
    }

    return 1;
}

static void orion_clear_comm_request(void)
{
    COMM_request.result  = 0;
    COMM_request.command = 0;
}

/* -----------------------------------------------------------
 * Leer mi propia entidad para detectar Alarma_src.
 * Esto debe ejecutarse en modo conectado.
 * ----------------------------------------------------------- */
static void orion_check_alarma_src_local(void)
{
    char request_buf[512];

    snprintf(request_buf, sizeof(request_buf),
        "GET /v2/entities/" IoT_NAME " HTTP/1.1\r\n"
        "Host: %s:%d\r\n"
        "Accept: application/json\r\n"
        "Connection: close\r\n"
        "\r\n",
        ORION_HOST,
        ORION_PORT
    );

    bprintf(">> [NORMAL] Leyendo mi Alarma_src...\r\n");

    if (!orion_send_request(request_buf)) {
        bprintf(">> [NORMAL] Error enviando GET de mi entidad.\r\n");
        orion_clear_comm_request();
        return;
    }

    bprintf(">> [NORMAL] SELF resp cruda: %.200s\r\n", COMM_request.HTTP_response);

    if (strlen((char *)COMM_request.HTTP_response) > 0) {

        cleanResponse(COMM_request.HTTP_response,
                      strlen((char *)COMM_request.HTTP_response));

        bprintf(">> [NORMAL] SELF resp limpia: %.250s\r\n", COMM_request.HTTP_response);

        cJSON *json_self = cJSON_Parse((char *)COMM_request.HTTP_response);

        if (json_self != NULL) {
            cJSON *src_obj = cJSON_GetObjectItem(json_self, "Alarma_src");

            if (src_obj != NULL) {
                cJSON *val = cJSON_GetObjectItem(src_obj, "value");

                if (val != NULL && cJSON_IsString(val) && val->valuestring != NULL) {
                    procesar_alarma_src_local(val->valuestring);
                }
            }

            cJSON_Delete(json_self);
        } else {
            bprintf(">> [NORMAL] Error parseando mi entidad para Alarma_src.\r\n");
        }
    }

    orion_clear_comm_request();
}

/* -----------------------------------------------------------
 * Parsear datos del nodo clonado.
 * ----------------------------------------------------------- */
static void parse_cloned_entity(uint8_t target_id)
{
    cJSON *json = cJSON_Parse((char *)COMM_request.HTTP_response);

    if (json == NULL) {
        bprintf(">> [CLON] Error parseando JSON de respuesta.\r\n");
        return;
    }

    /* Extraer Alarma */
    cJSON *alarma_obj = cJSON_GetObjectItem(json, "Alarma");
    if (alarma_obj != NULL) {
        cJSON *val = cJSON_GetObjectItem(alarma_obj, "value");

        if (val != NULL) {
            if (cJSON_IsString(val) && val->valuestring != NULL) {
                g_clone_alarm_active =
                    (strcmp(val->valuestring, "T") == 0 ||
                     strcmp(val->valuestring, "true") == 0 ||
                     strcmp(val->valuestring, "1") == 0) ? 1 : 0;
            }
            else if (cJSON_IsBool(val)) {
                g_clone_alarm_active = cJSON_IsTrue(val) ? 1 : 0;
            }
        }
    }

    /* Extraer Temperatura */
    cJSON *temp_obj = cJSON_GetObjectItem(json, "Temperatura");
    if (temp_obj != NULL) {
        cJSON *val = cJSON_GetObjectItem(temp_obj, "value");

        if (val != NULL && cJSON_IsString(val) && val->valuestring != NULL) {
            int32_t t_act, t_max, t_min, t_thr;

            if (parse_csv4_x10(val->valuestring, &t_act, &t_max, &t_min, &t_thr)) {
                g_clone_temp_current = t_act / 10.0f;
                g_clone_temp_max     = t_max / 10.0f;
                g_clone_temp_min     = t_min / 10.0f;
                g_clone_temp_thr     = t_thr / 10.0f;

                bprintf(">> [CLON] Temp parseada: %.1f %.1f %.1f %.1f\r\n",
                        g_clone_temp_current,
                        g_clone_temp_max,
                        g_clone_temp_min,
                        g_clone_temp_thr);
            } else {
                bprintf(">> [CLON] Error formato Temperatura: %s\r\n", val->valuestring);
            }
        }
    }

    /* Extraer Luz */
    cJSON *luz_obj = cJSON_GetObjectItem(json, "IntensidadLuz");
    if (luz_obj != NULL) {
        cJSON *val = cJSON_GetObjectItem(luz_obj, "value");

        if (val != NULL && cJSON_IsString(val) && val->valuestring != NULL) {
            int32_t l_act, l_max, l_min, l_thr;

            if (parse_csv4_x10(val->valuestring, &l_act, &l_max, &l_min, &l_thr)) {
                g_clone_ldr_current = l_act / 10.0f;
                g_clone_ldr_max     = l_max / 10.0f;
                g_clone_ldr_min     = l_min / 10.0f;
                g_clone_ldr_thr     = l_thr / 10.0f;

                bprintf(">> [CLON] Luz parseada: %.1f %.1f %.1f %.1f\r\n",
                        g_clone_ldr_current,
                        g_clone_ldr_max,
                        g_clone_ldr_min,
                        g_clone_ldr_thr);
            } else {
                bprintf(">> [CLON] Error formato IntensidadLuz: %s\r\n", val->valuestring);
            }
        }
    }

    cJSON_Delete(json);

    bprintf(">> [CLON] Datos de SensorSEU_%02d parseados y guardados. Alarma=%d\r\n",
            target_id,
            g_clone_alarm_active);
}

void Task_ORION_init(void)
{
    BaseType_t res_task;

    res_task = xTaskCreate(Task_ORION, "ORION", 2048, NULL,
                           NORMAL_PRIORITY, NULL);

    if (res_task != pdPASS) {
        bprintf("PANIC: Error al crear Tarea ORION\r\n");
        fflush(NULL);
        while (1);
    }
}

void Task_ORION(void *pvParameters)
{
    char body_buf[512];
    char request_buf[1024];

    vTaskDelay(15000 / portTICK_RATE_MS);

    while (1) {

        /* ============================================================
         * MODO 0: conectado / normal
         * ============================================================ */
        if (global_mode == 0) {

            /*
             * Leer mi propia entidad para detectar si un clon escribio Alarma_src.
             */
            orion_check_alarma_src_local();

            int32_t  temp_x10 = g_temp_x10;
            uint32_t ldr_pct  = g_ldr_pct;
            uint32_t pot_pct  = g_pot_pct;
            int32_t  temp_max = g_temp_max;
            int32_t  temp_min = g_temp_min;
            uint32_t ldr_max  = g_ldr_max;
            uint32_t ldr_min  = g_ldr_min;

            int temp_int = temp_x10 / 10;
            int temp_dec = (temp_x10 < 0 ? -temp_x10 : temp_x10) % 10;

            /*
             * IMPORTANTE:
             * No publicamos Alarma_src="" aqui.
             * Si lo hicieramos, borrariamos la orden escrita por un clon.
             */
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
                "\"modo\":{"
                "\"type\":\"string\","
                "\"value\":\"%s\"}"
                "}",
                temp_int, temp_dec,
                temp_max / 10, (temp_max < 0 ? -temp_max : temp_max) % 10,
                temp_min / 10, (temp_min < 0 ? -temp_min : temp_min) % 10,
                pot_pct / 10, pot_pct % 10,

                ldr_pct / 10, ldr_pct % 10,
                ldr_max / 10, ldr_max % 10,
                ldr_min / 10, ldr_min % 10,
                pot_pct / 10, pot_pct % 10,

                g_alarm_active ? "T" : "F",
                IoT_NAME
            );

            bprintf("JSON len: %d\r\n", (int)strlen(body_buf));
            bprintf("JSON: %s\r\n", body_buf);

            /*
             * Publicar mis datos.
             * OJO: aqui se publica en IoT_NAME, no en target_id.
             */
            snprintf(request_buf, sizeof(request_buf),
                "PATCH /v2/entities/" IoT_NAME "/attrs HTTP/1.1\r\n"
                "Host: %s:%d\r\n"
                "Content-Type: application/json\r\n"
                "Accept: application/json\r\n"
                "Connection: close\r\n"
                "Content-Length: %d\r\n"
                "\r\n"
                "%s",
                ORION_HOST,
                ORION_PORT,
                (int)strlen(body_buf),
                body_buf
            );

            if (!orion_send_request(request_buf)) {
                bprintf("ORION: error enviando publicacion\r\n");
                orion_clear_comm_request();
                vTaskDelay(2000 / portTICK_RATE_MS);
                continue;
            }

            bprintf("ORION resp: %.200s\r\n", COMM_request.HTTP_response);

            if (strstr((char *)COMM_request.HTTP_response, "204") != NULL ||
                strstr((char *)COMM_request.HTTP_response, "200") != NULL) {
                bprintf("ORION: publicado OK\r\n");
            } else {
                bprintf("ORION: error al publicar\r\n");
            }

            orion_clear_comm_request();

            /*
             * Para pruebas, 2 segundos. Luego puedes subirlo.
             */
            vTaskDelay(2000 / portTICK_RATE_MS);
        }

        /* ============================================================
         * MODO 1: clon
         * ============================================================ */
        else if (global_mode == 1) {

            uint8_t target_id = g_target_clone_id;

            /*
             * Si el boton derecho en Task_HW ha pedido apagar alarma remota,
             * escribimos Alarma_src sobre el nodo clonado.
             */
            if (g_send_alarm_off == 1) {

                g_send_alarm_off = 0;
                g_alarma_src_seq++;

                snprintf(body_buf, sizeof(body_buf),
                    "{"
                    "\"Alarma_src\":{"
                    "\"type\":\"string\","
                    "\"value\":\"%s_%02lu\"}"
                    "}",
                    IoT_NAME,
                    g_alarma_src_seq
                );

                snprintf(request_buf, sizeof(request_buf),
                    "PATCH /v2/entities/SensorSEU_%02d/attrs HTTP/1.1\r\n"
                    "Host: %s:%d\r\n"
                    "Content-Type: application/json\r\n"
                    "Accept: application/json\r\n"
                    "Connection: close\r\n"
                    "Content-Length: %d\r\n"
                    "\r\n"
                    "%s",
                    target_id,
                    ORION_HOST,
                    ORION_PORT,
                    (int)strlen(body_buf),
                    body_buf
                );

                bprintf(">> [CLON] target_id=%d\r\n", target_id);
                bprintf(">> [CLON] Body apagar: %s\r\n", body_buf);
                bprintf(">> [CLON] Request apagar:\r\n%s\r\n", request_buf);
                bprintf(">> [CLON] Silenciando alarma de SensorSEU_%02d...\r\n", target_id);

                if (!orion_send_request(request_buf)) {
                    bprintf(">> [CLON] Error enviando PATCH Alarma_src.\r\n");
                    orion_clear_comm_request();
                    vTaskDelay(2000 / portTICK_RATE_MS);
                    continue;
                }

                bprintf(">> [CLON] Resp apagar: %.200s\r\n", COMM_request.HTTP_response);

                if (strstr((char *)COMM_request.HTTP_response, "204") != NULL ||
                    strstr((char *)COMM_request.HTTP_response, "200") != NULL ||
                    strstr((char *)COMM_request.HTTP_response, "SEND OK") != NULL) {
                    bprintf(">> [CLON] Orden de apagado enviada a Orion.\r\n");
                } else {
                    bprintf(">> [CLON] No se pudo confirmar respuesta HTTP del apagado.\r\n");
                }

                orion_clear_comm_request();

                /*
                 * Esperamos un poco para no hacer GET inmediatamente encima del PATCH.
                 */
                vTaskDelay(1500 / portTICK_RATE_MS);
            }

            /*
             * Lectura normal del nodo clonado.
             */
            else {

                snprintf(request_buf, sizeof(request_buf),
                    "GET /v2/entities/SensorSEU_%02d HTTP/1.1\r\n"
                    "Host: %s:%d\r\n"
                    "Accept: application/json\r\n"
                    "Connection: close\r\n"
                    "\r\n",
                    target_id,
                    ORION_HOST,
                    ORION_PORT
                );

                bprintf(">> [CLON] Pidiendo datos a SensorSEU_%02d...\r\n", target_id);

                if (!orion_send_request(request_buf)) {
                    bprintf(">> [CLON] Error enviando GET de clon.\r\n");
                    orion_clear_comm_request();
                    vTaskDelay(1000 / portTICK_RATE_MS);
                    continue;
                }

                bprintf("ORION CLON resp cruda: %.200s\r\n", COMM_request.HTTP_response);

                if (strlen((char *)COMM_request.HTTP_response) > 0) {

                    cleanResponse(COMM_request.HTTP_response,
                                  strlen((char *)COMM_request.HTTP_response));

                    bprintf("ORION CLON resp limpia: %.300s\r\n", COMM_request.HTTP_response);

                    parse_cloned_entity(target_id);
                }

                orion_clear_comm_request();

                vTaskDelay(1000 / portTICK_RATE_MS);
            }
        }

        /* ============================================================
         * MODO 2: test
         * ============================================================ */
        else {
            vTaskDelay(10000 / portTICK_RATE_MS);
        }
    }
}