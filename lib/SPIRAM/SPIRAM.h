
#ifndef SPIRAM_H_
#define SPIRAM_H_
#include <Arduino.h>

//LY68L6400SLIT

#define PAGE_SIZE 1024
#define STREAM_CHUNK_SIZE 32

#define RDSR  5
#define WRSR  1
#define READ  3
#define WRITE 2

// SRAM Hold line override
#define HOLD 1

// SRAM modes
#define BYTE_MODE (0x00 | HOLD)
#define PAGE_MODE (0x80 | HOLD)
#define STREAM_MODE (0x40 | HOLD)

class SPIRAM
{
public:
    SPIRAM(int CS);
    unsigned char ReadByte(uint32_t addr);
    void WriteByte(uint32_t addr, unsigned char data);
    void SetMode(char mode);
    void WritePage(uint32_t addr, unsigned char * buf);
    void WriteStream(uint32_t addr, unsigned char * buf, uint32_t len);
    void Init();
    uint32_t usedBytes = 0;
private:
    int _CS;
    char currentMode;
    //char block[124000]; //You can use this if you just want to test on-board RAM
};
#endif