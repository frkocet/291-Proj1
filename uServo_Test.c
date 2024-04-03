//  EFM8_Servo.c: Uses timer 5 interrupt to generate a servo motor control signal.
//
//  Copyright (c) 2010-2018 Jesus Calvino-Fraga
//  ~C51~

#include <EFM8LB1.h>
#include <stdlib.h>
#include <stdio.h>

//volatile unsigned int servo_counter=0;
//volatile unsigned char servo1=150, servo2=150;

#define OUT0 P2_3 //MOTOR pin
#define OUT1 P1_6 //MOTOR pin
#define OUT2 P2_2 //MOTOR pin
#define OUT3 P1_7 //MOTOR pin
#define DEFAULT_F 15500L

#define COLPITTS P1_5

#define SYSCLK 72000000L // SYSCLK frequency in Hz
#define BAUDRATE 115200L
#define RELOAD_10us (0x10000L-(SYSCLK/(12L*100000L))) // 10us rate

volatile unsigned char pwm_count=0;

volatile unsigned int PWM1=100;
volatile unsigned int PWM2=100;
volatile unsigned int PWM3=100;
volatile unsigned int PWM4=100;

char _c51_external_startup (void)
{
	// Disable Watchdog with key sequence
	SFRPAGE = 0x00;
	WDTCN = 0xDE; //First key
	WDTCN = 0xAD; //Second key
  
	VDM0CN |= 0x80;
	RSTSRC = 0x02;

	#if (SYSCLK == 48000000L)	
		SFRPAGE = 0x10;
		PFE0CN  = 0x10; // SYSCLK < 50 MHz.
		SFRPAGE = 0x00;
	#elif (SYSCLK == 72000000L)
		SFRPAGE = 0x10;
		PFE0CN  = 0x20; // SYSCLK < 75 MHz.
		SFRPAGE = 0x00;
	#endif
	
	#if (SYSCLK == 12250000L)
		CLKSEL = 0x10;
		CLKSEL = 0x10;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 24500000L)
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 48000000L)	
		// Before setting clock to 48 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x07;
		CLKSEL = 0x07;
		while ((CLKSEL & 0x80) == 0);
	#elif (SYSCLK == 72000000L)
		// Before setting clock to 72 MHz, must transition to 24.5 MHz first
		CLKSEL = 0x00;
		CLKSEL = 0x00;
		while ((CLKSEL & 0x80) == 0);
		CLKSEL = 0x03;
		CLKSEL = 0x03;
		while ((CLKSEL & 0x80) == 0);
	#else
		#error SYSCLK must be either 12250000L, 24500000L, 48000000L, or 72000000L
	#endif
	
	// Configure the pins used for square output
	P2MDOUT|=0b_0000_0011;
	P0MDOUT |= 0x10; // Enable UART0 TX as push-pull output
	XBR0     = 0x01; // Enable UART0 on P0.4(TX) and P0.5(RX)                     
	XBR1     = 0X10; // Enable T0 on P0.0
	XBR2     = 0x40; // Enable crossbar and weak pull-ups

	#if (((SYSCLK/BAUDRATE)/(2L*12L))>0xFFL)
		#error Timer 0 reload value is incorrect because (SYSCLK/BAUDRATE)/(2L*12L) > 0xFF
	#endif
	// Configure Uart 0
	SCON0 = 0x10;
	CKCON0 |= 0b_0000_0000 ; // Timer 1 uses the system clock divided by 12.
	TH1 = 0x100-((SYSCLK/BAUDRATE)/(2L*12L));
	TL1 = TH1;      // Init Timer1
	TMOD &= ~0xf0;  // TMOD: timer 1 in 8-bit auto-reload
	TMOD |=  0x20;                       
	TR1 = 1; // START Timer1
	TI = 1;  // Indicate TX0 ready

	// Initialize timer 2 for periodic interrupts
	TMR2CN0=0x00;   // Stop Timer2; Clear TF2;
	CKCON0|=0b_0001_0000;
	TMR2RL=(-(SYSCLK/(2*DEFAULT_F))); // Initialize reload value
	TMR2=0xffff;   // Set to reload immediately
	ET2=1;         // Enable Timer2 interrupts
	TR2=1;         // Start Timer2
	
	EA=1;
	
	SFRPAGE=0x00;
	
	return 0;
}

void Timer2_ISR (void) interrupt 5
{
	TF2H = 0; // Clear Timer2 interrupt flag
	
	pwm_count++;
	if(pwm_count>100) pwm_count=0;
	
	OUT0=pwm_count>PWM1?0:1;
	OUT1=pwm_count>PWM2?0:1;
	OUT2=pwm_count>PWM3?0:1;
	OUT3=pwm_count>PWM4?0:1;
}

void eputs(char *String)
{	
	while(*String)
	{
		putchar(*String);
		String++;
	}
}

void PrintNumber(long int val, int Base, int digits)
{ 
	code const char HexDigit[]="0123456789ABCDEF";
	int j;
	#define NBITS 32
	xdata char buff[NBITS+1];
	buff[NBITS]=0;
	
	if(val<0)
	{
		putchar('-');
		val*=-1;
	}

	j=NBITS-1;
	while ( (val>0) | (digits>0) )
	{
		buff[j--]=HexDigit[val%Base];
		val/=Base;
		if(digits!=0) digits--;
	}
	eputs(&buff[j+1]);
}

//Drive motors from x and y coordinate inputs from serial monitor. 
//Example (assuming positive x right in direction and y is up)
// driveMotors(5, 5), would drive the car up and turn it
//PWM1 controls
// Either edit for tolerance or add tolerance to the number we input into the system
// Sweet allah defend us from the wizard inshalla
void driveMotors (float x, float y)
{
	int speedX = (2.5-x)*28;
	int speedY = (2.5-y)*24;
	if(speedX < 0){
		speedX*=-1;
	}
	if(speedY < 0){
		speedY*=-1;
	}
	if(y == 2.5 && x == 2.5){ //Neutral
		PWM3 = 0;
		PWM1 = 0;
		PWM4 = 0;
		PWM2 = 0;
	}else if(y >= 2.5 && x >= 2.5){ //Forward right
		PWM3 = 0;
		PWM1 = speedX + speedY;
		PWM4 = 0;
		PWM2 = speedY + 2;
	}else if(y >= 2.5 && x <= 2.5){ //Forward left
		PWM3 = 0;
		PWM1 = speedY;
		PWM4 = 0;
		PWM2 = speedX + speedY + 2;

	}else if(y < 2.5 && x < 2.5){ //Backward Left
		PWM3 = speedY;
		PWM1 = 0;
		PWM4 = speedX + speedY + 2;
		PWM2 = 0;
	}else if(y < 2.5 && x >= 2.5){ //Backward Right
		PWM3 = speedX + speedY;
		PWM1 = 0;
		PWM4 = speedY + 2;
		PWM2 = 0;
	}
}

void main (void)
{
	float x_1 = 2.5;
	float y_1 = 5;
	printf("\x1b[2J"); // Clear screen using ANSI escape sequence.
	printf("Square wave generator for the F38x.\r\n"
	       "Check pins P2.0 and P2.1 with the oscilloscope.\r\n");

	while(1)
	{
		driveMotors(x_1, y_1);
	}
}
