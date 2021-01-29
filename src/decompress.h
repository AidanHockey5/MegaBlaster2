#ifndef __DECOMPRESS_H__
#define __DECOMPRESS_H__

#define MINIZ_NO_STDIO
#define MINIZ_NO_ARCHIVE_APIS
#define MINIZ_NO_TIME
#define MINIZ_NO_ZLIB_APIS
#define MINIZ_NO_MALLOC

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int uint;

#include "SdFat.h"
#include "miniz.h"
#include "Arduino.h"

#define my_max(a,b) (((a) > (b)) ? (a) : (b))
#define my_min(a,b) (((a) < (b)) ? (a) : (b))

#define IN_BUF_SIZE 10480
static uint8 s_inbuf[IN_BUF_SIZE];

#define OUT_BUF_SIZE (TINFL_LZ_DICT_SIZE)
static uint8 s_outbuf[OUT_BUF_SIZE];

inline static bool Decompress(const char* pSrc_filename, const char* pDst_filename)
{
    File pInfile, pOutfile;
    uint32_t infile_size;
    const void *next_in = s_inbuf;
    size_t avail_in = 0;
    void *next_out = s_outbuf;
    size_t avail_out = OUT_BUF_SIZE;
    size_t total_in = 0, total_out = 0;

    if(!pInfile.open(pSrc_filename, O_READ))
    {
        Serial.println("Failed to open source file");
        return false;
    }
    infile_size = pInfile.size();


    if(!pOutfile.open(pDst_filename, O_WRITE | O_CREAT))
    {
        Serial.println("Failed to open destination file");
        return false;
    }
    pInfile.seekSet(10);
    //Decompression
    uint32_t infile_remaining = infile_size;

    tinfl_decompressor inflator;
    tinfl_init(&inflator);

    for ( ; ; )
    {
        size_t in_bytes, out_bytes;
        tinfl_status status;
        if (!avail_in)
        {
            // Input buffer is empty, so read more bytes from input file.
            uint32_t n = my_min(IN_BUF_SIZE, infile_remaining);
            pInfile.readBytes(s_inbuf, n); 
            next_in = s_inbuf;
            avail_in = n;

            infile_remaining -= n;
        }

        in_bytes = avail_in;
        out_bytes = avail_out;
        status = tinfl_decompress(&inflator, (const mz_uint8 *)next_in, &in_bytes, s_outbuf, (mz_uint8 *)next_out, &out_bytes, (infile_remaining ? TINFL_FLAG_HAS_MORE_INPUT : 0)); //| TINFL_FLAG_PARSE_ZLIB_HEADER);

        avail_in -= in_bytes;
        next_in = (const mz_uint8 *)next_in + in_bytes;
        total_in += in_bytes;

        avail_out -= out_bytes;
        next_out = (mz_uint8 *)next_out + out_bytes;
        total_out += out_bytes;

        if ((status <= TINFL_STATUS_DONE) || (!avail_out)) //This section of code isn't even called as 'status' is always in it's error state (-1)
        {
            // Output buffer is full, or decompression is done, so write buffer to output file.
            uint32_t n = OUT_BUF_SIZE - (uint)avail_out;
            if (pOutfile.write(s_outbuf, n) != n) //(fwrite(s_outbuf, 1, n, pOutfile) != n)
            {
                //printf("Failed writing to output file!\n");
                Serial.println("Failed writing to output file!");
                return false;
            }

            next_out = s_outbuf;
            avail_out = OUT_BUF_SIZE;
        }

        // If status is <= TINFL_STATUS_DONE then either decompression is done or something went wrong.
        if (status <= TINFL_STATUS_DONE)
        {
            if (status == TINFL_STATUS_DONE)
            {
                // Decompression completed successfully.
                break;
            }
            else
            {
                // Decompression failed.
                //printf("tinfl_decompress() failed with status %i!\n", status);
                Serial.print("tinfl_decompress() failed with status: "); Serial.println(status);
                return false;
            }
        }
    }
    pInfile.close();
    pOutfile.close();
    return true;
}

#endif