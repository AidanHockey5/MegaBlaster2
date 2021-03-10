#include <Arduino.h>
#include "Adafruit_ZeroTimer.h"

// timer tester
Adafruit_ZeroTimer zt3 = Adafruit_ZeroTimer(3);
Adafruit_ZeroTimer zt4 = Adafruit_ZeroTimer(4);

//define the interrupt handlers
void TC3_Handler(){
  Adafruit_ZeroTimer::timerHandler(3);
}

void TC4_Handler(){
  Adafruit_ZeroTimer::timerHandler(4);
}

void TC5_Handler(){
  Adafruit_ZeroTimer::timerHandler(5);
}

// the timer 3 callbacks
void Timer3Callback0()
{
  digitalWrite(12, LOW);
}

void Timer3Callback1()
{
  digitalWrite(12, HIGH);
}

// timer 4 callback, set dac output!
volatile uint16_t dacout=0;
void Timer4Callback0()
{
  //analogWrite(A0, dacout++); // too slow!

  // we'll write the DAC by hand
  // wait till it's ready
#if defined(__SAMD51__)
  while (DAC->SYNCBUSY.bit.DATA0);
  // and write the data
  DAC->DATA[0].reg = dacout++;
#else
  while (DAC->STATUS.reg & DAC_STATUS_SYNCBUSY);
  // and write the data
  DAC->DATA.reg = dacout++;
#endif

  // wraparound when we hit 10 bits
  if (dacout == 0x400) {
    dacout = 0;
  }
}

void setup() {
  pinMode(13, OUTPUT);
  pinMode(12, OUTPUT);
  analogWriteResolution(10);
  analogWrite(A0, 128); // initialize the DAC

  Serial.begin(115200);
  Serial.println("Timer callback tester");

  /********************* Timer #3, 16 bit, two PWM outs, period = 65535 */
  zt3.configure(TC_CLOCK_PRESCALER_DIV2, // prescaler
                TC_COUNTER_SIZE_16BIT,   // bit width of timer/counter
                TC_WAVE_GENERATION_NORMAL_PWM // frequency or PWM mode
                );

  zt3.setCompare(0, 0xFFFF/4);
  zt3.setCompare(1, 0xFFFF/2);
  zt3.setCallback(true, TC_CALLBACK_CC_CHANNEL0, Timer3Callback0);  // this one sets pin low
  zt3.setCallback(true, TC_CALLBACK_CC_CHANNEL1, Timer3Callback1);  // this one sets pin high
  zt3.enable(true);

  /********************* Timer #4, 8 bit, one callback with adjustable period */
  zt4.configure(TC_CLOCK_PRESCALER_DIV1, // prescaler
                TC_COUNTER_SIZE_8BIT,   // bit width of timer/counter
                TC_WAVE_GENERATION_MATCH_PWM  // match style
                );

  zt4.setPeriodMatch(150, 100, 0); // 1 match, channel 0
  zt4.setCallback(true, TC_CALLBACK_CC_CHANNEL0, Timer4Callback0);  // set DAC in the callback
  zt4.enable(true);

}

void loop() {

}
