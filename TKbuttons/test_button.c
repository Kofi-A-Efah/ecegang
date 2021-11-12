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
//0 default, 1 record, 2 playback left, 3 playback right
static int state_flag;

//which button is pressed and what time it's pressed 
#define record_size 300
static struct { enum buttons button; int timepressed; int buttonState;} temp_press[record_size]; 
static struct { enum buttons button; int timepressed; int buttonState;} press[record_size]; 
static int temp_presscount = 0;
static int presscount = 0;
volatile int sys_time_frame = 0;
static int start_frame_timing = 0;
static int left_right;

void rec_add_button(enum buttons button, int timepressed, int buttonState){
    temp_press[temp_presscount].button = button; 
    temp_press[temp_presscount].timepressed = timepressed;
    temp_press[temp_presscount].buttonState = buttonState;
    start_frame_timing = 1;
    temp_presscount++;
}

static struct pt pt_key, pt_anim, pt_record, pt_playback; ;
static int xc=10, yc=150, vxc=2, vyc=0;
//time from button pressed interrupt
void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    //int junk;   
    mT2ClearIntFlag();
    if (start_frame_timing) {
        sys_time_frame++;   
    } else sys_time_frame = 0;
}
char buffer[60];
int sys_time_seconds;
void printLine(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 10 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(1);
    tft_writeString(print_buffer);
}
static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    //read ports are odd, write ports are even ((0(0x01), 1(0x02)) (2(0x04), 3(0x08)) etc)
    //for now 0 - A, 3 - B, etc. 
    static int buttonPresses;
    static int buttonPressesZ;
    static int bPressed, yPressed, aPressed, xPressed;
    while(1) {
        //120 fps
        PT_YIELD_TIME_msec(8);
        //read port expander 1 Y port
        buttonPresses = readPE(GPIOY);
        
        //X button read
        if ((!(buttonPresses & 0x40)) && !xPressed) {
            xPressed = 1;
            if(state_flag == 1) rec_add_button(X, sys_time_frame, 1);
        }else if (((buttonPresses & 0x40)) && xPressed) {
            xPressed = 0;
            if(state_flag == 1) rec_add_button(X, sys_time_frame, 0);
        }
        
        //B button read
        if ((!(buttonPresses & 0x01)) && !bPressed ) {
            //mPORTAToggleBits(BIT_0);
            bPressed = 1;
            if(state_flag == 1) rec_add_button(B, sys_time_frame, 1);
        } else if ((buttonPresses & 0x01) && bPressed){
            bPressed = 0;
            if(state_flag == 1) rec_add_button(B, sys_time_frame, 0);
        }
        
        //Y button read
        if ((!(buttonPresses & 0x04)) && !yPressed ) {
            //writePE(GPIOY, (buttonPresses | 0x02));
            //mPORTAToggleBits(BIT_0);
            yPressed = 1;
            if(state_flag == 1) rec_add_button(Y, sys_time_frame, 1);
        } else if ((buttonPresses & 0x04) && yPressed){
            yPressed = 0;
            if(state_flag == 1) rec_add_button(Y, sys_time_frame, 0);
        }
        
        //A button read
        if ((!(buttonPresses & 0x10)) && !aPressed) {
            aPressed = 1;
            if(state_flag == 1) rec_add_button(A, sys_time_frame, 1);
        }else if (((buttonPresses & 0x10)) && aPressed) {
            aPressed = 0;
            if(state_flag == 1) rec_add_button(A, sys_time_frame, 0);
        }
        

  
        
//        if(buttonPresses & 0x04){
//            writePE(GPIOY, (buttonPresses & 0xfd));
//        }

    }
    
    PT_END(pt);
} // timer thread

 //WIP

static PT_THREAD (protothread_record(struct pt *pt))
{
    PT_BEGIN(pt);
    static int record_butt;
    static int playleft;
    static int playright;
    static int recPressed, playLeftPressed, playRightPressed;
    static int butt_press;

    while(1) {
        PT_YIELD_TIME_msec(8);
        butt_press = readPE(GPIOZ);
        //record button logic
        if ( !( butt_press & 0x01) && !recPressed) { // Check if button is pressed if pressed disable playback 
            recPressed = 1;
            if (state_flag == 1) {
                mPORTAClearBits(BIT_0);
                state_flag == 0;
            }  
            else {
                mPORTASetBits(BIT_0);
                state_flag = 1;
                sys_time_frame = 0;
                start_frame_timing = 0;
                playleft = 0;
                playright = 0;
                temp_presscount = 0;
            }
           
           // Turn on record light            
        } else if (((butt_press & 0x01))) recPressed = 0;
        
        //end recording left
        if ( state_flag == 1 && !( butt_press & 0x02 ) && !playLeftPressed ) { 
             left_right = 0;
             int i; 
             presscount = temp_presscount;
             for (i = 0; i < temp_presscount; i++) {
                press[i].button = temp_press[i].button;
                press[i].timepressed = temp_press[i].timepressed;
                press[i].buttonState = temp_press[i].buttonState;
             }
             state_flag = 0;
             playLeftPressed = 1;
             mPORTAClearBits(BIT_0);
        } else if ( butt_press & 0x02 ) playLeftPressed = 0;

        //playback left
        if ( state_flag == 0 && !( butt_press & 0x02 ) && !playLeftPressed) {
            sys_time_frame = 0;
            start_frame_timing = 1;
            if (presscount != 0) {
                state_flag = 2;
            }
//                for (i = 0; i < presscount; i++){
//                    while (sys_time_frame < press[presscount].timepressed);
//                    if (press[presscount].button == A) {
//                        writePE(GPIOY, (0x02));
//                    }
//                    if (press[presscount].button == B) {
//                        writePE(GPIOY, (0x08));
//                    }  
//                }
            playLeftPressed = 1;
        } else if ( butt_press & 0x02 ) playLeftPressed = 0;
        
        if ( state_flag == 0 && !( butt_press & 0x03 ) ) { //playback right
            
        }
        
        
        
        
    }
    
    PT_END(pt);
}

static PT_THREAD (protothread_playback(struct pt *pt))
{
    PT_BEGIN(pt);
    static int current_press, temp_GPIOY;
        while(1) {
            PT_YIELD_TIME_msec(8);
            //check to see if we are in playback mode
            if (state_flag == 2 || state_flag == 3){
                //check "current press" (next button to be pressed)
                while(press[current_press].timepressed <= sys_time_frame){
                    temp_GPIOY = readPE(GPIOY);
                    //check which button must be pressed
                    // TODO: get multiple buttons pressing at once working
                    switch (press[current_press].button){
                        case X:
                            if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x80));
                            } else writePE(GPIOY, (temp_GPIOY & 0x7f));
                            break;
                        case Y:
                            if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x08));
                            } else writePE(GPIOY, (temp_GPIOY & 0xf7));
                            break;
                        case B:
                            if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x02));
                            } else writePE(GPIOY, (temp_GPIOY & 0xfd));
                            break;
                        case A:
                            if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x20));
                            } else writePE(GPIOY, (temp_GPIOY & 0xdf));
                            break;
                            
                    }
                    current_press++;
                }
                if (current_press > presscount){
                    state_flag = 0;
                    writePE(GPIOY, 0x00);
                }
            } else current_press = 0;
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
     mPORTAClearBits(BIT_0 );	//Clear bits to ensure light is off.
     mPORTASetPinsDigitalOut(BIT_0 );    //Set port as output
     mPORTAClearBits(BIT_0 );
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(8) ;
        sys_time_seconds++ ;
        // toggle the LED on the big board
  //      mPORTAToggleBits(BIT_0);
        
        // draw sys_time
        sprintf(buffer,"%d", sys_time_seconds);
        printLine(1, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        sprintf(buffer, "%d", state_flag);
        printLine(3,buffer,ILI9340_YELLOW,ILI9340_BLACK);
        
          
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
    mPortYSetPinsOut(BIT_1 | BIT_3 | BIT_5 | BIT_7);    //Set port as output
    // PortY as inputs
    mPortYSetPinsIn(BIT_0 | BIT_2 | BIT_4 | BIT_6);    //Set port as input
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
    PT_INIT(&pt_playback);
    while (1) {
        PT_SCHEDULE(protothread_key(&pt_key));      
        PT_SCHEDULE(protothread_anim(&pt_anim));    
        PT_SCHEDULE(protothread_record(&pt_record));
        
        PT_SCHEDULE(protothread_playback(&pt_playback));        
    }
    
}

