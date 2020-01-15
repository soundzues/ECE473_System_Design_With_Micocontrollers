// siren.c  - R. Traylor  - 10.31.2013
// 
// Setup TCNT1 to be a varaible frequency tone generator. Its made variable by dynamically 
// changing the compare register.
//
// Setup TCNT3 to interrupt the processor at a rate of 1000 times a second.  When the 
// interrupt occurs, the ISR for TCNTR3 changes the frequency of timer TCNT1 to affect 
// a continually changing audio frequency at PORTB bit 5.
//
// set OC1A (PB5) as the audio output signal PORTB (PB7) is the PWM signal for creating 
// the volume control.
//
// Timer TCNT3 is set to interrupt the processor at a rate of 1000 times 
// a second.  When the interrupt occurs, the ISR for TCNTR3 changes the
// frequency of timer TCNT1 to affect a continually changing audio
// frequency at PORTB bit 5.
//
// to download: 
// wget http://www.ece.orst.edu/~traylor/ece473/inclass_exercises/timers_and_counters/siren_skeleton.c

#include <avr/io.h>
#include <avr/interrupt.h>


ISR(TIMER1_COMPA_vect) 
{
	PORTD ^= 0x80;
}
                                  

int main(){

  DDRB   |= (1<<PB7) | (1<<PB5);                       //set port B bit five and seven as outputs
  DDRD = 0xFF;
  PORTD = 0x80;
  DDRE = 0XFF;

//setup TCNT1
  TCCR1A = 0x00;                  //Normal mode operation

  TCCR1B = (1<<WGM12) | (1<<CS11);   //use OCR1A as source for TOP, use clk/1

  TCCR1C = 0x00;                          //no forced compare 

  OCR1A = 0x0269; //top value
  
  TIMSK |= (1<<OCIE1A); //enable interrupt

 
//setup TCNT3
// siren update frequency = (16,000,000)/(OCR3A) ~ set to about 1000 cycles/sec

 /*
  TCCR3A = (1<<COM3A1) | (0<<COM3A0);                           //CTC mode, output pin does not toggle 

  TCCR3B = (1<<WGM32) | (1<<CS30);                          //no prescaling      

  TCCR3C = 0x00;                          //no forced compare 

  OCR3A = 0x5000;                           //pick a speed from 0x1000 to 0xF000

  ETIMSK = (1<<OCIE3A);                          //enable timer 3 interrupt on OCIE3A
 */

 //TCNT2 setup for providing the volume control
 //fast PWM mode, TOP=0xFF, clear on match, clk/128
 //output is on PORTB bit 7 
// TCCR2 =  (1<<WGM21) | (1<<WGM20) | (1<<COM21) | (1<<COM20) | (1<<CS20) | (1<<CS21);
// OCR2  = 0x00;  //set to adjust volume control
 
//   TCCR3A = (1<<WGM30) | (1<<WGM31) | (1<<COM3A1) | (1<<COM3A0);
//   TCCR3B = (1<<WGM32) | (1<<WGM33) | (1<<CS30)


	TCCR3A |= (1<<COM3A1) | (1<<COM3A0) | (1<<WGM31);//initialize timer3 for vol
	TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS30);   //control
	TCCR3C = 0x00;
	OCR3A = 0x8000;
	ICR3 = 0xF000;



  sei();     //set GIE to enable interrupts
  while(1){} //do forever
 
}  // main
