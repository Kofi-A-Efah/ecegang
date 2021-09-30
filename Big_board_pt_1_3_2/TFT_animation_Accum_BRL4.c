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
#define boid_num 25


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
static struct pt pt_timer, pt_color, pt_anim;

typedef struct boid {
    _Accum x;
    _Accum y;
    _Accum vx;
    _Accum vy; 
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
_Accum close_dx;
_Accum close_dy;
_Accum xpos_avg;
_Accum ypos_avg;
_Accum xvel_avg;
_Accum yvel_avg;
_Accum divspeed;
_Accum speed;
_Accum neighboring_boids;
_Accum turnfactor = 0.69;
_Accum visualRange = 20; // 20
_Accum protectedRange = 4; // 2 squared
_Accum centeringfactor = 0.0005;
_Accum avoidfactor = 0.05;
_Accum matchingfactor = 0.01;
_Accum maxspeed = 3;
_Accum minspeed = 2;
int topmargin = 50;
int bottommargin = 50;
int leftmargin = 50;
int rightmargin = 50; 
const int top_screen = 240; 
const int right_screen = 320; 
// system 1 second interval tick
int sys_time_seconds ;
static int end_time; 

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
        tft_fillRoundRect(0,15, 100, 14, 1, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 15);
        tft_setTextColor(ILI9340_YELLOW); tft_setTextSize(2);
        sprintf(buffer,"%d", sys_time_seconds);
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

_Accum accabs(_Accum a) {
    return a < 0 ? int2Accum(-1)*a : a;
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
    }
    //boid_one = {x, y, vx, vy};
    tft_setCursor(120, 0);
    tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
    //static   boidNumString = "Number of Boids: " + boid_num;
    //tft_writeString(boidNumString);
    //sprintf(buffer, "%d", end_time); 
    
    tft_writeString(buffer);
      while(1) {
        // yield time 1 second
         int begin_time = PT_GET_TIME();   
         
         //start boid loop
         
         for ( i = 0; i < boid_num; i++) {
            if (boid_arr[i].x < top_screen && boid_arr[i].x >= 0 && 
                boid_arr[i].y < right_screen && boid_arr[i].y >= 0)
              tft_fillCircle(boid_arr[i].x, boid_arr[i].y, 2, ILI9340_BLACK);
            close_dx = int2Accum(0);
            close_dy = int2Accum(0);
            xvel_avg = int2Accum(0);
            yvel_avg = int2Accum(0);
            xpos_avg = int2Accum(0);
            ypos_avg = int2Accum(0);
            neighboring_boids = int2Accum(0);
            tempDistance = 0;
            
            //check all other boids
            if (boid_arr[i].x < top_screen && boid_arr[i].x >= 0 && 
                    boid_arr[i].y < right_screen && boid_arr[i].y >= 0){
                for ( j = 0; j < boid_num; j++ ) { // looping through every other boid
                    //if the boid is not itself
                  if ( i != j ) {
                      //get distance between boid and otherboid
                        dx = boid_arr[j].x - boid_arr[i].x;
                        dy = boid_arr[j].y - boid_arr[i].y;
                        if ( dx < visualRange && dy < visualRange ) {
                          tempDistance = dx * dx + dy * dy;
                        
                        
                        //Avoidance Code
                        if (tempDistance <= protectedRange*protectedRange){
                            close_dx += boid_arr[i].x - boid_arr[j].x;
                            close_dy += boid_arr[i].y - boid_arr[j].y;
                        }
                        
                          
                        //alignment code
                        else if (tempDistance < visualRange*visualRange) {
                          neighboring_boids += 1;
                          xpos_avg += boid_arr[j].x;
                          ypos_avg += boid_arr[j].y;                          
                          xvel_avg += boid_arr[j].vx;
                          yvel_avg += boid_arr[j].vy;   
                        }
                            
                        }      
                      
                    }              
                }
            }
            
            //stop checking other boids

           //avoidance stuff
           boid_arr[i].vx += close_dx * avoidfactor;
           boid_arr[i].vy += close_dy * avoidfactor;
           
           if (neighboring_boids != 0) {
              neighboring_boids = 1/neighboring_boids;
           }
           
           //alignment stuff
           boid_arr[i].vx += xvel_avg * neighboring_boids * matchingfactor;
           boid_arr[i].vy += yvel_avg * neighboring_boids * matchingfactor;
           if (neighboring_boids > 0) {
              xpos_avg = xpos_avg*neighboring_boids;
              ypos_avg = ypos_avg*neighboring_boids;  
            }
            boid_arr[i].vx += (xpos_avg - boid_arr[i].x)*centeringfactor;
            boid_arr[i].vy += (ypos_avg - boid_arr[i].y)*centeringfactor;
            
            
            if (boid_arr[i].x > top_screen - topmargin) {
                boid_arr[i].vx -= turnfactor; 
            } 
            if (boid_arr[i].y > right_screen - rightmargin){
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
                    boid_arr[i].y < right_screen && boid_arr[i].y >= 0)
            tft_drawPixel(boid_arr[i].x, boid_arr[i].y, ILI9340_GREEN);             
             
             
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

        tft_fillRect(0,0, 120, 14, ILI9340_BLACK);// x,y,w,h,radius,color
        tft_setCursor(0, 0);
        tft_setTextColor(ILI9340_WHITE);  tft_setTextSize(1);
        tft_writeString("Frame Rate: ");
        sprintf(buffer, "%d", end_time); 
        tft_writeString(buffer);
        PT_YIELD_TIME_msec(33); 
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

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
  PT_INIT(&pt_color);
  PT_INIT(&pt_anim);

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
      }
  } // main

// === end  ======================================================

