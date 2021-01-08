#ifndef __CLOCKS_H__
#define __CLOCKS_H__

#include <Arduino.h>
#include "si5351.h"
#include "Wire.h"

static inline Si5351 si5351(0x6F);

#define start_timer()    *((volatile uint32_t*)0xE0001000) = 0x40000001  // Enable CYCCNT register
#define stop_timer()   *((volatile uint32_t*)0xE0001000) = 0x40000000  // Disable CYCCNT register
#define get_timer()   *((volatile uint32_t*)0xE0001004)               // Get value from CYCCNT register

#endif