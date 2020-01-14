// lab1_code.c 
// Aditya Kothari
// 10.01.2019

//This program increments a binary display of the number of button pushes on switch 
//S0 on the mega128 board.

#include <avr/io.h>
#include <util/delay.h>

//*******************************************************************************
//                            debounce_switch                                  
// Adapted from Ganssel's "Guide to Debouncing"            
// Checks the state of pushbutton S0 It shifts in ones till the button is pushed. 
// Function returns a 1 only once per debounced button push so a debounce and toggle 
// function can be implemented at the same time.  Expects active low pushbutton on 
// Port D bit zero.  Debounce time is determined by external loop delay times 12. 
//*******************************************************************************
int8_t debounce_switch() {
  static uint16_t state = 0; //holds present state
  state = (state << 1) | (! bit_is_clear(PIND, 0)) | 0xE000;
  if (state == 0xF000) return 1;
  return 0;
}

//*******************************************************************************
// Check switch S0.  When found low for 12 passes of "debounce_switch(), increment
// PORTB.  This will make an incrementing count on the port B LEDS. 
//*******************************************************************************
int main()
{
DDRB = 0xFF;  //set port B to all outputs

//int8_t reset = 0x00;
int8_t count = 0;

int8_t LSB_4 = 0x00;
int8_t MSB_4 = 0x00;	
int8_t val = 0;
int8_t temp = 0;

while(1)
{     //do forever

 	if(debounce_switch()) 
 	{
		//incement port B value
		val++;
	
		//get lower 4 bytes
		LSB_4 = val & 0x0F;
	//	PORTB = LSB_4;

		MSB_4 = val & 0xF0;
	//	PORTB = MSB_4;

		if(LSB_4 <= 0x09)
		{
			temp = MSB_4 | LSB_4;
			PORTB = temp;
		}

		if(LSB_4 > 0x09)
		{
			//keep track of 4 MSB going to 0 to 9
			count++;

			//if 4 MSB go 0 to 9, 10 times reset
			if(count == 10)
			{
				//reset PORTB 
				val = val & 0x00;
				PORTB = val;

				//re start loop
				continue;
			}

			//reset lower 4 bytes
			LSB_4 = LSB_4 & 0x00;

			//add 1 -> 0x10 to 4 MSB's 
			MSB_4 = MSB_4 + 0X10;

			//put it in PORTB
			//val = MSB_4 | LSB_4;
			val = MSB_4 | LSB_4;

			PORTB = val;
		}

	}	
		
 }  //if switch true for 12 passes, increment port B
  _delay_ms(2);                    //keep in loop to debounce 24ms
 } 
