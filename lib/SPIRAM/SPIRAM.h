
#ifndef SPIRAM_H_
#define SPIRAM_H_
#include <Arduino.h>

//LY68L6400SLIT

#define PAGE_SIZE 1023

class SPIRAM
{
public:
    SPIRAM(int CS);
    unsigned char ReadByte(uint32_t addr);
    void WriteByte(uint32_t addr, unsigned char data);
    void Init();
    uint32_t usedBytes = 0;
private:
    int _CS;
    //char block[124000]; //You can use this if you just want to test on-board RAM
};
#endif