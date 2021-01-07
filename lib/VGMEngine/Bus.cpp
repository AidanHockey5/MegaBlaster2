#include "Bus.h"


Bus::Bus(uint8_t _D0, uint8_t _D1, uint8_t _D2, uint8_t _D3, uint8_t _D4, uint8_t _D5, uint8_t _D6, uint8_t _D7)
{
    pins[0] = _D0;
    pins[1] = _D1;
    pins[2] = _D2;
    pins[3] = _D3;
    pins[4] = _D4;
    pins[5] = _D5;
    pins[6] = _D6;
    pins[7] = _D7;
    prepare();
}

Bus::Bus(uint8_t _pins[8])
{
    memcpy(pins, _pins, sizeof(uint8_t)*8);
}

//Generic parallel write function. Might be slow on most hardware, but easy to use if you have arb. bus pins
void Bus::write(uint8_t data)
{
    for(uint8_t i = 0; i<8; i++)
       digitalWrite(pins[i], ((data >> i)&1)); //Turns out, there really isn't that much of a faster way to do this on SAMD lol
}

void Bus::prepare()
{
    for(uint8_t i=0; i<8; i++)
        pinMode(pins[i], OUTPUT);
    write(0);
}