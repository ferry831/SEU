#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "main.h"
#include "tareas.h"
#include "task_CONSOLE.h"
#include <stdio.h>

void Task_HW(void *pvParameters);

static uint32_t clamp_pct_x10_i32(int32_t v)
{
    if (v < 0) return 0;
    if (v > 1000) return 1000;
    return (uint32_t)v;
}

/* Convierte luz en porcentaje real 0.0 - 100.0 a escala 0 - 1000 */
static uint32_t light_to_bar_x10(float light_pct)
{
    int32_t v = (int32_t)(light_pct * 10.0f);
    return clamp_pct_x10_i32(v);
}

/*
 * Convierte temperatura en grados C a la misma escala que ya usabas:
 * 25.0 ºC -> 0
 * 30.0 ºC -> 1000
 */
static uint32_t temp_to_bar_x10(float temp_c)
{
    int32_t temp_x10 = (int32_t)(temp_c * 10.0f);

    if (temp_x10 <= 250) {
        return 0;
    }

    int32_t v = ((temp_x10 - 250) * 1000) / 50;
    return clamp_pct_x10_i32(v);
}

/* Potenciómetro 0-1000 -> id 00-26 */
static uint8_t pot_to_clone_id(uint32_t pot_pct)
{
    uint8_t id = (uint8_t)((pot_pct * 27U) / 1001U);
    if (id > 26) id = 26;
    return id;
}

void Task_HW_init(void) {
    xTaskCreate(Task_HW, "HW", 1024, NULL, HIGH_PRIORITY, NULL);
}

/* --- FUNCION MODO TEST --- */
extern UART_HandleTypeDef huart1; // Para comunicarnos directamente con el ESP8266

static void ejecutar_secuencia_test(uint8_t b_izq, uint8_t b_der, int32_t temp, uint32_t ldr, uint32_t pot)
{
    bprintf("\r\n=== INICIANDO SECUENCIA TEST ===\r\n");

    /* 1.- Barrido de LEDs */
    bprintf("1.- Barrido de LEDs...\r\n");
    for (int i = 0; i < 8; i++) {
        LED_setAll(1 << i);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    for (int i = 6; i >= 0; i--) {
        LED_setAll(1 << i);
        vTaskDelay(100 / portTICK_RATE_MS);
    }
    LED_setAll(0x00);

    /* 2.- Hacer sonar el buzzer */
    bprintf("2.- Sonando Buzzer...\r\n");
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
    vTaskDelay(300 / portTICK_RATE_MS);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

    /* 3.- Valores Analógicos */
    // Convertimos el potenciómetro a Voltios (asumiendo 3.3V max y ADC de 12 bits = 4095). 
    // Multiplicamos por 100 para imprimirlo con %lu.%lu y evitar que falle el %f.
    uint32_t pot_v_x100 = (pot * 330) / 4095;
    
    bprintf("3.- Valores Analogicos:\r\n");
    bprintf("    - Temp:  %ld.%ld C\r\n", temp/10, (temp<0?-temp:temp)%10);
    bprintf("    - LDR:   %lu.%lu %%\r\n", ldr/10, ldr%10);
    bprintf("    - Poten: %lu.%02lu V (%lu/4095)\r\n", pot_v_x100/100, pot_v_x100%100, pot);

    /* 4.- Valor de los botones */
    bprintf("4.- Botones -> IZQ: %d | DER: %d\r\n", b_izq, b_der);

    /* 5.- Enviar orden AT y mostrar salida */
    bprintf("5.- Enviando orden AT al ESP8266...\r\n");
    uint8_t at_resp[64] = {0};
    
    // Detenemos temporalmente la escucha en segundo plano (DMA) de la tarea WiFi 
    // para que no nos robe la respuesta del comando AT.
    HAL_UART_AbortReceive(&huart1); 
    __HAL_UART_FLUSH_DRREGISTER(&huart1); // Limpiamos basura previa
    
    // Transmitimos "AT" y leemos la respuesta
    HAL_UART_Transmit(&huart1, (uint8_t*)"AT\r\n", 4, 100);
    HAL_UART_Receive(&huart1, at_resp, sizeof(at_resp)-1, 500);
    
    // Aseguramos fin de cadena para que bprintf no se cuelgue
    at_resp[sizeof(at_resp)-1] = '\0'; 
    bprintf("    Salida AT: %s\r\n", at_resp);

    bprintf("=== FIN SECUENCIA TEST ===\r\n\r\n");
}

void Task_HW(void *pvParameters) {

    static uint8_t btn_izq_prev = 0;
    static uint8_t btn_der_prev = 0;

    static uint32_t both_btn_tick     = 0;
    static uint8_t  both_btn_counting = 0;

    /*
     * Silenciado local del clon.
     * Esto evita que el clon siga pitando inmediatamente mientras espera
     * a que el nodo real procese Alarma_src.
     */
    static uint8_t  clone_alarm_muted = 0;
    static uint32_t clone_mute_tick   = 0;

    while (1) {
        uint32_t now = xTaskGetTickCount();

        /* ---- Leer botones ---- */
        uint8_t btn_izq = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) ? 1 : 0;
        uint8_t btn_der = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET) ? 1 : 0;

        /* ---- Leer sensores locales siempre ---- */
        uint32_t pot_pct  = (POT_read() * 1000) / 4095;
        uint32_t ldr_pct  = LDR_read();
        int32_t  temp_x10 = NTC_read_x10();

        /* Actualizar globales locales para publicar en ORION en modo conectado */
        g_pot_pct  = pot_pct;
        g_ldr_pct  = ldr_pct;
        g_temp_x10 = temp_x10;

        /* En modo clon, el potenciómetro selecciona SensorSEU_00 ... SensorSEU_26 */
        if (global_mode == 1) {
            uint8_t new_target = pot_to_clone_id(pot_pct);

            if (new_target != g_target_clone_id) {
                g_target_clone_id = new_target;
                bprintf(">> [CLON] Nodo seleccionado: SensorSEU_%02d\r\n",
                        g_target_clone_id);
            }
        }

        /* ---- Calcular escala local de temperatura para bargraph ---- */
        uint32_t ntc_pct = 0;
        if (temp_x10 > 250) {
            ntc_pct = ((uint32_t)(temp_x10 - 250) * 1000) / 50;
            if (ntc_pct > 1000) ntc_pct = 1000;
        }

        /* ---- Cambio de modo: ambos botones ---- */
        if (btn_izq && btn_der) {
            if (!both_btn_counting) {
                both_btn_counting = 1;
                both_btn_tick = now;
            } else if ((now - both_btn_tick) >= 1000) {
                global_mode = (global_mode + 1) % 3;

                bprintf(">> Modo: %d (%s)\r\n",
                        global_mode,
                        global_mode == 0 ? "CONECTADO" :
                        global_mode == 1 ? "CLON" :
                                           "TEST");

                both_btn_counting = 0;
            }
        } else {
            both_btn_counting = 0;

            /* BTN_IZQ solo: cambiar sensor visualizado */
            if (btn_izq && !btn_izq_prev) {
                g_sensor_sel ^= 1;

                bprintf(">> Sensor visualizado: %s\r\n",
                        g_sensor_sel == 0 ? "LDR (luz)" : "NTC (temp)");
            }

            if (btn_der && !btn_der_prev) {

                if (global_mode == 1) {

                    /*
                     * En modo clon, el boton derecho manda una orden
                     * para apagar la alarma del nodo clonado.
                     *
                     * No hacemos aqui el PATCH. Solo activamos el flag.
                     * Task_ORION se encarga de escribir Alarma_src.
                     */
                    g_send_alarm_off = 1;

                    clone_alarm_muted = 1;
                    clone_mute_tick = now;

                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

                    bprintf(">> [HW CLON] Boton derecho: pedir apagar alarma de SensorSEU_%02d\r\n",
                            g_target_clone_id);

                } else {

                    if (g_alarm_active) {
                        g_alarm_active   = 0;
                        g_alarm_disarmed = 1;
                        g_disarm_tick    = now;

                        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);

                        bprintf(">> Alarma local silenciada. Reactivacion en 10s\r\n");
                    } else {
                        bprintf(">> Modo actual: %d (%s)\r\n",
                                global_mode,
                                global_mode == 0 ? "CONECTADO" :
                                global_mode == 1 ? "CLON" :
                                                   "TEST");
                    }
                }
            }
        }

        btn_izq_prev = btn_izq;
        btn_der_prev = btn_der;

        if (global_mode == 2) {
            // Ejecutamos todos los pasos
            ejecutar_secuencia_test(btn_izq, btn_der, temp_x10, ldr_pct, pot_pct);
            
            vTaskDelay(3000 / portTICK_RATE_MS);
            continue; 
        }

        /* ---- Actualizar máximos y mínimos locales ---- */
        if (temp_x10 > g_temp_max) g_temp_max = temp_x10;
        if (temp_x10 < g_temp_min) g_temp_min = temp_x10;
        if (ldr_pct  > g_ldr_max)  g_ldr_max  = ldr_pct;
        if (ldr_pct  < g_ldr_min)  g_ldr_min  = ldr_pct;

        /* ---- Reactivación alarma local 10s ---- */
        if (g_alarm_disarmed && (now - g_disarm_tick) >= 10000) {
            g_alarm_disarmed = 0;
            bprintf(">> Alarma local preparada.\r\n");
        }

        /* ---- Reactivación silenciado local del clon 10s ---- */
        if (clone_alarm_muted && (now - clone_mute_tick) >= 10000) {
            clone_alarm_muted = 0;
            bprintf(">> [CLON] Alarma clon preparada.\r\n");
        }

        /*
         * ---- Check alarma local ----
         * Solo debe hacerse en modo conectado.
         * En modo clon NO queremos generar alarma con sensores locales.
         */
        if (global_mode == 0) {
            if (!g_alarm_active && !g_alarm_disarmed) {
                if (ldr_pct > pot_pct || ntc_pct > pot_pct) {
                    g_alarm_active = 1;

                    bprintf("!! ALARMA LOCAL — LDR: %lu.%lu%% NTC: %lu.%lu%% nivel: %lu.%lu%%\r\n",
                            ldr_pct/10, ldr_pct%10,
                            ntc_pct/10, ntc_pct%10,
                            pot_pct/10, pot_pct%10);
                }
            }
        }

        /*
         * ---- Elegir qué valores se visualizan ----
         */
        uint32_t display_pct = 0;
        uint32_t alarm_level_pct = pot_pct;

        if (global_mode == 1) {
            /*
             * En modo clon:
             * - El valor visualizado viene del nodo remoto.
             * - El LED intermitente viene del umbral remoto, que es el 4º valor publicado.
             *
             * OJO:
             * g_clone_ldr_thr y g_clone_temp_thr NO son valores físicos de luz/temp.
             * Son el umbral del potenciómetro remoto en porcentaje.
             */

            if (g_sensor_sel == 0) {
                /*
                 * Mostrar luz remota.
                 * Ejemplo: g_clone_ldr_current = 18.6 -> display_pct = 186
                 */
                display_pct = light_to_bar_x10(g_clone_ldr_current);

                /*
                 * Umbral remoto de luz.
                 * Ejemplo: g_clone_ldr_thr = 72.8 -> alarm_level_pct = 728
                 */
                alarm_level_pct = light_to_bar_x10(g_clone_ldr_thr);

            } else {
                /*
                 * Mostrar temperatura remota.
                 * Ejemplo: g_clone_temp_current = 26.2 ºC.
                 * Se convierte a la escala del bargraph igual que la temperatura local.
                 */
                display_pct = temp_to_bar_x10(g_clone_temp_current);

                /*
                 * IMPORTANTE:
                 * El umbral publicado para temperatura también es el potenciómetro remoto,
                 * no una temperatura real. Por tanto se convierte como porcentaje, no como ºC.
                 */
                alarm_level_pct = light_to_bar_x10(g_clone_temp_thr);
            }
        } else {
            /*
             * En modo conectado/test:
             * - Valor mostrado = sensor local.
             * - LED intermitente = potenciómetro local.
             */
            display_pct = (g_sensor_sel == 0) ? ldr_pct : ntc_pct;
            alarm_level_pct = pot_pct;
        }


        if (alarm_level_pct > 1000) {
            alarm_level_pct = 1000;
        }

        /*
         * ---- Buzzer ----
         */
        uint8_t buzzer_on = 0;

        if (global_mode == 1) {
            buzzer_on = (g_clone_alarm_active && !clone_alarm_muted) ? 1 : 0;
        } else {
            buzzer_on = g_alarm_active ? 1 : 0;
        }

        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7,
                          buzzer_on ? GPIO_PIN_SET : GPIO_PIN_RESET);

        /*
         * ---- Parpadeo LED alarma ----
         */
        if ((now - g_last_flash_tick) >= 140) {
            g_flash_state ^= 1;
            g_last_flash_tick = now;
        }

        /*
         * ---- Bargraph LEDs ----
         */
        uint8_t alarm_led = (uint8_t)((alarm_level_pct * 7) / 1000);

        uint8_t num = (uint8_t)((display_pct * 8) / 1000);
        if (num > 8) num = 8;

        uint8_t pattern = (num == 8) ? 0xFF : (uint8_t)((1 << num) - 1);

        if (g_flash_state) {
            pattern |=  (1 << alarm_led);
        } else {
            pattern &= ~(1 << alarm_led);
        }

        LED_setAll(pattern);

      /*  if (global_mode == 1) {
            bprintf(">> [HW CLON] Sensor=%s | display=%lu.%lu | umbral_remoto=%lu.%lu | alarm_led=%d | pattern=0x%02X\r\n",
                    g_sensor_sel == 0 ? "LUZ" : "TEMP",
                    display_pct / 10, display_pct % 10,
                    alarm_level_pct / 10, alarm_level_pct % 10,
                    alarm_led,
                    pattern);
        }*/

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
