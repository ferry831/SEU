/*
 * task_SERIAL.h
 *
 *  Created on: 24 may. 2023
 *      Author: ppere
 */

#ifndef INC_TASK_CONSOLE_H_
#define INC_TASK_CONSOLE_H_

#include "utility_buff.h"
#include "main.h"

extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart1;

extern BUFF_BUFFER_t * IObuff;

void Task_CONSOLE( void *pvParameters );
void bprintf ( const char *fmt, ...);


void console_unlock(void);
void console_flush(void);

void bprintf_last ( const char *fmt, ...);


#endif /* INC_TASK_CONSOLE_H_ */
