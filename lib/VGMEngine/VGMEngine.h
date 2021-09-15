#ifndef _VGM_ENGINE_H_
#define _VGM_ENGINE_H_

#include "DeviceEnable.h" //YOU MUST ENABLE EVERY DEVICE YOU PLAN TO USE FIRST!

#include "VGMHeader.h"
#include "megastream.h"
#include "GD3.h"

#include "SPIRAM.h"
#include "YM2612.h"
#include "SN76489.h"

#include "clocks.h"

#define min(a,b) ((a) < (b) ? (a) : (b))
#define MAX_PCM_BUFFER_SIZE 8388607
#define MAX_DATA_BLOCKS_IN_BANK 40
#define COMMAND_ERROR_SKIP_THRESHOLD 20 //If you encounter x bad commands, just skip the track

enum VGMEngineState {PLAYING, IDLE, END_OF_TRACK, ERROR};

struct DataBlock //slightly modified version of Natalie's data block structure
{
    bool slotOccupied;
    //uint32_t Seq;
    //uint8_t DataBankId;
    //uint8_t DataBlockId;
    //uint8_t ChipCommand;
    //uint8_t ChipPort;
    uint32_t SampleRate;
    uint32_t DataStart;
    uint32_t absoluteDataStartInBank;
    uint8_t LengthMode;
    uint32_t DataLength;
    //uint32_t ReadOffset;
    //uint32_t BytesFilled;
};

class VGMEngineClass
{
public:
    VGMEngineClass();
    ~VGMEngineClass();
    bool begin(File *f);
    VGMHeader header;
    GD3 gd3;
    #if ENABLE_SPIRAM
        SPIRAM ram = SPIRAM(PIN_PA07);
    #endif
    #if ENABLE_SN76489
        SN76489* sn76489;
    #endif
    #if ENABLE_YM2612 
        YM2612* ym2612; 
    #endif
    bool load(bool singleChunk = false);
    void tick44k1();
    void tickDacStream();
    VGMEngineState play();
    uint16_t getLoops();
    uint16_t maxLoops = 3;
    bool loopOneOffs = false;
    //There are only 6 channels, but for chip writes we use (0x00, 0x01, 0x02, 0x04, 0x05 and 0x06)
    //The extra array positions are there just to avoid crashes in case the VGM file tries to write to an invalid channel
    bool ym2612CHControl[8] = {true, true, true, true, true, true, true, true};
    //There are 4 channels on PSG (3 tone and 1 noise)
    bool sn76489CHControl[4] = {true, true, true, true};
    //Keeps track of the currently latched channel so we can ignore writes if disabled
    uint8_t sn76489Latched = 0;
    VGMEngineState state = IDLE;
    bool resetISR = false;
    bool isBusy = false;
    void ramtest();
    void (*setDacStreamTimer)(uint32_t);
    void (*stopDacStreamTimer)(void);
private:
    File* file;
    static const uint32_t VGM_BUF_SIZE = 16384;
    volatile int32_t waitSamples = 0;
    volatile bool ready = false;
    bool bufLock = false;
    uint16_t badCommandCount = 0;
    uint32_t pcmBufferPosition = 0;
    uint32_t pcmBufferEndPosition = 0;
    uint32_t loopPos = 0; //The location of the 0x66 command
    uint16_t loopCount = 0;
    MegaStreamContext_t stream;
    uint8_t buf[VGM_BUF_SIZE];
    DataBlock dataBlocks[MAX_DATA_BLOCKS_IN_BANK];
    void resetDataBlocks();
    uint8_t dataBlockChipCommand;
    uint8_t dataBlockChipPort;
    uint8_t dataBlockStepSize;
    uint8_t dataBlockStepBase;
    uint32_t dacStreamBufPos;
    uint32_t dacStreamCurLength;
    int32_t dacSampleReady = false;
    uint8_t activeDacStreamBlock;
    int32_t dacSampleCountDown;
    bool isOneOff = false; //This track doesn't have a loop

    uint8_t readBufOne(); 
    uint16_t readBuf16();
    uint32_t readBuf32();

    // void timerConfig();
    // void startTimer();

    void setClocks();
    void chipSetup();
    bool storePCM(bool skip = false);
    bool topUp();
    uint16_t parseVGM();
};

//<3 natalie
const static unsigned char CommandLengthLUT[256] = {
  /*0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f*/
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x00
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x10
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x20
    2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, //0x30
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 2, //0x40
    2, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, //0x50
    0, 3, 1, 1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x60
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x70
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, //0x80
    5, 5, 6,11, 2, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0x90
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, //0xa0
    3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, //0xb0
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, //0xc0
    4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0xd0
    5, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0xe0
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, //0xf0
};

inline uint8_t VgmCommandLength(uint8_t Command) //including the command itself. only for fixed size commands
{ 
    //Also natalie, tyvm
    uint8_t val = CommandLengthLUT[Command];
    if (val) return val;
    return 0xFF;
}

extern VGMEngineClass VGMEngine;
#endif