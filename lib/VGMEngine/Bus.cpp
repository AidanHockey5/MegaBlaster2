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

void Bus::write(uint8_t data)
{
    //Optimized for MEGABLASTER 2 ARM CPU, NOT AT ALL PORTABLE
    uint32_t drev;                                      //Reversed byte for swapped bus (easier for PCB layout)
    __asm__("rbit %0, %1\n" : "=r"(drev) : "r"(data));  //use ARM ASM for supa' speed swapping bit order. 32 bit operation
    data = drev >> 24;                                  //Copy reversed byte from drev back to data after pushing over 24 bits
    uint32_t p = REG_PORT_OUT0;                         //Grab current state of entire PORTA reg
    p &= 0b11111000000001111111111111111;               //Mask out everything except our databus
    p |= data << 16;                                    //Insert our data
    REG_PORT_OUT0 = p;                                  //Write the entire port over with our new data inserted

    // for(uint8_t i = 0; i<8; i++)
    //    digitalWrite(pins[i], ((data >> i)&1)); //slow but portable way
}

void Bus::prepare()
{
    for(uint8_t i=0; i<8; i++)
        pinMode(pins[i], OUTPUT);
    write(0);
}