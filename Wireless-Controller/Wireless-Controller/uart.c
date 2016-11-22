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


/* includes ******************************************************************/

#include "uart.h"

/* private define ************************************************************/

// Controls the echoing of incoming data.
#define ECHO 0

#define UART_BAUD  9600L
#define UART_SPEED 1
#define UART_UBRR  ((F_CPU * UART_SPEED) / (16 * UART_BAUD) - 1)

#define UART_BUFFER_SIZE 100
#define UART_BUFFER_TYPE uint16_t

/* private typedef ***********************************************************/

// Structure that stores a buffer, it's insert location, the number of bytes in
// the buffer and an overrun flag.
struct BufferUART {
    uint8_t          buffer[UART_BUFFER_SIZE];
    UART_BUFFER_TYPE index;
    UART_BUFFER_TYPE bytes;
    uint8_t          overrun;
};

/* private macro *************************************************************/
/* private variables *********************************************************/

static volatile struct BufferUART uart_buffer_output;
static volatile struct BufferUART uart_buffer_input;

/* private function prototypes ***********************************************/


/*
** @brief  Initialises the UART driver.
** @param  none
** @retval none
*/
void uart_init(void)
{
    // Setup the buffers by clearing them.
    memset((void *)uart_buffer_output.buffer, 0, UART_BUFFER_SIZE);
    uart_buffer_output.index   = 0;
    uart_buffer_output.bytes   = 0;
    uart_buffer_output.overrun = 0;

    memset((void *)uart_buffer_input.buffer, 0, UART_BUFFER_SIZE);
    uart_buffer_input.index   = 0;
    uart_buffer_input.bytes   = 0;
    uart_buffer_input.overrun = 0;

    UCSR0A = 0;
    UCSR0B = 0;
    UCSR0C = 0;

    // Set the BAUD rate register.
    UBRR0H = (uint8_t)(UART_UBRR >> 8);
    UBRR0L = (uint8_t)(UART_UBRR >> 0);

    // Disable the UART double transmission speed mode.
    UCSR0A |= (0 << U2X0);

    // Disable multi-processor communication mode.
    UCSR0A |= (0 << MPCM0);

    // Enable the receive complete and the data register empty interrupt and
    // disable the transmit complete interrupt.
    UCSR0B |= (1 << RXCIE0) | (0 << TXCIE0) | (1 << UDRIE0);

    // Enable the receiver and transmitter.
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0);

    // Use 8 bit characters.
    UCSR0B |= (0 << UCSZ02);
    UCSR0C |= (1 << UCSZ01) | (1 << UCSZ00);

    // Enable the asynchronous mode.
    UCSR0C |= (0 << UMSEL01) | (0 << UMSEL00);

    // Disable parity.
    UCSR0C |= (0 << UPM01) | (0 << UPM00);

    // Use 1 stop bits.
    UCSR0C |= (0 << USBS0);

    // Write zero to the clock polarity bit because we are using asynchronous
    // mode.
    UCSR0C |= (0 << UCPOL0);
}

/*
** @brief  Returns the number of bytes in the input buffer.
** @param  none
** @retval Number of bytes in input buffer.
*/
uint8_t uart_input_available(void)
{
    return uart_buffer_input.bytes;
}

/*
** @brief  Adds a character to the internal buffer for transmission. Will not
**         insert the character if the buffer is full.
** @param  c Character to transmit.
** @retval Returns 1 on buffer overflow.
*/
uint8_t uart_putc(uint8_t c)
{
    uint8_t interrupts_enabled;

    // If the output buffer is full return 1 and set the buffer overrun flag.
    if(uart_buffer_output.bytes >= UART_BUFFER_SIZE) {
        uart_buffer_output.overrun = 1;
        return 1;
    }

    // Copy the status register and disable interrupts before adding to the
    // buffer. This will stop the ISR from modifying the buffer at the same
    // time.
    interrupts_enabled = (SREG >> SREG_I) & 0x1;
    cli();

    uart_buffer_output.buffer[uart_buffer_output.index] = c;
    uart_buffer_output.bytes++;

    // If the index has reached the end, wrap around.
    if(uart_buffer_output.index + 1 == UART_BUFFER_SIZE) {
        uart_buffer_output.index = 0;
    } else uart_buffer_output.index++;

    // Ensure that the data register empty interrupt is enabled to handle
    // transmission of the new data.
    UCSR0B |= (1 << UDRIE0);

    // Return the status register to its original state. Re-enabling interrupts
    // if they were disabled.
    if(interrupts_enabled) sei();

    return 0;
}

/*
** @brief  Returns the next character from the input buffer. Will block if the
**         buffer is empty.
** @param  none
** @retval Returns the next character.
*/
uint8_t uart_getc(void)
{
    uint8_t c;
    uint8_t interrupts_enabled;

    // Wait for input
    while(uart_buffer_input.bytes == 0);

    // Save the STATUS register I bit state and disable interrupts before
    // modifying the buffer.
    interrupts_enabled = (SREG >> SREG_I) & 0x1;
    cli();

    // Get the character from the buffer and decrement the buffer bytes. Ensure
    // that you wrap around the buffer boundaries.
    if(uart_buffer_input.index - uart_buffer_input.bytes < 0) {
        c = uart_buffer_input.buffer[uart_buffer_input.index -
                                     uart_buffer_input.bytes +
                                     UART_BUFFER_SIZE];
    } else {
        c = uart_buffer_input.buffer[uart_buffer_input.index -
                                     uart_buffer_input.bytes];
    }

    uart_buffer_input.bytes--;

    // Re-enable the interrupts if they were originally enabled.
    if(interrupts_enabled) sei();

    return c;
}

ISR(USART_RX_vect) {
    char c;

    // Grab the character from the UART receive buffer.
    c = UDR0;

    // Echo the character if echoing is enabled.
    if(ECHO) uart_putc(c);
    
    // If the is no space in the buffer set the overrun flag and return.
    if(uart_buffer_input.bytes >= UART_BUFFER_SIZE) {
        uart_buffer_input.overrun = 1;
        return;
    }

    // Insert the character into the buffer and increment the number of bytes
    // in the buffer.
    uart_buffer_input.buffer[uart_buffer_input.index] = c;
    uart_buffer_input.bytes++;

    // Increment the insert index ensuring the wrap about the buffer
    // boundaries.
    if(uart_buffer_input.index + 1 == UART_BUFFER_SIZE) {
        uart_buffer_input.index = 0;
    } else uart_buffer_input.index++;
}

ISR(USART_UDRE_vect) {
    // If there are bytes in the buffer transmit the next one.
    if(uart_buffer_output.bytes > 0) {
        if(uart_buffer_output.index - uart_buffer_output.bytes < 0) {
            UDR0 = uart_buffer_output.buffer[uart_buffer_output.index -
                                             uart_buffer_output.bytes +
                                             UART_BUFFER_SIZE];
        } else {
            UDR0 = uart_buffer_output.buffer[uart_buffer_output.index -
                                             uart_buffer_output.bytes];
        }

        uart_buffer_output.bytes--;
    } else {
        // If there is no more bytes to transmit clear the data register empty
        // interrupt.
        UCSR0B &= ~(1 << UDRIE0);
    }
}