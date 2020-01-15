//Aditya Kothari
//12/06/2019
//lab 5
//ATmega48 function 
#include <avr/io.h>
#include <stdlib.h>
#include "uart_functions_m48.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include "twi_master.h"
#include "lm73_functions.h"
#include <string.h>


char    temp_value[16];  //holds a string to refresh the LCD
char    buff_value[16];  //holds a string to refresh the LCD
char 	dec_final[8];
static uint8_t send = 0;
char value;
uint8_t value_new;
uint8_t i;                     //general purpose index
extern uint8_t lm73_wr_buf[2];  
extern uint8_t lm73_rd_buf[2];
char opstr[16];


int main()
{
	uint16_t lm73_temp;  //a place to assemble the temperature from the lm73
	init_twi();
	uart_init();
	lm73_wr_buf[0] = LM73_PTR_TEMP; //load lm73_wr_buf[0] with temperature pointer address
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 1); //start the TWI write process
	_delay_ms(2);    //wait for the xfer to finish
	sei();
	DDRC = 0x20;
	while(1)
	{
	   	//Get data from LM73
		_delay_us(150); //tenth second wait
		twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2);     //read temperature data from LM73 (2 bytes) 
		_delay_ms(2);    //wait for it to finish
		lm73_temp = lm73_rd_buf[0]; //save high temperature byte into lm73_temp
		lm73_temp = (lm73_temp << 8);     //shift it into upper byte 
		lm73_temp |= lm73_rd_buf[1]; //"OR" in the low temp byte to lm73_temp 
		itoa(lm73_temp>>7, temp_value, 10); //convert to string in array with itoa() from avr-libc
		strcat(temp_value, '\0'); 
		value =  uart_getc();

		//if character S recived, send the temorature data over to the m128
		if (value == ('S'))
		{ 
			uart_puts(temp_value);	 	//sends the temp oputup to M128
	
			_delay_us(10);
			uart_putc('\0'); //sends null character
			_delay_us(100);
			value = 0;
		} 



		_delay_ms(5);

	}
}
