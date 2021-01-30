#include "VGMEngine.h"

VGMEngineClass::VGMEngineClass()
{
    MegaStream_Create(&stream, buf, VGM_BUF_SIZE);
}
VGMEngineClass::~VGMEngineClass(){}

void VGMEngineClass::ramtest() //Tests 256 bytes of external RAM
{
    const uint32_t ram_test_size = 0xFF;
    bool pass = false;
    ram.Init();
    for(uint32_t i = 0; i<ram_test_size; i++)
        ram.WriteByte(i, i);
    for(uint32_t i = 0; i<ram_test_size; i++)
    {
        Serial.println(ram.ReadByte(i));
        if(ram.ReadByte(i) != i)
            pass = false;
        else
            pass = true;
    }
    pass == true ? Serial.println("RAM CHECK PASS") : Serial.println("RAM CHECK FAIL!!!");
}

bool VGMEngineClass::begin(File *f)
{
    ready = false;
    file = f;
    if(!header.read(file))
    {
        state = ERROR;
        return false;
    }
    gd3.read(file, header.gd3Offset+0x14);
    

    if(header.vgmDataOffset == 0)
        file->seek(0x40);
    else
        file->seek(header.vgmDataOffset+0x34);
    
    if(header.gd3Offset != 0)
    {
        loopPos = header.gd3Offset+0x14-1;
    }
    else
    {
        loopPos = header.EoF+4-1;
    }
    stopDacStreamTimer();
    chipSetup();
    #if ENABLE_SPIRAM
        ram.Init();
    #endif
    resetDataBlocks();
    dacSampleReady = false;
    activeDacStreamBlock = 0xFF;
    storePCM();
    pcmBufferPosition = 0;
    waitSamples = 0;
    loopCount = 0;
    badCommandCount = 0;
    dacSampleCountDown = 0;
    MegaStream_Reset(&stream);
    load();
    state = PLAYING;
    ready = true;
    return true;
}

void VGMEngineClass::resetDataBlocks()
{
    memset(dataBlocks, 0, sizeof(dataBlocks));
}

uint8_t empty = 0;
uint8_t VGMEngineClass::readBufOne()
{
    if(MegaStream_Used(&stream) < 1)
    {
        //digitalWrite(PA8, HIGH);
        load();
        empty = 1;
    }
    uint8_t b[1];
    MegaStream_Recv(&stream, b, 1);
    return b[0];
}

uint16_t VGMEngineClass::readBuf16()
{
    if(MegaStream_Used(&stream) < 2)
    {
        //digitalWrite(PA8, HIGH);
        load();
        empty = 16;
    }
    uint16_t d;
    uint8_t b[2];
    MegaStream_Recv(&stream, b, 2);
    d = uint16_t(b[0] + (b[1] << 8));
    return d;
}

uint32_t VGMEngineClass::readBuf32()
{
    if(MegaStream_Used(&stream) < 4)
    {
        //digitalWrite(PA8, HIGH);
        load();
        empty = 32;
    }
    uint32_t d;
    uint8_t b[4];
    MegaStream_Recv(&stream, b, 4);
    d = uint32_t(b[0] + (b[1] << 8) + (b[2] << 16) + (b[3] << 24));
    return d;
}

bool VGMEngineClass::topUp()
{
    if(MegaStream_Free(&stream) == 0)
        return true;
    byte b = file->read();
    MegaStream_Send(&stream, &b, 1);
    return false;
}

bool VGMEngineClass::storePCM(bool skip)
{
    bool isPCM = false;
    uint8_t openSlot = 0;
    uint32_t curChunk = 0;
    uint32_t count = 0;
    pcmBufferEndPosition = 0;
    while(file->peek() == 0x67) //PCM Block
    {
        for(uint8_t i = 0; i<MAX_DATA_BLOCKS_IN_BANK; i++)
        {
            if(!dataBlocks[i].slotOccupied)
            {
                dataBlocks[i].slotOccupied = true;
                openSlot = i;
                break;
            }
        }
        isPCM = true;
        file->read(); //0x67
        file->read(); //0x66
        file->read(); //datatype
        file->read(&dataBlocks[openSlot].DataLength, 4); //PCM chunk size
        //ram.usedBytes+=dataBlocks[openSlot].DataLength;
        dataBlocks[openSlot].absoluteDataStartInBank = pcmBufferEndPosition;
        pcmBufferEndPosition+=dataBlocks[openSlot].DataLength;
        if(skip)            //On loop, you'll want to just skip the PCM block since it's already loaded.
        {
            file->seekCur(dataBlocks[openSlot].DataLength);
            return false;
        }
        if(ram.usedBytes >= MAX_PCM_BUFFER_SIZE)
        {
            state = END_OF_TRACK;
            return true; //todo: Error out here or go to next track. return is temporary
        }
        else
        {
            //Using chunks in RAM stream-mode nearly halves loading times compared to byte-by-byte loading at the expense of a slight amount of wasted space
            uint32_t lastBlockEndPos = openSlot == 0 ? 0 : curChunk*STREAM_CHUNK_SIZE;
            uint8_t streamChunk[STREAM_CHUNK_SIZE];
            uint32_t chunksRequired = ceil((double)dataBlocks[openSlot].DataLength / (double)STREAM_CHUNK_SIZE);
            for(uint32_t i = 0; i<chunksRequired; i++)
            {
                file->readBytes(streamChunk, STREAM_CHUNK_SIZE);
                ram.WriteStream(curChunk*STREAM_CHUNK_SIZE, streamChunk, STREAM_CHUNK_SIZE);
                curChunk++;
            }
            count++;
            if(header.vgmDataOffset == 0)
                file->seek(0x40+pcmBufferEndPosition+(7*count)); //7 is for the PCM block header info and datasize and needs to be multiplied by the datablock # (count)
            else
                file->seek(header.vgmDataOffset+0x34+pcmBufferEndPosition+(7*count));

            dataBlocks[openSlot].DataStart = lastBlockEndPos;
            
            
            
            //byte-by-byte
            // uint32_t lastBlockEndPos = openSlot == 0 ? 0 : dataBlocks[openSlot-1].DataStart+dataBlocks[openSlot-1].DataLength;
            // for(uint32_t i = lastBlockEndPos; i<lastBlockEndPos+dataBlocks[openSlot].DataLength; i++)
            // {
            //     ram.WriteByte(i, file->read());
            //     pcmBufferEndPosition++;
            // }
            // dataBlocks[openSlot].DataStart = lastBlockEndPos;
        }
    } 
    
    //Serial.print("Used Bytes: "); Serial.println(ram.usedBytes);
    return isPCM;
}

bool VGMEngineClass::load(bool singleChunk)
{
    const uint16_t MAX_CHUNK_SIZE = 512;
    int32_t space = MegaStream_Free(&stream);
    if(space == 0)
        return true;
    bool didSingleChunk = false;
    uint8_t chunk[MAX_CHUNK_SIZE];
    while(MegaStream_Free(&stream) != 0 || didSingleChunk) //Fill up the entire buffer, or only grab a single chunk quickly
    {
        bool hitLoop = false;
        uint16_t chunkSize = min(space, MAX_CHUNK_SIZE); //Who's smaller, the space left in the buffer or the maximum chunk size?
        
        if(file->position() + chunkSize >= loopPos+1) //Loop code. A bit of math to see where the file pointer is. If it goes over the 0x66 position, we'll set a flag to move the file pointer to the loop point and adjust the chunk as to not grab data past the 0x66
        {
            chunkSize = loopPos+1 - file->position(); //+1 on loopPos is to make sure we include the 0x66 command in the buffer in order for loop-triggered events to work.
            hitLoop = true;
        }

        space -= chunkSize;                 //Reduce space by the chunkSize, then read from the SD card into the chunk. Send chunk to buffer.
        file->read(chunk, chunkSize);
        MegaStream_Send(&stream, chunk, chunkSize); 
        if(hitLoop)                         //Here is where we reset the file pointer back to the loop point
        {
            if(header.loopOffset !=0)
                file->seek(header.loopOffset+0x1C);
            else
            {
                if(header.vgmDataOffset == 0)
                    file->seek(0x40);
                else
                    file->seek(header.vgmDataOffset+0x34);
                storePCM(true);
            }
        }
        if(space <= 0)                      //No more space in the buffer? Just eject.
            return true;
        if(singleChunk)                     //Only want to grab a single chunk instead of filling the entire buffer? Set this flag.
            didSingleChunk = true;
    }
    //NOTE, Loop (0x66) will ALWAYS be (Gd3 offset - 1) OR (EoF offset - 1) if there is no Gd3 data
    return false;
}

void VGMEngineClass::chipSetup()
{
    si5351.reset();
    delay(10);
    #if ENABLE_SN76489
    sn76489->setClock(header.sn76489Clock);
    si5351.set_freq(header.sn76489Clock*SI5351_FREQ_MULT, SI5351_CLK1);
    delay(10);
    sn76489->reset();
    #endif
    #if ENABLE_YM2612
    ym2612->setClock(header.ym2612Clock);
    si5351.set_freq(header.ym2612Clock*SI5351_FREQ_MULT, SI5351_CLK0); //CLK0 YM
    delay(10);
    ym2612->reset();
    #endif
}

void VGMEngineClass::tick44k1()
{
    if(!ready)
        return;
    waitSamples--;      
}

void VGMEngineClass::tickDacStream()
{
    if(!ready)
        return;
    if(activeDacStreamBlock != 0xFF)
        dacSampleCountDown--;
}

VGMEngineState VGMEngineClass::play()
{
    switch(state)
    {
    case IDLE:
        return IDLE;
    break;
    case END_OF_TRACK:
        state = IDLE;
        return END_OF_TRACK;
    break;
    case PLAYING:
        load(); 
        while(dacSampleCountDown <= 0)
        {
            dacSampleReady = false;
            if(dacStreamBufPos+1 < dataBlocks[activeDacStreamBlock].DataStart+dataBlocks[activeDacStreamBlock].DataLength)
                ym2612->write(0x2A, ram.ReadByte(dacStreamBufPos++), 0);
            else if(bitRead(dataBlocks[activeDacStreamBlock].LengthMode, 7)) //Looping length mode defined in 0x93 DAC STREAM
            {
                dacStreamBufPos = dataBlocks[activeDacStreamBlock].DataStart;
            }
            else
                activeDacStreamBlock = 0xFF;
            dacSampleCountDown++;
        }
        while(waitSamples <= 0)
        {
            isBusy = true;
            waitSamples += parseVGM();
        }

        isBusy = false;
        if(loopCount >= maxLoops)
        {
            state = END_OF_TRACK;
        }
        return PLAYING;
    break;
    case ERROR:
        return ERROR;
    break;
    }
}

uint16_t VGMEngineClass::getLoops()
{
    return loopCount;
}

uint16_t VGMEngineClass::parseVGM()
{
    uint8_t cmd;
    while(true)
    {
        cmd = readBufOne();
        switch(cmd)
        {
            case 0x4F:
                sn76489->write(0x06);
                sn76489->write(readBufOne());
            break;
            case 0x50:
                sn76489->write(readBufOne());
            break;
            case 0x52:
                ym2612->write(readBufOne(), readBufOne(), 0);
            break;
            case 0x53:
                ym2612->write(readBufOne(), readBufOne(), 1);
            break;
            case 0x61:
                return readBuf16();
            case 0x62:
                return 735;
            case 0x63:
                return 882;
            case 0x67:
            {
                readBufOne(); //0x67
                readBufOne(); //0x66
                readBufOne(); //datatype
                uint32_t pcmSize = readBuf32();
                Serial.println("CALLED PCM STORE MID STREAM");
                if(pcmSize > MAX_PCM_BUFFER_SIZE)
                {
                    Serial.print("TOO BIG!");
                    return true; //todo: Error out here or go to next track. return is temporary
                }
                else
                {
                    for(uint32_t i = pcmBufferEndPosition; i<pcmSize+pcmBufferEndPosition; i++)
                    {
                        ram.WriteByte(i, readBufOne());
                        pcmBufferEndPosition++;
                    }
                }
                break;
            }
            break;
            case 0x70:
            case 0x71:
            case 0x72:
            case 0x73:
            case 0x74:
            case 0x75:
            case 0x76:
            case 0x77:
            case 0x78:
            case 0x79:
            case 0x7A:
            case 0x7B:
            case 0x7C:
            case 0x7D:
            case 0x7E:
            case 0x7F:
                return (cmd & 0x0F)+1;//+1; Removed +1. Remember, even if you return a 0, there is always going to a latency of at least 1 tick
            case 0x80:
            case 0x81:
            case 0x82:
            case 0x83:
            case 0x84:
            case 0x85:
            case 0x86:
            case 0x87:
            case 0x88:
            case 0x89:
            case 0x8A:
            case 0x8B:
            case 0x8C:
            case 0x8D:
            case 0x8E:
            case 0x8F:
            {
                ym2612->write(0x2A, ram.ReadByte(pcmBufferPosition++), 0);
                uint8_t wait = (cmd & 0x0F);
                if(wait == 0)
                    break;
                else
                    return wait; //Wait -1: Even if you return a 0, every loop has inherent latency of at least 1 tick. This compensates for that.
            }
            case 0xE0:
                pcmBufferPosition = readBuf32();
            break;
            case 0x66:
            {
                //Loop
                if(maxLoops != 0xFFFF) //If sent to short int max, just loop forever
                    loopCount++;
                activeDacStreamBlock = 0xFF;
                return 0;
            }
            case 0x90:
            {
                readBuf16();//skip stream ID and chip type
                dataBlockChipPort = readBufOne();
                dataBlockChipCommand = readBufOne();
                //Serial.print("0x90: ");// Serial.print(streamID, HEX); Serial.print(" "); Serial.print(chiptype, HEX); Serial.print(" "); Serial.print(pp, HEX); Serial.print(" "); Serial.print(cc, HEX); Serial.println("   --- SETUP STREAM CONTROL");
            }
            break;
            case 0x91:
            {
                readBuf16(); //Skip stream ID and Data bank ID. Stream ID will be const and so will data bank if you're just using OPN2/PSG
                dataBlockStepSize = readBufOne();
                dataBlockStepBase = readBufOne();
                //Serial.println("0x91: "); //Serial.print(streamID, HEX); Serial.print(" "); Serial.print(dbID, HEX); Serial.print(" "); Serial.print(ll, HEX); Serial.print(" "); Serial.print(bb, HEX); Serial.println("   --- SET STREAM DATA");
            }
            break;
            case 0x92:
            {
                readBufOne(); //skip stream ID
                uint32_t streamFrq = readBuf32();
                setDacStreamTimer(streamFrq);
                //Serial.println("0x92: "); //Serial.print(streamID, HEX); Serial.print(" "); Serial.print(streamFrq); Serial.println("   --- SET STREAM FRQ");
            }
            break;
            case 0x93:
            {
                //Come back to this one
                readBufOne(); //skip stream ID
                dacStreamBufPos = readBuf32();
                uint8_t lmode = readBufOne(); //Length mode
                //Serial.print("LMODE: "); Serial.print(lmode, BIN);
                dacStreamCurLength = readBuf32();
                //Serial.print("BUF POS: "); Serial.println(dacStreamBufPos);
                for(uint8_t i = 0; i<MAX_DATA_BLOCKS_IN_BANK; i++)
                {
                    if(dataBlocks[i].absoluteDataStartInBank == dacStreamBufPos)
                    {
                        activeDacStreamBlock = i;
                        dataBlocks[i].LengthMode = lmode;
                        //Serial.print("FOUND BLOCK: "); Serial.println(i);
                        break;
                    }
                }
                //Serial.println("0x93: "); //Serial.print(streamID, HEX); Serial.print(" "); Serial.print(dStart, HEX); Serial.print(" "); Serial.print(lengthMode, HEX); Serial.print(" "); Serial.print(dLength); Serial.println("   --- START STREAM");
            }
            break;
            case 0x94:
            {
                readBufOne(); //skip stream ID
                stopDacStreamTimer();
                activeDacStreamBlock = 0xFF;
                //Serial.println("0x94: "); //Serial.print(streamID, HEX); Serial.print(" "); Serial.println("   --- STOP STREAM");
            }
            break;
            case 0x95:
            {
                readBufOne(); //skip stream ID
                uint16_t blockID = readBuf16();
                uint8_t flags = readBufOne(); //flags
                dacStreamBufPos = dataBlocks[blockID].DataStart;
                dacStreamCurLength = dataBlocks[blockID].DataLength;
                activeDacStreamBlock = blockID;
                dataBlocks[blockID].LengthMode = bitRead(flags, 0) == 1 ? 0b10000001 : 0; //Set proper loop flag. VGMSpec, why the hell did you pick a different bit for the same flag as the 0x93 command. Stupid!
               //Serial.print("0x95: "); // Serial.print(streamID, HEX); Serial.print(" "); Serial.print(blockID, HEX); Serial.print(" "); Serial.print(flags, HEX); Serial.println("   --- START STREAM FAST");
                //Serial.print("BLOCK ID: "); Serial.println(blockID);
            }
            break;

            default:
                Serial.print("E:"); Serial.println(cmd, HEX);
                badCommandCount++;
                if(badCommandCount >= COMMAND_ERROR_SKIP_THRESHOLD)
                    state = END_OF_TRACK;
                return 0;
        }
    }    
}

VGMEngineClass VGMEngine;