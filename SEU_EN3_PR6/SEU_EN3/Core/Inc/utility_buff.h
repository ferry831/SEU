/*
 * utility_buff.c
 *
 *  Created on: 24 may. 2023
 *      Author: ppere
 */

#ifndef INC_UTILITY_BUFF_C_
#define INC_UTILITY_BUFF_C_


#include "FreeRTOS.h"
#include "task.h"

#include <FreeRTOS.h>
#include <stdint.h>
#include <semphr.h>

typedef uint8_t BUFF_ITEM_t;
typedef struct BUFFER_CIRCULAR_dummy {


	uint8_t    (*get)(void *, BUFF_ITEM_t *);
	uint8_t    (*put)(void *, BUFF_ITEM_t);
	uint32_t   (*isempty)(void *);
	uint32_t   (*isfull)(void *);
	uint32_t   (*puts)( void *,BUFF_ITEM_t * cad,uint32_t length);
	//uint32_t   (*gets)(uint_t * cad,uint32_t length);


	uint8_t    (*lock_priority)(void*);
	uint8_t    (*unlock_priority)(void*);
	uint32_t   (*puts_priority)( void *,BUFF_ITEM_t * cad,uint32_t length);


	SemaphoreHandle_t xSem;

	uint32_t 	cabeza;
	uint32_t	cola;
	uint32_t    size;
	BUFF_ITEM_t*	buff;
	uint32_t	n_elem;
	uint32_t 	lock;

} BUFF_BUFFER_t;

// API
BUFF_BUFFER_t * bufferCreat(int size);

#endif /* INC_UTILITY_BUFF_C_ */
