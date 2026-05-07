#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "main.h"
#include "tareas.h"
#include "task_CONSOLE.h"
#include <stdio.h>

void Task_HW(void *pvParameters);

void Task_HW_init(void) {
    xTaskCreate(Task_HW, "HW", 1024, NULL, HIGH_PRIORITY, NULL);
}

void Task_HW(void *pvParameters) {
    while (1) {
        uint32_t now = xTaskGetTickCount();

        /* ---- BTN_IZQ (PC7) ---- */
        static uint8_t btn_izq_prev = 0;
        uint8_t btn_izq = (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_7) == GPIO_PIN_RESET) ? 1 : 0;
        if (btn_izq && !btn_izq_prev) {
            g_sensor_sel ^= 1;
            bprintf(">> Sensor: %s\r\n", g_sensor_sel == 0 ? "LDR (luz)" : "NTC (temp)");
        }
        btn_izq_prev = btn_izq;

        /* ---- BTN_DER (PB6) ---- */
        static uint8_t btn_der_prev = 0;
        uint8_t btn_der = (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_6) == GPIO_PIN_RESET) ? 1 : 0;
        if (btn_der && !btn_der_prev) {
            if (g_alarm_active) {
                g_alarm_active   = 0;
                g_alarm_disarmed = 1;
                g_disarm_tick    = now;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_RESET);
                bprintf(">> Alarma silenciada. Reactivación en 10s\r\n");
            }
        }
        btn_der_prev = btn_der;

        /* ---- Leer sensores ---- */
        uint32_t pot_pct  = (POT_read() * 1000) / 4095;
        uint32_t ldr_pct  = LDR_read();
        int32_t  temp_x10 = NTC_read_x10();

        uint32_t ntc_pct = 0;
        if (temp_x10 > 250) {
            ntc_pct = ((uint32_t)(temp_x10 - 250) * 1000) / 50;
            if (ntc_pct > 1000) ntc_pct = 1000;
        }

        uint32_t display_pct = (g_sensor_sel == 0) ? ldr_pct : ntc_pct;

        /* ---- Reactivación alarma 10s ---- */
        if (g_alarm_disarmed && (now - g_disarm_tick) >= 10000) {
            g_alarm_disarmed = 0;
            bprintf(">> Alarma preparada.\r\n");
        }

        /* ---- Check alarma ---- */
        if (!g_alarm_active && !g_alarm_disarmed) {
            if (ldr_pct > pot_pct || ntc_pct > pot_pct) {
                g_alarm_active = 1;
                HAL_GPIO_WritePin(GPIOA, GPIO_PIN_7, GPIO_PIN_SET);
                bprintf("!! ALARMA — LDR: %lu.%lu%% NTC: %lu.%lu%% nivel: %lu.%lu%%\r\n",
                        ldr_pct/10, ldr_pct%10,
                        ntc_pct/10, ntc_pct%10,
                        pot_pct/10, pot_pct%10);
            }
        }

        /* ---- Parpadeo LED alarma ---- */
        if ((now - g_last_flash_tick) >= 140) {
            g_flash_state ^= 1;
            g_last_flash_tick = now;
        }

        /* ---- Bargraph LEDs ---- */
        uint8_t alarm_led = (uint8_t)((pot_pct * 7) / 1000);
        uint8_t num       = (uint8_t)((display_pct * 8) / 1000);
        if (num > 8) num = 8;
        uint8_t pattern = (num == 8) ? 0xFF : (uint8_t)((1 << num) - 1);
        if (g_flash_state) pattern |=  (1 << alarm_led);
        else               pattern &= ~(1 << alarm_led);
        LED_setAll(pattern);

        vTaskDelay(100 / portTICK_RATE_MS);
    }
}
