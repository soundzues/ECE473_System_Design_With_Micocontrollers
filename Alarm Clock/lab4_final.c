// Aditya Kothari
// Lab 4
// 11/19/2019

// Expected Connections:
// Encloder            mega128
// --------------       --------------------     
//     SHIFT_LD_N       PORTE bit 6                        
//     SCK              PORTB bit 1 (sclk)
//     CLK_INH          PORTE bit 7
//     SER_IN           no connect
//     gnd              ground (gnd on any port)
//     vdd              vcc    (vcc on any port)
//     sd_out           PORTB3 (MISO)
// 
// Bar Graph            mega128
// --------------       --------------------     
//     regclk           PORTB bit 0                        
//     srclk            PORTB bit 1 (sclk)
//     oe_n             ground
//     SER_IN           PORTB bit 2 (MOSI)
//     gnd              ground (gnd on any port)
//     vdd              vcc    (vcc on any port)
//     sd_out           no cconnect
//
// 
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define F_CPU 16000000
#include <stdlib.h>
#include "hd44780.h"


//***********************************************************************
//                            spi_init                               
//**********************************************************************

//GLOBAL VARS:
uint8_t data = 0x00; //hold data
uint8_t a_past = 0x03; // a of encoder 1 to be high in past
uint8_t b_past = 0x03; // b of encoder 1 to be high in past
uint8_t a_cur = 0x00; // current state of a
uint8_t b_cur = 0x00; // current state of b
int8_t direc = 0x00; //will hold the sign
uint8_t x = 0; //mode to be selectedi
uint8_t flag = 0;
uint8_t temp_0 = 1;
uint8_t c_past = 0x03;// b of encoder 2 to be high in past
uint8_t d_past = 0x03;// d of encoder 2 to be high in past
uint8_t c_cur = 0x00; // current state of c
uint8_t d_cur = 0x00; // current state of d
uint8_t flag_min = 0; //keep tracks of mins
uint16_t count_7ms = 0; //keep track of isr
uint8_t flag_sec = 0; //keeps track of sec
uint8_t flag_enc = 0; //enables/disables encoder
uint8_t flag_alarm = 0; //enables alarm mode
uint8_t flag_set_alarm = 0; //confirms user choice to set alarm
uint8_t flag_snooze = 0; //alows user to snoze alarm
uint8_t flag_disable_alarm = 0; //allows user to turn off alarm
uint8_t write = 0;
uint16_t ADC_val = 0;
uint8_t flag_ringer = 0; //if set causes the alarm to ring
uint16_t count_10_sec = 1;

//Lab2: Seven seg stuff:
//Array to hold different digits to turn on
//                  Digit: 0     1    2      3    4
//uint8_t digit_select[] = {0x00, 0x10, 0x30, 0x40};

uint8_t digit_select[] = {0x00, 0x10, 0x20, 0x30, 0x40};

//uint8_t digit_select[5];

//Array to hold different state of the digits

uint8_t segment_select[5]; //digit 4

//decimal to 7-segment LED display encodings, logic "0" turns on segment
//0          1           2           3           4           5           6           7           8           9
uint8_t dec_to_7seg[10]= {0b00000011, 0b10011111, 0b00100101, 0b00001101, 0b10011001, 0b01001001, 0b01000001, 0b00011111, 0b00000001, 0b00001001};


//******************************************************************************

//***********************************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|

void segsum(int16_t sum, int16_t alarm_sum) {
	uint16_t temp = sum;

	if(flag_alarm == 1)
	{
		temp = alarm_sum;
	}

	//extract out hours and mins
	uint16_t hours = temp/60;
	uint16_t mins =  temp%60;

	//determing digits for hours
	uint8_t tens_h = hours/10;
	uint8_t units_h = hours%10;

	//determing digits for minutes
	uint8_t tens_m = mins/10;
	uint8_t units_m = mins%10;

	//break up decimal sum into 4 digit-segments
	segment_select[0] = dec_to_7seg[units_m]; //digit 0
	segment_select[1] = dec_to_7seg[tens_m]; //digit 1

	//set it only once at the beginging  
	if(flag_sec == 0)
	{
		segment_select[2] = 0x3F;
	}

	segment_select[3] = dec_to_7seg[units_h]; //digit 2
	segment_select[4] = dec_to_7seg[tens_h]; //digit 3

	/*
	   if(flag_sec == 1)
	   {
	   segment_select[2] = 0xFF;
	   flag_sec = 0;
	   }*/

	if(hours < 10)
	{
		segment_select[4] = dec_to_7seg[0];
	}
	if(mins < 10)
	{
		segment_select[1] = dec_to_7seg[0];
	}


}//segment_sum


//******************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 
//
int8_t chk_buttons(uint8_t button)
{
	static uint16_t state[8] = {0}; //holds present state
	state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
	if (state[button] == 0xF000) return 1;
	return 0;
}



//*******************************************************************************
//			     spi_init
//enabling SPI and master mode. Selecting double speed of operation
//
void spi_init(void){


	DDRB  = 0xF7; //output mode for MOSI, SCLK, MISO set to input

	asm volatile ("nop");
	asm volatile ("nop");

	SPCR  = (1<<SPE)|(1<<MSTR); //Enable SPI,master mode

	SPSR  |= 0x01; //choose double speed operation
}//spi_init

//********************************************************************************
//				spi_read
//Getting rotation direction data from encoder about by manipulating SH/LD and cloc//k INHABIT
//

int8_t spi_read()
{

	PORTE |= (1<<PE7); //CLOCK INH HIGH

	asm volatile ("nop");
	asm volatile ("nop");

	PORTE &= (1<<PE7) | (0<<PE6); //load parallel data

	asm volatile ("nop");
	asm volatile ("nop");

	PORTE |= (1<<PE6); //shift data

	PORTE &= (1 <<PE6) | (0<<PE7); //CLOCK INH low

	SPDR = 0X00; //clear data register
	while (bit_is_clear(SPSR, 7)) {} 	//spin till SPI data has been read 

	return SPDR;
}

//********************************************************************************
//					SPI Write
//Displays mode of opereation e.g: mode 1 (x = 1) will increment count by 2.
//Sending mode of operation from Atmega (Push Buttons) to Bargraph

void spi_write()
{

	SPDR = x;		//send mode to the display 
	while (bit_is_clear(SPSR, 7)) {} 	//spin till SPI data has been sent 

	PORTB |= 0x01;	            	//send rising edge to regclk on HC595 

	PORTB &= 0xFE;	           	//send falling edge to regclk on HC595
}

//Interrupt vector
ISR(TIMER0_OVF_vect)
{
	//if snoze flag count for 10 sec
	if(flag_snooze == 1)
	{
		count_10_sec++;	
	}


	count_7ms++;

	//keep track of half sec
	if((count_7ms % 64) == 0)
	{
		segment_select[2] ^= 0xC0;
		flag_sec = 1;
		//	_delay_ms(5);
	}


	//keep track of 10 sec for sonooze
	if((count_10_sec % 1280) == 0)
	{
		//play alarm 
		flag_ringer = 1;

		//turn off snozee flag
		flag_snooze = 0;

		//reset the counter to 1;
		count_10_sec = 1;

		//	_delay_ms(5);
	}

	//keep track of min
	if((count_7ms % 7680) == 0)
	{
		flag_min = 1;
	}

	data = spi_read(); //read data from encoder

	uint8_t button; //var to hold button

	uint8_t temp_pb;

	temp_pb = PORTB;

	//Enable Tristate
	PORTB |= 0X70;


	asm volatile ("nop");
	asm volatile ("nop");

	//checking for button presses
	for(button = 1; button < 6; button++)
	{
		if(chk_buttons(button))
		{
			//if button pressed following mode is selected
			//x == mode
			x ^= button;

			_delay_ms(2);
		}
		
		//button 1: Change time mode 
		if(x == 0x01)
		{
			//enable encoder
			flag_enc = 1;
		}
		//button 2: Alarm mode 
		else if(x == 0x02)
		{
			flag_alarm = 1;
		}
		//button 3: allows user to set alarm
		else if(x == 0x03)
		{
			flag_set_alarm = 1;
			
			//send alarm armed message to LCD
//			clear_display();
			string2lcd("Alarm SET");

			//delay
			_delay_ms(2);
			
			//turn off button
			x = 0x00;
		}	
		//button 4: allows user to snooze (0x04)
		//will reset the set_alarm flag
		//add alarm count + 1
		else if(x == 0x04)
		{
	//		if(flag_set_alarm == 1)
	//		{	
				//ringer is off
				flag_ringer = 0;

				//set off 10 sec counter
				flag_snooze = 1;

				//delay
				_delay_ms(2);
			
				//turn off the button
				x = 0x00;
	//		}
		}
		//button 6: to turn of allarm function
		else if(x == 0x05)
		{
		//	flag_disable_alarm = 1;

			//turn off ringer
			flag_ringer = 0;
			
			//turn off allarm
			flag_set_alarm = 0;
			
			//turn off snooze
			flag_snooze = 0;

			//clear display
			clear_display();

			//turn off the button:
			x = 0x00;
		}
		else
		{
			//disable encoder
			flag_enc = 0;

			//disable alarm flag
			flag_alarm = 0;
		}
	}

	//Disable Tristate, restore previous state
	PORTB = temp_pb;

	asm volatile ("nop");
	asm volatile ("nop");

	spi_write(); //write data to bar graph

}


//Encoder Data Direction:
int8_t encoder_direction()
{
	int8_t var = 0; //will hold the sign
	int8_t flag_1 = 0; //if flag 1 is set it will not check encoder 2


	//break the data
	a_cur = data & 0x01; //extract out a
	b_cur = (data>>1) & 0x01; //extract out b
	c_cur = (data>>2) & 0x01; //extract c
	d_cur = (data>>3) & 0x01; //extract d


	//Encoder 1
	//
	//check for different states to get the direction
	//if past and present states were same no need to increment count
	//var = 0 => no change, var = 1 => increment, car = -1 => decrement
	//sets flag_1 if it detects change of state in encoder 1
	//
	if(((a_cur == a_past) && (b_cur == b_past)) || ((c_cur == c_past) && (d_cur == d_past)))
	{
		var = 0;
	}

	if(a_past == a_cur)
	{
		if((a_cur == 0) && (b_past > b_cur)){  var = 1; flag_1 = 1;} //CW
	} 
	if((a_past < a_cur)&&(b_past | b_cur) == 1){  var = -1; flag_1 = 1;} //ccw 

	//update the past states
	a_past = a_cur;
	b_past = b_cur;


	//Encoder 2
	//
	//check for different states to get the direction
	//if past and present states were same no need to increment count
	//var = 0 => no change, var = 1 => increment, car = -1 => decrement
	//if flag_1 set won't check encoder 2's state
	//
	if(flag_1 == 0)
	{

		if(c_past == c_cur)
		{
			if((c_cur == 0) && (d_past > d_cur)){  var = 1;} //CW
		}
		if((c_past < c_cur)&&(d_past | d_cur) == 1){  var = -1;} //ccw 

		//update the past states
		c_past = c_cur;
		d_past = d_cur;
	}

	return var;
}

void lcd_write()
{
	if(write == 1)
	{
		//make a function for lcd write
		clear_display();			//clear out the display
		string2lcd("Armed");
	}
}

//Timer Counter 0 setup running clock
void timer0_init()
{
	ASSR |= (1<<AS0); //run off external 32khz osc (TOSC)

	TIMSK |= (1<<TOIE0); //Enable interrupt

	TCCR0 = (1<<CS00); // No Prescale
}

//Timer to create the PWM signal
void timer2_init()
{
	TCCR2 |= (1<<WGM20) | (1<<WGM21) | (1<<COM21) | (1<<CS20); //PWM mode, Clear on compare match, clock I/O
}

//TImer to create frequency for tone
void timer1_init()
{
	TCCR1A = 0x00;                  //Normal mode operation

  	TCCR1B = (1<<WGM12) | (1<<CS11);   //use OCR1A as source for TOP, use clk/1

  	TCCR1C = 0x00;                          //no forced compare

  	OCR1A = 0x0269; //top value

  	TIMSK |= (1<<OCIE1A); //enable interrupt
}

//Timer to create volume signal
void timer3_init()
{
	TCCR3A |= (1<<COM3A1) | (1<<COM3A0) | (1<<WGM31);//initialize timer3 for vol
        TCCR3B |= (1<<WGM33) | (1<<WGM32) | (1<<CS30);   //control
        TCCR3C = 0x00;
        OCR3A = 0x6000;
        ICR3 = 0xF000;

}

ISR(TIMER1_COMPA_vect)
{
	//toggle port d to genrate anoying tone
	if(flag_ringer == 1)
	{
        	PORTD ^= 0x80;
	}
	if(flag_ringer == 0)
	{
		//testing delay if it fixes speaker
//		asm volatile ("nop");
	}
}



//ADC init
void ADC_init()
{
	DDRF  &= ~(_BV(PF7)); //make port F bit 7 is ADC input  
	PORTF &= ~(_BV(PF7));  //port F bit 7 pullups must be off
//	ADMUX = 0x47; //writes to ADMUX for single-ended, input PORTF bit 7, right adjusted, 10 bits
	ADMUX |= (1<<REFS0)|(1<<MUX2)|(1<<MUX1)|(1<<MUX0);//single-ended, input PORTF bit 7, right adjusted, 10 bits
	ADCSRA |= (1<<ADEN)|(1<<ADPS2)|(1<<ADPS1)|(1<<ADPS0);//ADC enabled, don't start yet, single shot mode
}


void ADC_conversion(uint16_t ADC_val)
{
//	uint8_t brightness = 5*(ADC_val/700);	
//	OCR2 = brightness;

/*
	if(ADC_val > 500)
	{
		OCR2 = 0xE0;
	}
	if(ADC_val < 500)
	{
		OCR2 = 0x20;
	}
*/

	if(ADC_val > 0 && ADC_val <= 100)
	{
		OCR2 = 32;
	}

	if(ADC_val > 100 && ADC_val <= 200)
	{
		OCR2 = 64;
	}

	if(ADC_val > 200 && ADC_val <= 300)
	{
		OCR2 = 128;
	}

	if(ADC_val > 300  && ADC_val <= 400)
	{
		OCR2 = 160;
	}
	
	if(ADC_val > 400 && ADC_val <= 500)
	{
		OCR2 = 192;
	}

	if(ADC_val > 500 && ADC_val <= 800)
	{
		OCR2 = 224;
	}

}



//**********************************************************************
//                                main                                 
//**********************************************************************
int main(){

	timer1_init(); //initialize timer 1

	timer3_init();	//initialize timer 3
	
	timer2_init(); //initalize timer 2

	timer0_init(); //initalize timer 0

	sei(); //intialize interrupt

	DDRE = 0XFF; //output mode PORTE
	DDRD = 0xFF;
  	
	PORTD = 0x80;


	asm volatile ("nop");
	asm volatile ("nop");

	uint8_t display_count = 1; 		//holds count for display 

	uint8_t i = 0; 				//dummy counter

	int16_t count = 0x0000;

	int16_t alarm_count = 0x0000;


	ADC_init();				//initalize ADC 
//	ADCSRA |= (1<<ADSC); //poke ADSC and start conversion	

	spi_init();  				//initalize SPI port

	lcd_init(); 				//initalize lcd 

	//clear display
	clear_display();			//clear out the display
//	string2lcd("Alarm off");
  //      cursor_home();	
	//_delay_ms(2);


	while(1){                             	//main while loop
		ADCSRA |= (1<<ADSC); //poke ADSC and start conversion	
	
		while(bit_is_clear(ADCSRA, ADIF)){};
	
		ADCSRA |= (1<<ADIF); //poke ADSC and start conversion	

		ADC_val = ADC;

		ADC_conversion(ADC_val);

//		OCR2 = 0xf0&ADC;
		//reset iterator
		if(i == 5)
		{
			i = 0;
		}

		//ADC Conversion
//		ADC_conversion();

		//insert loop delay for debounce

		_delay_ms(2);
		//make PORTA an input port with pullups
		DDRA = 0x00;

		//NOP after changing DDRX register
		asm volatile ("nop");
		asm volatile ("nop");

		PORTA = 0xFF;


		//get encoder direction
		direc = encoder_direction(); 		

		//incrementing/decrementing count depending on mode and direction
		//count = count + direction*2^mode
		//direction can be -1 or +1
		//mode can be 0,1,2,3 

		//	count = count + temp_0*(direc*(display_count<<x));

		//change time mode, enables encoder
		//allows user to change count ie time
		if(flag_enc == 1)
		{
			count = count + direc*display_count;
		}

		//alarm mode, emables encoder
		//allows user to set time for alarm
		if(flag_alarm == 1)
		{
			//	alarm_count = count;
			alarm_count = alarm_count + direc*display_count;
		}

		if(flag_enc == 0)
		{

			if(flag_min == 1)
			{
				count = count + 1;
				flag_min = 0;
			}

		}
		//	count = count + display_count;

		//bound the count to 0 - 1023
		if(count < 0)
		{
			count = 0x0000;
		}
		if(count > 1440)
		{
			count = 0x0000;
		}

		//bound the count to 0 - 1023
		if(alarm_count < 0)
		{
			alarm_count = 0x0000;
		}
		if(alarm_count > 1440)
		{
			alarm_count = 0x0000;
		}

		//break up the disp_value to 4, BCD digits in the array: call (segsum)
		segsum(count, alarm_count);

		//make PORTA an output
		DDRA = 0xFF;

		//NOP after changing DDRX register
		asm volatile ("nop");
		asm volatile ("nop");

		//update digit to display

		//send 7 segment code to LED segments
		PORTA = segment_select[i];

		//send PORTB the digit to display
		PORTB = digit_select[i];

		//bound a counter (0-4) to keep track of digit to display 
		i++;

		//Alarm functionality:
		//if the alarm is set, check for count equal to alarm count
		if(flag_set_alarm == 1)
		{	
			//check if clock time and alrm time match
			if(count == alarm_count)
			{
				//play alarm tone
				flag_ringer = 1;

				//test purposes
				//x ^= 0x08;

				//reset the alarm clock
				flag_set_alarm = 0;
			}
		}
	} //while(1)
} //main
