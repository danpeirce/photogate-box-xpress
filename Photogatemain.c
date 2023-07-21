/*********************************************************************************************
Photogatemain.c Target PIC18F2620 Controls the PIC MCU as a two photogate timer.
	extensive rewrite for new project (started in 2018). 
	Copyright (C) 2018   Dan Peirce B.Sc.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.


This program is written for a PIC18F2620 chip

***********************************************************************************************/

#include <xc.h>
#include <usart.h>    // XC8 Compiler Library for USART functions 
#include <stdlib.h>   // XC8 Compiler Library for atoi() function
#include <delays.h>   // XC8 Compiler Library for delay functions 
#include <timers.h>   // XC8 Compiler Library for timer functions 
#include <capture.h>  // XC8 Compiler Library for capture functions 
#include <stdio.h>     


union four_bytes
{
    unsigned long a_long;
    struct
    {
        unsigned lower_int;
        unsigned upper_int;
    };
};

union flags
{
    unsigned char a_byte;
    struct
    {
        unsigned bit0:1;
        unsigned bit1:1;
        unsigned bit2:1;
        unsigned bit3:1;
        unsigned bit4:1;
        unsigned bit5:1;
        unsigned bit6:1;
        unsigned bit7:1;
    };
};

#pragma config WDT = OFF

#pragma config OSC = EC   // using an external clock (oscillator connected to pin 9 of PIC18F2620)

#pragma config MCLRE = OFF
#pragma config LVP = OFF
#pragma config PBADEN = OFF      // PORTB<4:0> are digital IO 
#pragma config CCP2MX = PORTBE   // switch CCP2 from RC1 to RB3

void txbuffertask(void);
void sendTime(unsigned int *listTmr);
void running(void);
void zero(void);
void singlerun(void);    // places singlerun flag on screen
void StopwatchMsg(void);
void photogateMsg(void);
void pendulumMsg(void);
void PhotogateScr(void);
void clearW2(void);
void pulseMsg(void);
void picketf1Msg(void);
void showms(void);

#define SHIFTOUT 0x0E
#define REVERT 'r'
#define TIMEBUFSIZE 20

void initialization(void);
void defaultS(void);
void stopwatchS(void);
void gateS(void);
void pulseS(void);
//void pulsekS(void); 
void pendulumS(void);
void modesS(void);
void picketfence1S(void);
void cycleTimesS(void);


unsigned int timerCountOvrF = 0; // used to count Timer1 or Timer3 overflows and thus acts as the 
                                // upper 16 bits of a 32 bit timer

long count = 0;
void (*stateMtasks)(void) = defaultS;

char buffer[124];
char outIndexBuff = 0; 
char inIndexBuff = 0;
static char code[] = { SHIFTOUT, 'w', '2', 0 };
static char code1[] = { SHIFTOUT, 'w', '1', 0 };
static char codeP[] = { SHIFTOUT, 'p',  0 };
static char codeC[] = { 0x0C, 0 };
static char codeM[] = { SHIFTOUT, '`', 16+0x20, 16+0x20, 0 }; 

union flags debounceSW;
union flags inputSW;
union flags memflags;

unsigned int listTmr[20];
unsigned int indexTmr = 0;
unsigned int cyclecount = 0;
unsigned int lastcount = 0;
unsigned long millisec = 0;
unsigned int OvrFtrigger = 262;


//*********************************************************************************
//                               main
//*********************************************************************************
void main(void)
{
    unsigned int i;
    //char gate_mode = 0;
    Delay10KTCYx(20); 
    for (i=0; i<20; i++) listTmr[i] = 0;
    initialization();
    debounceSW.a_byte = 0;
    inputSW.a_byte = 0;
    
    
    while(1)
    { 
              
        if(TXIF && (inIndexBuff > 0)) txbuffertask();
        if (PIR1bits.TMR1IF) // Timer1 clock has overflowed
        {
            PIR1bits.TMR1IF = 0; // reset Timer1 clock interrupt flag
            timerCountOvrF++;
        }
        inputSW.bit0 = PORTCbits.RC3;
        inputSW.bit1 = PORTCbits.RC4;
        stateMtasks();
    } 
}

void defaultS(void)
{
    timerCountOvrF = 0;
    listTmr[0] = 0;
    listTmr[1] = 1;
    memflags.bit0 = 0;
    clearW2();
    StopwatchMsg();
    stateMtasks = modesS;
}



void modesS(void)
{
    // Task: Cycle Display of Modes
    if ( (timerCountOvrF > 4u) && inputSW.bit1 )   
    {
        timerCountOvrF = 0;  // overflow has enough resolution
        if (listTmr[1] == 4u )
        {
            listTmr[1] = 3;
            photogateMsg();
        }
        else if (listTmr[1] == 3u )
        {
            listTmr[1] = 2;
            picketf1Msg();
        }
        else if (listTmr[1] == 2u )
        {
            listTmr[1] = 1;
            StopwatchMsg();
        }
        else if (listTmr[1] == 1u )
        {
            listTmr[1] = 0;
            pulseMsg();
        }
        else if (listTmr[1] == 0u )
        {
            listTmr[1] = 4;
            pendulumMsg();
        }
    }
    
    // Task: Select Mode
    if (inputSW.bit0)
    {
        if (listTmr[1] == 0u) 
        {
            stateMtasks = pulseS;   
            //PhotogateScr();
            PIR1bits.CCP1IF = 0; //clear flag for next event
            indexTmr = 0;
            timerCountOvrF = 0;
            listTmr[0] = 0;
            listTmr[1] = 0;
            zero();
        }    
        else if (listTmr[1] == 1u) 
        {
            if (!inputSW.bit1)
            {
                stateMtasks = stopwatchS ;
                indexTmr = 0;
                timerCountOvrF = 0;
                zero();
            }
        }
        else if (listTmr[1] == 4u) 
        {
            if (!inputSW.bit1)
            {
                stateMtasks = pendulumS ;

                PIR1bits.CCP1IF = 0; //clear flag for next event
                indexTmr = 0;
                timerCountOvrF = 0;
                listTmr[0] = 0;
                listTmr[1] = 0;
                zero();
            }
        }
        else if (listTmr[1] == 2u) 
        {
            if (!inputSW.bit1)
            {
                stateMtasks = picketfence1S ;
          
                PIR1bits.CCP1IF = 0; //clear flag for next event
                indexTmr = 0;
                timerCountOvrF = 0;
                listTmr[0] = 0;
                listTmr[1] = 0;
                zero();
            }
        }
        else if (listTmr[1] == 3u) 
        {
            if (!inputSW.bit1)
            {
                stateMtasks = gateS ;
                PIR1bits.CCP1IF = 0; //clear flag for next event
                OpenCapture1(C1_EVERY_FALL_EDGE & CAPTURE_INT_OFF);  
                PIR1bits.CCP1IF = 0; //clear flag for next event
                indexTmr = 0;
                timerCountOvrF = 0;
                listTmr[0] = 0;
                listTmr[1] = 0;
                zero();
            }
        }
    }
}

void gateS(void)
{
    if (PIR1bits.CCP1IF)
    {
        listTmr[indexTmr] = ReadCapture1();
        indexTmr++;
        listTmr[indexTmr] = timerCountOvrF;
        indexTmr++;
        if(indexTmr == 2u) 
        {
            zero();
            millisec = 262;
            OvrFtrigger = timerCountOvrF +4;
        }
        OpenCapture1(C1_EVERY_RISE_EDGE & CAPTURE_INT_OFF);
        PIR1bits.CCP1IF = 0; //clear flag for next event
    } 
    if ( (indexTmr == 2u) &&  (timerCountOvrF == OvrFtrigger))
    {
        showms();
        OvrFtrigger = OvrFtrigger + 4;
        millisec = millisec + 262;
    }
    if ((inputSW.bit0) && (!memflags.bit0) && (timerCountOvrF>2))
    {
        memflags.bit0 = 1;
        singlerun();
    }
    if (inputSW.bit1) 
    {
        stateMtasks = defaultS;
        OpenCapture1(C1_EVERY_FALL_EDGE & CAPTURE_INT_OFF);
        PIR1bits.CCP1IF = 0; //clear flag for next event
    }
    if (indexTmr == 4) 
    {    
        sendTime(listTmr);
        indexTmr = 0;
        timerCountOvrF = 0;
        OpenCapture1(C1_EVERY_FALL_EDGE & CAPTURE_INT_OFF);
        PIR1bits.CCP1IF = 0; //clear flag for next event
        
        if (memflags.bit0)
        {
            memflags.bit0 = 0;
            listTmr[0] = 0;
            listTmr[1] = 3;
            photogateMsg();
            stateMtasks = modesS;
            
        }
    }
}


void pulseS(void)
{
    if (PIR1bits.CCP1IF)
    {
        listTmr[indexTmr] = ReadCapture1();
        indexTmr++;
        listTmr[indexTmr] = timerCountOvrF;
        indexTmr++;
        PIR1bits.CCP1IF = 0; //clear flag for next event
        if(indexTmr == 2u) 
        {
            zero();
            millisec = 262;
            OvrFtrigger = timerCountOvrF +4;
        }
    } 
    if (inputSW.bit1) stateMtasks = defaultS;                      // reset
    if ((inputSW.bit0) && (!memflags.bit0) && (timerCountOvrF>2))  // set to single run
    {
        memflags.bit0 = 1;
        singlerun();
    }
    if ( (indexTmr == 2u) &&  (timerCountOvrF == OvrFtrigger))
    {
        showms();
        OvrFtrigger = OvrFtrigger + 4;
        millisec = millisec + 262;
    }        
    if (indexTmr == 4) 
    {    
        sendTime(listTmr);
        indexTmr = 0;
        timerCountOvrF = 0;
        if (memflags.bit0)
        {
            memflags.bit0 = 0;
            listTmr[0] = 0;
            listTmr[1] = 0;
            pulseMsg();
            stateMtasks = modesS;
            
        }
    }
}

void pendulumS(void)
{
    if (PIR1bits.CCP1IF)
    {
        listTmr[indexTmr] = ReadCapture1();
        indexTmr++;
        listTmr[indexTmr] = timerCountOvrF;
        indexTmr++;
        PIR1bits.CCP1IF = 0; //clear flag for next event
        if(indexTmr == 2u) 
        {
            zero();
            millisec = 262;
            OvrFtrigger = timerCountOvrF +4;
        }
    } 
    if (inputSW.bit1) stateMtasks = defaultS;
    if ((inputSW.bit0) && (!memflags.bit0) && (timerCountOvrF>2))
    {
        memflags.bit0 = 1;
        singlerun();
    }   
    if ( (indexTmr >= 2u) &&  (timerCountOvrF == OvrFtrigger))
    {
        showms();
        OvrFtrigger = OvrFtrigger + 4;
        millisec = millisec + 262;
    }
    
    if (indexTmr == 6) 
    {    
        listTmr[2] = listTmr[4]; // move relevant time point 
        listTmr[3] = listTmr[5]; // in position for sendTime
        sendTime(listTmr);
        indexTmr = 0;
        timerCountOvrF = 0;
        if (memflags.bit0)
        {
            memflags.bit0 = 0;
            listTmr[0] = 0;
            listTmr[1] = 4;
            pendulumMsg();
            stateMtasks = modesS;
            
        }
    }
}

void picketfence1S(void)
{
    if (PIR1bits.CCP1IF)
    {
        listTmr[indexTmr] = ReadCapture1();
        indexTmr++;
        listTmr[indexTmr] = timerCountOvrF;
        indexTmr++;
        PIR1bits.CCP1IF = 0; //clear flag for next 
        if(indexTmr == 2u) 
        {
            zero();
            millisec = 262;
            OvrFtrigger = timerCountOvrF +4;
        }
    } 
    if (inputSW.bit1) stateMtasks = defaultS;
      
    if ( (indexTmr >= 2u) &&  (timerCountOvrF == OvrFtrigger))
    {
        showms();
        OvrFtrigger = OvrFtrigger + 4;
        millisec = millisec + 262;
    }
    
    if (indexTmr > (TIMEBUFSIZE-3u)) // the last two positions in the 
    {                                  // buffer kept for point that 
                                       // will get overwritten
        listTmr[TIMEBUFSIZE-1u] = listTmr[3]; // save for later 
        listTmr[TIMEBUFSIZE-2u] = listTmr[2]; // recall
        sendTime(listTmr);
        indexTmr = 4;
        timerCountOvrF = 0;
        stateMtasks = cycleTimesS;
    }
}

void cycleTimesS(void)
{
    if (inputSW.bit0    ) 
    {
        stateMtasks = picketfence1S ;

        PIR1bits.CCP1IF = 0; //clear flag for next event
        indexTmr = 0;
        timerCountOvrF = 0;
        listTmr[0] = 0;
        listTmr[1] = 0;
        zero();
    }
    
    // Task: Cycle Display of Times
    if ( (timerCountOvrF > 4u) && inputSW.bit1 )  
    {
        listTmr[2] = listTmr[indexTmr];
        indexTmr++;
        listTmr[3] = listTmr[indexTmr];
        indexTmr++;
        clearW2();
        sendTime(listTmr);
        timerCountOvrF = 0;
        if (indexTmr > (TIMEBUFSIZE-1u) ) indexTmr = 4;
    }
}

void stopwatchS(void)
{
    if (!debounceSW.bit0)
    {
        if (!inputSW.bit0 && (cyclecount>2)) cyclecount--;
        if (inputSW.bit0) 
        {
            cyclecount++;
            if (cyclecount == 1)
            {
                listTmr[indexTmr] = ReadTimer1();
                indexTmr++;
                listTmr[indexTmr] = timerCountOvrF;
                indexTmr++;
                if(indexTmr == 2u) 
                {
                    zero();
                    millisec = 262;
                    OvrFtrigger = timerCountOvrF +4;
                }
            }
        }
        if (cyclecount > 10) // little or no bounce on rising edge
        {
            cyclecount = 0;
            debounceSW.bit0 = 1;
        }
    }
    else
    {
        if (inputSW.bit0 && (cyclecount>2)) cyclecount--;
        if (!inputSW.bit0) cyclecount++;
        if (cyclecount > 100) 
        {
            cyclecount = 0;
            debounceSW.bit0 =0;
        }
    }
    if (inputSW.bit1) stateMtasks = defaultS;
    if ( (indexTmr == 2u) &&  (timerCountOvrF == OvrFtrigger))
    {
        showms();
        OvrFtrigger = OvrFtrigger + 4;
        millisec = millisec + 262;
    }
    if (indexTmr == 4) 
    {    
        sendTime(listTmr);
        indexTmr = 0;
        timerCountOvrF = 0;
    }
}

void sendTime(unsigned int *listTmr)
{
    union four_bytes valone, valtwo, result;
    
    valtwo.upper_int = listTmr[3];
    valtwo.lower_int = listTmr[2];
    valone.upper_int = listTmr[1];
    valone.lower_int = listTmr[0];
    result.a_long = valtwo.a_long - valone.a_long;
    if (result.a_long < 1000000 ) inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s%lu us\n", code, result.a_long);
    else if (result.a_long < 10000000ul )
    {
        float resultfloat;
        resultfloat = result.a_long / 1000000.0;
        inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s%f s\n", code, resultfloat);
    }
    else
    {
        float resultfloat;
        resultfloat = result.a_long /1000000.0;
        inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s%.3f s\n", code, resultfloat);
    }
}

void running(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s- - -\n", code);
}

// indication that first value obtained will persist in display
void singlerun(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%sSingle Run\n", codeM);
}

void showms(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s%lu mS\n", code, millisec);
}
void zero(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s  0 mS\n", code);
}
void picketf1Msg(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s5. Picket F1 \n", code1);
}

void pulseMsg(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s2. Pulse \n", code1);
}

void StopwatchMsg(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s1. Stopwatch \n", code1);
}

void PhotogateScr(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s%s\n", codeC,codeP);
}

void photogateMsg(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s4. Gate \n", code1);
}

void pendulumMsg(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s3. Pendulum \n", code1);
}

void clearW2(void)
{
    inIndexBuff = inIndexBuff + sprintf( buffer+inIndexBuff, "%s", code);
}

void txbuffertask(void)
{
    TXREG = buffer[outIndexBuff];
    outIndexBuff++;
    if (inIndexBuff == outIndexBuff) 
    {
        inIndexBuff = 0;
        outIndexBuff= 0;
    }
}

void initialization(void)
{

    // Configure USART module

	TRISCbits.TRISC7 = 1;     // and RX (RC7) as input so far is tied high
    TRISCbits.TRISC6 = 0;     // set TX (RC6) as output  
    TRISCbits.TRISC5 = 0;     // unused pin
	TRISCbits.TRISC4 = 1;     // Mode reset switch	
	TRISCbits.TRISC3 = 1;     // Mode/start/stop switch	
	// Configure RC2/CCP1 and RB3/CCP2 as inputs
    // Photogate 1 is on RC2/CCP1/Pin 13 and 
    // Photogate 2 is on RB3/CCP2/Pin 24 
    TRISCbits.TRISC2 = 1;     // set RC2(CCP1) as input
	TRISCbits.TRISC1 = 0;     // unused pin
    TRISCbits.TRISC0 = 0;     // unused pin	

	TRISBbits.TRISB7 = 0;     // unused pin
	TRISBbits.TRISB6 = 0;     // unused pin
	TRISBbits.TRISB5 = 0;     // unused pin
	TRISBbits.TRISB4 = 0;     // unused pin
    // Configure RC2/CCP1 and RB3/CCP2 as inputs
    // Photogate 1 is on RC2/CCP1/Pin 13 and 
    // Photogate 2 is on RB3/CCP2/Pin 24     
    TRISBbits.TRISB3 = 1;     // set RB3(CCP2) as input
    TRISBbits.TRISB2 = 0;     // unused pin
	TRISBbits.TRISB1 = 0;     // unused pin 	

	TRISA = 0; // none of the pins are used
	
    OpenUSART( USART_TX_INT_OFF & USART_RX_INT_OFF & USART_ASYNCH_MODE & USART_EIGHT_BIT & 
             USART_CONT_RX & USART_BRGH_HIGH, 16 );   
          // baud rate is 2 000 000 / (SPBRG+1)
          // SPBRG = 1, baud rate is 1 000 000, 
          // SPBRG = 16, baud rate is 115 200 (good for hyperterminal debugging)

    OpenTimer1(TIMER_INT_OFF & T1_16BIT_RW & T1_SOURCE_INT & T1_PS_1_8 & T1_CCP1_T3_CCP2);
    WriteTimer1(0);  // thinking of having having timers running always
    PIR1bits.TMR1IF = 0;
    OpenCapture1(C1_EVERY_FALL_EDGE & CAPTURE_INT_OFF); 
    {
        int i = 0; 
        for (i=0; i< 35 ;i++) Delay10KTCYx(200);
    }

}	




