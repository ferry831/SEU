#include <stdint.h>
#include "task_COMM.h"
#include "FreeRTOS.h"
#include <stdio.h>
#include "cmsis_os.h"
#include <stdlib.h>
#include <stdio.h>
#include "semphr.h"
#include <string.h>

#include <task.h>

#include "main.h"
#include "task_CONSOLE.h"
#include "tareas.h"

#include "task_CONSOLE.h"



scomm_request_t COMM_request;
SemaphoreHandle_t COMM_xSem = NULL;
SemaphoreHandle_t COMM_WAIT_xSem = NULL;
uint32_t global_comm_it;

uint8_t buff_recv[2048];

uint32_t global_wifi_it;
uint32_t global_wifi_ready;

 uint8_t aux_buff_WIFI[2048];
 uint8_t buff_WIFI_response[2048];

void Task_COMM_init(void){
	BaseType_t res_task;

	global_comm_it=0;

	COMM_xSem=xSemaphoreCreateMutex();

	if( COMM_xSem == NULL ){
		printf("PANIC: Error al crear el Semáforo ORION\r\n");
		fflush(NULL);
		while(1);
	}

	COMM_WAIT_xSem= xSemaphoreCreateBinary();

		if( COMM_WAIT_xSem == NULL ){
			printf("PANIC: Error al crear el Semáforo ORION 2\r\n");
			fflush(NULL);
			while(1);
		}


 	res_task=xTaskCreate(Task_COMM,"COMMUNICATION",2048,NULL,	NORMAL_PRIORITY,NULL);
 	if( res_task != pdPASS ){
 	 				printf("PANIC: Error al crear Tarea Comunicaciones\r\n");
 	 				fflush(NULL);
 	 				while(1);
 	}
}

void Task_COMM( void *pvParameters ){

	int signal;

	WIFI_Boot();

	while (1) {

	    signal=1;
		do {
			if (xSemaphoreTake(COMM_xSem, 20000/portTICK_RATE_MS  ) != pdTRUE ){
				bprintf("\r\n\n\nHARAKIRI!!\n\n\n");
		   		HAL_NVIC_SystemReset();
		}

		// aquí tengo la exclusión mutua asegurada.
		if (COMM_request.command==1) //any thread has a request
			signal=0;
		else{
			xSemaphoreGive(COMM_xSem); // i’m going out critical section
			osDelay(10);
		}

		}while(signal);

		COMM_request.command=2; // busy, block until any thread gets this response.
		xSemaphoreGive(COMM_xSem); // i’m going out critical section
		// Here is safe access to COMM_request because other thread has put COMM_request.command to 1 so before write any other thread must read 0 in this item.
		COMM_request.HTTP_response=ESP_Send_Request(COMM_request.dst_address,COMM_request.dst_port,COMM_request.HTTP_request);
		// remove headers and +IPD messages from ESP32 as a result you have json pure string IF it is completely recover from server in a unique +IPD message
		if (COMM_request.HTTP_response!= NULL ) cleanResponse(COMM_request.HTTP_response,strlen((char *)COMM_request.HTTP_response));
		// if ESP does not receive response or there are any errors result must be different to 1
		COMM_request.result=1;
		global_comm_it++;
		}
}

void cleanResponse(uint8_t * data,int maxlen)
{
	int t,i;

	uint8_t * j,*from, *pc;


	 i=0;
	 while ((j=(uint8_t *)strstr((char *)data,"+IPD"))){

		 from=(uint8_t *)strstr((char *)j,":");
		 from++;

		 	 for (pc=from;pc<(data+2048);pc++)
			 	*(j++)=*(from++);
	 };
	 	 do
			                  {
			                  }while(data[i++]!='{');
			                  i--;
	for (t=0;t<(2048-i);t++)
		data[t]=data[t+i];

	i=0;

    for (t=0;t<2048;t++)
    	if (data[t]=='}')
    		i=t;
    data[i+1]=0;

}
//////////////////////////////////////////////////////////////////7777
// WIFI RELATED functions
///////////////////////////////////////////////////////////////////////


void WIFI_RESET(void){
	// RESET
	unsigned int ct;
	 HAL_GPIO_WritePin(ESP8266_RESET_GPIO_Port, ESP8266_RESET_Pin, GPIO_PIN_RESET);
	 for (ct=0;ct<1000000;ct++);
	 HAL_UART_Init(&huart1);
	 for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
	 HAL_UART_Receive_DMA(&huart1, buff_recv,2048);



	 HAL_GPIO_WritePin(ESP8266_RESET_GPIO_Port, ESP8266_RESET_Pin, GPIO_PIN_SET);

	 vTaskDelay(1000/portTICK_RATE_MS );

   	 HAL_UART_DMAStop(&huart1);
	 //bprintf("XXXXX %s",buff_recv);
	 //bprintf("XXXXX\r\n\n\n\n");
}

void WIFI_Boot_TEST(void)

{
	unsigned int ct;

 	bprintf("Reseting...\r\n");

 	WIFI_RESET();
 	bprintf("Initializing...\r\n");

 	HAL_UART_Init(&huart1);



 	// version
 	for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
 	HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
 	HAL_UART_Transmit(&huart1, ( unsigned char *)"AT\r\n",strlen("AT\r\n"),10000);

	 vTaskDelay(100/portTICK_RATE_MS );
 	HAL_UART_DMAStop(&huart1);

	bprintf("1: %s",buff_recv);
}


void WIFI_Boot(void)

{
	unsigned int ct;

 	bprintf("Reseting...\r\n");

 	WIFI_RESET();
 	bprintf("Initializing...\r\n");

 	HAL_UART_Init(&huart1);



 	// version
 	for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
 	HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
 	HAL_UART_Transmit(&huart1, ( unsigned char *)"AT\r\n",strlen("AT\r\n"),10000);

	 vTaskDelay(100/portTICK_RATE_MS );
 	HAL_UART_DMAStop(&huart1);

	//bprintf("1: %s",buff_recv);


	// Pon en modo station=1,  station+access_point=3
	for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
	HAL_UART_Transmit(&huart1, ( unsigned char *) "AT+CWMODE=1\r\n",strlen("AT+CWMODE=1\r\n"),100000);
	 vTaskDelay(100/portTICK_RATE_MS );
	HAL_UART_DMAStop(&huart1);
	//bprintf("3: %s",buff_recv);

	// Programa la contraseña del access-point
	for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_recv,2048);

	// EJERCICIO 4.
	for (ct=0; ct<2048; ct++) buff_recv[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_recv, 2048);
	HAL_UART_Transmit(&huart1, (unsigned char *)"AT+CWJAP=\"routerSEU\",\"00000000\"\r\n",
	                  strlen("AT+CWJAP=\"routerSEU\",\"00000000\"\r\n"), 10000);
	vTaskDelay(5000/portTICK_RATE_MS );
	HAL_UART_DMAStop(&huart1);
	bprintf("Initialized.\r\n");

}

int ESP_TimeOut_tworesponses(TickType_t timeout,char *src,char * resp,char * resp2, char *msg1,char * msg){

	TickType_t localtimeout=xTaskGetTickCount();



	localtimeout=xTaskGetTickCount();
	while (
		   (
			(strstr(src,resp)==NULL)
			&&
			(strstr(src,resp2)==NULL)
		   )
		   &&
		   (
		    (xTaskGetTickCount()-localtimeout)<(timeout/portTICK_RATE_MS)
		   )
		  )
		{};

	if ((xTaskGetTickCount()-localtimeout)>=(timeout/portTICK_RATE_MS)){
		 bprintf("%s: %s\r\n", msg1, "TIMEOUT2");
		 bprintf("***********\r\n%s:\r\n*******", msg );
  		 return 1;
	}
	else return 0;
}



int ESP_TimeOut(TickType_t timeout,char *src,char * resp, char *msg1,char * msg){

	TickType_t localtimeout=xTaskGetTickCount();



	localtimeout=xTaskGetTickCount();
	while ((strstr(src,resp)==NULL)&&((xTaskGetTickCount()-localtimeout)<(timeout/portTICK_RATE_MS)))
		{};

	if ((xTaskGetTickCount()-localtimeout)>=(timeout/portTICK_RATE_MS)){
		 bprintf("%s: %s\r\n", msg1, "TIMEOUT");
		 bprintf("***********\r\n%s:\r\n*******", msg );
  		 return 1;
	}
	else return 0;
}

uint8_t * ESP_Send_Request(uint8_t * dst_address, uint32_t dst_port, uint8_t * request){
    int ct;
    int st;
    int lc;

    st=1;

    while(st!=5){

    switch (st){
    				case 1: // connect
    					    // abrir conexión con
    						for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
    						sprintf(( char *)aux_buff_WIFI,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",dst_address,(int)dst_port);
    						HAL_UART_Transmit(&huart1, ( unsigned char *) aux_buff_WIFI,strlen((const char *)aux_buff_WIFI),10000);
    						if (ESP_TimeOut(2000,buff_recv,"CONNECT\r\n", "CONNECT",buff_recv))
    							return NULL;
    						//vTaskDelay(400/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);
    						//bprintf("6e: %s",buff_recv);
    						st=2;
    						break;
    				case 2: // preparacion de envio
    						// enviar una peticion HTTP

    						lc=strlen((const char *)request);
    						sprintf((char *)aux_buff_WIFI,"AT+CIPSEND=%d\r\n",lc);
    						for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
    						HAL_UART_Transmit(&huart1,( const uint8_t *)aux_buff_WIFI,strlen((const char *)aux_buff_WIFI),10000);
    						vTaskDelay(10/portTICK_RATE_MS );
    						if (ESP_TimeOut(2000,buff_recv,">", "SEND",buff_recv))
    							st=4; // cerrar la conexion

    						//vTaskDelay(2000/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);
    						//bprintf("7: %s",buff_recv);
    						st=3;
    						break;
    				case 3: // ahora HTTP

    						for (ct=0;ct<2048;ct++) buff_WIFI_response[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, buff_WIFI_response,2048);
    						HAL_UART_Transmit(&huart1, request,strlen( (const char *)request),10000);
    						if (ESP_TimeOut_tworesponses(4000,buff_WIFI_response, "reasonPhrase","\"Europe/Madrid\"","SEND2",buff_WIFI_response)) // "reasonPhrase",
    							st=4;
    						vTaskDelay(200/portTICK_RATE_MS );
    						//vTaskDelay(2000/portTICK_RATE_MS );

    						HAL_UART_DMAStop(&huart1);
    						//	bprintf("8: %s",buff_WIFI_response);
    						st=4;
    						break;
    				case 4: // cerrar conexión

    						for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
    						HAL_UART_Transmit(&huart1, ( unsigned char *) "AT+CIPCLOSE\r\n",strlen("AT+CIPCLOSE\r\n"),10000);
    						vTaskDelay(20/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);
    						//bprintf("6: %s",buff_recv);
    						st=5;
    						break;
    				case 6: bprintf("WIFI error\r\n");
    						st=4;
    						break;
    }//switch
    }// while
	return buff_WIFI_response;
}



