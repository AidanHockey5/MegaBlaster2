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
    if(addr == 0x21 || addr == 0x2C) //Prevent writes to test registers
        return;
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
    // if(addr == 0x24)
    // {
    //     Serial.print("0x24: 0x"); Serial.print(data, HEX); Serial.print("   -- A1:"); Serial.println(a1);
    // }
    // if(addr == 0x25)
    // {
    //     Serial.print("0x25: 0x"); Serial.print(data, HEX); Serial.print("   -- A1:"); Serial.println(a1);
    // }
    // if(addr == 0x27)
    // {
    //     Serial.print("0x27: 0x"); Serial.print(data, HEX); Serial.print("   -- A1:"); Serial.println(a1);
    // }
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

void YM2612::setYMTimerA(uint16_t value)
{
    write(0x25, value & 0x03, 0);
    write(0x24, value>>2, 0);
    write(0x27, 0b00010101, 0);
}

void YM2612::clearYMTimerA()
{
    write(0x27, 0, 0);
}

void YM2612::SetVoice(Voice v)
{
  write(0x22, 0x00, 0); // LFO off
  write(0x27, 0x00, 0); // CH3 Normal
  write(0x28, 0x00, 0); // Turn off all channels
  write(0x2B, 0x00, 0); // DAC off

  for(int a1 = 0; a1<=1; a1++)
  {
    for(int i=0; i<3; i++)
    {
          uint8_t DT1MUL, TL, RSAR, AMD1R, D2R, D1LRR = 0;

          //Operator 1
          DT1MUL = (v.M1[8] << 4) | v.M1[7];
          TL = v.M1[5];
          RSAR = (v.M1[6] << 6) | v.M1[0];
          AMD1R = (v.M1[10] << 7) | v.M1[1];
          D2R = v.M1[2];
          D1LRR = (v.M1[4] << 4) | v.M1[3];

          // if(i == 0 && a1 == 0)
          // {
          // Serial.println("-----OPERATOR 1, A0-----");
          // Serial.print("DT1MUL: "); Serial.println(DT1MUL, HEX);
          // Serial.print("TL: "); Serial.println(TL, HEX);
          // Serial.print("RSAR: "); Serial.println(RSAR, HEX);
          // Serial.print("AMD1R: "); Serial.println(AMD1R, HEX);
          // Serial.print("D2R: "); Serial.println(D2R, HEX);
          // Serial.print("D1LRR: "); Serial.println(D1LRR, HEX);
          // }

          // Serial.println();
          // if(i == 0 && a1 == 1)
          // {
          // Serial.println("-----OPERATOR 1, A1-----");
          // Serial.print("DT1MUL: "); Serial.println(DT1MUL, HEX);
          // Serial.print("TL: "); Serial.println(TL, HEX);
          // Serial.print("RSAR: "); Serial.println(RSAR, HEX);
          // Serial.print("AMD1R: "); Serial.println(AMD1R, HEX);
          // Serial.print("D2R: "); Serial.println(D2R, HEX);
          // Serial.print("D1LRR: "); Serial.println(D1LRR, HEX);
          // }

          write(0x30 + i, DT1MUL, a1); //DT1/Mul
          write(0x40 + i, TL, a1); //Total Level
          write(0x50 + i, RSAR, a1); //RS/AR
          write(0x60 + i, AMD1R, a1); //AM/D1R
          write(0x70 + i, D2R, a1); //D2R
          write(0x80 + i, D1LRR, a1); //D1L/RR
          write(0x90 + i, 0x00, a1); //SSG EG
          
          //Operator 2
          DT1MUL = (v.C1[8] << 4) | v.C1[7];
          TL = v.C1[5];
          RSAR = (v.C1[6] << 6) | v.C1[0];
          AMD1R = (v.C1[10] << 7) | v.C1[1];
          D2R = v.C1[2];
          D1LRR = (v.C1[4] << 4) | v.C1[3];
          write(0x34 + i, DT1MUL, a1); //DT1/Mul
          write(0x44 + i, TL, a1); //Total Level
          write(0x54 + i, RSAR, a1); //RS/AR
          write(0x64 + i, AMD1R, a1); //AM/D1R
          write(0x74 + i, D2R, a1); //D2R
          write(0x84 + i, D1LRR, a1); //D1L/RR
          write(0x94 + i, 0x00, a1); //SSG EG
           
          //Operator 3
          DT1MUL = (v.M2[8] << 4) | v.M2[7];
          TL = v.M2[5];
          RSAR = (v.M2[6] << 6) | v.M2[0];
          AMD1R = (v.M2[10] << 7) | v.M2[1];
          D2R = v.M2[2];
          D1LRR = (v.M2[4] << 4) | v.M2[3];
          write(0x38 + i, DT1MUL, a1); //DT1/Mul
          write(0x48 + i, TL, a1); //Total Level
          write(0x58 + i, RSAR, a1); //RS/AR
          write(0x68 + i, AMD1R, a1); //AM/D1R
          write(0x78 + i, D2R, a1); //D2R
          write(0x88 + i, D1LRR, a1); //D1L/RR
          write(0x98 + i, 0x00, a1); //SSG EG
                   
          //Operator 4
          DT1MUL = (v.C2[8] << 4) | v.C2[7];
          TL = v.C2[5];
          RSAR = (v.C2[6] << 6) | v.C2[0];
          AMD1R = (v.C2[10] << 7) | v.C2[1];
          D2R = v.C2[2];
          D1LRR = (v.C2[4] << 4) | v.C2[3];
          write(0x3C + i, DT1MUL, a1); //DT1/Mul
          write(0x4C + i, TL, a1); //Total Level
          write(0x5C + i, RSAR, a1); //RS/AR
          write(0x6C + i, AMD1R, a1); //AM/D1R
          write(0x7C + i, D2R, a1); //D2R
          write(0x8C + i, D1LRR, a1); //D1L/RR
          write(0x9C + i, 0x00, a1); //SSG EG

          uint8_t FBALGO = (v.CH[1] << 3) | v.CH[2];
          write(0xB0 + i, FBALGO, a1); // Ch FB/Algo
          write(0xB4 + i, 0xC0, a1); // Both Spks on

          write(0x28, 0x00 + i + (a1 << 2), 0); //Keys off
    }
  }
}

void YM2612::KeyOn(byte key, byte slot)
{
    if(slot > 5)
        return;
    uint8_t offset = slot % 3;
    bool setA1 = slot > 2;

    SetFrequency(NoteToFrequency(key), slot);
    write(0x28, 0xF0 + offset + (setA1 << 2), 0);   
}

void YM2612::KeyOff(byte slot)
{
    if(slot > 5)
        return;
    uint8_t offset = slot % 3;
    bool setA1 = slot > 2;

    write(0x28, 0x00 + offset + (setA1 << 2), 0);
}

float YM2612::NoteToFrequency(uint8_t note)
{
    //Elegant note/freq system by diegodorado
    //Check out his project at https://github.com/diegodorado/arduinoProjects/tree/master/ym2612
    const static float freq[12] = 
    {
      //You can create your own note frequencies here. C4#-C5. There should be twelve entries.
      //YM3438 datasheet note set
      277.2, 293.7, 311.1, 329.6, 349.2, 370.0, 392.0, 415.3, 440.0, 466.2, 493.9, 523.3

    }; 
    const static float multiplier[] = 
    {
      0.03125f,   0.0625f,   0.125f,   0.25f,   0.5f,   1.0f,   2.0f,   4.0f,   8.0f,   16.0f,   32.0f 
    }; 
    float f = freq[note%12];
    const float tune = -0.065; 
    return (f+(f*tune))*multiplier[(note/12)];
}

void YM2612::SetFrequency(uint16_t frequency, uint8_t slot)
{
  int block = 2;
  
  uint16_t frq;
  while(frequency >= 2048)
  {
    frequency /= 2;
    block++;
  }
  frq = (uint16_t)frequency;
  bool setA1 = slot > 2;

  // unsigned char f1 = ((frq >> 8) & mask(3)) | ((block & mask(3)) << 3);
  // unsigned char f2 = frq;
  // Serial.println("-------");
  // Serial.print("0xA4: "); Serial.println(f1, HEX);
  // Serial.print("0xA0: "); Serial.println(f2, HEX);
  // Serial.print("A1 SET: "); Serial.println(setA1 ? "TRUE" : "FALSE");
  // Serial.println(0xA4+channel%3, HEX);
  // Serial.println("-------");

  write(0xA4 + slot%3, ((frq >> 8) & mask(3)) | ((block & mask(3)) << 3), setA1);
  write(0xA0 + slot%3, frq, setA1);
}

void YM2612::TestToneOn(bool channelControl[8])
{
    for(uint8_t i = 0; i < 6; i++)
    {
        if(channelControl[i]) //Only play the note if the channel is enabled
            KeyOn(108, i); 
    }
}

void YM2612::TestToneOff()
{
    for(uint8_t i = 0; i < 6; i++)
    {
        KeyOff(i); 
    }
}

YM2612::~YM2612(){}