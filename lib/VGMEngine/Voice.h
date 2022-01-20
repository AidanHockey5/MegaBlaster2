#ifndef VOICE_H_
#define VOICE_H_

//OPM File Format https://vgmrips.net/wiki/OPM_File_Format
typedef struct
{
  unsigned char LFO[5];
  unsigned char CH[7];
  unsigned char M1[11];
  unsigned char C1[11];
  unsigned char M2[11];
  unsigned char C2[11];
} Voice;

static const Voice testVoice = (Voice)
{
    {0, 0, 0, 0, 0}, //LFO
    {64, 7, 2, 0, 0, 120, 0}, //CH
    {26, 8, 5, 2, 2, 32, 3, 3, 3, 0, 0}, //M1
    {29, 5, 4, 2, 1, 32, 3, 4, 3, 0, 0}, //C1
    {28, 4, 2, 2, 3, 31, 3, 1, 3, 0, 0}, //M2
    {31, 10, 3, 1, 1, 12, 3, 1, 3, 0, 0}  //C2
}; 

#endif