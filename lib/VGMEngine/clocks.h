#ifndef __CLOCKS_H__
#define __CLOCKS_H__

#include <Arduino.h>
#include "si5351.h"
#include "Wire.h"

static inline Si5351 si5351(0x6F);
static inline Si5351 si5351A(0x60);

#define start_timer()    *((volatile uint32_t*)0xE0001000) = 0x40000001  // Enable CYCCNT register
#define stop_timer()   *((volatile uint32_t*)0xE0001000) = 0x40000000  // Disable CYCCNT register
#define get_timer()   *((volatile uint32_t*)0xE0001004)               // Get value from CYCCNT register

#define nop __asm__ __volatile__ ("nop\n\t")

#define NTSC_COLORBURST 3579545
#define NTSC_YMCLK 7670454

#endif