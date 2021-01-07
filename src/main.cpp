//SPI SD CARD LIBRARY WARNING:
//CHIP SELECT FEATURES MANUALLY ADJUSTED IN SDFAT LIB (in SdSpiDriver.h). MUST USE LIB INCLUDED WITH REPO!!!

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SdFat.h"
#include "U8g2lib.h" //Run this command in the terminal below if PIO asks for a dependancy: pio lib install "U8g2"
#include "YM2612.h"
#include "SN76489.h"
#include "Adafruit_ZeroTimer.h"
#include "logo.h"
#include "SpinSleep.h"
#include "SerialUtils.h"
#include "clocks.h"

#include "VGMEngine.h"

//Debug variables
#define DEBUG true //Set this to true for a detailed printout of the header data & any errored command bytes
#define DEBUG_LED A4
bool commandFailed = false;
uint8_t failedCmd = 0x00;

//Structs
enum FileStrategy {FIRST_START, NEXT, PREV, RND, REQUEST};
enum PlayMode {LOOP, PAUSE, SHUFFLE, IN_ORDER};

//Prototypes
void setup();
void loop();
void handleSerialIn();
void tick();
void setISR();
void pauseISR();
void removeMeta();
void prebufferLoop();
void injectPrebuffer();
void fillBuffer();
bool topUpBuffer(); 
void clearBuffers();
void handleButtons();
void prepareChips();
void readGD3();
void drawOLEDTrackInfo();
bool startTrack(FileStrategy fileStrategy, String request = "");
bool vgmVerify();
uint8_t VgmCommandLength(uint8_t Command);
uint8_t readBuffer();
uint16_t readBuffer16();
uint32_t readBuffer32();
uint32_t readSD32();
uint16_t parseVGM();

Adafruit_ZeroTimer zerotimer = Adafruit_ZeroTimer(3);

Bus bus(0, 1, 8, 9, 11, 10, 12, 13);

YM2612 opn(&bus, 3, NULL, 6, 4, 5, 7);
SN76489 sn(&bus, 2);

//OLED
U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0);
//U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0);
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C oled(U8G2_R0);
bool isOledOn = true;

//Buttons
const int prev_btn = PORT_PB00;
const int rand_btn = PORT_PB01;
const int next_btn = PORT_PB04;
const int option_btn = PORT_PB05;
const int select_btn = PORT_PB09;

//SD & File Streaming
SdFat SD;
File file;
#define MAX_FILE_NAME_SIZE 128
char fileName[MAX_FILE_NAME_SIZE];
uint32_t numberOfFiles = 0;
uint32_t currentFileNumber = 0;

//Counters
uint32_t bufferPos = 0;
uint32_t cmdPos = 0;
uint16_t waitSamples = 0;
uint32_t pcmBufferPosition = 0;

//VGM Variables
uint16_t loopCount = 0;
uint8_t maxLoops = 3;
bool fetching = false;
volatile bool ready = false;
bool samplePlaying = false;
PlayMode playMode = SHUFFLE;
bool doParse = false;

void setup()
{
  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.set_freq(7670454ULL*100ULL, SI5351_CLK0); //CLK0 YM
  si5351.set_freq(3579545ULL*100ULL, SI5351_CLK1); //CLK1 PSG - VALUES IN 0.01Hz
  //DEBUG
  pinMode(DEBUG_LED, OUTPUT);
  digitalWrite(DEBUG_LED, LOW);

  resetSleepSpin();

  //Button configs
  //Group 1 is port B, PINCFG just wants the literal pin number on the port, DIRCLR and OUTSET want the masked PORT_XX00 *not* the PIN_XX00
  //Prev button input pullup
  PORT->Group[1].PINCFG[0].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN);
  PORT->Group[1].DIRCLR.reg = PORT_PB00;
  PORT->Group[1].OUTSET.reg = PORT_PB00;

  //Rand button input pullup
  PORT->Group[1].PINCFG[1].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN);
  PORT->Group[1].DIRCLR.reg = PORT_PB01;
  PORT->Group[1].OUTSET.reg = PORT_PB01;

  //Next button input pullup
  PORT->Group[1].PINCFG[4].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN);
  PORT->Group[1].DIRCLR.reg = PORT_PB04;
  PORT->Group[1].OUTSET.reg = PORT_PB04;

  //Option button input pullup
  PORT->Group[1].PINCFG[5].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN); 
  PORT->Group[1].DIRCLR.reg = PORT_PB05;
  PORT->Group[1].OUTSET.reg = PORT_PB05;

  //Select button input pullup
  PORT->Group[1].PINCFG[9].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN); 
  PORT->Group[1].DIRCLR.reg = PORT_PB09;
  PORT->Group[1].OUTSET.reg = PORT_PB09;

  //COM
  Wire.begin();
  SPI.begin();
  Serial.begin(115200);

  //Set Chips
  VGMEngine.ym2612 = &opn;
  VGMEngine.sn76489 = &sn;

  //OLED
  oled.begin();
  oled.setFont(u8g2_font_fub11_tf);
  oled.drawXBM(0,0, logo_width, logo_height, logo);
  oled.sendBuffer();
  delay(3000);
  oled.clearDisplay();

  //SD
  REG_PORT_DIRSET0 = PORT_PA15; //Set PA15 to output
  if(!SD.begin(PORT_PA15, SPI_FULL_SPEED))
  {
    Serial.println("SD Mount Failed!");
    oled.clearBuffer();
    oled.drawStr(0,16,"SD Mount");
    oled.drawStr(0,32,"failed!");
    oled.sendBuffer();
    while(true){Serial.println("SD MOUNT FAILED"); delay(1000);}
  }

  Serial.flush();

  //Prepare files
  removeMeta();

  File countFile;
  while ( countFile.openNext( SD.vwd(), O_READ ))
  {
    countFile.close();
    numberOfFiles++;
  }

  countFile.close();
  SD.vwd()->rewind();

  //Begin
  startTrack(FIRST_START);
}

void removeMeta() //Remove useless meta files
{
  File tmpFile;
  while ( tmpFile.openNext( SD.vwd(), O_READ ))
  {
    memset(fileName, 0x00, MAX_FILE_NAME_SIZE);
    tmpFile.getName(fileName, MAX_FILE_NAME_SIZE);
    if(fileName[0]=='.')
    {
      if(!SD.remove(fileName))
      if(!tmpFile.rmRfStar())
      {
        Serial.print("FAILED TO DELETE META FILE"); Serial.println(fileName);
      }
    }
    if(String(fileName) == "System Volume Information")
    {
      if(!tmpFile.rmRfStar())
        Serial.println("FAILED TO REMOVE SVI");
    }
    tmpFile.close();
  }
  tmpFile.close();
  SD.vwd()->rewind();
}

void TC3_Handler() 
{
  Adafruit_ZeroTimer::timerHandler(3);
}

void TimerCallback0(void) //44.1KHz tick
{
  VGMEngine.tick();
}

void pauseISR()
{
  zerotimer.enable(false);
}

void setISR()
{
  //44.1KHz target, actual 44,117Hz
  const uint16_t compare = 1088;
  tc_clock_prescaler prescaler = TC_CLOCK_PRESCALER_DIV1;
  zerotimer.enable(false);
  zerotimer.configure(prescaler,       // prescaler
        TC_COUNTER_SIZE_16BIT,       // bit width of timer/counter
        TC_WAVE_GENERATION_MATCH_PWM // frequency or PWM mode
        );
  zerotimer.setCompare(0, compare);
  zerotimer.setCallback(true, TC_CALLBACK_CC_CHANNEL0, TimerCallback0);
  zerotimer.enable(true);
}

void drawOLEDTrackInfo()
{
  ready = false;
  if(isOledOn)
  {
    oled.setPowerSave(0);
    oled.clearDisplay();
    oled.setFont(u8g2_font_helvR08_te);
    oled.sendBuffer();
    oled.drawStr(0,10, widetochar(VGMEngine.gd3.enTrackName));
    oled.drawStr(0,20, widetochar(VGMEngine.gd3.enGameName));
    oled.drawStr(0,30, widetochar(VGMEngine.gd3.releaseDate));
    oled.drawStr(0,40, widetochar(VGMEngine.gd3.enSystemName));
    String fileNumberData = "File: " + String(currentFileNumber+1) + "/" + String(numberOfFiles);
    char* cstr;
    cstr = &fileNumberData[0u];
    oled.drawStr(0,50, cstr);
    String playmodeStatus;
    if(playMode == LOOP)
      playmodeStatus = "LOOP";
    else if(playMode == SHUFFLE)
      playmodeStatus = "SHUFFLE";
    else
      playmodeStatus = "IN ORDER";
    cstr = &playmodeStatus[0u];
    oled.drawStr(0, 60, cstr);
    oled.sendBuffer();
  }
  else
  {
    oled.clearDisplay();
    oled.setPowerSave(1);
    oled.sendBuffer();
  }
  ready = true;
}

//Mount file and prepare for playback. Returns true if file is found.
bool startTrack(FileStrategy fileStrategy, String request)
{
  pauseISR();
  ready = false;
  File nextFile;
  memset(fileName, 0x00, MAX_FILE_NAME_SIZE);

  switch(fileStrategy)
  {
    case FIRST_START:
    {
      nextFile.openNext(SD.vwd(), O_READ);
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
      currentFileNumber = 0;
    }
    break;
    case NEXT:
    {
      if(currentFileNumber+1 >= numberOfFiles)
      {
          SD.vwd()->rewind();
          currentFileNumber = 0;
      }
      else
          currentFileNumber++;
      nextFile.openNext(SD.vwd(), O_READ);
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
    }
    break;
    case PREV:
    {
      if(currentFileNumber != 0)
      {
        currentFileNumber--;
        SD.vwd()->rewind();
        for(uint32_t i = 0; i<=currentFileNumber; i++)
        {
          nextFile.close();
          nextFile.openNext(SD.vwd(), O_READ);
        }
        nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
        nextFile.close();
      }
      else
      {
        currentFileNumber = numberOfFiles-1;
        SD.vwd()->rewind();
        for(uint32_t i = 0; i<=currentFileNumber; i++)
        {
          nextFile.close();
          nextFile.openNext(SD.vwd(), O_READ);
        }
        nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
        nextFile.close();
      }
    }
    break;
    case RND:
    {
      randomSeed(micros());
      uint32_t randomFile = currentFileNumber;
      if(numberOfFiles > 1)
      {
        while(randomFile == currentFileNumber)
          randomFile = random(numberOfFiles-1);
      }
      currentFileNumber = randomFile;
      SD.vwd()->rewind();
      nextFile.openNext(SD.vwd(), O_READ);
      {
        for(uint32_t i = 0; i<randomFile; i++)
        {
          nextFile.close();
          nextFile.openNext(SD.vwd(), O_READ);
        }
      }
      nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      nextFile.close();
    }
    break;
    case REQUEST:
    {
      request.trim();
      char *cstr = &request[0u]; //Convert to C-string
      if(SD.exists(cstr))
      {
        file.close();
        strcpy(fileName, cstr);
        Serial.println("File found!");
      }
      else
      {
        Serial.println("ERROR: File not found! Continuing with current song.");
        goto fail;
      }
    }
    break;
  }

  if(SD.exists(fileName))
    file.close();
  file = SD.open(fileName, FILE_READ);
  if(!file)
  {
    Serial.println("Failed to read file");
    goto fail;
  }
  else
  {
    delay(100);
    if(VGMEngine.begin(&file))
    {
      printlnw(VGMEngine.gd3.enGameName);
      printlnw(VGMEngine.gd3.enTrackName);
      printlnw(VGMEngine.gd3.enSystemName);
      printlnw(VGMEngine.gd3.releaseDate);
      drawOLEDTrackInfo();
      setISR();
      return true;
    }
    else
    {
      Serial.println("Header Verify Fail");
      goto fail;
    }
  }

  fail:
  setISR();
  return false;
}

//Count at 44.1KHz
void tick()
{
  VGMEngine.tick();
}

//Poll the serial port
void handleSerialIn()
{
  while(Serial.available())
  {
    pauseISR();
    char serialCmd = Serial.read();
    switch(serialCmd)
    {
      case '+':
        startTrack(NEXT);
      break;
      case '-':
        startTrack(PREV);
      break;
      case '*':
        startTrack(RND);
      break;
      case '/':
        playMode = SHUFFLE;
        drawOLEDTrackInfo();
      break;
      case '.':
        playMode = LOOP;
        drawOLEDTrackInfo();
      break;
      case '?':
        printlnw(VGMEngine.gd3.enGameName);
        printlnw(VGMEngine.gd3.enTrackName);
        printlnw(VGMEngine.gd3.enSystemName);
        printlnw(VGMEngine.gd3.releaseDate);
      break;
      case '!':
        isOledOn = !isOledOn;
        drawOLEDTrackInfo();
      break;
      case 'r':
      {
        String req = Serial.readString();
        req.remove(0, 1); //Remove colon character
        startTrack(REQUEST, req);
      }
      break;
      default:
        continue;
    }
  }
  Serial.flush();
  setISR();
}

//Check for button input
bool buttonLock = false;
void handleButtons()
{
//Direct IO examples for reading pins
//if (REG_PORT_IN0 & PORT_PA19)  // if (digitalRead(12) == HIGH)
//if (!(REG_PORT_IN0 | ~PORT_PA19)) // if (digitalRead(12) == LOW) FOR NON-PULLED PINS!
//if(!(REG_PORT_IN1 & next_btn)) //LOW for PULLED PINS

  bool togglePlaymode = false;
  uint32_t count = 0;

  if(!(REG_PORT_IN1 & next_btn)) //Check if buttons are LOW indicating that they are pressed
    startTrack(NEXT);            //Remember, these button pins have pullups enabled on them
  if(!(REG_PORT_IN1 & prev_btn))
    startTrack(PREV);
  if(!(REG_PORT_IN1 & rand_btn))
    startTrack(RND);
  if(!(REG_PORT_IN1 & option_btn))
    togglePlaymode = true;
  else
    buttonLock = false;
  while(!(REG_PORT_IN1 & option_btn))
  {
    pauseISR();
    if(count >= 100) 
    {
      //toggle OLED after one second of holding OPTION button
      isOledOn = !isOledOn;
      drawOLEDTrackInfo();
      togglePlaymode = false;
      buttonLock = true;
      break;
    } 
    delay(10);
    count++;
  }
  if(buttonLock)
  {
    togglePlaymode = false;
    setISR();
  }

  if(togglePlaymode)
  {
    togglePlaymode = false;
    if(playMode == SHUFFLE)
      playMode = LOOP;
    else if(playMode == LOOP)
      playMode = IN_ORDER;
    else if(playMode == IN_ORDER)
      playMode = SHUFFLE;

    if(playMode == LOOP)
    {
      VGMEngine.maxLoops = 0xFFFF;
    }
    else
    {
      VGMEngine.maxLoops = maxLoops;
    }
    
    drawOLEDTrackInfo();
    setISR();
  }
}

void loop()
{    
  while(!VGMEngine.play()) //needs to account for LOOP playmode
  {
    if(Serial.available() > 0)
      handleSerialIn();
    handleButtons();
  }
  //Hit max loops and/or VGM exited
  if(playMode == SHUFFLE)
    startTrack(RND);
  if(playMode == IN_ORDER)
    startTrack(NEXT);
}
