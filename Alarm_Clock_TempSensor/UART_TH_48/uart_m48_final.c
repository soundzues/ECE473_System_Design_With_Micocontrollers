//Aditya Kothari
//12/06/2019
//lab 5 & 6
//ATmega48 function for sending remote data to m128
#include <avr/io.h>
#include <stdlib.h>
#include "uart_functions_m48.h"
#include <avr/interrupt.h>
#include <util/delay.h>
#include "twi_master.h"
#include "lm73_functions.h"
#include <string.h>


uint8_t i;                     //general purpose index
extern uint8_t lm73_wr_buf[2];  
extern uint8_t lm73_rd_buf[2];
char rx_char;
uint8_t send_flag = 0;
uint16_t lm73_temp;  //a place to assemble the temperature from the lm73
char temp[16];

int main()
{
	DDRC = 0x20;
	_delay_ms(2);

	init_twi();
	uart_init();
	lm73_wr_buf[0] = LM73_PTR_TEMP; //load lm73_wr_buf[0] with temperature pointer address
	twi_start_wr(LM73_ADDRESS, lm73_wr_buf, 1); //start the TWI write process
	_delay_ms(2);    //wait for the xfer to finish
	sei();
	while(1)
	{
		twi_start_rd(LM73_ADDRESS, lm73_rd_buf, 2);     //read temperature data from LM73 (2 bytes) 
		_delay_ms(2);    //wait for it to finish
		lm73_temp = lm73_rd_buf[0]; //save high temperature byte into lm73_temp
		lm73_temp = (lm73_temp << 8);     //shift it into upper byte 
		lm73_temp |= lm73_rd_buf[1]; //"OR" in the low temp byte to lm73_temp
	       	lm73_temp = lm73_temp/128;	
		itoa(lm73_temp, temp, 10); //convert to string in array with itoa() from avr-libc 

		//Look for charcter S
		rx_char = uart_getc();
		
		if(rx_char == 'S')
		{	
	   		send_flag = 1;
	   		rx_char = 'x';
		}
	

		if (send_flag == 1)
		{ 
			uart_puts(temp);	 	//sends the temp oputup to M128
			_delay_us(2);
			uart_putc('\0'); //sends null character
			_delay_us(2);

			send_flag = 0;
		} 

	}
}

