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
    //SPI.endTransaction();
    usedBytes = 0;
}

unsigned char SPIRAM::ReadByte(uint32_t addr)
{
    SetMode(BYTE_MODE);
    unsigned char data;
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(READ);
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    data = SPI.transfer(0x00);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
    //SPI.endTransaction();
    return data;
}

void SPIRAM::WriteByte(uint32_t addr, unsigned char data)
{
    SetMode(BYTE_MODE);
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(WRITE);
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    SPI.transfer(data);
    // SPI.endTransaction();
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
}

void SPIRAM::SetMode(char mode)
{
    if(mode != currentMode)
    {
        REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
        SPI.transfer(WRSR);
        SPI.transfer(mode);
        REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
        currentMode = mode;
    }
}

void SPIRAM::WritePage(uint32_t addr, unsigned char * buf)
{
    SetMode(PAGE_MODE);
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(WRITE);
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    for(uint16_t i = 0; i<PAGE_SIZE; i++)
        SPI.transfer(buf[i]);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
    usedBytes+=PAGE_SIZE;
}

void SPIRAM::WriteStream(uint32_t addr, unsigned char * buf, uint32_t len)
{
    SetMode(STREAM_MODE);
    REG_PORT_OUTCLR0 = PORT_PA07; //CS LOW
    SPI.transfer(WRITE);
    SPI.transfer((uint8_t)(addr >> 16));
    SPI.transfer((uint8_t)(addr >> 8));
    SPI.transfer((uint8_t)addr);
    for(uint32_t i = 0; i<len; i++)
        SPI.transfer(buf[i]);
    REG_PORT_OUTSET0 = PORT_PA07; //CS HIGH
    usedBytes+=len;
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