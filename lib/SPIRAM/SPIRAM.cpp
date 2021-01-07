#include "SPIRAM.h"
#include <SPI.h>

//SPIClass SPI();
//PA07 CS
//PINS DIRECTLY WRITTEN TO FOR MAXIMUM SPEED
SPIRAM::SPIRAM(int CS)
{
    _CS = CS;
}

void SPIRAM::Init()
{
    SPI.begin();
    SPI.setBitOrder(MSBFIRST); // Set the SPI-2 bit order (*) 
    SPI.setDataMode(SPI_MODE0);
    SPI.setClockDivider(SPI_CLOCK_DIV2);

    REG_PORT_DIRSET0 = PORT_PA07; //CS SET TO OUTPUT
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    delayMicroseconds(1);

    //Reset
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH 
    delayMicroseconds(1);
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    delayMicroseconds(1);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH 
    

    //Set read mode to "byte"
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(0x05);
    SPI.transfer(0x00);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH

    //Set write mode to "byte"
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(0x01);
    SPI.transfer(0x00);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
    SPI.endTransaction();
}

unsigned char SPIRAM::ReadByte(uint32_t addr)
{
    //CS B12
    unsigned char data;
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(0x03);
    SPI.transfer((uint8_t)(addr >> 16)); //&0xff
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    data = SPI.transfer(0x00);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
    SPI.endTransaction();
    return data;
}

void SPIRAM::WriteByte(uint32_t addr, unsigned char data)
{
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(0x02);
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    SPI.transfer(data);
    SPI.endTransaction();
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
}


//Used for testing on-board RAM
// void SPIRAM::Init()
// {
//     return;
// }

// unsigned char SPIRAM::ReadByte(uint32_t addr)
// {
//     return block[addr];
// }

// void SPIRAM::WriteByte(uint32_t addr, unsigned char data)
// {
//     block[addr] = data;
// }
