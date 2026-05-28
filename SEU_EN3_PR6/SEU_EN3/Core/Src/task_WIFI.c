#include <stdint.h>
#include "main.h"
#include "FreeRTOS.h"
#include "task_WIFI.h"
#include "task_CONSOLE.h"
#include <stdio.h>
#include <string.h>
#include "tareas.h"

/* Aliases para compatibilidad con el código del profesor */
#define D7_GPIO_Port  GPIOA
#define D7_Pin        GPIO_PIN_8   /* PA8 = LED3 en nuestro shield */
#define D8_GPIO_Port  GPIOB
#define D8_Pin        GPIO_PIN_0   /* PB0 = LED7 en nuestro shield */

uint32_t global_wifi_it;
uint32_t global_wifi_ready;

 uint8_t buff_WIFI[2048];
 //uint8_t buff_WIFI_response[2048];

void Task_WIFI_init(void){
	BaseType_t res_task;
	global_wifi_ready=0;
	res_task=xTaskCreate( Task_WIFI,"Tarea_WIFI",2048,NULL,	NORMAL_PRIORITY,NULL);
	if( res_task != pdPASS ){
		printf("PANIC: Error al crear Tarea WIFI\r\n");
		fflush(NULL);
		while(1);
}
}
void Task_WIFI( void *pvParameters ){

	global_wifi_it=0;
	WIFI_Boot();
	global_wifi_ready=1;

	while (1) {
		global_wifi_it++;
	    vTaskDelay(10/portTICK_RATE_MS );
	}
}

void WIFI_RESET(void) {
    unsigned int ct;
    HAL_GPIO_WritePin(ESP8266_RESET_GPIO_Port, ESP8266_RESET_Pin, GPIO_PIN_RESET);
    for (ct=0; ct<1000000; ct++);
    HAL_UART_Init(&huart1);
    for (ct=0; ct<2048; ct++) buff_WIFI[ct]=0;
    HAL_UART_Receive_DMA(&huart1, buff_WIFI, 2048);

    /* Comentar o eliminar estas líneas: */
    // HAL_GPIO_WritePin(D1_GPIO_Port, D1_Pin, GPIO_PIN_RESET);
    // HAL_GPIO_WritePin(D2_GPIO_Port, D2_Pin, GPIO_PIN_RESET);
    // ... etc hasta D8 ...
    // HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_SET);

    HAL_GPIO_WritePin(ESP8266_RESET_GPIO_Port, ESP8266_RESET_Pin, GPIO_PIN_SET);
    vTaskDelay(1000 / portTICK_RATE_MS);
    // HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_SET);
    HAL_UART_DMAStop(&huart1);
}

void WIFI_Boot_TEST(void)

{
	unsigned int ct;

 	bprintf("Reseting...\r\n");

 	WIFI_RESET();
 	bprintf("Initializing...\r\n");

 	HAL_UART_Init(&huart1);

 	//HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_RESET);
 	//HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_RESET);

 	// version
 	for (ct=0;ct<2048;ct++) buff_WIFI[ct]=0;
 	HAL_UART_Receive_DMA(&huart1, buff_WIFI,2048);
 	HAL_UART_Transmit(&huart1, ( unsigned char *)"AT\r\n",strlen("AT\r\n"),10000);
 	//HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_SET);
	 vTaskDelay(100/portTICK_RATE_MS );
 	HAL_UART_DMAStop(&huart1);
 	//HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_SET);
	bprintf("1: %s",buff_WIFI);
}


void WIFI_Boot(void)

{
	unsigned int ct;

 	bprintf("Reseting...\r\n");

 	WIFI_RESET();
 	bprintf("Initializing...\r\n");

 	HAL_UART_Init(&huart1);

 	//HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_RESET);
 	//HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_RESET);

 	// version
 	for (ct=0;ct<2048;ct++) buff_WIFI[ct]=0;
 	HAL_UART_Receive_DMA(&huart1, buff_WIFI,2048);
 	HAL_UART_Transmit(&huart1, ( unsigned char *)"AT\r\n",strlen("AT\r\n"),10000);
 	HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_SET);
	 vTaskDelay(100/portTICK_RATE_MS );
 	HAL_UART_DMAStop(&huart1);
 	HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_SET);
	bprintf("1: %s",buff_WIFI);


	// version
	for (ct=0;ct<2048;ct++) buff_WIFI[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_WIFI,2048);
	HAL_UART_Transmit(&huart1, ( unsigned char *)"AT+GMR\r\n",strlen("AT+GMR\r\n"),10000);
	HAL_GPIO_WritePin(D7_GPIO_Port, D7_Pin, GPIO_PIN_SET);
	 vTaskDelay(1000/portTICK_RATE_MS );
	HAL_UART_DMAStop(&huart1);
	HAL_GPIO_WritePin(D8_GPIO_Port, D8_Pin, GPIO_PIN_SET);

	bprintf("2: %s",buff_WIFI);


	// Pon en modo station=1,  station+access_point=3
	for (ct=0;ct<2048;ct++) buff_WIFI[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_WIFI,2048);
	HAL_UART_Transmit(&huart1, ( unsigned char *) "AT+CWMODE=1\r\n",strlen("AT+CWMODE=1\r\n"),100000);
	 vTaskDelay(100/portTICK_RATE_MS );
	HAL_UART_DMAStop(&huart1);
	bprintf("3: %s",buff_WIFI);

	// Programa la contraseña del access-point
	for (ct=0;ct<2048;ct++) buff_WIFI[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_WIFI,2048);
	HAL_UART_Transmit(&huart1,( unsigned char *) "AT+CWJAP=\"" SSID "\",\"" SSID_PASS "\"\r\n",strlen("AT+CWJAP=\"" SSID "\",\"" SSID_PASS "\"\r\n"),10000);
	vTaskDelay(5000/portTICK_RATE_MS );
	HAL_UART_DMAStop(&huart1);
	bprintf("4: %s",buff_WIFI);

	// verifica si hay IP
	for (ct=0;ct<2048;ct++) buff_WIFI[ct]=0;
	HAL_UART_Receive_DMA(&huart1, buff_WIFI,2048);
	HAL_UART_Transmit(&huart1, ( unsigned char *)"AT+CIFSR\r\n",strlen("AT+CIFSR\r\n"),10000);
	vTaskDelay(5000/portTICK_RATE_MS );
	HAL_UART_DMAStop(&huart1);
	bprintf("5: %s",buff_WIFI);

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
		{vTaskDelay(1/portTICK_RATE_MS );};

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
		{vTaskDelay(1/portTICK_RATE_MS );};

	if ((xTaskGetTickCount()-localtimeout)>=(timeout/portTICK_RATE_MS)){
		 bprintf("%s: %s\r\n", msg1, "TIMEOUT");
		 bprintf("***********\r\n%s:\r\n*******", msg );
  		 return 1;
	}
	else return 0;
}


int ESP_TimeOut_HTTP(TickType_t timeout,char *src,char * resp, char *msg1,char * msg){
    char * pt;
	TickType_t localtimeout=xTaskGetTickCount();
	localtimeout=xTaskGetTickCount();
	
	while ((strstr(src,"HTTP/1.1")==NULL)&&((xTaskGetTickCount()-localtimeout)<(timeout/portTICK_RATE_MS)))
		{vTaskDelay(1/portTICK_RATE_MS );};
	
	if ((xTaskGetTickCount()-localtimeout)>=(timeout/portTICK_RATE_MS)){
		 bprintf("%s: %s\r\n", msg1, "TIMEOUT_HTTP_1");
		 bprintf("***********\r\n%s:\r\n*******", msg );
  		 return 1;
	}

	while ((strstr(src,resp)==NULL)&&((xTaskGetTickCount()-localtimeout)<(timeout/portTICK_RATE_MS)))
		{vTaskDelay(1/portTICK_RATE_MS );};
	
	if ((xTaskGetTickCount()-localtimeout)>=(timeout/portTICK_RATE_MS)){
		 bprintf("%s: %s\r\n", msg1, "TIMEOUT_HTTP");
		 bprintf("***********\r\n%s:\r\n*******", msg );
  		 return 1;
	}
	{vTaskDelay(100/portTICK_RATE_MS );};
	// Parse Content-Length from HTTP response
	char *content_length_ptr = strstr(src, "Content-Length:");
	int body_length = 0;
	if (content_length_ptr != NULL) {
		sscanf(content_length_ptr, "Content-Length: %d", &body_length);
	}
    pt=strstr(content_length_ptr, "\r\n\r\n");

	while ((strlen(pt)<body_length)&&((xTaskGetTickCount()-localtimeout)<(timeout/portTICK_RATE_MS)))
		{vTaskDelay(1/portTICK_RATE_MS );};
		
	if ((xTaskGetTickCount()-localtimeout)>=(timeout/portTICK_RATE_MS)){
		 bprintf("%s: %s\r\n", msg1, "TIMEOUT_HTTP");
		 bprintf("***********\r\n%s:\r\n*******", msg );
  		 return 1;
	
	}

	{ 
		char * from1, * from, *from_ini ;
		while(1){
		
		// Buscar el inicio de "+IPD"
	from1 = (uint8_t *)strstr((const char *)src, "+IPD");
	if (from1 == NULL) goto final_IPD_1;
	
	// Avanzar hasta encontrar ":"
	from=from1;
	while (*from != ':' && *from != '\0') from++;
	if (*from == '\0') goto final_IPD;
	from++; // Saltar el ':'
		

	for (int i=0;i<2048;i++) {
		from1[i]=from[i];
		if (from1[i]==0) goto final_IPD;
	}
	
	final_IPD:
   }
	final_IPD_1:

	return 0;
	
	from_ini=strstr(src,"HTTP/1.1");
	for (int i=0;i<2048;i++) {
		src[i]=from_ini[i];
		if (from1[i]==0) break;
	}
	
   }

	return 0;
}


/*

uint8_t * ESP_Send_Request(uint8_t * dst_address, uint32_t dst_port, uint8_t * request){
    int ct;
    int st;
    int lc;

    st=1;

    TickType_t t_start;
    TickType_t t_end;


    t_start=xTaskGetTickCount();
    while(st!=5){

    switch (st){
    				case 1: // connect

    					    // abrir conexión con
    						for (ct=0;ct<2048;ct++) buff_recv[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, buff_recv,2048);
    						sprintf(( char *)aux_buff_WIFI,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",dst_address,(int)dst_port);
    						HAL_UART_Transmit(&huart1, ( unsigned char *) aux_buff_WIFI,strlen((const char *)aux_buff_WIFI),10000);
    						if (ESP_TimeOut(5000,buff_recv,"CONNECT\r\n", "CONNECT Send Request",buff_recv))
    						{		bprintf("TC: %s\r\n\r\n",aux_buff_WIFI);
									return NULL;
							}
    						//vTaskDelay(400/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);

    						//

    						//bprintf("%d Connect milliseconds\r\n",(xTaskGetTickCount()-t_start)/portTICK_RATE_MS);
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
    						//bprintf("%d Send milliseconds\r\n",(xTaskGetTickCount()-t_start)/portTICK_RATE_MS);
    						st=3;
    						break;
    				case 3: // ahora HTTP

    						for (ct=0;ct<2048;ct++) buff_WIFI_response[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, buff_WIFI_response,2048);
    						HAL_UART_Transmit(&huart1, request,strlen( (const char *)request),10000);

							if (ESP_TimeOut_HTTP(10000,buff_WIFI_response,"\r\n\r\n", "SEND3",buff_WIFI_response))
								st=4; // cerrar la conexion		
							//
 							//if (ESP_TimeOut(4000,buff_WIFI_response,"CLOSED", "SEND2",buff_WIFI_response))
    						//if (ESP_TimeOut(4000,buff_WIFI_response,"HTTP/1.1", "SEND2",buff_WIFI_response))
    						//if (ESP_TimeOut_tworesponses(4000,buff_WIFI_response, "HTTP/1.1","","SEND2",buff_WIFI_response)) // "reasonPhrase",
    						//	st=4;
							vTaskDelay(10/portTICK_RATE_MS );
    						//vTaskDelay(2000/portTICK_RATE_MS );

    						HAL_UART_DMAStop(&huart1);
    						//bprintf("%d Recepc milliseconds\r\n",(xTaskGetTickCount()-t_start)/portTICK_RATE_MS);
    						//bprintf("8: %s",buff_WIFI_response);
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

	vTaskDelay(1/portTICK_RATE_MS );
    }// while
    t_end=xTaskGetTickCount();
    //bprintf("%d milliseconds\r\n",(t_end-t_start)/portTICK_RATE_MS);
	return buff_WIFI_response;
}

*/

uint8_t * ESP_Send_Request(scomm_request_t *request, uint32_t timeout){
    int ct;
    int st;
    int lc;

    st=1;

    TickType_t t_start;
    TickType_t t_end;


    t_start=xTaskGetTickCount();
    while(st!=5){

    switch (st){
    				case 1: // connect

    					    // abrir conexión con
    						for (ct=0;ct<2048;ct++) request->HTTP_response[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, request->HTTP_response,2048);
    						sprintf(( char *)request->AUXILIARY,"AT+CIPSTART=\"TCP\",\"%s\",%d\r\n",request->dst_address,(int)request->dst_port);
    						HAL_UART_Transmit(&huart1, ( unsigned char *) request->AUXILIARY,strlen((const char *)request->AUXILIARY),10000);
    						if (ESP_TimeOut(timeout,request->HTTP_response,"CONNECT\r\n", "CONNECT Send Request",request->HTTP_response))
    						{		bprintf("TC: %s\r\n\r\n",request->AUXILIARY);
									return NULL;
							}
    						//vTaskDelay(400/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);

    						//

    						//bprintf("%d Connect milliseconds\r\n",(xTaskGetTickCount()-t_start)/portTICK_RATE_MS);
    						//bprintf("6e: %s",buff_recv);
    						st=2;
    						break;
    				case 2: // preparacion de envio
    						// enviar una peticion HTTP

    						lc=strlen((const char *)request->HTTP_request);
    						sprintf((char *)request->AUXILIARY, "AT+CIPSEND=%d\r\n", lc);
    						for (ct=0;ct<2048;ct++) request->HTTP_response[ct]=0;
    						HAL_UART_Receive_DMA(&huart1, request->HTTP_response,2048);
    						HAL_UART_Transmit(&huart1,( const uint8_t *)request->AUXILIARY,strlen((const char *)request->AUXILIARY),10000);
    						vTaskDelay(10/portTICK_RATE_MS );
    						if (ESP_TimeOut(2000,request->HTTP_response,">", "SEND",request->HTTP_response))
    							st=4; // cerrar la conexion

    						//vTaskDelay(2000/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);
    						//bprintf("7: %s",request.buff_recv);
    						//bprintf("%d Send milliseconds\r\n",(xTaskGetTickCount()-t_start)/portTICK_RATE_MS);
    						st=3;
    						break;
    				case 3: // ahora HTTP
							for (ct=0;ct<2048;ct++) request->HTTP_response[ct]=0;
    					
    						HAL_UART_Receive_DMA(&huart1, request->HTTP_response,2048);
    						HAL_UART_Transmit(&huart1, request->HTTP_request,strlen( (const char *)request->HTTP_request),10000);

							if (ESP_TimeOut_HTTP(10000,request->HTTP_response,"\r\n\r\n", "SEND3",request->HTTP_response))
								st=4; // cerrar la conexion		
							/*
 							if (ESP_TimeOut(4000,request.HTTP_response,"CLOSED", "SEND2",request.HTTP_response))
    						if (ESP_TimeOut(4000,request.HTTP_response,"HTTP/1.1", "SEND2",request.HTTP_response))
    						//if (ESP_TimeOut_tworesponses(4000,request.HTTP_response, "HTTP/1.1","","SEND2",request.HTTP_response)) // "reasonPhrase",
    							st=4;
								*/
    						vTaskDelay(10/portTICK_RATE_MS );
    						//vTaskDelay(2000/portTICK_RATE_MS );

    						HAL_UART_DMAStop(&huart1);
    						//bprintf("%d Recepc milliseconds\r\n",(xTaskGetTickCount()-t_start)/portTICK_RATE_MS);
    						//bprintf("8: %s",buff_WIFI_response);
    						st=4;
    						break;
    				case 4: // cerrar conexión

    						for (ct=0;ct<2048;ct++) request->AUXILIARY[ct]=0; // no se puede macachar la respuesta
    						HAL_UART_Receive_DMA(&huart1, request->AUXILIARY,2048);
    						HAL_UART_Transmit(&huart1, ( unsigned char *) "AT+CIPCLOSE\r\n",strlen("AT+CIPCLOSE\r\n"),10000);
    						vTaskDelay(20/portTICK_RATE_MS );
    						HAL_UART_DMAStop(&huart1);
    						//bprintf("6: %s",request->HTTP_response);
    						st=5;
    						break;
    				case 6: bprintf("WIFI error\r\n");
    						st=4;
    						break;

    }//switch

	vTaskDelay(1/portTICK_RATE_MS );
    }// while
    t_end=xTaskGetTickCount();
	
    //bprintf("%d milliseconds\r\n",(t_end-t_start)/portTICK_RATE_MS);
	return request->HTTP_response;
}





