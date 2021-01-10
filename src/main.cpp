//SPI SD CARD LIBRARY WARNING:
//CHIP SELECT FEATURES MANUALLY ADJUSTED IN SDFAT LIB (in SdSpiDriver.h). MUST USE LIB INCLUDED WITH REPO!!!

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SdFat.h"
//#include "U8g2lib.h" //Run this command in the terminal below if PIO asks for a dependancy: pio lib install "U8g2"
#include "menu.h"
#include "menuIO/serialIO.h"
#include "plugin/SdFatMenu.h"
#include "menuIO/U8x8Out.h"
#include "U8x8lib.h"

#include "YM2612.h"
#include "SN76489.h"
#include "Adafruit_ZeroTimer.h"
#include "logo.h"
#include "SpinSleep.h"
#include "SerialUtils.h"
#include "clocks.h"

extern "C" {
  #include "trngFunctions.h" //True random number generation
}

#include "VGMEngine.h"

const uint32_t MANIFEST_MAGIC = 0x12345678;

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
void CreateManifest();
bool startTrack(FileStrategy fileStrategy, String request = "");
bool vgmVerify();
uint32_t freeKB();
uint8_t VgmCommandLength(uint8_t Command);
uint32_t readFile32(FatFile *f);
uint16_t parseVGM();
String GetPathFromManifest(uint32_t index);

Adafruit_ZeroTimer zerotimer = Adafruit_ZeroTimer(3);

Bus bus(0, 1, 8, 9, 11, 10, 12, 13);

YM2612 opn(&bus, 3, NULL, 6, 4, 5, 7);
SN76489 sn(&bus, 2);

//SD & File Streaming
SdFat SD;
File file, manifest;
#define MAX_FILE_NAME_SIZE 128
char fileName[MAX_FILE_NAME_SIZE];
uint32_t numberOfFiles = 0;
uint32_t numberOfDirectories = 0;
uint32_t currentFileNumber = 0;

//OLED & Menus
#define U8_Width 128
#define U8_Height 64
U8X8_SH1106_128X64_NONAME_HW_I2C u8x8(U8X8_PIN_NONE);
const char* constMEM hexDigit MEMMODE="0123456789ABCDEF";
const char* constMEM hexNr[] MEMMODE={"0","x",hexDigit,hexDigit};
using namespace Menu;
char buf1[]="0x11";//<-- menu will edit this text
using namespace Menu;
result filePick(eventMask event, navNode& nav, prompt &item);
SDMenuT<CachedFSO<SdFat,32>> filePickMenu(SD,"Music","/",filePick,enterEvent);
#define MAX_DEPTH 2
MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
  ,SUBMENU(filePickMenu)
  ,OP("Something else...",doNothing,noEvent)
  ,EXIT("<Back")
);

MENU_OUTPUTS(out,MAX_DEPTH
  ,U8X8_OUT(u8x8,{0,0,16,8}) //0,0 Char# x y
  //,SERIAL_OUT(Serial)
  ,NONE//must have 2 items at least
);
serialIn serial(Serial);
NAVROOT(nav,mainMenu,MAX_DEPTH,serial,out);

//U8G2_SH1106_128X64_NONAME_F_HW_I2C oled(U8G2_R0);

bool isOledOn = true;

//Buttons
const int prev_btn = PORT_PB00;
const int rand_btn = PORT_PB01;
const int next_btn = PORT_PB04;
const int option_btn = PORT_PB05;
const int select_btn = PORT_PB09;

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
  //COM
  Wire.begin();
  Wire.setClock(400000L);
  SPI.begin();
  Serial.begin(115200);

  si5351.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  si5351.drive_strength(SI5351_CLK0, SI5351_DRIVE_4MA);
  si5351.drive_strength(SI5351_CLK1, SI5351_DRIVE_4MA);
  si5351.set_freq(NTSC_YMCLK*100ULL, SI5351_CLK0); //CLK0 YM
  si5351.set_freq(NTSC_COLORBURST*100ULL, SI5351_CLK1); //CLK1 PSG - VALUES IN 0.01Hz

  //RNG
  trngInit();
  randomSeed(trngGetRandomNumber());

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

  //Set Chips
  VGMEngine.ym2612 = &opn;
  VGMEngine.sn76489 = &sn;

  //u8g2 OLED
  u8x8.begin();
  u8x8.setBusClock(600000); //Overclock display, should only be 400KHz max, but we're little stinkers
  u8x8.setFont(u8x8_font_torussansbold8_r);

  //OLED
  // oled.begin();
  // oled.setFont(u8g2_font_fub11_tf);
  // oled.drawXBM(0,0, logo_width, logo_height, logo);
  // oled.sendBuffer();
  // delay(3000);
  // oled.clearDisplay();

  //SD
  REG_PORT_DIRSET0 = PORT_PA15; //Set PA15 to output
  if(!SD.begin(PORT_PA15, SPI_FULL_SPEED))
  {
    Serial.println("SD Mount Failed!");
    // oled.clearBuffer();
    // oled.drawStr(0,16,"SD Mount");
    // oled.drawStr(0,32,"failed!");
    // oled.sendBuffer();
    while(true){Serial.println("SD MOUNT FAILED"); delay(1000);}
  }
  filePickMenu.begin();
  nav.useAccel=true;

  Serial.flush();

  //Prepare files
  removeMeta();
  CreateManifest();

  //Begin
  startTrack(FIRST_START);
}

uint32_t freeKB()
{
  uint32_t kb = SD.vol()->freeClusterCount();
  kb *= SD.vol()->blocksPerCluster()/2;
  return kb;
}

String GetPathFromManifest(uint32_t index) //Gives a VGM file path back from the manifest file
{
  String selection;
  manifest.seek(0);
  manifest.open("MANIFEST", O_READ);
  manifest.readStringUntil('\n'); //Skip machine generated preamble
  uint32_t i = 0;
  while(true) //byte-wise string reads for bulk of seeking to be a little nicer to the RAM
  {           //This part just skips every entry until we arrive to the line we want
    if(i == index)
      break;
    if(manifest.read() == '\n')
      i++;
    if(i > numberOfFiles)
      return "ERROR";
  }
  selection = manifest.readStringUntil('\n');
  selection.replace(String(index) + ":", "");
  return selection;
}

uint32_t readFile32(FatFile *f)
{
  uint32_t d = 0;
  uint8_t v0 = f->read();
  uint8_t v1 = f->read();
  uint8_t v2 = f->read();
  uint8_t v3 = f->read();
  d = uint32_t(v0 + (v1 << 8) + (v2 << 16) + (v3 << 24));
  return d;
}

void CreateManifest()
{
  //Manifest format
  //Preamble (String)
  //<file paths>(Strings)
  //...
  //Magic Number (uint32_t BIN)
  //Total # files (uint32_t BIN)
  //Last SD Free Space in KB (uint32_t BIN)

  FatFile d, f;
  String path = "";
  char name[MAX_FILE_NAME_SIZE];
  Serial.println("Checking file manifest...");
rebuild:
  if(!SD.exists("MANIFEST"))
  {
    manifest.open("MANIFEST", O_RDWR | O_CREAT);
    const uint32_t empty = 0x0;
    manifest.write(&MANIFEST_MAGIC, 4); //Magic #
    manifest.write(&empty, 4); //Total # files
    manifest.write(&empty, 4); //Last free space
    manifest.close();
  }

  manifest.open("MANIFEST", O_READ);

  manifest.seekEnd(-12); //Verify magic number to make sure file isn't completely corrupted
  if(readFile32(&manifest) != MANIFEST_MAGIC)
  {
    Serial.println("MANIFEST MAGIC BAD!");
    manifest.remove();
    manifest.close();
    goto rebuild;
  }
  else
  {
    Serial.println("MANIFEST MAGIC OK");
  }
  
  manifest.seekEnd(-8);
  numberOfFiles = readFile32(&manifest);

  manifest.seekEnd(-4); //Read-in old manifest size
  uint32_t prevKB = readFile32(&manifest);

  // Serial.print("Free: "); Serial.println(freeKB());
  // Serial.print("File: "); Serial.println(prevKB);

  if(prevKB != freeKB()) //Used space mismatch. Most likely files have changed.
  {
    numberOfFiles = 0;
    Serial.println("File changes detected! Re-indexing. Please wait...");
    manifest.close();
    manifest.remove();
    manifest.open("MANIFEST", O_RDWR );
    manifest.println("MACHINE GENERATED FILE. DO NOT MODIFY");
    while(d.openNext(SD.vwd(), O_READ)) //Go through root directories
    {
      if(d.isDir() && !d.isRoot()) //Include all dirs except root
      {
        d.getName(name, MAX_FILE_NAME_SIZE);
        path = String(name);
        numberOfDirectories++;
        while(f.openNext(&d, O_READ)) //Once you're in a dir, go through each file and record them
        {
          f.getName(name, MAX_FILE_NAME_SIZE);
          //Serial.println(path + "/" + String(name)); //Replace with manifest file right
          manifest.print(numberOfFiles++);
          manifest.print(":");
          manifest.println(path + "/" + String(name));
          f.close();
        } 
      }
      d.close();
    }
    uint32_t tmp = freeKB();
    manifest.write(&MANIFEST_MAGIC, 4);
    manifest.write(&numberOfFiles, 4);
    manifest.write(&tmp, 4);
  }
  else
    Serial.println("No change in files, continuing...");

  manifest.close();
  Serial.println("Indexing complete");
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
  // ready = false;
  // if(isOledOn)
  // {
  //   oled.setPowerSave(0);
  //   oled.clearDisplay();
  //   oled.setFont(u8g2_font_helvR08_te);
  //   oled.sendBuffer();
  //   oled.drawStr(0,10, widetochar(VGMEngine.gd3.enTrackName));
  //   oled.drawStr(0,20, widetochar(VGMEngine.gd3.enGameName));
  //   oled.drawStr(0,30, widetochar(VGMEngine.gd3.releaseDate));
  //   oled.drawStr(0,40, widetochar(VGMEngine.gd3.enSystemName));
  //   String fileNumberData = "File: " + String(currentFileNumber+1) + "/" + String(numberOfFiles);
  //   char* cstr;
  //   cstr = &fileNumberData[0u];
  //   oled.drawStr(0,50, cstr);
  //   String playmodeStatus;
  //   if(playMode == LOOP)
  //     playmodeStatus = "LOOP";
  //   else if(playMode == SHUFFLE)
  //     playmodeStatus = "SHUFFLE";
  //   else
  //     playmodeStatus = "IN ORDER";
  //   cstr = &playmodeStatus[0u];
  //   oled.drawStr(0, 60, cstr);
  //   oled.sendBuffer();
  // }
  // else
  // {
  //   oled.clearDisplay();
  //   oled.setPowerSave(1);
  //   oled.sendBuffer();
  // }
  // ready = true;
}

//Mount file and prepare for playback. Returns true if file is found.
bool startTrack(FileStrategy fileStrategy, String request)
{
  String filePath = "";

  pauseISR();
  ready = false;
  File nextFile;
  memset(fileName, 0x00, MAX_FILE_NAME_SIZE);

  switch(fileStrategy)
  {
    case FIRST_START:
    {
      // nextFile.openNext(SD.vwd(), O_READ);
      // nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
      // nextFile.close();
      //fileName = GetPathFromManifest().c_str();
      filePath = GetPathFromManifest(0);
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
      uint32_t rng = random(numberOfFiles-1);
      filePath = GetPathFromManifest(rng);
    }
    break;
    case REQUEST:
    {
      request.trim();
      if(SD.exists(request.c_str()))
      {
        file.close();
        filePath = request;
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

  filePath.trim();
  Serial.println(filePath);
  if(SD.exists(filePath.c_str()))
    file.close();
  file = SD.open(filePath.c_str(), FILE_READ);
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
  return;
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
    nav.doNav(navCmd(enterCmd));
    //startTrack(NEXT);            //Remember, these button pins have pullups enabled on them
  if(!(REG_PORT_IN1 & prev_btn))
    nav.doNav(navCmd(escCmd));
    //startTrack(PREV);
  if(!(REG_PORT_IN1 & rand_btn))
    nav.doNav(navCmd(upCmd));
    //startTrack(RND);
  if(!(REG_PORT_IN1 & option_btn))
    nav.doNav(navCmd(downCmd));
    //togglePlaymode = true;
  // else
  //   buttonLock = false;
  if(!(REG_PORT_IN1 & select_btn))
    nav.doNav(navCmd(enterCmd));

  // while(!(REG_PORT_IN1 & option_btn)) //Turn off OLED
  // {
  //   pauseISR();
  //   if(count >= 100) 
  //   {
  //     //toggle OLED after one second of holding OPTION button
  //     isOledOn = !isOledOn;
  //     drawOLEDTrackInfo();
  //     togglePlaymode = false;
  //     buttonLock = true;
  //     break;
  //   } 
  //   delay(10);
  //   count++;
  // }
  // if(buttonLock)
  // {
  //   togglePlaymode = false;
  //   setISR();
  // }

  // if(togglePlaymode)
  // {
  //   togglePlaymode = false;
  //   if(playMode == SHUFFLE)
  //     playMode = LOOP;
  //   else if(playMode == LOOP)
  //     playMode = IN_ORDER;
  //   else if(playMode == IN_ORDER)
  //     playMode = SHUFFLE;

  //   if(playMode == LOOP)
  //   {
  //     VGMEngine.maxLoops = 0xFFFF;
  //   }
  //   else
  //   {
  //     VGMEngine.maxLoops = maxLoops;
  //   }
    
  //   drawOLEDTrackInfo();
  //   setISR();
  // }
}

void loop()
{    
  while(!VGMEngine.play()) //needs to account for LOOP playmode
  {
    if(!VGMEngine.isBusy)
      nav.poll();
    if(Serial.available() > 0) //NOTE TO SELF: YOU HAVE PAUSE THIS FUNCTION WITH A RETURN FOR NOW, MAKE SURE TO REENABLE IT!!! IT'S NOT BROKEN YOU BIG GOOF
      handleSerialIn();
    handleButtons();
  }
  //Hit max loops and/or VGM exited
  if(playMode == SHUFFLE)
    startTrack(RND);
  if(playMode == IN_ORDER)
    startTrack(NEXT);
}




//UI

//implementing the handler here after filePick is defined...
result filePick(eventMask event, navNode& nav, prompt &item) 
{
  // switch(event) {//for now events are filtered only for enter, so we dont need this checking
  //   case enterCmd:
      if (nav.root->navFocus==(navTarget*)&filePickMenu) {
        Serial.println();
        Serial.print("selected file:");
        Serial.println(filePickMenu.selectedFile);
        Serial.print("from folder:");
        Serial.println(filePickMenu.selectedFolder);
        startTrack(REQUEST, filePickMenu.selectedFolder+filePickMenu.selectedFile);
      }
  //     break;
  // }
  return proceed;
}