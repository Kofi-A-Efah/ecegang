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
////////////////////////////////////
#include <stdio.h>
#include <stdlib.h>
  

// XBOX LAYOUT
enum buttons{A, B, X, Y, LB, LT, RB, RT, left, right, up, down};

static struct press {
    
    
};

static struct pt pt_key, pt_anim, pt_record; ;
static int xc=10, yc=150, vxc=2, vyc=0;

static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    static int buttonPresses;
    static int buttonPressesZ;
    static int zeroPressed, onePressed,twoPressed, threePressed, fourPressed;
    while(1) {
        PT_YIELD_TIME_msec(30);
        buttonPresses = readPE(GPIOY);
        if ((!(buttonPresses & 0x01)) && !zeroPressed ) {
            mPORTAToggleBits(BIT_0);
            zeroPressed = 1;
        } else if (buttonPresses & 0x01) zeroPressed = 0;
        
        if ((!(buttonPresses & 0x04)) && !onePressed ) {
            writePE(GPIOY, (buttonPresses | 0x02));
            onePressed = 1;
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
    static int record_pressed;
    while(1) {
        PT_YIELD_TIME_msec(30);
        
        
        
    }
    
    PT_END(pt);
}


static PT_THREAD (protothread_anim(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(32);

        // erase disk
         tft_fillCircle(xc, yc, 4, ILI9340_BLACK); //x, y, radius, color
        // compute new position
         xc = xc + vxc;
         if (xc<5 || xc>235) vxc = -vxc;         
         //  draw disk
         tft_fillCircle(xc, yc, 4, ILI9340_GREEN); //x, y, radius, color
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

int main(void) {    
    // RB2 - input // when low set B1 high
    // RB0 - input // when low led on
    // RB1  - output
    
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

