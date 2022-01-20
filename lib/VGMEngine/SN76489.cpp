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

bool SN76489::UpdateNoiseControl()
{
    byte noiseControlData;

    switch (currentNote[noiseCh]) 
    {
        case 60:
            // Note: C4, Periodic noise, shift rate = clock speed (Hz) / 512
            noiseControlData = 0x00;
            break;

        case 62:
            // Note: D4, Periodic noise, shift rate = clock speed (Hz) / 1024
            noiseControlData = 0x01;
            break;

        case 64:
            // Note: E4, Periodic noise, shift rate = clock speed (Hz) / 2048
            noiseControlData = 0x02;
            break;

        case 65:
            // Note: F4, Perioic noise, shift rate = Square voice 3 frequency
            noiseControlData = 0x03;
            break;

        case 67:
            // Note: G4, White noise, shift rate = clock speed (Hz) / 512
            noiseControlData = 0x04;
            break;

        case 69:
            // Note: A4, White noise, shift rate = clock speed (Hz) / 1024
            noiseControlData = 0x05;
            break;

        case 71:
            // Note: B4, White noise, shift rate = clock speed (Hz) / 2048
            noiseControlData = 0x06;
            break;

        case 72:
            // Note: C5, White noise, shift rate = Square voice 3 frequency
            noiseControlData = 0x07;
            break;

        default:
            return false;
    }

    // Send the noise control byte to the SN76489 and return true
    writeRaw(0x80 | 0x60 | noiseControlData);
    return true;
}

void SN76489::SetNoiseOn(uint8_t key)
{
    currentNote[noiseCh] = key;
    currentVelocity[noiseCh] = 127;
    bool updateAttenuationFlag = UpdateNoiseControl();
    if(updateAttenuationFlag)
        UpdateAttenuation(noiseCh);
}

void SN76489::SetNoiseOff(uint8_t key)
{
    currentNote[noiseCh] = key;
    currentVelocity[noiseCh] = 0;
    bool updateAttenuationFlag = UpdateNoiseControl();
    if(updateAttenuationFlag)
        UpdateAttenuation(noiseCh);
}

void SN76489::SetChannelOn(uint8_t key, uint8_t voice)
{
    bool updateAttenuationFlag;
    if(voice < MAX_CHANNELS_PSG)
    {
        currentNote[voice] = key;
        currentVelocity[voice] = 127;
        updateAttenuationFlag = UpdateSquarePitch(voice);
        if (updateAttenuationFlag) 
            UpdateAttenuation(voice);
    }
}

void SN76489::SetChannelOff(uint8_t key, uint8_t voice)
{
    if (key != currentNote[voice])
        return;
    currentVelocity[voice] = 0;
    UpdateAttenuation(voice);
}

bool SN76489::UpdateSquarePitch(uint8_t voice)
{
    float pitchInHz;
    unsigned int frequencyData;
    if (voice < 0 || voice > 2)
        return false;
    pitchInHz = 440 * pow(2, (float(currentNote[voice] - 69) / 12)); //removed pitch bend
    frequencyData = clkfrq / float(32 * pitchInHz);
    if (frequencyData > 1023)
        return false;
    SetSquareFrequency(voice, frequencyData);
    return true;
}

void SN76489::SetSquareFrequency(uint8_t voice, int frequencyData)
{
    if (voice < 0 || voice > 2)
        return;
    writeRaw(0x80 | frequencyRegister[voice] | (frequencyData & 0x0f));
    writeRaw(frequencyData >> 4);
}

void SN76489::UpdateAttenuation(uint8_t voice)
{
    uint8_t attenuationValue;
    if (voice < 0 || voice > 3) 
        return;
    attenuationValue = (127 - currentVelocity[voice]) >> 3;
    writeRaw(0x80 | attenuationRegister[voice] | attenuationValue);
}

void SN76489::TestToneOn(bool channelControl[4])
{
    for(uint8_t i = 0; i < 3; i++)
    {
        if(channelControl[i])
            SetChannelOn(60, i);
    }
    if(channelControl[3]) //noise channel
        SetNoiseOn(60);
}

void SN76489::TestToneOff(bool channelControl[4])
{
    for(uint8_t i = 0; i < 3; i++)
    {
        SetChannelOff(60, i);
    }
    SetNoiseOff(60);
}

SN76489::~SN76489(){}