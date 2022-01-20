#ifndef _SN76489_H_
#define _SN76489_H_
#include "Bus.h"
#include "SpinSleep.h"
#include "clocks.h"

class SN76489
{
public:
    SN76489(Bus* _bus, uint8_t _WE);
    ~SN76489();
    void reset();
    void write(uint8_t data);
    void setClock(uint32_t frq);
    void SetChannelOn(uint8_t key, uint8_t voice);
    void SetChannelOff(uint8_t key, uint8_t voice);
    void SetNoiseOn(uint8_t key);
    void SetNoiseOff(uint8_t key);
    bool UpdateNoiseControl();
    void UpdateAttenuation(uint8_t voice);
    void SetSquareFrequency(uint8_t voice, int frequencyData);
    void TestToneOn(bool channelControl[4]);
    void TestToneOff(bool channelControl[4]);
    bool UpdateSquarePitch(uint8_t voice);
private:
    //ChipClock* clk;
    uint32_t clkfrq = NTSC_COLORBURST;
    Bus* bus;
    uint8_t WE;
    const uint8_t attenuationRegister[4] = {0x10, 0x30, 0x50, 0x70};
    const uint8_t frequencyRegister[3] = {0x00, 0x20, 0x40};
    uint8_t currentNote[4] = {0, 0, 0, 0};
    uint8_t currentVelocity[4] = {0, 0, 0, 0};
    const int noiseCh = 3;
    const int MAX_CHANNELS_PSG = 3;
    unsigned char psgFrqLowByte = 0;
    void writeRaw(uint8_t data);
};

//clock bus we






#endif