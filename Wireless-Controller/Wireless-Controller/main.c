/*
 * GccApplication2.c
 *
 * Created: 28/09/2016 10:32:56 AM
 * Author : s4318884
 */ 



#define F_CPU 16000000UL        //Says to the compiler which is our clock frequency, permits the delay functions to be very accurate
#include <avr/io.h>            //General definitions of the registers values
#include <util/delay.h>

#define BAUDRATE 9600        //The baudrate that we want to use
#define BAUD_PRESCALLER (((F_CPU / (BAUDRATE * 16UL))) - 1)    //The formula that does all the required maths


void USART_init(void){
	
	UBRR0H = (uint8_t)(BAUD_PRESCALLER>>8);
	UBRR0L = (uint8_t)(BAUD_PRESCALLER);
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = ((1<<UCSZ00)|(1<<UCSZ01));
}

void USART_send( unsigned char data){
	
	while(!(UCSR0A & (1<<UDRE0)));
	UDR0 = data;
	
}


void InitADC()
{
	// Select Vref=AVcc
	ADMUX |= (1<<REFS0);
	//set prescaller to 128 and enable ADC
	ADCSRA |= (1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0)|(1<<ADEN);
}

uint16_t ReadADC(uint8_t ADCchannel)
{
	//select ADC channel with safety mask
	ADMUX = (ADMUX & 0xF0) | (ADCchannel & 0x0F);
	//single conversion mode
	ADCSRA |= (1<<ADSC);
	// wait until ADC conversion is complete
	while( ADCSRA & (1<<ADSC) );
	return ADC;
}

int main(void)
{
    /* Replace with your application code */
	PIND = 0x00;
	uint8_t laser;
	uint8_t motor1;
	uint8_t motor2;
	uint32_t adc1;
	uint32_t adc2;
	
	InitADC();
	USART_init();

	
    while (1) 
    {
		if(PIND & (1<<PIND5)){
			laser = 1;
		} else {
			laser = 0;
		}
		adc1 = ReadADC(4)/5;//)*200)*1024;
		motor1 = adc1;
		adc2 = ReadADC(5)/5;//*200)*1024;
		motor2 = adc2;
		USART_send('>');
		_delay_ms(3);
		USART_send(motor1);_delay_ms(3);
		USART_send(motor2);_delay_ms(3);
		USART_send(90);_delay_ms(3);
		USART_send(100);_delay_ms(3);
		USART_send(laser);_delay_ms(3);
		USART_send('<');_delay_ms(3);
		_delay_ms(30);


    }
}

