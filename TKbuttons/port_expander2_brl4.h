/* 
 * File:   port_expander2.h
 * Author: Sean edited by kofi
 *
 * Created on October 3, 2016, 10:28 AM
 */

#ifndef PORT_EXPANDER2_H
#define	PORT_EXPANDER2_H
/* Library for interacting with MCP23S17 port expander */
#include "plib.h"

#define PE_OPCODE_HEADER 0b01000000
#define READ 0b00000001
#define WRITE 0b00000000

// IOCON Settings
#define SET_BANK     0x80
#define CLEAR_BANK   0x00
#define SET_MIRROR   0x40
#define CLEAR_MIRROR 0x00
#define SET_SEQOP    0x20
#define CLEAR_SEQOP  0x00
#define SET_DISSLW   0x10
#define CLEAR_DISSLW 0x00
#define SET_HAEN     0x08
#define CLEAR_HAEN   0x00
#define SET_ODR      0x04
#define CLEAR_ODR    0x00
#define SET_INTPOL   0x02
#define CLEAR_INTPOL 0x00


// **** Register Addresses (BANK=0) ****
// Note: Rename all PortA to PortYY, PortB to PortZZ to avoid confusion with PIC
#define IODIRYY   0x00
#define IODIRZZ   0x01
#define IPOLYY    0x02
#define IPOLZZ    0x03
#define GPINTENYY 0x04
#define GPINTENZZ 0x05
#define DEFVALYY  0x06
#define DEFVALZZ  0x07
#define INTCONYY  0x08
#define INTCONZZ  0x09
#define IOCON    0x0A
//#define IOCON    0x0B
#define GPPUYY    0x0C
#define GPPUZZ    0x0D
#define INTFYY    0x0E
#define INTFZZ    0x0F
#define INTCAPYY  0x10
#define INTCAPZZ  0x11
#define GPIOYY    0x12
#define GPIOZZ    0x13
#define OLATYY    0x14
#define OLATZZ    0x15

/* Initializes SPI channel 2 with 8-bit modeRB9 for CS: 
 * pins used:
 *  --  CS - RPB9 (Pin 18)
 *  -- SCK - SCK1 (Pin 25) // RPB14
 *  -- SDI - RPB2 (Pin  6) We'll figure it out
 *  -- SDO - RPB5 (Pin 14) We'll figure it out
 * Sets the clock divisor for pb_clock to 4, giving 10MHz (fastest possible for
 * our MCP23S17 package)
 */
void initPE2();

void mPortYYSetPinsOut(unsigned char);

void mPortZZSetPinsOut(unsigned char);

void mPortYYSetPinsIn(unsigned char);

void mPortZZSetPinsIn(unsigned char);

void mPortYYIntEnable(unsigned char);

void mPortYYIntDisable(unsigned char);

void mPortZZIntEnable(unsigned char);

void mPortZZIntDisable(unsigned char);

void mPortYYEnablePullUp(unsigned char);

void mPortZZEnablePullUp(unsigned char);

void mPortYYDisablePullUp(unsigned char);

void mPortZZDisablePullUp(unsigned char);


/* Takes a register address on port expander and a data byte, and writes the 
 * data to the target register. */
inline void writePE2(unsigned char, unsigned char);

/* Takes a register address on port expander and returns the data byte from that
 * target register. */
inline unsigned char readPE2(unsigned char);

// spi2 lock so that port expander OR DAC can use the channel
extern volatile int spi1_lock ;
#endif	/* PORT_EXPANDER_H */

