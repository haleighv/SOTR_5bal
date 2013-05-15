/***************************
* Filename: usart.c
*
* Description: Provides print methods for various
*              datatypes using USART.
*
* Authors: Doug Gallatin and Jason Schray
* Edited by: Tim Peters & James Humphrey
*
* Revisions:
* 5/10/12 HAV implemented queue usage in transmit function
* 5/10/12 HAV Added USART_Write_Task
***************************/
#include "FreeRTOS.h"
#include "semphr.h"
#include <stdlib.h>
#include <stdint.h>
#include <avr/io.h>
#include "usart.h"

xQueueHandle xUsartQueue;

/************************************
* Function: usart_init
*
* Description: Initializes the USART module with
*  the specified baud rate and clk speed.
*
* Param buadin: The desired Baud rate.
* Param clk_seedin: The clk speed of the ATmega328p
************************************/
void USART_Init(uint16_t baudin, uint32_t clk_speedin) {
    uint32_t ubrr = clk_speedin/(16UL)/baudin-1;
    UBRR0H = (unsigned char)(ubrr>>8) ;// & 0x7F;
    UBRR0L = (unsigned char)ubrr;
    /* Enable receiver and transmitter */
    UCSR0B = (1<<RXEN0)|(1<<TXEN0);
    /* Set frame format: 8data, 1stop bit */
    UCSR0C = (1<<UCSZ01)|(1<<UCSZ00);
	// clear U2X0 for Synchronous operation
    UCSR0A &= ~(1<<U2X0);
	
	xUsartQueue = xQueueCreate( 150, sizeof( uint8_t ));
}

/************************************
* Function: USART_Write
*
* Description: Adds a byte of data to 
*			   the back of a queue.
*
* Param data: 8bit data value
************************************/
void USART_Write(uint8_t data) {
	xQueueSendToBack( xUsartQueue, &data, 0);
}

/************************************
* Function: USART_Write_Unprotected
*
* Description:Transmits a byte of data 
*			  via UART
*
* Param data: 8bit data value
************************************/
void USART_Write_Unprotected(uint8_t data) {
	/* Wait for empty transmit buffer */
	while ( !( UCSR0A & (1<<UDRE0)) );
	/* Put data into buffer, sends the data */
	UDR0 = data;
}

/************************************
* Function: USART_Read
*
* Description:the receive data function. 
*        Note that this a blocking call
*        Therefore you may not get control 
*        back after this is called until a 
*        much later time. It may be helpful 
*        to use the istheredata() function 
*        to check before calling this function.
*
* Return UDR0: Received data
************************************/
uint8_t USART_Read(void) {
    /* Wait for data to be received */
    while ( !(UCSR0A & (1<<RXC0)) );
    /* Get and return received data from buffer */
    return UDR0;
}

/************************************
* Function: USART_Write_Task
*
* Description: Sends queued data over UART
*
* Param vParam: This parameter is not used.
************************************/
void USART_Write_Task(void *vParam) {
	uint8_t uart_data;
    while (1) {
		xQueueReceive( xUsartQueue, &uart_data, portMAX_DELAY);
		USART_Write_Unprotected(uart_data);
	}	
}