//Feat. Kunoichi PSG fixes https://git.agiri.ninja/natalie/megagrrl/-/commit/33cfe2fa5d3d8d72bca1c528f5ab86dcdce07068
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
    dcsg_latched_ch = 0;
    for (uint8_t i=0;i<3;i++) dcsg_freq[i] = 0;
    for (uint8_t i=0;i<4;i++) 
    {
        writeRaw(0x80 | (i<<5) | 0x10 | 0xf); //full atten
        if (i != 3) 
        { //set freq to 0
            writeRaw(0x80 | (i<<5));
            writeRaw(0);
        }
    }
}

void SN76489::write(uint8_t data)
{
    if (fixDcsgFrequency) 
    {
        if ((data & 0x80) == 0) 
        { //ch 1~3 frequency high byte write
            //note: whether or not dcsg_latched_ch is actually still in the latch on the chip (ch3 updates might blow it away) doesn't matter, because we always rewrite the low byte anyway. it's what's latched from *our* POV
            dcsg_freq[dcsg_latched_ch] = (dcsg_freq[dcsg_latched_ch] & 0b1111) | ((data & 0b111111) << 4);
            //write both registers now
            if (dcsg_latched_ch == 2) 
            { //ch3
                writeDcsgCh3Freq();
            } 
            else 
            {
                uint8_t low = 0x80 | (dcsg_latched_ch<<5) | (dcsg_freq[dcsg_latched_ch] & 0b1111);
                if (dcsg_freq[dcsg_latched_ch] == 0) low |= 1;
                writeRaw(low);
                writeRaw((dcsg_freq[dcsg_latched_ch] >> 4) & 0b111111);
            }
        } 
        else if ((data & 0b10010000) == 0b10000000 && (data&0b01100000)>>5 != 3) 
        { //ch 1~3 frequency low byte write
            dcsg_latched_ch = (data>>5)&3;
            dcsg_freq[dcsg_latched_ch] = (dcsg_freq[dcsg_latched_ch] & 0b1111110000) | (data & 0b1111);
            uint8_t val = data;
            if (dcsg_freq[dcsg_latched_ch] == 0) 
            {
                val |= 1;
            }
            writeRaw(val);
        } 
        else 
        { //attenuation or noise ch control write
            if ((data & 0b10010000) == 0b10010000) 
            { //attenuation
                uint8_t ch = (data>>5)&0b00000011;
                dcsgAttenuation[ch] = data;
                //data = filterDcsgAttenWrite(data);
                //data |= 0b00001111; //if we haven't reached the first wait, force full attenuation
                if (ch == 2)
                {
                    //when ch 3 atten is updated, we also need to write frequency again. this is due to the periodic noise fix.
                    //TODO: this would be better if it only does it if actually transitioning in or out of mute, rather than on every atten update
                    writeDcsgCh3Freq();
                }
            } 
            else if ((data & 0b11110000) == 0b11100000) 
            { //noise control
                dcsgNoisePeriodic = (data & 0b00000100) == 0; //FB
                dcsgNoiseSourceCh3 = (data & 0b00000011) == 0b00000011; //NF0, NF1
                //periodic noise fix: update ch3 frequency when noise control settings change
                //TODO: only update it when it matters :P
                writeDcsgCh3Freq();
            }
            writeRaw(data);
        }
    } 
    else 
    { //not fixing dcsg frequency
        writeRaw(data); //just write it normally
    }
}

void SN76489::writeDcsgCh3Freq()
{
    uint32_t freq = dcsg_freq[2];
    //if the fix is enabled, and dcsg noise is set to "periodic" mode, and it gets its freq from ch3, and ch3 is muted, then adjust ch3 freq
    if (fixDcsgPeriodic && dcsgNoisePeriodic && dcsgNoiseSourceCh3 && dcsgAttenuation[2] == 0b11011111) {
        //increase the value by 6.25%, which actually decreases output freq, because dcsg freq regs are "upside down"
        freq *= 10625;
        freq /= 10000;
    }

    if (freq == 0) freq = 1;
    //first byte
    uint8_t out = 0b11000000 | (freq & 0b00001111);
    writeRaw(out);

    //second byte
    out = (freq >> 4) & 0b00111111; //mask is needed to make sure overflows don't end up in dcsg bit 7
    writeRaw(out);
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