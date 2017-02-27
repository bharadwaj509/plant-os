/*
	Platform: LPC2148 .
		

    reading ADC data on LCD 

	Hardware Setup:-
    Insert jumper8 ,
    P0.28	and P0.29 to analog voltage 	
	LCD data pin :-P0.16-P0.23 
	RS-P1.28
	RW-GND
	EN-P1.29	
*/ 



#include  <lpc214x.h>  
#include <stdlib.h>
#include <stdio.h>
#include <stdio.h>                       
#include <ctype.h>                       
#include <math.h>   
#define Fosc            12000000                    
#define Fcclk           (Fosc * 5)                 
#define Fcco            (Fcclk * 4)                 
#define Fpclk           (Fcclk / 4) * 1    
  
#define DATA_PORT() IO0SET=(1<<16)		 //  select data port on LCD
#define READ_DATA() IO0SET=(1<<17)		 //  select read operation on LCD
#define EN_HI() IO0SET=(1<<18)			 //  Enable LCD
#define  UART_BPS	9600	 			 //Set Baud Rate 

#define COMMAND_PORT() IO0CLR=(1<<16)	 //  select command port on LCD
#define WRITE_DATA() IO0CLR=(1<<17)		 //  select write operation on LCD
#define EN_LOW() IO0CLR=(1<<18)			 //  disable LCD
  
  
void delay(int time);
void DECtoASCII(unsigned int Data);
void Delay(unsigned char Ticks);
void Init_UART0(void);
void UART0_SendByte(unsigned char data);
void ADC_Init(void);
void UART0_SendByte(unsigned char data);
void lcd_float(float f);
void lcd_int(unsigned int n);  
  
  
  
int final_recieved_data=0;        
int mean_sensor= 0;
int count=0;
int first_digit=0;
int second_digit=0;
int third_digit=0;
int first_2_digit=0;
int second_2_digit=0;
int third_2_digit=0;


unsigned int ADC_Data1=0;
unsigned int ADC_Data2=0;
unsigned int ADC_Data0=0;
unsigned char Temp[4];

int final_data_1st_sensor=0;
int final_data_2nd_sensor=0;
int j[5];
int k [5];
int min=128;
int max=128;	
int int_t = 0, int_c = 0;			//store integer values of ADC
float flt_t, flt_c;			        //store float values of ADC
double Data;
int Maximum=128,Minimum=128;
long int Vmaxi,Vmini,temp,temp1;
long float  Vmaxf,Vminf,Vmaxfa;
int iii = 0,ii=0;
int i3=0,j3=0;

void delay(int time)
{
 for (i3=0;i3<time;i3++)
 for(j3=0;j3<1000;j3++);
}

void Delay(unsigned char Ticks)	  //  generate small delay
{  
 unsigned int i=0;
 if(Ticks==0)
 {
  Ticks=1;
 }

 for(;Ticks>0;Ticks--)
 for(i=0; i<60000; i++);
}
void wrcommand(unsigned char command)
{
	IOCLR1=1<<28;
	IOPIN0=(command<<16);
	IOSET1=1<<29;
	delay(2);
	IOCLR1=1<<29;
}

void wrdata(unsigned char data)
{
	IOSET1=1<<28;
	IOPIN0=data<<16;
	IOSET1=1<<29;
	delay(2);
	IOCLR1=1<<29;
}

void LCD_initialization()
{
	wrcommand(0x38);
	wrcommand(0x80);
	wrcommand(0x01);
	wrcommand(0x06);
	wrcommand(0x0C);
}


void LCD_String(unsigned char *string)
{
	while(*string)
	wrdata(*string++);
}

void ADC_Init()			// init ADC peripheral
{
						// SEL = 1 	ADC0 channel 1	Channel 1
						// CLKDIV = Fpclk / 1000000 - 1 ;1MHz
	 AD0CR=0x00200E00;	// BURST = 0   // CLKS = 0  // PDN = 1 
 						// START = 1   // EDGE = 0 (CAP/MAT
} 

  
unsigned char ADC_Conversion(unsigned char Channel)
{
	unsigned int Temp=0;
	AD0CR = (AD0CR&0xFFFFFF00) | Channel;			   //Select AD0.1 for conversion
	AD0CR|=(1 << 24);							   //Trigger conversion
	while((AD0DR1&0x80000000)==0);			   //Wait for the conversion to be completed
	Temp = AD0DR1;						   //Store converted data
	Temp = (Temp>>8) & 0x00FF;
	return(Temp);
}   

                          
int main(void)
{ 
	PINSEL0 = 0x00000000;		    
	PINSEL1 = 0x05000000;		// Enable AD0.1 and AD0.2
	PINSEL2=  0x00000000;
	IO0DIR=0xffffffff;
	IO1DIR=0xffffffff;
	ADC_Init();
	LCD_initialization();


	LCD_String("ADC1=");	
	wrcommand(0xc0);
	LCD_String("ADC2=");	
	while(1)	
	{ 
		AD0CR = (AD0CR&0xFFFFFF00)|0x02;		   //Select AD0.1 for conversion
		AD0CR|=(1 << 24);						   //Trigger conversion
		while((AD0DR1&0x80000000)==0);			   //Wait for the conversion to be completed
		ADC_Data1 = AD0DR1;						   //Store converted data
		ADC_Data1 = (ADC_Data1>>8) & 0x00FF;
		Delay(10);

		AD0CR = (AD0CR & 0xFFFFFF00) | 0x04 ;	   //Select AD0.2 for conversion
		AD0CR|=(1 << 24);						   //Trigger conversion
		while((AD0DR2&0x80000000)==0);			   //Wait for the conversion to be completed
		ADC_Data2 = AD0DR2;						   //Store converted data
		ADC_Data2 = (ADC_Data2>>8) & 0x00FF;
		wrcommand(0x80+5);
		wrdata(ADC_Data1/100+48);
		wrdata((ADC_Data1/10)%10+48);
		wrdata(ADC_Data1%10+48);
	 
		wrcommand(0xc0+5);
		wrdata(ADC_Data2/100+48);
		wrdata((ADC_Data2/10)%10+48);
		wrdata(ADC_Data2%10+48);
	
	}
}
	