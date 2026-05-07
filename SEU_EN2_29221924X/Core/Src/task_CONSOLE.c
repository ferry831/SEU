/*
 * tareas_auxiliares.c
 *
 *  Created on: May 3, 2022
 *      Author: pperez
 */

#include "FreeRTOS.h"
#include <stdio.h>
#include <task.h>
#include "cmsis_os.h"
#include "main.h"
#include <string.h>
#include <stm32f4xx_hal_dma.h>

#include "task_CONSOLE.h"

#include <stdarg.h>

char bprint_buff[2048];
BUFF_BUFFER_t * IObuff;

void Task_CONSOLE( void *pvParameters ){

	uint32_t it;
	BUFF_ITEM_t car;
	HAL_StatusTypeDef res;

    it=0;
	while(1){

		IObuff->get(IObuff,&car);
		do
			{
			res=HAL_UART_Transmit(& huart2,&car,1,10000);
			if (res!=HAL_OK) while(1);
			}while(res!= HAL_OK);

		it++;

	}
}


void print_bufferln(char * name,char * cad){

	 IObuff->puts(IObuff,(BUFF_ITEM_t *)name,strlen(name));
	 IObuff->puts(IObuff,(BUFF_ITEM_t *)": ",2);
	 IObuff->puts(IObuff,(BUFF_ITEM_t *)cad,strlen(cad));
	 IObuff->puts(IObuff,(BUFF_ITEM_t *)"\r\n",2);

}

void print_buffer(char * name,char * cad){


     if (strlen(name)>0){
    	 IObuff->puts(IObuff,(BUFF_ITEM_t *)name,strlen(name));
    	 IObuff->puts(IObuff,(BUFF_ITEM_t *)": ",2);
     }
	 IObuff->puts(IObuff,(BUFF_ITEM_t *)cad,strlen(cad));
}

void console_flush(void){
	 IObuff->lock_priority(IObuff);
}
void console_unlock(void){
	 IObuff->unlock_priority(IObuff);
}


void bprintf_last ( const char *fmt, ...){
	  va_list ap;

	  va_start(ap, fmt);
	  vsprintf(bprint_buff,fmt,ap);
	  va_end(ap);
 	  IObuff->puts_priority(IObuff,(BUFF_ITEM_t *)bprint_buff,strlen(bprint_buff));

}

void bprintf ( const char *fmt, ...){
	  va_list ap;


	  va_start(ap, fmt);
	  vsprintf(bprint_buff,fmt,ap);
	  va_end(ap);
 	  IObuff->puts(IObuff,(BUFF_ITEM_t *)bprint_buff,strlen(bprint_buff));

}
