//UART Example for inclass coding
//Roger Traylor 12.4.12
//Connect two mega128 boards via rs232 and they should end to each
//other a message and a sequence number.
//
//Change the message you send to your partner for checkoff.
//
//You can test this code by a "loopback" if you connect rx to tx
//on the DB9 connector.
//

#include <avr/io.h>
#include <stdlib.h>
#include <string.h>
#include "uart_functions.h"
#include "hd44780.h"
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t           i = 0;
volatile uint8_t  rcv_rdy;
char              rx_char; 
char              lcd_str_array[16];  //holds string to send to lcd
uint8_t           send_seq=0;         //transmit sequence number
char              lcd_string[3];      //holds value of sequence number

void spi_init(void){
  DDRB   = DDRB | 0x07;           //Turn on SS, MOSI, SCLK pins
  SPCR  |= (1<<SPE) | (1<<MSTR);  //set up SPI mode
  SPSR  |= (1<<SPI2X);            //run at double speed 
}//spi_init    

int main()
{
  DDRF |= 0x08; //lcd strobe bit
  uart_init();  //RCV INTERUPT ENABLED
  spi_init();
  lcd_init();
  clear_display();
  cursor_home();
  sei();

    	while(1)
    	{
		_delay_ms(1000);
		
		uart_putc('S');

		_delay_ms(10);

		//Recieve 
		if(rcv_rdy == 1)
		{
			clear_display();
			string2lcd(lcd_str_array); //send to lcd
			rcv_rdy = 0; //reset recv flag
		}
 	}//while
}//main


ISR(USART0_RX_vect)
{
 //   string2lcd("Test");
 // static  uint8_t  i;
  rx_char = UDR0;              //get character
  lcd_str_array[i++]=rx_char;  //store in array 
 //if entire string has arrived, set flag, reset index
// uint8_t testvar=1;
  if(rx_char == '\0')
  { 
//    string2lcd("Test");
    rcv_rdy=1; 
    i=0;  
  }

 //  char2lcd(rx_char);
  // _delay_ms(10);
}
//************************************//