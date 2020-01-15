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
#include "uart_functions_m48.h"
#include <avr/interrupt.h>
#include <util/delay.h>

uint8_t           i;
//volatile uint8_t  rcv_rdy;
char              rx_char; 
char              lcd_str_array[16];  //holds string to send to lcd
uint8_t           send_seq=0;         //transmit sequence number
char              lcd_string[3];      //holds value of sequence number


int main(){
  //DDRF |= 0x08; //lcd strobe bit
  uart_init();  

  sei();
  while(1){

  uart_putc('c'); 
//**************  start tx portion ***************
/*
    uart_puts("Hi! Dilbert: ");
    itoa(send_seq,lcd_string,10);
    uart_puts(lcd_string);
    uart_putc('\0');
    for(i=0;i<=9;i++){_delay_ms(100);}
    send_seq++;
    send_seq=(send_seq%20);*/
//**************  end tx portion ***************
  }//while
}//main
