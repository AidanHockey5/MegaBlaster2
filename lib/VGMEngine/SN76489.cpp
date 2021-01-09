#include "SN76489.h"

SN76489::SN76489(Bus* _bus, uint8_t _WE)
{
    bus = _bus;
    WE = _WE;
    pinMode(WE, OUTPUT);
    digitalWrite(WE, HIGH);
}

void SN76489::setClock(uint32_t frq)
{
    clkfrq = frq;
}

void SN76489::reset()
{
    writeRaw(0x9F);
    writeRaw(0xBF);
    writeRaw(0xDF);
    writeRaw(0xFF);
}

void SN76489::write(uint8_t data)
{
    if((data & 0x80) == 0)
    {
        if((psgFrqLowByte & 0x0F) == 0)
        {
            if((data & 0x3F) == 0)
            psgFrqLowByte |= 1;
        }
        writeRaw(psgFrqLowByte);
        writeRaw(data);
    }
    else if((data & 0x90) == 0x80 && (data & 0x60)>>5 != 3)
        psgFrqLowByte = data;
    else
        writeRaw(data);
}



void SN76489::writeRaw(uint8_t data)
{
    //32 clock cycles is 280.11nS
    //120MHz nop is about 8.3nS
    REG_PORT_OUTSET1 = PORT_PB17; //WE HIGH 
    bus->write(data);
    REG_PORT_OUTCLR1 = PORT_PB17; //WE LOW
    sleepClocks(NTSC_COLORBURST, 37);     //Around 10uS or so. The PSG is very slow. Datasheet requests 32 cycles, but 37 should give us time to account for CYCCNT inaccuracies 
    REG_PORT_OUTSET1 = PORT_PB17; //WE HIGH
    sleepClocks(NTSC_COLORBURST, 1);      //Safety delay so we don't slam into the second write cycle before the PSG is finished with the first
}

SN76489::~SN76489(){}