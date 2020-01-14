// Encoder Test

// Every half second, a new led is lit on the bargraph display via SPI
// !!Disconnect existing PORTA and PORTB connections!!
//
// Expected Connections:
// Encloder            mega128
// --------------       --------------------     
//     SHIFT_LD_N       PORTE bit 6                        
//     SCK              PORTB bit 1 (sclk)
//     CLK_INH          PORTE bit 7
//     SER_IN           no connect
//     gnd              ground (gnd on any port)
//     vdd              vcc    (vcc on any port)
//     sd_out           PORTB bit 3

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#define F_CPU 16000000
//***********************************************************************
//                            spi_init                               
//**********************************************************************
//Lab2: Seven seg stuff:

//Array to hold different digits to turn on
//                  Digit: 0     1     2     3
uint8_t digit_select[] = {0x00, 0x10, 0x30, 0x40};
//Array to hold different state of the digits

uint8_t segment_select[4]; //digit 4

//decimal to 7-segment LED display encodings, logic "0" turns on segment
                         //0          1           2           3           4           5           6           7           8           9
uint8_t dec_to_7seg[10]= {0b00000011, 0b10011111, 0b00100101, 0b00001101, 0b10011001, 0b01001001, 0b01000001, 0b00011111, 0b00000001, 0b00001001};


void segsum(int16_t sum) {
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

int8_t chk_buttons(uint8_t button)
{
        static uint16_t state[8] = {0}; //holds present state
        state[button] = (state[button] << 1) | (! bit_is_clear(PINA, button)) | 0xE000;
        if (state[button] == 0xF000) return 1;
        return 0;
}


//GLOBAL VARS:
uint8_t data = 0x00; //hold data
uint8_t a_past = 0x03; // a of encoder to be high in past
uint8_t b_past = 0x03; // b of encoder to be high in past
uint8_t a_cur = 0x00; // current state of a
uint8_t b_cur = 0x00; // current state of b
int8_t direc = 0x00; //will hold the sign

uint8_t x = 0; //mode to be selected


void spi_init(void){


    DDRB  = 0xF7; //output mode for MOSI, SCLK, MISO set to input

    asm volatile ("nop");
    asm volatile ("nop");

    SPCR  = (1<<SPE)|(1<<MSTR); //Enable SPI,master mode

    SPSR  |= 0x01; //choose double speed operation
 }//spi_init

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
	data = spi_read(); //read data from encoder

	uint8_t button; //var to hold button
	
	uint8_t temp_pb;

	temp_pb = PORTB;

	//Enable Tristate
	PORTB |= 0X70;
	

	asm volatile ("nop");
	asm volatile ("nop");
	
	for(button = 0; button < 4; button++)
	{
		if(chk_buttons(button))
		{
			x = button;

			_delay_ms(2);
		}
	}

	//Disable Tristate, restore previous state
	PORTB = temp_pb;

	asm volatile ("nop");
	asm volatile ("nop");

	spi_write();

}


//Encoder Data Direction:
int8_t encoder_direction()
{	
//	_delay_ms(2);
	int8_t var = 0; //will hold the sign
//	uint8_t cw = 0; // clock wise
//	uint8_t ccw = 0; // anti clock wise


	//break the data
	a_cur = data & 0x01; //extract out a
	b_cur = (data>>1) & 0x01; 

	//check for different states to get the direction
	//if past and present states were same no need to increment count
	//var = 0 => no change, var = 1 => increment, car = -1 => decrement
	if((a_cur == a_past) && (b_cur == b_past))
	{
		var = 0;
	}
	
	if(a_past == a_cur)
	{
	//	if((a_cur == 1) && (b_past < b_cur)){ var = 1;} //CW
	//	if((a_cur == 1) && (b_past > b_cur)){ var = -1;} //CCW
		if((a_cur == 0) && (b_past > b_cur)){ var = 1;} //CW
//		if((a_cur == 0) && (b_past < b_cur)){ var = -1;} //CCW
	}
//	if((a_past < a_cur)&&(b_past | b_cur) == 0){ var = 1;} //cw 
	if((a_past < a_cur)&&(b_past | b_cur) == 1){ var = -1;} //ccw 
	//if((a_past > a_cur)&&(b_past | b_cur) == 1){ var = 1;} //cw 
	//if((a_past > a_cur)&&(b_past | b_cur) == 0){ var = -1;} //ccw 

	a_past = a_cur;
	b_past = b_cur;

	//set flag to 1
//	flag++;

	return var;
}

/*
int8_t encoder_direction()
{
	//extract encoder data
	data_enc = data & data 
}*/

//**********************************************************************
//                                main                                 
//**********************************************************************
int main(){     

//setting interrupt:
ASSR |= (0<<AS0); //run off external 32khz osc (TOSC)
sei(); //intialize interrupt

//Timer Counter setup running of i/o clock
TIMSK |= (1<<TOIE0); //Enable interrupt
TCCR0 = (1<<CS02) | (1<<CS00); //Normal mode, Prescale 128


DDRE = 0XFF; //output mode PORTE

asm volatile ("nop");
asm volatile ("nop");

uint8_t display_count = 1; 		//holds count for display 

uint8_t i = 0; 				//dummy counter

uint8_t temp; 				

int16_t count = 0x0000;

uint8_t j = 0;


spi_init();  	//Intalized PRTB = F7	//initalize SPI port
while(1){                             	//main while loop
 	
//reset iterator
    	if(i == 4)
    	{
      		i = 0;
    	}

//insert loop delay for debounce
        _delay_ms(2);

//make PORTA an input port with pullups
        DDRA = 0x00;
	
//NOP after changing DDRX register
        asm volatile ("nop");
        asm volatile ("nop");

        PORTA = 0xFF;

 //enable tristate buffer for pushbutton switch
  //      PORTB |= 0x70;

//read spi and get count
    	direc = encoder_direction(); 		

//	if(flag > 3)
//	{
		count = count + (direc*(display_count<<x));

		//reset flag
//		flag = 0;
//	}

//	count = count - 3;

       //disable tristate buffer for pushbutton switches
//      PORTB = 0x00;
//        PORTB |= 0x50;


        //bound the count to 0 - 1023

    	if(count < 0)
    	{
	   	 count = 0x03FF;
    	}
    	if(count > 1023)
    	{
	    	count = 0x0001;
    	}

        //break up the disp_value to 4, BCD digits in the array: call (segsum)
        segsum(count);


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


    /*
    if(temp != 0)
    {	    

    	SPDR = display_count;		//send display_count to the display 

    	while (bit_is_clear(SPSR, 7)) {} 	//spin till SPI data has been sent 

    	PORTB |= 0x01;	            	//send rising edge to regclk on HC595 

    	PORTB &= 0xFE;	           	//send falling edge to regclk on HC595

    	display_count = display_count << 1;			//shift display_count for next time 

    	if(display_count > 127) {display_count = 1;} //put indicator back to 1st positonin

	temp = 0; //reset temp
   }*/

//    for(i=0; i<=4; i++){_delay_ms(100);}         //0.5 sec delay
  } //while(1)
} //main
