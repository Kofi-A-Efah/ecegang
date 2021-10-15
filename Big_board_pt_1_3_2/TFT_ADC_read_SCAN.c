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

// === thread structures ============================================
// thread control structs
// note that UART input and output are threads
static struct pt pt_timer, pt_adc ;
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


void __ISR(_TIMER_2_VECTOR, ipl2) Timer2Handler(void)
{
    mT2ClearIntFlag();    // clear the timer interrupt flag
    mPORTBClearBits(BIT_4); //Set CS low
    
    adc5 = ReadADC10(0);
    AcquireADC10();
    //adc5 = adc5 + ((generate_period/2 - adc5)>>4) ;
    WriteSPI2(DAC_config_chan_A | (adc5  + (2048/2) )); 
    //SetDCOC3PWM(generate_period/2);
    while (SPI2STATbits.SPIBUSY); // Wait for end of transaction
    junk = ReadSPI2();
    // dac chan a
    
    mPORTBSetBits(BIT_4); // End Transaction
    
    scaled_pwm = (int) (4096 * ((float)pwm_on_time/(float)generate_period));
    motor_disp = motor_disp + ((scaled_pwm - motor_disp)>>4) ;
//    SetDCOC3PWM(pwm_on_time);
    
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
        // NEVER exit while
      } // END WHILE(1)
  PT_END(pt);
} // timer thread

// === ADC Thread =============================================
// 


static PT_THREAD (protothread_adc(struct pt *pt))
{
    PT_BEGIN(pt);
            
    while(1) {
        // yield time 1 second
        PT_YIELD_TIME_msec(100);
         
        
        // print raw ADC, floating voltage, fixed voltage
        
        sprintf(buffer, "Motordisp=%04d\n", motor_disp);
        
        printLine2(5, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        sprintf(buffer, "scaled_pwm=%d\n", scaled_pwm);
        printLine2(5, buffer, ILI9340_YELLOW, ILI9340_BLACK);
        green_text ;
        cursor_pos(3,1);
//        sprintf(PT_send_buffer,"AN11=%04d AN5=%04d ", adc_11, adc_5);
        PT_SPAWN(pt, &pt_DMA_output, PT_DMA_PutSerialBuffer(&pt_DMA_output) );
//        clr_right ;
        
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
  motor_disp = 0;
  // === setup system wide interrupts  ========
  INTEnableSystemMultiVectoredInt();
  pwm_on_time = generate_period/2;    // the ADC ///////////////////////////////////////
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

  // init the display
  tft_init_hw();
  tft_begin();
  tft_fillScreen(ILI9340_BLACK);
  //240x320 vertical display
  tft_setRotation(0); // Use tft_setRotation(1) for 320x240

  // seed random color
  srand(1);

  // round-robin scheduler for threads
  while (1){
      PT_SCHEDULE(protothread_timer(&pt_timer));
      PT_SCHEDULE(protothread_adc(&pt_adc));
      }
  } // main

// === end  ======================================================

