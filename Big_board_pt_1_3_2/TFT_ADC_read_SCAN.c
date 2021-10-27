/*
 * File:        Read ADC
 * Author:      Bruce Land
 * 
 * Target PIC:  PIC32MX250F128B
 */

////////////////////////////////////
// clock AND protoThreads configure!
// You MUST check this file!
#include "config_1_3_2.h"
// threading library
#include "pt_cornell_1_3_2_python.h"

////////////////////////////////////
// graphics libraries
#include "tft_master.h"
#include "tft_gfx.h"
// need for rand function
#include <stdlib.h>
#include <math.h>
////////////////////////////////////


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


#define DAC_config_chan_A 0b0011000000000000
#define DAC_config_chan_B 0b1011000000000000
// string buffer
char buffer[60];
int generate_period = (40000);
int pwm_on_time;
int kp = 500; 
float ki = 0.03125; 
int kd = 1000;
int int_const; 
int der_const; 
int max_adc_value = 365; //180
int min_adc_value = 905; //0
float step_adc; 
int desired_degree = 90;

//.85 V for 90 -> 0.11 -> 40%  
//0.96 - 0.69 = 0.27
// PID 
// 285 for top 
// 895 for bottom

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_adc, pt_serial, pt_slider ;
// The following threads are necessary for UART control
static struct pt pt_input, pt_output, pt_DMA_output ;

// system 1 second interval tick
int sys_time_seconds ;

// === print utilities ==============================================
// print a line on the TFT
// string buffer
char buffer[60];
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

//DAC stuff
volatile SpiChannel spiChn = SPI_CHANNEL2 ;	// the SPI channel to use
// for 60 MHz PB clock use divide-by-3
volatile int spiClkDiv = 2 ; // 20 MHz DAC clock
//== Timer 2 interrupt handler ===========================================
// actual scaled DAC 
volatile  int DAC_data;
volatile  int adc5;
volatile  int adc11;
volatile int junk;
volatile int motor_disp;
volatile int scaled_pwm;
volatile float degree5;
volatile float err_degree; 
//volatile float prop;
volatile int time = 0.001;
volatile float rate_err;

typedef signed int fix16 ;
#define multfix16(a,b) ((fix16)(((( signed long long)(a))*(( signed long long)(b)))>>16)) //multiply two fixed 16:16
#define float2fix16(a) ((fix16)((a)*65536.0)) // 2^16
#define fix2float16(a) ((float)(a)/65536.0)
#define fix2int16(a)    ((int)((a)>>16))
#define int2fix16(a)    ((fix16)((a)<<16))
#define divfix16(a,b) ((fix16)((((signed long long)(a)<<16)/(b)))) 
#define sqrtfix16(a) (float2fix16(sqrt(fix2float16(a)))) 
#define absfix16(a) abs(a)

fix16 ADC_scale = float2fix16(3.3/1023.0); //Vref/(full scale)
float adcToDegree(int adc);
volatile float prev_err_degree;
volatile float sum_err;

void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    mT2ClearIntFlag();    // clear the timer interrupt flag
    mPORTBClearBits(BIT_4); //Set CS low
    
    adc5 = ReadADC10(0);
    AcquireADC10();
    //adc5 = adc5 + ((generate_period/2 - adc5)>>4) ;
    WriteSPI2(DAC_config_chan_A | (adc5  + (2048/2) )); 
    degree5 = adcToDegree(adc5); 
    err_degree = desired_degree - degree5;
    rate_err = (err_degree - prev_err_degree)/time; 
    sum_err = sum_err + (err_degree * time); 
    //prop = kp * err_degree;
    
    
    //SetDCOC3PWM(generate_period/2);
    while (SPI2STATbits.SPIBUSY); // Wait for end of transaction
    junk = ReadSPI2();
    // dac chan a
    
    mPORTBSetBits(BIT_4); // End Transaction
    
    pwm_on_time = (kp * err_degree) + (kd * rate_err) + (ki * sum_err);
    if(pwm_on_time <= 0) pwm_on_time = 0;
    else if (pwm_on_time >= generate_period) pwm_on_time = generate_period;
    
    scaled_pwm = (int) (4096 * ((float)pwm_on_time/(float)generate_period));
    motor_disp = motor_disp + ((scaled_pwm - motor_disp)>>4) ;
    //prev_err_degree = err_degree;
    SetDCOC3PWM(pwm_on_time);
    //pwm_on_time = prop_const * (desired_pwn - scaled_pwm);
    
//    
    
    mPORTBClearBits(BIT_4); // Set CS Low to start new transaction
    WriteSPI2(DAC_config_chan_B | (motor_disp));
    while (SPI2STATbits.SPIBUSY); // Wait for end of transaction
    junk = ReadSPI2();
    mPORTBSetBits(BIT_4); // End transaction
    
}

void printLine2(int line_number, char* print_buffer, short text_color, short back_color){
    // line number 0 to 31 
    /// !!! assumes tft_setRotation(0);
    // print_buffer is the string to print
    int v_pos;
    v_pos = line_number * 20 ;
    // erase the pixels
    tft_fillRoundRect(0, v_pos, 239, 16, 1, back_color);// x,y,w,h,radius,color
    tft_setTextColor(text_color); 
    tft_setCursor(0, v_pos);
    tft_setTextSize(2);
    tft_writeString(print_buffer);
}

// === Timer Thread =================================================
// update a 1 second tick counter
static PT_THREAD (protothread_timer(struct pt *pt))
{
    PT_BEGIN(pt);
      while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(1000) ;
        sys_time_seconds++ ;
        
        // draw sys_time
        sprintf(buffer,"%d", sys_time_seconds);
        printLine2(1, buffer, ILI9340_YELLOW, ILI9340_BLACK);
                  
        sprintf(buffer, "desired_degree=%d\n", desired_degree);       
        printLine2(2, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        
        sprintf(buffer, "kp=%d\n", kp);
        printLine2(3, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        
        sprintf(buffer, "kd=%d\n", kd);
        printLine2(4, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        
        sprintf(buffer, "ki=%f\n", ki);
        printLine2(5, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        green_text ;
        cursor_pos(3,1);
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread
int new_slider = 0;
float slider_value; 
int slider_id; 
// === ADC Thread =============================================
// 
static PT_THREAD (protothread_slider(struct pt *pt))
{
    PT_BEGIN(pt);
    // 
    while(1){
        // wait for a new slider value from Python
        PT_YIELD_UNTIL(pt, new_slider==1);
        new_slider = 0; //clear flag
        // parse frequency command
        if (slider_id == 1) {
            desired_degree = (int)slider_value;
        }
        if (slider_id == 2) {
            kp = (int)slider_value;
        }
        if (slider_id == 3){
            kd = (int)slider_value;
        }
        if (slider_id == 4){
            ki = (float)slider_value;
        }
    } // END WHILE(1)   
    PT_END(pt);  
} // thread python_string

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
            // signal parsing thread
//            new_string = 1;
            // output to thread which parses the string
            // while striping off the '$'
 //           strcpy(receive_string, PT_term_buffer+1);
 //       }            

        
        
        
        
    } // END WHILE(1)   
    PT_END(pt);  
} 

static PT_THREAD (protothread_adc(struct pt *pt))
{
    PT_BEGIN(pt);
            
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(100);
         
        
//        sprintf(PT_send_buffer,"AN11=%04d AN5=%04d ", adc_11, adc_5);
        PT_SPAWN(pt, &pt_DMA_output, PT_DMA_PutSerialBuffer(&pt_DMA_output) );
//        clr_right ;
        
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // animation thread

float adcToDegree(int adc) {
    float temp = (float)(adc - min_adc_value); 
    if (temp >= 0) {
        temp = 0; 
    }
    else {
        temp = (temp/step_adc) * 180; //575
        if (temp >= 180){
            temp = 180;
        }
    }  
    return temp;  
}

// === Main  ======================================================
void main(void) {
 //SYSTEMConfigPerformance(PBCLK);
  
  ANSELA = 0; ANSELB = 0; 

  // === config threads ==========
  // turns OFF UART support and debugger pin, unless defines are set
  PT_setup();
  motor_disp = 0;
  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();
  pwm_on_time = 0; //generate_period/2; 
  prev_err_degree = 0; 
  // the ADC ///////////////////////////////////////
    // configure and enable the ADC
    // Sample Rate of 1kHz, so number of cycles to overflow will be 40MHz / 1kHz
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_1,generate_period - 1  ); 
    ConfigIntTimer2(T2_INT_ON | T2_INT_PRIOR_2);
    mT2ClearIntFlag(); // clear interrupt flag
    
    OpenOC3(OC_ON | OC_TIMER2_SRC | OC_PWM_FAULT_PIN_DISABLE, pwm_on_time, 0 ); // Enable Output Compare module with pwm mode
    PPSOutput(4, RPB9, OC3);
    CloseADC10();	// ensure the ADC is off before setting the configuration

	// define setup parameters for OpenADC10
	// Turn module on | ouput in integer | trigger mode auto | enable autosample
    // ADC_CLK_AUTO -- Internal counter ends sampling and starts conversion (Auto convert)
    // ADC_AUTO_SAMPLING_ON -- Sampling begins immediately after last conversion completes; SAMP bit is automatically set
    // ADC_AUTO_SAMPLING_OFF -- Sampling begins with AcquireADC10();
    #define PARAM1  ADC_FORMAT_INTG16 | ADC_CLK_AUTO | ADC_AUTO_SAMPLING_OFF //

	// define setup parameters for OpenADC10
	// ADC ref external  | disable offset test | disable scan mode | do 1 sample | use single buf | alternate mode off
	#define PARAM2  ADC_VREF_AVDD_AVSS | ADC_OFFSET_CAL_DISABLE | ADC_SCAN_OFF | ADC_SAMPLES_PER_INT_1 | ADC_ALT_BUF_OFF | ADC_ALT_INPUT_OFF
        //
	// Define setup parameters for OpenADC10
    // use peripherial bus clock | set sample time | set ADC clock divider
    // ADC_CONV_CLK_Tcy2 means divide CLK_PB by 2 (max speed)
    // ADC_SAMPLE_TIME_5 seems to work with a source resistance < 1kohm
    #define PARAM3 ADC_CONV_CLK_PB | ADC_SAMPLE_TIME_5 | ADC_CONV_CLK_Tcy2 

	// define setup parameters for OpenADC10
	// set AN5 and as analog inputs
	#define PARAM4	ENABLE_AN5_ANA // 

	// define setup parameters for OpenADC10
    // DO not skip the channels you want to scan
    // do not specify channels  5 and 11
    #define PARAM5 SKIP_SCAN_ALL
	//#define PARAM5	SKIP_SCAN_AN0 | SKIP_SCAN_AN1 | SKIP_SCAN_AN2 | SKIP_SCAN_AN3 | SKIP_SCAN_AN4 | SKIP_SCAN_AN6 | SKIP_SCAN_AN7 | SKIP_SCAN_AN8 | SKIP_SCAN_AN9 | SKIP_SCAN_AN10 | SKIP_SCAN_AN12 | SKIP_SCAN_AN13 | SKIP_SCAN_AN14 | SKIP_SCAN_AN15

	// use ground as neg ref for A 
    // actual channel number is specified by the scan list
    SetChanADC10( ADC_CH0_NEG_SAMPLEA_NVREF | ADC_CH0_POS_SAMPLEA_AN5); // 
	OpenADC10( PARAM1, PARAM2, PARAM3, PARAM4, PARAM5 ); // configure ADC using the parameters defined above

	EnableADC10(); // Enable the ADC
  ///////////////////////////////////////////////////////
    
    //DAC stuff
    
    /// SPI setup //////////////////////////////////////////
    // SCK2 is pin 26 
    // SDO2 is in PPS output group 2, could be connected to RB5 which is pin 14
    PPSOutput(2, RPB5, SDO2);
    // control CS for DAC
    mPORTBSetPinsDigitalOut(BIT_4);
    mPORTBSetBits(BIT_4);
    // divide Fpb by 2, configure the I/O ports. Not using SS in this example
    // 16 bit transfer CKP=1 CKE=1
    // possibles SPI_OPEN_CKP_HIGH;   SPI_OPEN_SMP_END;  SPI_OPEN_CKE_REV
    // For any given peripherial, you will need to match these
    SpiChnOpen(spiChn, SPI_OPEN_ON | SPI_OPEN_MODE16 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV , spiClkDiv);
    
    
  // init the threads
  PT_INIT(&pt_timer);
  PT_INIT(&pt_adc);
  PT_INIT(&pt_serial);
  PT_INIT(&pt_slider);
  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);
  step_adc = (max_adc_value - min_adc_value);

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_adc(&pt_adc));
      PT_SCHEDULE(protothread_serial(&pt_serial));
      PT_SCHEDULE(protothread_slider(&pt_slider));

      
      }
  } // main

// === end  ======================================================

