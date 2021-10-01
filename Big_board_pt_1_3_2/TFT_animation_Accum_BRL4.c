/*
 * File:        Test of compiler fixed point
 * Author:      Bruce Land
 * Adapted from:
 *              main.c by
 * Author:      Syed Tahmid Mahbub
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_3_2.h"
// threading library
#include "pt_cornell_1_3_2.h"

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
// fixed point types
#include <stdfix.h>
////////////////////////////////////
#define boid_num 125


/* Demo code for interfacing TFT (ILI9340 controller) to PIC32
 * The library has been modified from a similar Adafruit library
 */
// Adafruit data:
/***************************************************
  This is an example sketch for the Adafruit 2.2" SPI display.
  This library works with the Adafruit 2.2" TFT Breakout w/SD card
  ----> http://www.adafruit.com/products/1480

  Check out the links above for our tutorials and wiring diagrams
  These displays use SPI to communicate, 4 or 5 pins are required to
  interface (RST is optional)
  Adafruit invests time and resources providing this open source code,
  please support Adafruit and open-source hardware by purchasing
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.
  MIT license, all text above must be included in any redistribution
 ****************************************************/

// string buffer
char buffer[60];

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_color, pt_anim, pt_serial, pt_slider;

typedef struct boid {
    _Accum x;
    _Accum y;
    _Accum vx;
    _Accum vy; 
	_Accum close_dx;
	_Accum close_dy;
	_Accum xpos_avg;
	_Accum ypos_avg;
	_Accum xvel_avg;
	_Accum yvel_avg;
	_Accum neighboring_boids;
	int notReset;
    int oddEven;
}boid_t;
// Array of Boids
boid_t boid_arr[boid_num];
// Parameters
static int x;
static int y;
static _Accum vx;
static _Accum vy;
static int i, j;
_Accum tempDistance;
_Accum dx;
_Accum dy;
/*
_Accum close_dx;
_Accum close_dy;
_Accum xpos_avg;
_Accum ypos_avg;
_Accum xvel_avg;
_Accum yvel_avg;
_Accum neighboring_boids;
*/
_Accum divspeed;
_Accum speed;
_Accum turnfactor = 0.2;
_Accum visualRange = 20; // 20
_Accum SqvisualRange = 400; 
_Accum protectedRange = 4; // 2 squared
_Accum centeringfactor = 0.0005;
_Accum avoidfactor = 0.05;
_Accum matchingfactor = 0.05;
_Accum maxspeed = 3;
_Accum minspeed = 2;
int topmargin = 50;
int bottommargin = 50;
int leftmargin = 50;
int rightmargin = 50; 
const int top_screen = 240; 
const int right_screen = 320; 
int topbound; 
int rightbound; 
// system 1 second interval tick
int sys_time_seconds ;
static int end_time; 
char new_slider = 0;
int slider_id;
float slider_value ; // value could be large
char receive_string[64];

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
//     tft_setCursor(0, 0);
//     tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
//     tft_writeString("Time in seconds since boot\n");
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        
        // draw sys_time
        tft_fillRoundRect(0,15, 200, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 15);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
        tft_writeString(buffer); 
        
        tft_fillRect(0,0, 120, 14, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 0);
        tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
        tft_writeString("Frame Rate: ");
        sprintf(buffer, "%d", end_time); 
        tft_writeString(buffer);
        
        tft_fillRect(0,40, 200, 14, ILI9340_BLACK);
        tft_setCursor(0,40);
        tft_setTextColor(ILI9340_WHITE); tft_setTextSize(1);
        tft_writeString("Number of Boids: "); 
        sprintf(buffer, "%d", boid_num);
        tft_writeString(buffer);

        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === Animation Thread =============================================
// update a 1 second tick counter

#define float2Accum(a) ((_Accum)(a))
#define Accum2float(a) ((float)(a))
#define int2Accum(a) ((_Accum)(a))
#define Accum2int(a) ((int)(a))
//#define accabs(a) ((a >= 0) ? a : -a) 

_Accum accabs(_Accum a) {
    return a < 0 ? -a : a;
}

//static _Accum xc=int2Accum(10), yc=int2Accum(150), vxc=int2Accum(2), vyc=0;
//static _Accum g = float2Accum(0.1), drag = float2Accum(.01);

static PT_THREAD (protothread_anim(struct pt *pt))
{
    PT_BEGIN(pt);
    //boid_t boid_arr[boid_num];
    for ( i = 0; i < boid_num; i++ ) { 
        x = (rand() % (top_screen - topmargin - bottommargin)) + bottommargin; 
        y = (rand() % (right_screen - rightmargin - leftmargin)) + leftmargin; 
        vx = ((_Accum)(rand() & 0xffff) >> 16) + minspeed; 
        vy = ((_Accum)(rand() & 0xffff) >> 16) + minspeed; 
        boid_arr[i].x = x;
        boid_arr[i].y = y;
        boid_arr[i].vx = vx;
        boid_arr[i].vy = vy;
		boid_arr[i].close_dx = 0;
		boid_arr[i].close_dy = 0;
		boid_arr[i].xvel_avg = 0;
		boid_arr[i].yvel_avg = 0;
		boid_arr[i].xpos_avg = 0;
		boid_arr[i].ypos_avg = 0;
		boid_arr[i].notReset = 0;
        boid_arr[i].oddEven = 0;
    }
    //boid_one = {x, y, vx, vy};
    tft_setCursor(120, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    //static   boidNumString = "Number of Boids: " + boid_num;
    //tft_writeString(boidNumString);
    //sprintf(buffer, "%d", end_time); 
    
    tft_writeString(buffer);
    topbound = top_screen - topmargin;
    rightbound = right_screen - rightmargin; 
      while(1) {
        // yield time 1 second
         int begin_time = PT_GET_TIME();   
         
         //start boid loop
         
         for ( i = 0; i < boid_num; i++) {
			 //clear the boid on the TFT
            if (boid_arr[i].x < top_screen && boid_arr[i].x >= 0 && 
                boid_arr[i].y < right_screen && boid_arr[i].y >= 0){
				tft_drawPixel(boid_arr[i].x, boid_arr[i].y, ILI9340_BLACK); 
				//tft_fillCircle(boid_arr[i].x, boid_arr[i].y, 2, ILI9340_BLACK);
            }
			
			//reset all boid "factor" parameters
			if (boid_arr[i].notReset){
				boid_arr[i].close_dx = int2Accum(0);
				boid_arr[i].close_dy = int2Accum(0);
				boid_arr[i].xvel_avg = int2Accum(0);
				boid_arr[i].yvel_avg = int2Accum(0);
				boid_arr[i].xpos_avg = int2Accum(0);
				boid_arr[i].ypos_avg = int2Accum(0);
				boid_arr[i].neighboring_boids = int2Accum(0);
			}
            
            //check all other boids

//            if (boid_arr[i].x < top_screen && boid_arr[i].x >= 0 && 
//                    boid_arr[i].y < right_screen && boid_arr[i].y >= 0){

				// looping through every other boid
				//in this loop we do not recheck previous boids since their data
				//has already been used to update other boids		
                for ( j = i + 1 + boid_arr[i].oddEven; j < boid_num; j = j + 2) { 
				
				  //get distance between boid and otherboid
					dx = accabs(boid_arr[j].x - boid_arr[i].x);
					dy = accabs(boid_arr[j].y - boid_arr[i].y);
					if ( dx < visualRange && dy < visualRange ) {
						  tempDistance = dx * dx + dy * dy;
						
						
						//Avoidance Code
						if (tempDistance <= protectedRange){
							//update boid
							boid_arr[i].close_dx += boid_arr[i].x - boid_arr[j].x;
							boid_arr[i].close_dy += boid_arr[i].y - boid_arr[j].y;
							//update otherboid
							if (boid_arr[j].notReset){
								boid_arr[j].close_dx = int2Accum(0);
								boid_arr[j].close_dy = int2Accum(0);
								boid_arr[j].xvel_avg = int2Accum(0);
								boid_arr[j].yvel_avg = int2Accum(0);
								boid_arr[j].xpos_avg = int2Accum(0);
								boid_arr[j].ypos_avg = int2Accum(0);
								boid_arr[j].neighboring_boids = int2Accum(0);
								boid_arr[j].notReset = 0;
							}
							boid_arr[j].close_dx += boid_arr[j].x - boid_arr[i].x;
							boid_arr[j].close_dy += boid_arr[j].y - boid_arr[i].y;
						}
						
						  
						//alignment code
						else if (tempDistance < SqvisualRange) {
							//update boid
							boid_arr[i].neighboring_boids += 1;
							boid_arr[i].xpos_avg += boid_arr[j].x;
							boid_arr[i].ypos_avg += boid_arr[j].y;                          
							boid_arr[i].xvel_avg += boid_arr[j].vx;
							boid_arr[i].yvel_avg += boid_arr[j].vy;
							//update otherboid
							if (boid_arr[j].notReset){
								boid_arr[j].close_dx = int2Accum(0);
								boid_arr[j].close_dy = int2Accum(0);
								boid_arr[j].xvel_avg = int2Accum(0);
								boid_arr[j].yvel_avg = int2Accum(0);
								boid_arr[j].xpos_avg = int2Accum(0);
								boid_arr[j].ypos_avg = int2Accum(0);
								boid_arr[j].neighboring_boids = int2Accum(0);
								boid_arr[j].notReset = 0;
							}
							boid_arr[j].neighboring_boids += 1;
							boid_arr[j].xpos_avg += boid_arr[i].x;
							boid_arr[j].ypos_avg += boid_arr[i].y;                          
							boid_arr[j].xvel_avg += boid_arr[i].vx;
							boid_arr[j].yvel_avg += boid_arr[i].vy; 
						}
					}                     
                }
            
            //stop checking other boids

           //avoidance stuff
           boid_arr[i].vx += boid_arr[i].close_dx * avoidfactor;
           boid_arr[i].vy += boid_arr[i].close_dy * avoidfactor;
           
           if (boid_arr[i].neighboring_boids != 0) {
              boid_arr[i].neighboring_boids = 1/boid_arr[i].neighboring_boids;
           }
           
           //alignment stuff
           boid_arr[i].vx += boid_arr[i].xvel_avg * boid_arr[i].neighboring_boids * matchingfactor;
           boid_arr[i].vy += boid_arr[i].yvel_avg * boid_arr[i].neighboring_boids * matchingfactor;
           if (boid_arr[i].neighboring_boids > 0) {
              boid_arr[i].xpos_avg = boid_arr[i].xpos_avg*boid_arr[i].neighboring_boids;
              boid_arr[i].ypos_avg = boid_arr[i].ypos_avg*boid_arr[i].neighboring_boids;  
            }
            boid_arr[i].vx += (boid_arr[i].xpos_avg - boid_arr[i].x)*centeringfactor;
            boid_arr[i].vy += (boid_arr[i].ypos_avg - boid_arr[i].y)*centeringfactor;            

//end of that if statement, you know the one
//            }
            
            if (boid_arr[i].x > topbound) {
                boid_arr[i].vx -= turnfactor; 
            } 
            if (boid_arr[i].y > rightbound){
                boid_arr[i].vy -= turnfactor; 
            }
            if (boid_arr[i].y < leftmargin) {
                boid_arr[i].vy += turnfactor;
            }
            if (boid_arr[i].x < bottommargin){
                boid_arr[i].vx += turnfactor;
            }

            if (accabs(boid_arr[i].vx) > accabs(boid_arr[i].vy)) {
                speed = accabs(boid_arr[i].vx) + (accabs(boid_arr[i].vy) >> 2);
            }
            else {
                speed = accabs(boid_arr[i].vy) + (accabs(boid_arr[i].vx) >> 2);
            }
            
            if (speed != 0){
                divspeed = 1/speed;
            } 
            else {
                divspeed = 0; 
            }
            
            if (speed < minspeed) {
                boid_arr[i].vx = boid_arr[i].vx * divspeed * minspeed;
                boid_arr[i].vy = boid_arr[i].vy * divspeed * minspeed;
            }
            if (speed > maxspeed) {
                boid_arr[i].vx = boid_arr[i].vx * divspeed * maxspeed;
                boid_arr[i].vy = boid_arr[i].vy * divspeed * maxspeed;
            }           

                    
            boid_arr[i].x = (_Accum)(boid_arr[i].x + boid_arr[i].vx); 
            boid_arr[i].y = (_Accum)(boid_arr[i].y + boid_arr[i].vy); 
            if (boid_arr[i].x < top_screen && boid_arr[i].x >= 0 && 
                    boid_arr[i].y < right_screen && boid_arr[i].y >= 0){
                tft_drawPixel(boid_arr[i].x, boid_arr[i].y, ILI9340_WHITE);  
            }
            
			//boid no longer has reset values
			boid_arr[i].notReset = 1;
            
            if (boid_arr[i].oddEven) boid_arr[i].oddEven = 0;
            else boid_arr[i].oddEven = 1;
            
         }
         
         //end boid loop
         
        //PT_YIELD_TIME_msec(32);
        // erase disk
         //tft_fillCircle(Accum2int(xc), Accum2int(yc), 4, ILI9340_BLACK); //x, y, radius, color
         // compute new velocities
         //vyc = vyc + g - (vyc * drag) ;
         //vxc = vxc - (vxc * drag);      
         //xc = xc + vxc;
         //yc = yc + vyc;         
         //if (xc<int2Accum(5) || xc>int2Accum(235)) vxc = -vxc; 
         //if (yc>int2Accum(315)) vyc = -vyc;         
         //  draw disk
         //tft_fillCircle(Accum2int(xc), Accum2int(yc), 4, ILI9340_GREEN); //x, y, radius, color
         //tft_drawPixel(boid.x, boid.y, ILI9340_GREEN); 
        
        end_time = PT_GET_TIME() - begin_time; 
        PT_YIELD_TIME_msec(33 - end_time); 
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread
//sliders thread 
static PT_THREAD (protothread_sliders(struct pt *pt))
{
    PT_BEGIN(pt);
    while(1){
        PT_YIELD_UNTIL(pt, new_slider==1);
        // clear flag
        new_slider = 0; 
        if (slider_id == 1){
           protectedRange = (_Accum)(slider_value * slider_value); 
        }
        
        if (slider_id==2 ){
            visualRange = (_Accum)(slider_value);
            SqvisualRange = (_Accum) (slider_value * slider_value);
        }
        if (slider_id==3 ){
            centeringfactor = (_Accum)(1/slider_value); 
        }
        if (slider_id == 4){
            avoidfactor = (_Accum)(slider_value);
        }
        if (slider_id == 5){
            matchingfactor = (_Accum)(slider_value);
        }
    } // END WHILE(1)   
    PT_END(pt);  
} // thread slider
//serial thread 
static PT_THREAD (protothread_serial(struct pt *pt))
{
    PT_BEGIN(pt);
    static char junk;
    //   
    //
    while(1){
        // There is no YIELD in this loop because there are
        // YIELDS in the spawned threads that determine the 
        // execution rate while WAITING for machine input
        // =============================================
        // NOTE!! -- to use serial spawned functions
        // you MUST edit config_1_3_2 to
        // (1) uncomment the line -- #define use_uart_serial
        // (2) SET the baud rate to match the PC terminal
        // =============================================
        
        // now wait for machine input from python
        // Terminate on the usual <enter key>
        PT_terminate_char = '\r' ; 
        PT_terminate_count = 0 ; 
        PT_terminate_time = 0 ;
        // note that there will NO visual feedback using the following function
        PT_SPAWN(pt, &pt_input, PT_GetMachineBuffer(&pt_input) );
        
        // Parse the string from Python
        // There can be toggle switch, button, slider, and string events
        
        // toggle switch
//        if (PT_term_buffer[0]=='t'){
//            // signal the button thread
//            new_toggle = 1;
//            // subtracting '0' converts ascii to binary for 1 character
//            toggle_id = (PT_term_buffer[1] - '0')*10 + (PT_term_buffer[2] - '0');
//            toggle_value = PT_term_buffer[3] - '0';
//        }
        
        // pushbutton
//        if (PT_term_buffer[0]=='b'){
//            // signal the button thread
//            new_button = 1;
//            // subtracting '0' converts ascii to binary for 1 character
//            button_id = (PT_term_buffer[1] - '0')*10 + (PT_term_buffer[2] - '0');
//            button_value = PT_term_buffer[3] - '0';
//        }
        
        // slider
        if (PT_term_buffer[0]=='s'){
            sscanf(PT_term_buffer, "%c %d %f", &junk, &slider_id, &slider_value);
            new_slider = 1;
        }
        
        // listbox
//        if (PT_term_buffer[0]=='l'){
//            new_list = 1;
//            list_id = PT_term_buffer[2] - '0' ;
//            list_value = PT_term_buffer[3] - '0';
//            //printf("%d %d", list_id, list_value);
//        }
        
        // radio group
//        if (PT_term_buffer[0]=='r'){
//            new_radio = 1;
//            radio_group_id = PT_term_buffer[2] - '0' ;
//            radio_member_id = PT_term_buffer[3] - '0';
//            //printf("%d %d", radio_group_id, radio_member_id);
//        }
        
        // string from python input line
//        if (PT_term_buffer[0]=='$'){
//            // signal parsing thread
//            new_string = 1;
//            // output to thread which parses the string
//            // while striping off the '$'
//            strcpy(receive_string, PT_term_buffer+1);
//        }                                  
    } // END WHILE(1)   
    PT_END(pt);  
} 
// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();

  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();

  // init the threads
  PT_INIT(&pt_timer);
  //PT_INIT(&pt_color);
  PT_INIT(&pt_anim);
  PT_INIT(&pt_serial);
  PT_INIT((&pt_slider));

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240
  // seed random color
  
  srand(0);
  
  //rand() % 240, rand() % 320
  
  
  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_anim(&pt_anim));
      PT_SCHEDULE(protothread_serial(&pt_serial));
      PT_SCHEDULE(protothread_sliders(&pt_slider));
      }
  } // main

// === end  ======================================================

