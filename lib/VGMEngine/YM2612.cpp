#include "YM2612.h"

YM2612::YM2612(Bus* _bus, uint8_t _CS, uint8_t _RD, uint8_t _WR, uint8_t _A0, uint8_t _A1, uint8_t _IC)
{
    //clk = _clk;
    bus = _bus;
    CS = _CS;
    RD = _RD;
    WR = _WR;
    A0 = _A0;
    A1 = _A1;
    IC = _IC;

    pinMode(CS, OUTPUT);
    if(RD != NULL)
        pinMode(RD, OUTPUT);
    pinMode(WR, OUTPUT);
    pinMode(A0, OUTPUT);
    pinMode(A1, OUTPUT);
    pinMode(IC, OUTPUT);

    digitalWrite(CS, HIGH);
    if(RD != NULL)
        digitalWrite(RD, HIGH);
    digitalWrite(WR, HIGH);
    digitalWrite(IC, HIGH);
    digitalWrite(A0, LOW);
    digitalWrite(A1, LOW);
    //reset();
}

void YM2612::write(uint8_t addr, uint8_t data, bool a1)
{
    //OPTIMIZED FOR MB2 ARM CPU, NOT PORTABLE!
    a1 == true ? REG_PORT_OUTSET1 = PORT_PB14 : REG_PORT_OUTCLR1 = PORT_PB14; //A1 CHECK
    REG_PORT_OUTCLR1 = PORT_PB13; //A0 LOW
    nop;
    bus->write(addr);
    REG_PORT_OUTCLR1 = PORT_PB16; //CS LOW
    REG_PORT_OUTCLR1 = PORT_PB15; //WR LOW
    delayMicroseconds(2);
    REG_PORT_OUTSET1 = PORT_PB15; //WR HIGH
    REG_PORT_OUTSET1 = PORT_PB16; //CS HIGH
    REG_PORT_OUTSET1 = PORT_PB13; //A0 HIGH
    nop;
    bus->write(data);
    REG_PORT_OUTCLR1 = PORT_PB15; //WR LOW
    REG_PORT_OUTCLR1 = PORT_PB16; //CS LOW
    delayMicroseconds(2);
    REG_PORT_OUTSET1 = PORT_PB15; //WR HIGH
    REG_PORT_OUTSET1 = PORT_PB16; //CS HIGH
    if(addr >= 0x21 && addr <= 0x9E) //Twww YM3438 application manual timings
        delayMicroseconds(11); //~83 cycles
    else if(addr >= 0xA0 && addr <= 0xB6)
        delayMicroseconds(6); //~47 cycles
    else
        delayMicroseconds(2); //~17 cycles
}

void YM2612::reset()
{
    digitalWrite(IC, LOW);
    delayMicroseconds(25);
    digitalWrite(IC, HIGH);
    delayMicroseconds(25);
}

void YM2612::setClock(uint32_t frq)
{
    clkfrq = frq;
}

YM2612::~YM2612(){}