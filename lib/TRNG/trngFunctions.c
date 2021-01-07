  /* Copyright 2019 Ron Sutton
Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation 
the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE 
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
/* These functions provide access to the features of ATSAMD51J19A True Random Number Generator peripheral */
#pragma optimize( "", off )
#include "Arduino.h"
#include "trngFunctions.h"

volatile uint32_t rNum;                        // stores most recent random number read from TRNG peripheral

// This function sets up and starts the TRNG
void trngInit(void) {

  MCLK->APBCMASK.reg |= MCLK_APBCMASK_TRNG;    // Activate the CLK_TRNG_APB clock
  NVIC_SetPriority(TRNG_IRQn, 0);              // Set NVIC priority for TRNG to 0 
  NVIC_EnableIRQ(TRNG_IRQn);                   // Connect TRNG to NVIC 
  TRNG->INTENCLR.reg = TRNG_INTENCLR_DATARDY;  // Disable the TRNG interrupt
  TRNG->CTRLA.reg = TRNG_CTRLA_ENABLE;         // Enable the TRNG peripheral

}

// This function waits for the next random number (signified by a change to rNum) and returns it
uint32_t trngGetRandomNumber(void)
{
  uint32_t i = rNum;
  TRNG->INTENSET.reg = TRNG_INTENSET_DATARDY;  // Enable the TRNG interrupt
  while (i == rNum)                            // Wait for the random number to change when interrupt occurs...
      ;
  TRNG->INTENCLR.reg = TRNG_INTENCLR_DATARDY;  // Disable the TRNG interrupt so it doesn't slow system when TRNG is not needed...
  return rNum;                                 //  and return the random number.
}

// This interrupt handler is called when a new random number is available; it reads the random number and clears the interrupt
void TRNG_Handler()                            // Called when TRNG data is available
{
    rNum = TRNG->DATA.reg;                     // Read & store the TRNG data output
    TRNG->INTFLAG.bit.DATARDY = 1;             // Clear the DATA READY interrupt flag
}