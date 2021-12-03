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
//#include "port_expander2_brl4.h"



//volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
//volatile int spiClkDiv = 4 ; // 10 MHz max speed for prot expander!!

////////////////////////////////////
// graphics libraries
// SPI channel 1 connections to TFT
//#include "tft_master.h"
//#include "tft_gfx.h"
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
static int state_flag = 0;

//which button is pressed and what time it's pressed 
#define record_size 300
static struct { enum buttons button; int timepressed; int buttonState;} temp_press[record_size]; 
static struct { enum buttons button; int timepressed; int buttonState;} press[record_size]; 
static int temp_presscount = 0;
static int presscount = 0;
volatile int sys_time_frame = 0;
// Counter doesn't go up until this variable is 1
static int start_frame_timing = 0;
// Did the recording end on the left or the right? 0 for left, 1 for right.
static int left_right;

void rec_add_button(enum buttons button, int timepressed, int buttonState){
    temp_press[temp_presscount].button = button; 
    temp_press[temp_presscount].timepressed = timepressed;
    temp_press[temp_presscount].buttonState = buttonState;
    start_frame_timing = 1;
    temp_presscount++;
}

static struct pt pt_key, pt_anim, pt_record, pt_playback, pt_spitest, pt_spitest2;
static int xc=10, yc=150, vxc=2, vyc=0;
//time from button pressed interrupt
void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void) {
    //int junk;   
    mT2ClearIntFlag();
    if (start_frame_timing) {
        sys_time_frame++;   
    } else sys_time_frame = 0;
}
//char buffer[60];
//int sys_time_seconds;
//void printLine(int line_number, char* print_buffer, short text_color, short back_color){
//    // line number 0 to 31 
//    /// !!! assumes tft_setRotation(0);
//    // print_buffer is the string to print
//    int v_pos;
//    v_pos = line_number * 10 ;
//    // erase the pixels
//    tft_fillRoundRect(0, v_pos, 239, 8, 1, back_color);// x,y,w,h,radius,color
//    tft_setTextColor(text_color); 
//    tft_setCursor(0, v_pos);
//    tft_setTextSize(1);
//    tft_writeString(print_buffer);
//}
static PT_THREAD (protothread_key(struct pt *pt))
{
    PT_BEGIN(pt);
    //read ports are odd, write ports are even ((0(0x01), 1(0x02)) (2(0x04), 3(0x08)) etc)
    //for now 0 - A, 3 - B, etc. 
    //PORT EXPANDER 2 (BOTTOM)  // PORT EXPANDER 1 (TOP)
    // SO2 - RB2        // SI1 - RB1 
    // SI2 - RB5        // SO1 - RA1
    // CS - RB9        //  SCK - RB14
    // SCK2 - RB15      // CS - RA0
    //RA 4

    // JOYSTICK (PORTY)
    // UP  0 & 1
    // LEFT  2 & 3
    // RIGHT 4 & 5
    // DOWN  6 & 7

    //LED ON A2 OF PIC32

    //PORTZZ
    //Z0 Z1 Z2 (PLAYBACK LEFT ,REC, PLAYBACKRIGHT)

    //PORTYY
    // B Y A X
    
    // Y IS 0 & 1
    // X IS 2 & 3
    // A IS 4 & 5
    // B IS 6 & 7

    // PORTZZ
    // LT LB RT RB
    // RT 0 & 1  RED
    // RB 2 & 3 YELLOW
    // LB 4 & 5 GREEN
    // LT 6 & 7 BLUE
    static int buttonPressesY;
    static int buttonPressesZ;
    static int buttonPressesYY;
    static int buttonPressesZZ;
    static int bPressed, yPressed, aPressed, xPressed, leftPressed, rightPressed;
    static int upPressed, downPressed, ltPressed, lbPressed, rtPressed, rbPressed;
    while(1) {
        //120 fps
        PT_YIELD_TIME_msec(8);
        //read port expander 1 Y port
        buttonPressesY   = readPE(GPIOY);
        buttonPressesZ  = readPE(GPIOZ);
        buttonPressesYY = readPE2(GPIOY);
        buttonPressesZZ = readPE2(GPIOZ);
        
        //YXAB Buttons
        
        //Y button read
        if ((!(buttonPressesYY & 0x01)) && !yPressed ) {
            //mPORTAToggleBits(BIT_0);
            yPressed = 1;
            if(state_flag == 1) rec_add_button(Y, sys_time_frame, 1);
        } else if ((buttonPressesYY & 0x01) && yPressed){
            yPressed = 0;
            if(state_flag == 1) rec_add_button(Y, sys_time_frame, 0);
        }
        
        //X button read
        if ((!(buttonPressesYY & 0x04)) && !xPressed) {
            xPressed = 1;
            if(state_flag == 1) rec_add_button(X, sys_time_frame, 1);
        }else if (((buttonPressesYY & 0x04)) && xPressed) {
            xPressed = 0;
            if(state_flag == 1) rec_add_button(X, sys_time_frame, 0);
        }
        
        //A button read
        if ((!(buttonPressesYY & 0x10)) && !aPressed) {
            aPressed = 1;
            if(state_flag == 1) rec_add_button(A, sys_time_frame, 1);
        }else if (((buttonPressesYY & 0x10)) && aPressed) {
            aPressed = 0;
            if(state_flag == 1) rec_add_button(A, sys_time_frame, 0);
        }
        
        //B button read
        if ((!(buttonPressesYY & 0x40)) && !bPressed) {
            bPressed = 1;
            if(state_flag == 1) rec_add_button(B, sys_time_frame, 1);
        }else if (((buttonPressesYY & 0x40)) && bPressed) {
            bPressed = 0;
            if(state_flag == 1) rec_add_button(B, sys_time_frame, 0);
        }
        
        //Joystick
        
        //Up button read
        if ((!(buttonPressesY & 0x01)) && !upPressed ) {
            upPressed = 1;
            if(state_flag == 1) rec_add_button(up, sys_time_frame, 1);
        } else if ((buttonPressesY & 0x01) && upPressed){
            upPressed = 0;
            if(state_flag == 1) rec_add_button(up, sys_time_frame, 0);
        }
        
        // Left button read
        if ((!(buttonPressesY & 0x04)) && !leftPressed) {
            leftPressed = 1;
            if(state_flag == 1) rec_add_button(left, sys_time_frame, 1);
        }else if (((buttonPressesY & 0x04)) && leftPressed) {
            leftPressed = 0;
            if(state_flag == 1) rec_add_button(left, sys_time_frame, 0);
        }        
          
        //Right button read
        if ((!(buttonPressesY & 0x10)) && !rightPressed) {
            rightPressed = 1;
            if(state_flag == 1) rec_add_button(right, sys_time_frame, 1);
        }else if (((buttonPressesY & 0x10)) && rightPressed) {
            rightPressed = 0;
            if(state_flag == 1) rec_add_button(right, sys_time_frame, 0);
        }        
        
        // Down button read
        if ((!(buttonPressesY & 0x40)) && !downPressed) {
            downPressed = 1;
            if(state_flag == 1) rec_add_button(down, sys_time_frame, 1);
        }else if (((buttonPressesY & 0x40)) && downPressed) {
            downPressed = 0;
            if(state_flag == 1) rec_add_button(down, sys_time_frame, 0);
        }       
        
        //RT RB LT LB buttons
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                              
       //RT button read
        if ((!(buttonPressesZZ & 0x01)) && !rtPressed) {
            rtPressed = 1;
            if(state_flag == 1) rec_add_button(RT, sys_time_frame, 1);
        }else if (((buttonPressesZZ & 0x01)) && rtPressed) {
            rtPressed = 0;
            if(state_flag == 1) rec_add_button(RT, sys_time_frame, 0);
        }        
        
        //RB button read
        if ((!(buttonPressesZZ & 0x04)) && !rbPressed) {
            rbPressed = 1;
            if(state_flag == 1) rec_add_button(RB, sys_time_frame, 1);
        }else if (((buttonPressesZZ & 0x04)) && rbPressed) {
            rbPressed = 0;
            if(state_flag == 1) rec_add_button(RB, sys_time_frame, 0);
        }       
        
        //LT button read
        if ((!(buttonPressesZZ & 0x10)) && !ltPressed) {
            ltPressed = 1;
            if(state_flag == 1) rec_add_button(LT, sys_time_frame, 1);
        }else if (((buttonPressesZZ & 0x10)) && ltPressed) {
            ltPressed = 0;
            if(state_flag == 1) rec_add_button(LT, sys_time_frame, 0);
        }        
        
        //LB button read
        if ((!(buttonPressesZZ & 0x40)) && !lbPressed) {
            lbPressed = 1;
            if(state_flag == 1) rec_add_button(LB, sys_time_frame, 1);
        }else if (((buttonPressesZZ & 0x40)) && lbPressed) {
            lbPressed = 0;
            if(state_flag == 1) rec_add_button(LB, sys_time_frame, 0);
        }
    }
    
    PT_END(pt);
} // timer thread

 //WIP
// state_flag: 0 default, 1 record, 2 playback left, 3 playback right

static int recPressed, playLeftPressed, playRightPressed;
static PT_THREAD (protothread_record(struct pt *pt))
{
    PT_BEGIN(pt);
    static int record_butt;
    static int playleft;
    static int playright;
    //static int recPressed, playLeftPressed, playRightPressed; defined globally
    static int butt_press;

    while(1) {
        
        PT_YIELD_TIME_msec(8);
        butt_press = readPE(GPIOZ);
        //record button logic
        if ( !( butt_press & 0x02) && !recPressed) { // Check if button is pressed if pressed disable playback 
            recPressed = 1;
            if (state_flag == 1) {
                mPORTAClearBits(BIT_2);
                state_flag = 0;
            }  
            else {
                mPORTASetBits(BIT_2);
                state_flag = 1;
                sys_time_frame = 0;
                start_frame_timing = 0;
                playleft = 0;
                playright = 0;
                temp_presscount = 0;
            }
           
           // Turn on record light            
        } else if (((butt_press & 0x02))) recPressed = 0;   
        
  
        
//        //end recording left
        if ( state_flag == 1 && !( butt_press & 0x01 ) && !playLeftPressed ) { 
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
             mPORTAClearBits(BIT_2);
        } else if ( butt_press & 0x01 ) playLeftPressed = 0;
        //playback left
        else if ( state_flag == 0 && !( butt_press & 0x01 ) && !playLeftPressed) {
            sys_time_frame = 0;
            start_frame_timing = 1;
            if (presscount != 0) {
                state_flag = 2;
            }
            playLeftPressed = 1;
        } else if ( butt_press & 0x01 ) playLeftPressed = 0;

        // END RECORDING RIGHT 
        if ( state_flag == 1 && !( butt_press & 0x04 ) && !playRightPressed ) { 
             left_right = 1;
             int i; 
             presscount = temp_presscount;
             for (i = 0; i < temp_presscount; i++) {
                press[i].button = temp_press[i].button;
                press[i].timepressed = temp_press[i].timepressed;
                press[i].buttonState = temp_press[i].buttonState;
             }
             state_flag = 0;
             playRightPressed = 1;
             mPORTAClearBits(BIT_2);
        } else if ( butt_press & 0x04 ) playRightPressed = 0;
                //playback right        
        else if ( state_flag == 0 && !( butt_press & 0x04 ) && !playRightPressed) {
            sys_time_frame = 0;
            start_frame_timing = 1;
            if (presscount != 0) {
                state_flag = 3;
            }
            playRightPressed = 1;
        } else if ( butt_press & 0x04 ) playRightPressed = 0;
        

    }
    PT_END(pt);
}

static PT_THREAD (protothread_playback(struct pt *pt))
{
    PT_BEGIN(pt);
    static int current_press, temp_GPIOY, temp_GPIOYY, temp_GPIOZZ;
    static int prev_GPIOY, prev_GPIOYY, prev_GPIOZZ;
        while(1) {
            PT_YIELD_TIME_msec(8);
            //check to see if we are in playback mode
            if (state_flag == 2 || state_flag == 3){
                //check "current press" (next button to be pressed)
                temp_GPIOY = readPE(GPIOY);
                temp_GPIOYY = readPE2(GPIOY);
                temp_GPIOZZ = readPE2(GPIOZ);
                if(sys_time_frame == 0){
                    prev_GPIOY = temp_GPIOY;
                    prev_GPIOYY = temp_GPIOYY;
                    prev_GPIOZZ = temp_GPIOZZ;
                }
                if((prev_GPIOY != temp_GPIOY) || (prev_GPIOYY != temp_GPIOYY)
                        || (prev_GPIOZZ != temp_GPIOZZ)){
                    state_flag = 0;
                    writePE(GPIOY, 0x00);
                    writePE(GPIOZ, 0x00);
                    writePE2(GPIOY, 0x00);
                    writePE2(GPIOZ, 0x00);
                }
                while((press[current_press].timepressed <= sys_time_frame)
                        && state_flag != 0){

                    // JOYSTICK (PORTY)
                    // UP    0 & 1
                    // LEFT  2 & 3
                    // RIGHT 4 & 5
                    // DOWN  6 & 7

                    //LED ON A4 OF PIC32

                    //PORTZZ
                    //Z0 Z1 Z2 (PLAYBACK LEFT ,REC, PLAYBACKRIGHT)

                    //PORTYY
                    // B Y A X
                    // Y IS 0 & 1
                    // X IS 2 & 3
                    // A IS 4 & 5
                    // B IS 6 & 7

                    // PORTZZ
                    // LT LB RT RB
                    // RT 0 & 1
                    // RB 2 & 3
                    // LB 4 & 5
                    // LT 6 & 7
                    //check which button must be pressed
                    switch (press[current_press].button){
                        
                        //YXAB Buttons
                        
                        case Y:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOY, (temp_GPIOYY | 0x02));
                            } else writePE2(GPIOY, (temp_GPIOYY & 0xfd));
                            break;
                        case X:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOY, (temp_GPIOYY | 0x08));
                            } else writePE2(GPIOY, (temp_GPIOYY & 0xf7));
                            break;
                        case A:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOY, (temp_GPIOYY | 0x20));
                            } else writePE2(GPIOY, (temp_GPIOYY & 0xdf));
                            break;
                        case B:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOY, (temp_GPIOYY | 0x80));
                            } else writePE2(GPIOY, (temp_GPIOYY & 0x7f));
                            break;

// reverse directions depending on which playback is pressed                             
// facing left: left_right = 0
// facing right: left_right = 1                            

                        case up:
                            if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x02));
                            } else writePE(GPIOY, (temp_GPIOY & 0xfd));
                            break;       
                        case left:
                            if ( state_flag == 3 && left_right == 0  ) {
                                if(press[current_press].buttonState == 1){
                                    writePE(GPIOY, (temp_GPIOY | 0x20));
                                } else writePE(GPIOY, (temp_GPIOY & 0xdf));
                            }
                            else if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x08));
                            } else writePE(GPIOY, (temp_GPIOY & 0xf7));
                            break;
                        case right:
                            if (state_flag == 3 && left_right == 0 ) {
                                if(press[current_press].buttonState == 1){
                                    writePE(GPIOY, (temp_GPIOY | 0x08));
                                } else writePE(GPIOY, (temp_GPIOY & 0xf7));
                            }
                            else if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x20));
                            } else writePE(GPIOY, (temp_GPIOY & 0xdf));
                            break;
                        case down:
                            if(press[current_press].buttonState == 1){
                                writePE(GPIOY, (temp_GPIOY | 0x80));
                            } else writePE(GPIOY, (temp_GPIOY & 0x7f));
                            break;  
                            
                        //RT RB LT LB Buttons   
                            
                        case RT:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOZ, (temp_GPIOZZ | 0x02));
                            } else writePE2(GPIOZ, (temp_GPIOZZ & 0xfd));
                            break;
                        case RB:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOZ, (temp_GPIOZZ | 0x08));
                            } else writePE2(GPIOZ, (temp_GPIOZZ & 0xf7));
                            break;
                        case LT:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOZ, (temp_GPIOZZ | 0x20));
                            } else writePE2(GPIOZ, (temp_GPIOZZ & 0xdf));
                            break;
                        case LB:
                            if(press[current_press].buttonState == 1){
                                writePE2(GPIOZ, (temp_GPIOZZ | 0x80));
                            } else writePE2(GPIOZ, (temp_GPIOZZ & 0x7f));
                            break; 
                    
                    }
                    current_press++;
                }
                prev_GPIOY = readPE(GPIOY);
                prev_GPIOYY = readPE2(GPIOY);
                prev_GPIOZZ = readPE2(GPIOZ);
                if (current_press > presscount){
                    state_flag = 0;
                    writePE(GPIOY, 0x00);
                    writePE(GPIOZ, 0x00);
                    writePE2(GPIOY, 0x00);
                    writePE2(GPIOZ, 0x00);
                }
            } else current_press = 0;
        }
    PT_END(pt);
}

static PT_THREAD (protothread_spitest(struct pt *pt))
{
    static int ledtoggle = 0;
    static int buttonPressY, buttonPressYY;
    static int yPressed, xPressed;
    PT_BEGIN(pt);
    while(1){
        PT_YIELD_TIME_msec(8);
        buttonPressY = readPE(GPIOY);
        //buttonPressYY = readPE2(GPIOY);
        if ((!(buttonPressY & 0x01)) && !yPressed ) {
            //mPORTAToggleBits(BIT_0);
            yPressed = 1;
            writePE2(GPIOY, 0x08);
        } // bit 4
        else if ((buttonPressY & 0x01) && yPressed){
            yPressed = 0;
            writePE2(GPIOY, 0x10 );
        }
                
        if ((!(buttonPressY & 0x04)) && !xPressed) {
            xPressed = 1;
            mPORTASetBits(BIT_2);
            
        }
        else if (((buttonPressY & 0x04)) && xPressed) {
            xPressed = 0;
            mPORTAClearBits(BIT_2);
        }
        
    }
    PT_END(pt);
}

static PT_THREAD (protothread_spitest2(struct pt *pt))
{
    static int ledtoggle = 0;
    PT_BEGIN(pt);
    while(1){
        PT_YIELD_TIME_msec(100);
        if(ledtoggle){
            writePE(GPIOY, 0xff);
            writePE2(GPIOZ, 0xff);
            writePE2(GPIOY, 0xff);
            ledtoggle = 0;
        } else{
            writePE(GPIOY, 0x00);
            writePE2(GPIOZ, 0x00);
            writePE2(GPIOY, 0x00);
            ledtoggle = 1;
        }
    }
    PT_END(pt);
}

//static PT_THREAD (protothread_anim(struct pt *pt))
//{
//    PT_BEGIN(pt);
//     // timer readout
//     sprintf(buffer,"%s", "Time in sec since boot\n");
//     printLine(0, buffer, ILI9340_WHITE, ILI9340_BLACK);
//     
//     // set up LED to blink
//     mPORTAClearBits(BIT_2 );	//Clear bits to ensure light is off.
//     mPORTASetPinsDigitalOut(BIT_2 );    //Set port as output
//     mPORTAClearBits(BIT_2);
//      while(1) {
//        // yield time 1 second
//        PT_YIELD_TIME_msec(8) ;
//        sys_time_seconds++ ;
//        // toggle the LED on the big board
//  //      mPORTAToggleBits(BIT_0);
//        
//        // draw sys_time
//        sprintf(buffer,"%d", sys_time_seconds);
//        printLine(1, buffer, ILI9340_YELLOW, ILI9340_BLACK);
//        sprintf(buffer, "%d", state_flag);
//        printLine(3,buffer,ILI9340_YELLOW,ILI9340_BLACK);
//        
//          
//        // !!!! NEVER exit while !!!!
//      } // END WHILE(1)
//  PT_END(pt);
//} // animation thread

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
    // PortYY on Expander ports as digital outputs
    mPortYSetPinsOut2(BIT_1 | BIT_3 | BIT_5 | BIT_7);    //Set port as output
    // PortYY as inputs
    mPortYSetPinsIn2(BIT_0 | BIT_2 | BIT_4 | BIT_6);    //Set port as input
    // PortZZ on Expander ports as digital outputs
    mPortZSetPinsOut2(BIT_1 | BIT_3 | BIT_5 | BIT_7);    //Set port as output
    // PortZZ as inputs
    mPortZSetPinsIn2(BIT_0 | BIT_2 | BIT_4 | BIT_6);    //Set port as input
    
   // mPortYEnablePullUp(BIT_0);
//    tft_init_hw();
//    tft_begin();
//    tft_fillScreen(ILI9340_BLACK);
//    //240x320 vertical display
//    tft_setRotation(0); // Use tft_setRotation(1) for 320x240    
    mPORTAClearBits(BIT_2);	//Clear bits to ensure light is off.
    mPORTASetPinsDigitalOut(BIT_2);    //Set port as output
    writePE(GPIOY, 0x00);
    writePE2(GPIOY, 0x00);
    writePE2(GPIOZ, 0x00);

    PT_INIT(&pt_key);
//    PT_INIT(&pt_anim);
    PT_INIT(&pt_record);
    PT_INIT(&pt_playback);
    //PT_INIT(&pt_spitest);
    PT_INIT(&pt_spitest2);
    while (1) {
        PT_SCHEDULE(protothread_key(&pt_key));      
     //   PT_SCHEDULE(protothread_anim(&pt_anim));    
       PT_SCHEDULE(protothread_record(&pt_record));
       //PT_SCHEDULE(protothread_spitest(&pt_spitest));
        //PT_SCHEDULE(protothread_spitest2(&pt_spitest2));
        PT_SCHEDULE(protothread_playback(&pt_playback));        
    }
    
}

