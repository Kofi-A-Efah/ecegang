#include "port_expander2_brl4.h"

#define SET_CS    {mPORTASetBits(BIT_0);}
#define CLEAR_CS  {mPORTAClearBits(BIT_0);}

// === spi bit widths ====================================================
// hit the SPI control register directly, SPI2
// Change the SPI bit modes on the fly, mid-transaction if necessary
inline void SPI1_Mode16(void){  // configure SPI2 for 16-bit mode
    SPI1CONSET = 0x400;
    SPI1CONCLR = 0x800;
}
// ========
inline void SPI1_Mode8(void){  // configure SPI2 for 8-bit mode
    SPI1CONCLR = 0x400;
    SPI1CONCLR = 0x800;
}
// ========
inline void SPI1_Mode32(void){  // configure SPI2 for 32-bit mode
    SPI1CONCLR = 0x400;
    SPI1CONSET = 0x800;
}

void initPE() {
  // control CS for DAC
  volatile SpiChannel pe_spi = SPI_CHANNEL1;
  volatile int spiClkDiv = 4; // 10 MHz max speed for this DAC
  mPORTASetPinsDigitalOut(BIT_0); // use RPB9 (pin 21)
  mPORTASetBits(BIT_0); // CS active low
  PPSOutput(2, RPB1, SDO1); // use RPB5 (pin 14) for SDO2
  PPSInput(3, SDI1,RPA1); // SDI2
  
  SpiChnOpen(pe_spi, SPI_OPEN_ON | SPI_OPEN_MODE8 | SPI_OPEN_MSTEN | SPI_OPEN_CKE_REV, spiClkDiv);
  
  writePE(IOCON, ( CLEAR_BANK   | CLEAR_MIRROR | SET_SEQOP |
                   CLEAR_DISSLW | CLEAR_HAEN   | CLEAR_ODR |
                   CLEAR_INTPOL ));
}

void clearBits(unsigned char addr, unsigned char bitmask){
  if (addr <= 0x15){
    unsigned char cur_val = readPE(addr);
    writePE(addr, cur_val & (~bitmask));
  }
}

void setBits(unsigned char addr, unsigned char bitmask){
  if (addr <= 0x15){
    unsigned char cur_val = readPE(addr);
    writePE(addr, cur_val | (bitmask));
  }
}

void toggleBits(unsigned char addr, unsigned char bitmask){
  if (addr <= 0x15){
    unsigned char cur_val = readPE(addr);
    writePE(addr, cur_val ^ (bitmask));
  }
}

unsigned char readBits(unsigned char addr, unsigned char bitmask){
  if (addr <= 0x15){
    unsigned char cur_val = readPE(addr) & bitmask ;
    return cur_val ;
  }
}

void mPortYYSetPinsOut(unsigned char bitmask){
  clearBits(IODIRYY, bitmask);
}

void mPortZZSetPinsOut(unsigned char bitmask){
  clearBits(IODIRZZ, bitmask);
}

void mPortYYSetPinsIn(unsigned char bitmask){
  setBits(IODIRYY, bitmask);
}

void mPortZZSetPinsIn(unsigned char bitmask){
  setBits(IODIRZZ, bitmask);
}

void mPortYYIntEnable(unsigned char bitmask){
  setBits(GPINTENYY, bitmask);
}

void mPortZZIntEnable(unsigned char bitmask){
  setBits(GPINTENZZ, bitmask);
}

void mPortYYIntDisable(unsigned char bitmask){
  clearBits(GPINTENYY, bitmask);
}

void mPortZZIntDisable(unsigned char bitmask){
  clearBits(GPINTENZZ, bitmask);
}

void mPortYYEnablePullUp(unsigned char bitmask){
  setBits(GPPUYY, bitmask);
}

void mPortZZEnablePullUp(unsigned char bitmask){
  setBits(GPPUZZ, bitmask);
}

void mPortYYDisablePullUp(unsigned char bitmask){
  clearBits(GPPUYY, bitmask);
}

void mPortZZDisablePullUp(unsigned char bitmask){
  clearBits(GPPUZZ, bitmask);
}

inline void writePE(unsigned char reg_addr, unsigned char data) {
  unsigned char junk = 0;
   
  // test for ready
  while (TxBufFullSPI1());
  
  // CS low to start transaction
  CLEAR_CS
  // 8-bits
  SPI1_Mode8();
  // OPCODE and HW Address (Should always be 0b0100000), set LSB for write
  WriteSPI1((PE_OPCODE_HEADER | WRITE));
  // test for done
  while (SPI1STATbits.SPIBUSY); // wait for byte to be sent
  junk = ReadSPI1();
  // Input Register Address

  WriteSPI1(reg_addr);
  while (SPI1STATbits.SPIBUSY); // wait for byte to be sent
  junk = ReadSPI1();
  // One byte of data to write to register
  
  WriteSPI1(data);
  // test for done
  while (SPI1STATbits.SPIBUSY); // wait for end of transaction
  junk = ReadSPI1();
  // CS high
  SET_CS
  
}

inline unsigned char readPE(unsigned char reg_addr) {
  unsigned char out = 0;
  
  // test for ready
  while (TxBufFullSPI1());
  
  // CS low to start transaction
  CLEAR_CS
  
  // 8-bits
  SPI_Mode8();
  // OPCODE and HW Address (Should always be 0b0100000), clear LSB for write
  WriteSPI1((PE_OPCODE_HEADER | READ));
  // test for done
  while (SPI1STATbits.SPIBUSY); // wait for byte to be sent
  out = ReadSPI1(); //junk
  // Input Register Address
  // test for ready
  while (TxBufFullSPI1());
  // 8-bits
  
  WriteSPI2(reg_addr);
  while (SPI1STATbits.SPIBUSY); // wait for byte to be sent
  out = ReadSPI1(); // junk
  // One byte of dummy data to write to register
  // test for ready
  while (TxBufFullSPI1());
  // 8-bits
  
  WriteSPI1(out);
  // test for done
  while (SPI1STATbits.SPIBUSY); // wait for end of transaction
  out = ReadSPI1(); // bingo
  // CS high
  SET_CS
  
  return out;
}
