// lab2_skel.c 
// Aditya Kothari
// ECE 473, Lab2 

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base.

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#include <avr/io.h>
#include <util/delay.h>

//Array to hold different digits to turn on

//uint8_t dig_select[4] = {0x00, //digit 4
//			0x10, //digit 3 
//			  0x20, //digit 2
//		 	  0x30, //digit 1	 
//			 };

uint8_t digit_select[] = {0x00, 0x10, 0x30, 0x40};
//Array to hold different state of the digits
uint8_t segment_select[4]; //digit 4

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[10]= {0b00000011, 0b10011111, 0b00100101, 0b00001101, 0b10011001, 0b01001001, 0b01000001, 0b00011111, 0b00000001, 0b00001001}; 



//******************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 
//

uint8_t chk_buttons(uint8_t button) 
{
	static uint16_t state[8] = {0}; //holds present state
	state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
	if (state[button] == 0xF000) return 1;
	return 0;
}

//******************************************************************************

//***********************************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|

void segsum(uint16_t sum) {
  uint16_t temp = sum;
  
  //determine how many digits there are 
  //extract digit
  uint8_t thousands = (temp/1000);
  //update temp
  temp = temp%1000;

  uint8_t hundreds = (temp/100);
  temp = temp%100;

  uint8_t tens = (temp/10);
  temp = temp%10;

  uint8_t units = temp;
  
  //break up decimal sum into 4 digit-segments
  segment_select[0] = dec_to_7seg[units]; //digit 0
  segment_select[1] = dec_to_7seg[tens]; //digit 1
  segment_select[2] = dec_to_7seg[hundreds]; //digit 2
  segment_select[3] = dec_to_7seg[thousands]; //digit 3

  //blank out leading zero digits
  if(sum < 10)
  {
	 //FF => turn off leds 
	 segment_select[1] = 0xFF;
	 segment_select[2] = 0xFF;
	 segment_select[3] = 0xFF;
  }
  else if(sum < 100)
  {

	 segment_select[2] = 0xFF;
	 segment_select[3] = 0xFF;
  }
  else if(sum < 1000)
  {
	  segment_select[3] = 0xFF;
  }
  //now move data to right place for misplaced colon position
}//segment_sum

//***********************************************************************************



//***********************************************************************************
int main()
{
//set port bits 4-7 B as outputs
	DDRB = 0XF0;
//Keep track of count
uint16_t num = 0x0000;

//Button pressed for loop
uint8_t button = 0;

//Debug flag
int flag = -5;

//for loop
int i = 0;

while(1){

  	asm volatile ("nop");
  //insert loop delay for debounce
 // 	_delay_ms(2);
  
 //make PORTA an input port with pullups
  	DDRA = 0x00;
       	PORTA = 0xFF;	
  //enable tristate buffer for pushbutton switch
  	PORTB = 0x70;
  
  //now check each button and increment the count as needed
  //cycle through each and every buttons checking for pushed button:
   	for(button = 0; button < 8; button++)
	{
//		flag = 1;
		
		//If button is pushed, increment count respectively			
		if(chk_buttons(button))
		{
//			flag = 1;
			num = num + (1<<button);
			asm volatile ("nop");
			asm volatile ("nop");
/*			if(num > 10)
			{
				flag = 1;
			}*/
		}
		
/*
		//		flag = 1;
		//if button is pushed
		if(chk_buttons(button))
		{

//				flag = 1;

		//	PORTB = 0x00;
			//increment count as needed:
			if(button == 0) //first button pressed
			{
			//	flag = 1;
		//		DDRA = 0xFF;	
		//		PORTA = 0b00001111;
				num = num + 1;
			}
			
			if(button == 1) //second button pressed
			{
				num = num + 2;
			}
			
			if(button == 2) //third button pressed
			{
				num = num + 4;
			}	
			if(button == 3) //fourth button pressed
			{
				num = num + 8;
			}	
			if(button == 4) //fifth button pressed
			{
				num = num + 16;
			}
			if(button == 5) //sixth button pressed
			{
				num = num + 32;
			}
			if(button == 6) //seventh button pressed
			{
				num = num + 64;
			}
			if(button == 7) //eigth button pressed
			{
				num = num + 128;
			}*/
		//}
	}

  //disable tristate buffer for pushbutton switches
//  	PORTB = 0x00;
 	PORTB = 0x50;

  //bound the count to 0 - 1023
  	if(num > 1023)
	{
		//reset num to 1
		num = 0x0001;
	}
  //break up the disp_value to 4, BCD digits in the array: call (segsum)
  	segsum(num);

  //bound a counter (0-4) to keep track of digit to display 
  //make PORTA an output
  
  //NOP before changing states
	asm volatile ("nop");
  	asm volatile ("nop");
	DDRA = 0xFF;
 	 
  //NOP before changing states 
/*	//test
	if(flag == 1)
	{
		DDRA = 0xFF;	
		PORTA = 0b11111110;
		flag = 0;
	}
*/
/* //test
	if(flag == 1)
	{
	 	PORTB = digit_select[3];
		PORTA = 0b00000001;
 	}*/

  	//update digit to display
	for(i = 0; i < 4; i++)
	{
  		//send 7 segment code to LED segments
		PORTA = segment_select[i];

  		//send PORTB the digit to display
		PORTB = digit_select[i];
		
  //		asm volatile ("nop");
  //		asm volatile ("nop");
  		

		/*
		//test
		if(i == 0)
		{
			PORTA = 0b01111111;
		}
		if(i == 1)
		{
			PORTA = 0b00111111;
		}
		if(i == 2)
		{
			PORTA = 0b00011111;
		}
		if(i == 3)
		{
			PORTA = 0b00001111;
		}
		if(i == 4)
		{
			PORTA = 0b00000111;
		}*/
//		test
//		PORTB = digit_select[0];
	//	PORTA = segment_select[i];
//		PORTA = 0b00111111;
//		_delay_ms(200)
		_delay_ms(2);
	}
  }//while
	
}//main
