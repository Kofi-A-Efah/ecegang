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
//#include "port_expander_brl4.h"

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

/*
  
 */
static struct pt pt_key; ;

static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    while(1) {
        PT_YIELD_TIME_msec(1000);
        
        if (mPORTBReadBits(BIT_0) == 0 ) {
            mPORTAToggleBits(BIT_0);       
        }
        else if ( mPORTBReadBits(BIT_2) == 0) {
            mPORTBSetBits(BIT_1);       
        }  
        
    }
    
    PT_END(pt);
} // timer thread

int main(void) {    
    // RB2 - input // when low set B1 high
    // RB0 - input // when low led on
    // RB1  - output
    mPORTBSetPinsDigitalOut(BIT_1); // Set RB1 to output
    mPORTBSetPinsDigitalIn(BIT_0 | BIT_2); // Set RB0 and RB2  to input
    mPORTASetBits(BIT_0);	//Clear bits to ensure light is off.
    mPORTASetPinsDigitalOut(BIT_0);    //Set port as output
    PT_setup();
    PT_INIT(&pt_key);
    
    while (1) {
        PT_SCHEDULE(protothread_key(&pt_key));        
    }
    
}

