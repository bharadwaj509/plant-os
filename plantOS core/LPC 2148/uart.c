/*		Platform: LPC2148 

  
	  This application code demonstrates UART0 peripheral on LPC2148.
		On reset it sends a string and transmits back the recieved character.

		Hardware Setup:-
    Connect a DB9 cable between PC and UART0 or UART1.
		COMPORT Settings
		Baudrate:-9600
		Databits:-8
		Parity:-None
		Stopbits:1
		Setup terminal software to receive data in string format

		Clock Settings:
		FOSC	>>	12MHz (onboard)
		PLL		>>	M=5, P=2
		CCLK	>>  60MHz
		PCLK	>>  15MHz */
	
#include  <lpc214x.h>		 
#define Fosc            12000000                    
#define Fcclk           (Fosc * 5)                  
#define Fcco            (Fcclk * 4)                 
#define Fpclk           (Fcclk / 4) * 1             
#define  UART_BPS	9600	 
int i=0,j=0,x=0,y=0;
unsigned char data2;
unsigned char data_recieve_UART0,data1,data2;

 



void Delay_Ticks(unsigned int Delay)  
  {  
   unsigned int i;
   for(; Delay>0; Delay--) 
   for(i=0; i<50000; i++);
   }


void Init_UART0(void)					
{  
   unsigned int Baud16;
   U0LCR = 0x83;		            // DLAB = 1
   Baud16 = (Fpclk / 16) / UART_BPS;  
   U0DLM = Baud16 >>8;							
   U0DLL = Baud16 ;						
   U0LCR = 0x03;
}	

void ART0_SendByte(unsigned char data)	   
{  
   U0THR = data;				    
   while( (U0LSR&0x40)==0 );	    
}

unsigned char UART0_RecievedByte( )	   
{    
   if((U0LSR&0x01)==1)
   	return U0RBR; 
	else
  return 0;    
}


void UART0_SendStr(const unsigned char *str)	 
{  
   while(1)
   {  
    if( *str == '\0' ) break;
    UART0_SendByte(*str++);	    
   }
}
 
void Init_UART1(void)				   
{  
   unsigned int Baud16;
   U1LCR = 0x83;		            // DLAB = 1
   Baud16 = (Fpclk / 16) / UART_BPS;  
   U1DLM = Baud16 / 256;							
   U1DLL = Baud16 % 256;						
   U1LCR = 0x03;
}	

void UART1_SendByte(unsigned char data)		  
{  
   U1THR = data;				    
   while( (U1LSR&0x40)==0 );	    
}

unsigned char UART1_RecievedByte()	   
{     
   if((U1LSR & 0x01)==1)
   	return U1RBR; 
   else
    return 0;
}

void UART1_SendStr(const unsigned char *str)		   
{  
   while(1)
   {  
      if( *str == '\0' ) break;
      UART1_SendByte(*str++);	    
   }
}


void delay(int time)
{
for (i=0;i<time;i++)
for(j=0;j<10000;j++);
}


int  main(void)
{  
   PINSEL0 = 0x00050005;		    // Enable UART0 Rx and Tx pins
   PINSEL1 = 0x00000000;
   PINSEL2 = 0x00000000;
   IODIR1=0xffffffff;
   IODIR0=0xffffffff;
   Init_UART0();
   Init_UART1();
   

   UART0_SendStr(" Hi UART0 ");
   //while((U0LSR&0x01)==0);
   UART1_SendStr(" Hi UART1");
  	 
   while(1)	
   {
     data1=UART0_RecievedByte();
		 if(data1)
		 {
           UART0_SendByte(data1);
         }
		 
     data2=	UART1_RecievedByte();
		 if(data2)
		 {
		   UART1_SendByte(data2);
		 }
	
	}
}

   




 
 
 






   
