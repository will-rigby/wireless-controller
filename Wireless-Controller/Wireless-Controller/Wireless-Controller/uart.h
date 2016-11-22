/*
********************************************************************************
** @file  uart.c
** @brief Provides UART functionality. Uses buffers to store incomming and
**        outgoing characters and interrupts to handle the transmission and
**        reception of data.
********************************************************************************
** external functions
********************************************************************************
** uart_init            Initialises the UART driver.
** uart_input_available Returns the number of bytes in the input buffer.
** uart_putc            Adds a character to the internal buffer for
**                      transmission. Will not insert the character if the
**                      buffer is full.
** uart_getc            Returns the next character from the input buffer. Will
**                      block if the buffer is empty.
********************************************************************************
*/


#ifndef UART_H
#define UART_H


/* includes ******************************************************************/

#include "config.h"

/* external typedef **********************************************************/
/* external define ***********************************************************/
/* external macro ************************************************************/
/* external variables ********************************************************/
/* external function prototypes **********************************************/

void uart_init(void);
uint8_t uart_input_available(void);
uint8_t uart_putc(uint8_t c);
uint8_t uart_getc(void);


#endif // UART_H