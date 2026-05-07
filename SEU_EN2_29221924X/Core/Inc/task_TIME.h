/*
 * task_TIME.h
 *
 *  Created on: May 25, 2023
 *      Author: pperez
 */

#ifndef INC_TASK_TIME_H_
#define INC_TASK_TIME_H_

#include <time.h>

extern uint32_t global_time_it;

void   Task_TIME_init(void);
void   Task_TIME( void *pvParameters );

int    task_TIME_timeAvailable(void);
time_t task_TIME_getTime(void);

#endif /* INC_TASK_TIME_H_ */
