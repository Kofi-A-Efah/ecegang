/* 
 * File:   test_button.c
 * Author: Lab User
 *
 * Created on November 3, 2021, 9:14 PM
 */
////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_3_2.h"
// threading library
#include "pt_cornell_1_3_2_python.h"
// yup, the expander
#include "port_expander_brl4.h"

//volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
//volatile int spiClkDiv = 4 ; // 10 MHz max speed for prot expander!!

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// need for sine function
#include <math.h>
// The fixed point types
#include <stdfix.h>
// need for knowing the system time 
#include <time.h> 
////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
  

// XBOX LAYOUT
enum buttons{A, B, X, Y, LB, LT, RB, RT, left, right, up, down};
int record[20];
static int recflg;

//which button is pressed and what time it's pressed 
static struct { enum buttons button; int timepressed;} press[50]; 
static int presscount = 0;
volatile int sys_time_frame;

static struct pt pt_key, pt_anim, pt_record; ;
static int xc=10, yc=150, vxc=2, vyc=0;
//time from button pressed interrupt
void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    //int junk;   
    mT2ClearIntFlag();
    sys_time_frame++;   

}

static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    //read ports are odd, write ports are even ((0(0x01), 1(0x02)) (3(0x04), 4(0x08)) etc)
    //for now 0 - A, 3 - B, etc. 
    static int buttonPresses;
    static int buttonPressesZ;
    static int zeroPressed, onePressed,twoPressed, threePressed, fourPressed;
    while(1) {
        //120 fps
        PT_YIELD_TIME_msec(8);
        buttonPresses = readPE(GPIOY);
        if ((!(buttonPresses & 0x01)) && !zeroPressed ) {
            mPORTAToggleBits(BIT_0);
            zeroPressed = 1;
            if (recflg == 1){
                press[presscount].button = A; 
                press[presscount].timepressed = sys_time_frame;
                presscount++;
            }    
        } else if (buttonPresses & 0x01) zeroPressed = 0;
        
        if ((!(buttonPresses & 0x04)) && !onePressed ) {
            //writePE(GPIOY, (buttonPresses | 0x02));
            mPORTAToggleBits(BIT_0);
            onePressed = 1;
            if (recflg == 1){
                press[presscount].button = B; 
                press[presscount].timepressed = sys_time_frame;
                presscount++;
            }
        } else if (buttonPresses & 0x04) onePressed = 0;
        
        if(buttonPresses & 0x04){
            writePE(GPIOY, (buttonPresses & 0xfd));
        }

    }
    
    PT_END(pt);
} // timer thread

 //WIP

static PT_THREAD (protothread_record(struct pt *pt))
{
    PT_BEGIN(pt);
    static int record_but;
    static int playleft;
    static int playright;
    while(1) {
        PT_YIELD_TIME_msec(8);
        
        if ( !( readPE(GPIOZ) & 0x01) ) { // Check if button is pressed if pressed disable playback 
           recflg = 1;
           sys_time_frame = 0;
           playleft = 0;
           playright = 0;
           presscount = 0;
           // Turn on record light            
        }
        
        else if ( recflg == 1) {
            if (!(readPE (GPIOZ) & 0x01)){
                recflg == 0;
            }
        }

        else if ( recflg == 0 && !( readPE(GPIOZ) & 0x02 ) ) { //playback left 
            sys_time_frame = 0;
            int i;
            for (i = 0; i < presscount; i++){
                while (sys_time_frame < press[presscount].timepressed);
                if (press[presscount].button == A) {
                    writePE(GPIOY, (0x02));
                }
                if (press[presscount].button == B) {
                    writePE(GPIOY, (0x08));
                }  
            }
        }
        
        else if ( recflg == 0 && !( readPE(GPIOZ) & 0x03 ) ) { //playback right
            
        }
        
        
        
        
    }
    
    PT_END(pt);
}



static PT_THREAD (protothread_anim(struct pt *pt))
{
    PT_BEGIN(pt);
     // timer readout
     sprintf(buffer,"%s", "Time in sec since boot\n");
     printLine(0, buffer, ILI9340_WHITE, ILI9340_BLACK);
     
     // set up LED to blink
     mPORTASetBits(BIT_0 );	//Clear bits to ensure light is off.
     mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
     
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        // toggle the LED on the big board
        mPORTAToggleBits(BIT_0);
        
        // draw sys_time
        sprintf(buffer,"%d", sys_time_seconds);
        printLine(1, buffer, ILI9340_YELLOW, ILI9340_BLACK);
          
        // !!!! NEVER exit while !!!!
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

int main(void) {    
    // RB2 - input // when low set B1 high
    // RB0 - input // when low led on
    // RB1  - output
    //40 MHz clock/120 fps = 666667 -> 666666 due to interrupt starting at 0
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1, 333332);
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag();
    
    PT_setup();
    initPE();
    INTEnableSystemMultiVectoredInt();
    // PortY on Expander ports as digital outputs
    mPortYSetPinsOut(BIT_1);    //Set port as output
    // PortY as inputs
    mPortYSetPinsIn(BIT_0);    //Set port as input
    mPortZSetPinsIn(BIT_0 | BIT_1 | BIT_2);    //Set port as input
    
   // mPortYEnablePullUp(BIT_0);
    tft_init_hw();
    tft_begin();
    tft_fillScreen(ILI9340_BLACK);
    //240x320 vertical display
    tft_setRotation(0); // Use tft_setRotation(1) for 320x240    
    mPORTAClearBits(BIT_0);	//Clear bits to ensure light is off.
    mPORTASetPinsDigitalOut(BIT_0);    //Set port as output

    PT_INIT(&pt_key);
    PT_INIT(&pt_anim);
    PT_INIT(&pt_record);
    while (1) {
        PT_SCHEDULE(protothread_key(&pt_key));      
        PT_SCHEDULE(protothread_anim(&pt_anim));    
        PT_SCHEDULE(protothread_record(&pt_record));        
    }
    
}

