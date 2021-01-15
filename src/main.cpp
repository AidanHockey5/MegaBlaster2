//SPI SD CARD LIBRARY WARNING:
//CHIP SELECT FEATURES MANUALLY ADJUSTED IN SDFAT LIB (in SdSpiDriver.h). MUST USE LIB INCLUDED WITH REPO!!!

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include "SdFat.h"
#include "U8g2lib.h"
#include "menu.h"
#include "menuIO/serialIO.h"
#include "plugin/SdFatMenu.h"
#include <menuIO/u8g2Out.h>
#include "YM2612.h"
#include "SN76489.h"
#include "Adafruit_ZeroTimer.h"
#include "logo.h"
#include "SpinSleep.h"
#include "SerialUtils.h"
#include "clocks.h"
#include "Bounce2.h"
#include "LinkedList.h"

extern "C" {
  #include "trngFunctions.h" //True random number generation
}

#include "VGMEngine.h"

//Debug variables
#define DEBUG true //Set this to true for a detailed printout of the header data & any errored command bytes
#define DEBUG_LED A4
bool commandFailed = false;
uint8_t failedCmd = 0x00;

//Structs
enum FileStrategy {FIRST_START, NEXT, PREV, RND, REQUEST};
enum PlayMode {LOOP, PAUSE, SHUFFLE_ALL, IN_ORDER, SHUFFLE_DIR};
enum MenuState {IN_MENU, IN_VGM};

//Prototypes
void setup();
void loop();
void handleSerialIn();
void tick();
void setISR();
void pauseISR();
void handleButtons();
void prepareChips();
void readGD3();
void drawOLEDTrackInfo();
bool startTrack(FileStrategy fileStrategy, String request = "");
bool vgmVerify();
void showIndexProgressOLED();
uint8_t VgmCommandLength(uint8_t Command);
uint32_t countFilesInDir(String dir); 
uint32_t getFileIndexInDir(String dir, String fname, uint32_t dirSize = 0);
String getFilePathFromCurrentDirFileIndex();
uint32_t readFile32(FatFile *f);
String GetPathFromManifest(uint32_t index);
void getDirIndices(String dir, String fname);
uint32_t freeKB();
void CreateManifest(bool createNew = false);
result filePick(eventMask event, navNode& nav, prompt &item);
result doCreateManifest(); //Required for UI
result onMenuIdle(menuOut& o, idleEvent e);
void removeMeta();
void clearRandomHistory();

const uint32_t MANIFEST_MAGIC = 0x12345678;
#define MANIFEST_FILE_NAME ".MANIFEST"
#define MANIFEST_DIR "_SYS/"
#define MANIFEST_PATH MANIFEST_DIR MANIFEST_FILE_NAME

//SD & File Streaming
SdFat SD;
static File file, manifest;
#define MAX_FILE_NAME_SIZE 128
char fileName[MAX_FILE_NAME_SIZE];
uint32_t numberOfFiles = 0;
uint32_t numberOfDirectories = 0;
String currentDir = "";
uint32_t currentDirFileCount = 0;
uint32_t currentDirFileIndex = 0;
uint32_t currentDirDirCount = 0; //How many directories are IN the current directory, mainly used for root only.

uint32_t dirStartIndex, dirEndIndex, dirCurIndex = 0; //Range inside the manifest file where the current files in the current dir are
LinkedList<int> randList = LinkedList<int>(); //Used to keep a history of file indices when in shuffle mode to allow for forward/backwards playback controls
int randIndex = 0; 

Adafruit_ZeroTimer zerotimer = Adafruit_ZeroTimer(3);

Bus bus(0, 1, 8, 9, 11, 10, 12, 13);

YM2612 opn(&bus, 3, NULL, 6, 4, 5, 7);
SN76489 sn(&bus, 2);

//VGM Variables
uint16_t loopCount = 0;
uint8_t maxLoops = 3;
bool fetching = false;
volatile bool ready = false;
bool samplePlaying = false;
PlayMode playMode = IN_ORDER;
bool doParse = false;

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0);
#define fontName u8g2_font_6x12_mr     
#define fontX 6
#define fontY 14
#define offsetX 0
#define offsetY 0
#define U8_Width 128
#define U8_Height 64
const colorDef<uint8_t> colors[6] MEMMODE={
  {{0,0},{0,1,1}},//bgColor
  {{1,1},{1,0,0}},//fgColor
  {{1,1},{1,0,0}},//valColor
  {{1,1},{1,0,0}},//unitColor
  {{0,1},{0,0,1}},//cursorColor
  {{1,1},{1,0,0}},//titleColor
};

using namespace Menu;
SDMenuT<CachedFSO<SdFat,32>> filePickMenu(SD,"Music","/",filePick,enterEvent);

CHOOSE(playMode,modeMenu,"Mode:",doNothing,noEvent,noStyle
  ,VALUE("Loop",PlayMode::LOOP,doNothing,noEvent)
  ,VALUE("In Order",PlayMode::IN_ORDER,doNothing,noEvent)
  ,VALUE("Shuffle All",PlayMode::SHUFFLE_ALL,doNothing,noEvent)
  ,VALUE("Shuffle Dir",PlayMode::SHUFFLE_DIR,doNothing,noEvent)
);

#define MAX_DEPTH 2
MENU(mainMenu,"Main menu",doNothing,noEvent,wrapStyle
  ,SUBMENU(filePickMenu)
  ,SUBMENU(modeMenu)
  ,OP("Rebuild Manifest",doCreateManifest,enterEvent)
  //,EXIT("<Back")
);

MENU_OUTPUTS(out,MAX_DEPTH
  ,U8G2_OUT(u8g2,colors,fontX,fontY,offsetX,offsetY,{0,0,U8_Width/fontX,U8_Height/fontY})
  //,U8X8_OUT(u8x8,{0,0,16,8}) //0,0 Char# x y
  //,SERIAL_OUT(Serial)
  ,NONE//must have 2 items at least
);
serialIn serial(Serial);
NAVROOT(nav,mainMenu,MAX_DEPTH,serial,out);

MenuState menuState = IN_MENU;

bool isOledOn = true;

//Buttons
const int prev_btn = 47;    //PORT_PB00;
const int rand_btn = 48;    //PORT_PB01;
const int next_btn = 49;    //PORT_PB04;
const int option_btn = 50;  //PORT_PB05;
const int select_btn = 19;  //PORT_PB09;
Bounce buttons[5];

//Counters
uint32_t bufferPos = 0;
uint32_t cmdPos = 0;
uint16_t waitSamples = 0;
uint32_t pcmBufferPosition = 0;

void setup()
{
  //COM
  Wire.begin();
  Wire.setClock(600000L);
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
  for(uint8_t i = 0; i<5; i++)
  {
    buttons[i] = Bounce();
    buttons[i].interval(25);
  }
  buttons[0].attach(next_btn, INPUT_PULLUP);
  buttons[1].attach(prev_btn, INPUT_PULLUP);
  buttons[2].attach(option_btn, INPUT_PULLUP);
  buttons[3].attach(select_btn, INPUT_PULLUP);
  buttons[4].attach(rand_btn, INPUT_PULLUP);

  //Set Chips
  VGMEngine.ym2612 = &opn;
  VGMEngine.sn76489 = &sn;

  opn.reset();
  sn.reset();

  //u8g2 OLED
  u8g2.begin();
  u8g2.setBusClock(600000);
  u8g2.setFont(fontName);

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
    u8g2.clearBuffer();
    u8g2.drawStr(0,16,"SD Mount");
    u8g2.drawStr(0,32,"failed!");
    u8g2.sendBuffer();
    while(true){Serial.println("SD MOUNT FAILED"); delay(1000);}
  }
  filePickMenu.begin();
  nav.useAccel=true;

  Serial.flush();

  //Prepare files
  removeMeta();
  CreateManifest();

  nav.timeOut=5;
  nav.idleTask = onMenuIdle;
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
  if(isOledOn)
  {
    u8g2.clearBuffer();
    u8g2.setDrawColor(1);
    u8g2.setPowerSave(0);
    u8g2.clearDisplay();
    u8g2.setFont(u8g2_font_helvR08_tr);
    u8g2.sendBuffer();
    u8g2.drawStr(0,10, widetochar(VGMEngine.gd3.enTrackName));
    u8g2.drawStr(0,20, widetochar(VGMEngine.gd3.enGameName));
    u8g2.drawStr(0,30, widetochar(VGMEngine.gd3.releaseDate));
    u8g2.drawStr(0,40, widetochar(VGMEngine.gd3.enSystemName));
    char* cstr;
    String playmodeStatus;
    if(playMode == LOOP)
      playmodeStatus = "LOOP";
    else if(playMode == SHUFFLE_ALL)
      playmodeStatus = "SHUFFLE ALL";
    else if(playMode == SHUFFLE_DIR)
      playmodeStatus = "SHUFFLE DIR";
    else if(playMode == IN_ORDER)
    {
      String fileNumberData = "Track: " + String(dirCurIndex-dirStartIndex) + "/" + String(dirEndIndex-dirStartIndex); 
      cstr = &fileNumberData[0u];
      u8g2.drawStr(0,50, cstr);
      playmodeStatus = "IN ORDER";
    }
    cstr = &playmodeStatus[0u];
    u8g2.drawStr(0, 60, cstr);
    u8g2.sendBuffer();
  }
  else
  {
    u8g2.clearDisplay();
    u8g2.setPowerSave(1);
    u8g2.sendBuffer();
  }
  u8g2.setFont(fontName);
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
      filePath = GetPathFromManifest(0);
    }
    break;
    case NEXT:
    {
      if(playMode == IN_ORDER)
      {
        if(dirCurIndex != dirEndIndex)
          dirCurIndex++;
        else
          dirCurIndex = dirStartIndex;
        filePath = GetPathFromManifest(dirCurIndex);
      }
      else if(playMode == SHUFFLE_ALL || SHUFFLE_DIR)
      {
        if(randIndex == randList.size()-1 || randList.size() == 0) //End of random list, generate new random track and add to list
        {
          uint32_t rng = playMode == SHUFFLE_ALL ? random(numberOfFiles-1) : random(dirStartIndex, dirEndIndex+1);
          randList.add(rng);
          randIndex = randList.size()-1;
          filePath = GetPathFromManifest(rng);
        }
        else //Otherwise, move up in history
        {
          randIndex++;
          filePath = GetPathFromManifest(randList.get(randIndex));
        }
        dirCurIndex = randIndex;
      }
    }
    break;
    case PREV:
    {
      if(playMode == IN_ORDER)
      {
        if(dirCurIndex != dirStartIndex)
          dirCurIndex--;
        else
          dirCurIndex = dirEndIndex;
        filePath = GetPathFromManifest(dirCurIndex);
      }
      else if(playMode == SHUFFLE_ALL || playMode == SHUFFLE_DIR)
      {
        if(randIndex != 0) //If you're not at the end of the history list, go back in history
        {
          randIndex--;
        }
        filePath = GetPathFromManifest(randList.get(randIndex));
        dirCurIndex = randIndex;
      }
    }
    break;
    case RND:
    { //This request will disrupt the random history and immediatly create a new node at the end of the random history list
      playMode = SHUFFLE_ALL; 
      uint32_t rng = random(numberOfFiles-1);
      randList.add(rng);
      randIndex = randList.size()-1;
      filePath = GetPathFromManifest(rng);
      dirCurIndex = rng;
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
      if(menuState == IN_VGM)
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
        if(playMode == IN_ORDER || playMode == SHUFFLE_ALL)
          startTrack(NEXT);
        // else if(playMode == SHUFFLE_ALL)
        //   startTrack(RND);
      break;
      case '-':
        if(playMode == IN_ORDER || playMode == SHUFFLE_ALL)
          startTrack(PREV);
      break;
      case '*':
        startTrack(RND);
      break;
      case '/':
        playMode = SHUFFLE_ALL;
        //drawOLEDTrackInfo();
      break;
      case '.':
        playMode = LOOP;
        //drawOLEDTrackInfo();
      break;
      case '?':
        printlnw(VGMEngine.gd3.enGameName);
        printlnw(VGMEngine.gd3.enTrackName);
        printlnw(VGMEngine.gd3.enSystemName);
        printlnw(VGMEngine.gd3.releaseDate);
      break;
      case '!':
        isOledOn = !isOledOn;
        //drawOLEDTrackInfo();
      break;
      case 'r':
      {
        String req = Serial.readString();
        req.remove(0, 1); //Remove colon character
        startTrack(REQUEST, req);
      }
      break;
      // case 'x': //Just used for debugging
      // {
      //   startTrack(REQUEST, GetPathFromManifest(2040));
      // }
      // break;
      default:
        continue;
    }
  }
  Serial.flush();
  setISR();
}

void clearRandomHistory()
{
  randList.clear();
  randIndex = 0;
}

//Returns the number of files in a directory. Note that dirs will also increment the index
uint32_t countFilesInDir(String dir) 
{
  SD.chdir(dir.c_str());
  File countFile;
  uint32_t count = 0;
  currentDirDirCount = 0;
  char firstName[MAX_FILE_NAME_SIZE] = "";
  char curName[MAX_FILE_NAME_SIZE] = "";
  while (countFile.openNext(SD.vwd(), O_READ))
  {
    if(currentDirFileCount == 0) //Get the name of the first file in the dir
      countFile.getName(firstName, MAX_FILE_NAME_SIZE);
    else
    {
      countFile.getName(curName, MAX_FILE_NAME_SIZE);
      if(strcmp(curName, firstName) == 0) //If the current file name is the same as the first, we've looped in our dir and we can exit
      {
        countFile.close();
        SD.chdir("/"); //Go back to root
        return count;
      }
    }
    if(countFile.isDir())
      currentDirDirCount++;
    count++;
    countFile.close();
  }
  SD.chdir("/");
  return count;
  //If the dir is empty, count will be 0
}

void getDirIndices(String dir, String fname)
{
  fname += '\r'; //stupid invisible carriage return
  if(dir.startsWith("/"))
    dir.replace("/", "");
  if(dir == "")
    dir = "~/";
  // Serial.print("INCOMING DIR: "); Serial.println(dir);
  // Serial.print("INCOMING FNAME: "); Serial.println(fname);
  dirStartIndex = 0xFFFFFFFF;
  dirEndIndex = 0; 
  dirCurIndex = 0;
  manifest.open(MANIFEST_PATH, O_READ);
  manifest.seek(0);
  manifest.readStringUntil('\n'); //Skip machine generated preamble
  for(uint32_t i = 0; i<numberOfFiles; i++)
  {
    String cur = manifest.readStringUntil('\n');
    cur.replace(String(i)+":", "");
    if(cur.startsWith(dir))
    {
      if(dirStartIndex == 0xFFFFFFFF)
        dirStartIndex = i;
      if(cur.endsWith(fname)) 
        dirCurIndex = i;
      dirEndIndex = i;
    }
    else if(dirStartIndex != 0xFFFFFFFF)
      break;
  }
  if(dirStartIndex == 0xFFFFFFFF)
    dirStartIndex = 0;
  // Serial.print("START: "); Serial.println(dirStartIndex);
  // Serial.print("CURRENT: "); Serial.println(dirCurIndex);
  // Serial.print("END: "); Serial.println(dirEndIndex);
}

//Get the file's index inside of a dir. If you pass 0, this function will count the files in the dir, otherwise, you can specify a dir size in advance if you've already ran "countFilesInDir()" to save time
uint32_t getFileIndexInDir(String dir, String fname, uint32_t dirSize)
{
  SD.chdir(dir.c_str());
  if(dirSize == 0)
    dirSize = countFilesInDir(dir);
  File countFile;
  char curName[MAX_FILE_NAME_SIZE] = "";
  char searchName[MAX_FILE_NAME_SIZE] = "";
  fname.toCharArray(searchName, MAX_FILE_NAME_SIZE);
  for(uint32_t i = 0; i<dirSize; i++)
  {
    countFile.openNext(SD.vwd(), O_READ);
    countFile.getName(curName, MAX_FILE_NAME_SIZE);
    if(strcmp(curName, searchName) == 0)
    {
        countFile.close();
        SD.chdir("/");
        return i;
    }
    countFile.close();
  }
  SD.chdir("/");
  countFile.close();
  return 0xFFFFFFFF; //Int max = error
}

// String getFilePathFromCurrentDirFileIndex()
// {
//   uint32_t index = 0;
//   File nextFile;
//   SD.chdir(currentDir.c_str()); //Make sure the system is on the same dir we are
//   memset(fileName, 0x00, MAX_FILE_NAME_SIZE);
//   while(nextFile.openNext(SD.vwd(), O_READ))
//   {
//     if(index == currentDirFileIndex) //This set of ifs is to account for dirs that might be in the root directory. We need to skip over those, so we'll just push the current index further if we encounter a dir
//     {
//       if(nextFile.isDir())
//       {
//         if(currentDirFileIndex+1 >= currentDirFileCount)
//         {
//           currentDirFileIndex = 0;
//         }
//         else
//           currentDirFileIndex++;
//       }
//       else
//         break; //Found the correct file
//     }
//     index++;
//     nextFile.close(); 
//   }

//   nextFile.getName(fileName, MAX_FILE_NAME_SIZE);
//   nextFile.close();
//   return currentDir + String(fileName);
// }

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

String GetPathFromManifest(uint32_t index) //Gives a VGM file path back from the manifest file
{
  String selection;
  manifest.open(MANIFEST_PATH, O_READ);
  manifest.seek(0);
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
  selection.replace("~/", ""); //for root dirs
  SD.chdir("/");
  return selection;
}

uint32_t freeKB()
{
  uint32_t kb = SD.vol()->freeClusterCount();
  kb *= SD.vol()->blocksPerCluster()/2;
  return kb;
}

void CreateManifest(bool createNew)
{
  //Manifest format
  //Preamble (String)
  //<file paths>(Strings)
  //...
  //Magic Number (uint32_t BIN)
  //Total # files (uint32_t BIN)
  //Last SD Free Space in Blocks (uint32_t BIN)
  u8g2.clearBuffer();
  u8g2.setDrawColor(1);
  u8g2.drawStr(0,16,"Indexing files");
  u8g2.drawStr(0,32,"Please wait...");
  u8g2.sendBuffer();
  FatFile d, f;
  uint32_t prevBlocks;
  String path = "";
  char name[MAX_FILE_NAME_SIZE];
  Serial.println("Checking file manifest...");
  bool createNewManifest = createNew;

  SD.chdir("/");
  if(!SD.exists(MANIFEST_PATH))
    createNewManifest = true;
  else
  {
    manifest.open(MANIFEST_PATH, O_READ | O_WRITE);
    manifest.seekEnd(-12); //Verify magic number to make sure file isn't completely corrupted
    if(readFile32(&manifest) != MANIFEST_MAGIC)
    {
      Serial.println("MANIFEST MAGIC BAD!");
      createNewManifest = true;
    }
    else
    {
      Serial.println("MANIFEST MAGIC OK");
      manifest.seekEnd(-8);
      numberOfFiles = readFile32(&manifest);
      Serial.println(numberOfFiles);
      manifest.seekEnd(-4); //Read-in old manifest size
      prevBlocks = readFile32(&manifest);
      if(prevBlocks != SD.vol()->freeClusterCount())
        createNewManifest = true;
    }
  }

  if(createNewManifest)
  {
    if(!manifest.remove())
    {
      if(SD.exists(MANIFEST_PATH))
        Serial.println("Failed to remove old file");
    }
    if(manifest.isOpen())
      manifest.close();
    manifest.open(MANIFEST_PATH, O_RDWR | O_CREAT);
    numberOfFiles = 0;
    prevBlocks = 0;

    u8g2.drawStr(0,48,"Changes Detected...");
    u8g2.drawStr(0,64,"Rebuilding Manifest...");
    u8g2.sendBuffer();
    Serial.println("File changes detected! Re-indexing. Please wait...");
    manifest.seek(0);
    manifest.println("MACHINE GENERATED FILE. DO NOT MODIFY");
    while(d.openNext(SD.vwd(), O_READ)) //Go through root directories
    {
      if(d.isDir() && !d.isRoot()) //Include all dirs except root
      {
        d.getName(name, MAX_FILE_NAME_SIZE);
        path = String(name);
        if(path == MANIFEST_DIR) //Ignore the system file holding the manifest
          continue;
        numberOfDirectories++;
        while(f.openNext(&d, O_READ)) //Once you're in a dir, go through each file and record them
        {
          f.getName(name, MAX_FILE_NAME_SIZE);
          // if(strcmp(name, MANIFEST_FILE_NAME) == 0)
          //   continue;
          //Serial.println(path + "/" + String(name)); //Replace with manifest file right
          manifest.print(numberOfFiles++);
          manifest.print(":");
          manifest.println(path + "/" + String(name));
          f.close();
        } 
      }
      // else //Get any files in the root dir here
      // {
        // d.getName(name, MAX_FILE_NAME_SIZE);
        // manifest.print(numberOfFiles++);
        // manifest.print(":");
        // manifest.println(String(name));
      // }
      d.close();
    }
    SD.vwd()->rewind();
    while(d.openNext(SD.vwd(), O_READ)) //Handle files in root separatly at the end to keep them in order
    {
      if(!d.isDir() && !d.isRoot())
      {
        d.getName(name, MAX_FILE_NAME_SIZE);
        manifest.print(numberOfFiles++);
        manifest.print(":~/");
        manifest.println(String(name));
      }
      d.close();
    }
    manifest.close();
    manifest.open(MANIFEST_PATH, O_AT_END | O_WRITE);
    uint32_t tmp = SD.vol()->freeClusterCount();
    manifest.write(&MANIFEST_MAGIC, 4);
    manifest.write(&numberOfFiles, 4);
    manifest.write(&tmp, 4);
  }
  else
    Serial.println("No change in files, continuing...");

  manifest.close();
  Serial.println("Indexing complete");
  u8g2.clearDisplay();
  u8g2.sendBuffer();
  nav.refresh();
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

void loop()
{    
  switch(VGMEngine.play())
  {
    case VGMEngineState::IDLE:
    break;
    case VGMEngineState::END_OF_TRACK:
      if(playMode == IN_ORDER || playMode == SHUFFLE_ALL || playMode == SHUFFLE_DIR)
        startTrack(NEXT);
    break;
    case VGMEngineState::PLAYING:
    break;
  }

  if(Serial.available() > 0) 
    handleSerialIn();

  //Debounced buttons
  for(uint8_t i = 0; i<5; i++)
  {
    buttons[i].update();
  }

  if(buttons[0].fell()) //Next
  {
    if(menuState == IN_MENU)
      nav.doNav(navCmd(enterCmd));
    else if(menuState == IN_VGM)
    {
      if(playMode == IN_ORDER || playMode == SHUFFLE_ALL || playMode == SHUFFLE_DIR)
        startTrack(NEXT);
      // else if(playMode == SHUFFLE_ALL)
      //   startTrack(RND);
    }
  }
  if(buttons[1].fell()) //Prev
  {
    if(menuState == IN_MENU)
        nav.doNav(navCmd(escCmd));
    else if(menuState == IN_VGM)
    {
      if(playMode == IN_ORDER || playMode == SHUFFLE_ALL || playMode == SHUFFLE_DIR)
        startTrack(PREV);
    }
  }
  if(buttons[2].fell()) //Option
  {
    if(menuState == IN_MENU)
      nav.doNav(navCmd(downCmd));
  }
  if(buttons[3].fell())                         //Select
    {
      if(menuState == IN_MENU) //If you're in the file-picker, the select button is used to enter dirs
        nav.doNav(navCmd(enterCmd)); 
      else if(menuState == IN_VGM) //Otherwise, you can use the select key to go back to the file picker where you left off
      {
        nav.refresh();
        menuState = IN_MENU;
      }
    }
  if(buttons[4].fell())//Rand
  {
    if(menuState == IN_MENU)
      nav.doNav(navCmd(upCmd));
  }

  //UI
  if (nav.changed(0) && menuState == IN_MENU) {//only draw if menu changed for gfx device
    //change checking leaves more time for other tasks
    u8g2.firstPage();
    do nav.doOutput(); while(u8g2.nextPage());
  }
}




//UI

//implementing the handler here after filePick is defined...
result filePick(eventMask event, navNode& nav, prompt &item) 
{
  // switch(event) {//for now events are filtered only for enter, so we dont need this checking
  //   case enterCmd:
      if (nav.root->navFocus==(navTarget*)&filePickMenu) {
        // Serial.println();
        // Serial.print("selected file:");
        // Serial.println(filePickMenu.selectedFile);
        // Serial.print("from folder:");
        // Serial.println(filePickMenu.selectedFolder);
        if(filePickMenu.selectedFile == MANIFEST_FILE_NAME)
          return proceed;
        menuState = IN_VGM;
        if(filePickMenu.selectedFolder != currentDir)
        {
          currentDir = filePickMenu.selectedFolder;
          if(playMode == SHUFFLE_DIR || playMode == SHUFFLE_ALL)
            clearRandomHistory();
        }
        getDirIndices(filePickMenu.selectedFolder, filePickMenu.selectedFile);
        if(playMode == SHUFFLE_DIR || playMode == SHUFFLE_ALL)
        {
          randList.add(dirCurIndex);
          randIndex++;
        }
        startTrack(REQUEST, filePickMenu.selectedFolder+filePickMenu.selectedFile);
      }
  return proceed;
}

result doCreateManifest()
{
  CreateManifest(true);
  return proceed;
}

result onMenuIdle(menuOut& o,idleEvent e) 
{
  if (e==idling && VGMEngine.state == PLAYING) 
  {
    switch(e)
    {
      case idleStart:
        {drawOLEDTrackInfo(); Serial.println("Idle Start"); }
      break;
      case idling:
      break;
      case idleEnd:
      break;
    }
  }
  return proceed;
}

  //Handy old code

  //Direct IO examples for reading pins
  //if (REG_PORT_IN0 & PORT_PA19)  // if (digitalRead(12) == HIGH)
  //if (!(REG_PORT_IN0 | ~PORT_PA19)) // if (digitalRead(12) == LOW) FOR NON-PULLED PINS!
  //if(!(REG_PORT_IN1 & next_btn)) //LOW for PULLED PINS

  // //Group 1 is port B, PINCFG just wants the literal pin number on the port, DIRCLR and OUTSET want the masked PORT_XX00 *not* the PIN_XX00
  // //Prev button input pullup
  // PORT->Group[1].PINCFG[0].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN);
  // PORT->Group[1].DIRCLR.reg = PORT_PB00;
  // PORT->Group[1].OUTSET.reg = PORT_PB00;

  // //Rand button input pullup
  // PORT->Group[1].PINCFG[1].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN);
  // PORT->Group[1].DIRCLR.reg = PORT_PB01;
  // PORT->Group[1].OUTSET.reg = PORT_PB01;

  // //Next button input pullup
  // PORT->Group[1].PINCFG[4].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN);
  // PORT->Group[1].DIRCLR.reg = PORT_PB04;
  // PORT->Group[1].OUTSET.reg = PORT_PB04;

  // //Option button input pullup
  // PORT->Group[1].PINCFG[5].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN); 
  // PORT->Group[1].DIRCLR.reg = PORT_PB05;
  // PORT->Group[1].OUTSET.reg = PORT_PB05;

  // //Select button input pullup
  // PORT->Group[1].PINCFG[9].reg=(uint8_t)(PORT_PINCFG_INEN|PORT_PINCFG_PULLEN); 
  // PORT->Group[1].DIRCLR.reg = PORT_PB09;
  // PORT->Group[1].OUTSET.reg = PORT_PB09;