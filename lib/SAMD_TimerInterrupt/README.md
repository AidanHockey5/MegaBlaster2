# SAMD_TimerInterrupt Library

[![arduino-library-badge](https://www.ardu-badge.com/badge/SAMD_TimerInterrupt.svg?)](https://www.ardu-badge.com/SAMD_TimerInterrupt)
[![GitHub release](https://img.shields.io/github/release/khoih-prog/SAMD_TimerInterrupt.svg)](https://github.com/khoih-prog/SAMD_TimerInterrupt/releases)
[![GitHub](https://img.shields.io/github/license/mashape/apistatus.svg)](https://github.com/khoih-prog/SAMD_TimerInterrupt/blob/master/LICENSE)
[![contributions welcome](https://img.shields.io/badge/contributions-welcome-brightgreen.svg?style=flat)](#Contributing)
[![GitHub issues](https://img.shields.io/github/issues/khoih-prog/SAMD_TimerInterrupt.svg)](http://github.com/khoih-prog/SAMD_TimerInterrupt/issues)

---
---

## Table of Contents

* [Why do we need this SAMD_TimerInterrupt library](#why-do-we-need-this-samd_timerinterrupt-library)
  * [Features](#features)
  * [Why using ISR-based Hardware Timer Interrupt is better](#why-using-isr-based-hardware-timer-interrupt-is-better)
  * [Currently supported Boards](#currently-supported-boards)
  * [Important Notes about ISR](#important-notes-about-isr)
* [Changelog](#changelog)
  * [Releases v1.2.0](#releases-v120)
  * [Releases v1.1.1](#releases-v111)
  * [Releases v1.0.1](#releases-v101)
  * [Releases v1.0.0](#releases-v100)
* [Prerequisites](#prerequisites)
* [Installation](#installation)
  * [Use Arduino Library Manager](#use-arduino-library-manager)
  * [Manual Install](#manual-install)
  * [VS Code & PlatformIO](#vs-code--platformio)
* [Packages' Patches](#packages-patches)
  * [1. For Arduino SAMD boards](#1-for-arduino-samd-boards)
      * [For core version v1.8.10+](#for-core-version-v1810)
      * [For core version v1.8.9-](#for-core-version-v189-)
  * [2. For Adafruit SAMD boards](#2-for-adafruit-samd-boards)
  * [3. For Seeeduino SAMD boards](#3-for-seeeduino-samd-boards)
* [Libraries' Patches](#libraries-patches)
  * [1. For application requiring 2K+ HTML page](#1-for-application-requiring-2k-html-page)
  * [2. For Ethernet library](#2-for-ethernet-library)
  * [3. For EthernetLarge library](#3-for-ethernetlarge-library)
  * [4. For Etherne2 library](#4-for-ethernet2-library)
  * [5. For Ethernet3 library](#5-for-ethernet3-library)
  * [6. For UIPEthernet library](#6-for-uipethernet-library)
  * [7. For fixing ESP32 compile error](#7-for-fixing-esp32-compile-error)
* [HOWTO Fix `Multiple Definitions` Linker Error](#howto-fix-multiple-definitions-linker-error)
* [New from v1.0.0](#new-from-v100)
* [Usage](#usage)
  * [1. Using only Hardware Timer directly](#1-using-only-hardware-timer-directly)
    * [1.1 Init Hardware Timer](#11-init-hardware-timer)
    * [1.2 Set Hardware Timer Interval and attach Timer Interrupt Handler function](#12-set-hardware-timer-interval-and-attach-timer-interrupt-handler-function)
  * [2. Using 16 ISR_based Timers from 1 Hardware Timer](#2-using-16-isr_based-timers-from-1-hardware-timer)
    * [2.1 Init Hardware Timer and ISR-based Timer](#21-init-hardware-timer-and-isr-based-timer)
    * [2.2 Set Hardware Timer Interval and attach Timer Interrupt Handler functions](#22-set-hardware-timer-interval-and-attach-timer-interrupt-handler-functions)
* [Examples](#examples)
  * [  1. Argument_None](examples/Argument_None)
  * [  2. ISR_16_Timers_Array](examples/ISR_16_Timers_Array)
  * [  3. ISR_RPM_Measure](examples/ISR_RPM_Measure)
  * [  4. ISR_Timer_Complex_Ethernet](examples/ISR_Timer_Complex_Ethernet)
  * [  5. ISR_Timer_Complex_WiFiNINA](examples/ISR_Timer_Complex_WiFiNINA)
  * [  6. RPM_Measure](examples/RPM_Measure)
  * [  7. SwitchDebounce](examples/SwitchDebounce)
  * [  8. TimerInterruptTest](examples/TimerInterruptTest)
  * [  9. TimerInterruptLEDDemo](examples/TimerInterruptLEDDemo)
  * [ 10. **Change_Interval**](examples/Change_Interval)
  * [ 11. **ISR_16_Timers_Array_Complex**](examples/ISR_16_Timers_Array_Complex)
* [Example ISR_Timer_Complex_WiFiNINA](#example-isr_timer_complex_wifinina)
* [Debug Terminal Output Samples](#debug-terminal-output-samples)
  * [1. ISR_Timer_Complex_WiFiNINA on Arduino SAMD21 SAMD_NANO_33_IOT using WiFiNINA](#1-isr_timer_complex_wifinina-on-arduino-samd21-samd_nano_33_iot-using-wifinina)
  * [2. TimerInterruptTest on Adafruit SAMD51 ITSYBITSY_M4](#2-timerinterrupttest-on-adafruit-samd51-itsybitsy_m4)
  * [3. Argument_None on Arduino SAMD21 SAMD_NANO_33_IOT](#3-argument_none-on-arduino-samd21-samd_nano_33_iot)
  * [4. ISR_16_Timers_Array on Arduino SAMD21 SAMD_NANO_33_IOT](#4-isr_16_timers_array-on-arduino-samd21-samd_nano_33_iot)
  * [5. Change_Interval on Arduino SAMD21 SAMD_NANO_33_IOT](#5-change_interval-on-arduino-samd21-samd_nano_33_iot)
* [Debug](#debug)
* [Troubleshooting](#troubleshooting)
* [Releases](#releases)
* [Issues](#issues)
* [TO DO](#to-do)
* [DONE](#done)
* [Contributions and Thanks](#contributions-and-thanks)
* [Contributing](#contributing)
* [License](#license)
* [Copyright](#copyright)


---
---

### Why do we need this [SAMD_TimerInterrupt library](https://github.com/khoih-prog/SAMD_TimerInterrupt)

### Features

This library enables you to use Interrupt from Hardware Timers on an SAMD-based board, such as SAMD21 Nano-33-IoT, Adafruit SAMD51 Itsy-Bitsy M4, etc.

As **Hardware Timers are rare, and very precious assets** of any board, this library now enables you to use up to **16 ISR-based Timers, while consuming only 1 Hardware Timer**. Timers' interval is very long (**ulong millisecs**).

Now with these new **16 ISR-based timers**, the maximum interval is **practically unlimited** (limited only by unsigned long miliseconds) while **the accuracy is nearly perfect** compared to software timers. 

The most important feature is they're ISR-based timers. Therefore, their executions are **not blocked by bad-behaving functions / tasks**. This important feature is absolutely necessary for mission-critical tasks. 

The [**ISR_Timer_Complex**](examples/ISR_Timer_Complex) example will demonstrate the nearly perfect accuracy compared to software timers by printing the actual elapsed millisecs of each type of timers.

Being ISR-based timers, their executions are not blocked by bad-behaving functions / tasks, such as connecting to WiFi, Internet and Blynk services. You can also have many `(up to 16)` timers to use.

This non-being-blocked important feature is absolutely necessary for mission-critical tasks.

You'll see blynkTimer Software is blocked while system is connecting to WiFi / Internet / Blynk, as well as by blocking task 
in loop(), using delay() function as an example. The elapsed time then is very unaccurate

### Why using ISR-based Hardware Timer Interrupt is better

Imagine you have a system with a **mission-critical** function, measuring water level and control the sump pump or doing something much more important. You normally use a software timer to poll, or even place the function in loop(). But what if another function is **blocking** the loop() or setup().

So your function **might not be executed, and the result would be disastrous.**

You'd prefer to have your function called, no matter what happening with other functions (busy loop, bug, etc.).

The correct choice is to use a Hardware Timer with **Interrupt** to call your function.

These hardware timers, using interrupt, still work even if other functions are blocking. Moreover, they are much more **precise** (certainly depending on clock frequency accuracy) than other software timers using millis() or micros(). That's necessary if you need to measure some data requiring better accuracy.

Functions using normal software timers, relying on loop() and calling millis(), won't work if the loop() or setup() is blocked by certain operation. For example, certain function is blocking while it's connecting to WiFi or some services.

The catch is **your function is now part of an ISR (Interrupt Service Routine), and must be lean / mean, and follow certain rules.** More to read on:

[**HOWTO Attach Interrupt**](https://www.arduino.cc/reference/en/language/functions/external-interrupts/attachinterrupt/)

---

### Currently supported Boards

  - **Arduino SAMD21 (ZERO, MKR, NANO_33_IOT, etc.)**.
  - **Adafruit SAM21 (Itsy-Bitsy M0, Metro M0, Feather M0, Gemma M0, etc.)**.
  - **Adafruit SAM51 (Itsy-Bitsy M4, Metro M4, Grand Central M4, Feather M4 Express, etc.)**.
  - **Seeeduino SAMD21/SAMD51 boards (SEEED_WIO_TERMINAL, SEEED_FEMTO_M0, SEEED_XIAO_M0, Wio_Lite_MG126, WIO_GPS_BOARD, SEEEDUINO_ZERO, SEEEDUINO_LORAWAN, SEEED_GROVE_UI_WIRELESS, etc.)** 

---

### Important Notes about ISR

1. Inside the attached function, **delay() won’t work and the value returned by millis() will not increment.** Serial data received while in the function may be lost. You should declare as **volatile any variables that you modify within the attached function.**

2. Typically global variables are used to pass data between an ISR and the main program. To make sure variables shared between an ISR and the main program are updated correctly, declare them as volatile.

---
---

## Changelog

### Releases v1.2.0

1. Add better debug feature.
2. Optimize code and examples to reduce RAM usage
3. Add Table of Contents

### Releases v1.1.1

1. Add example [**Change_Interval**](examples/Change_Interval) and [**ISR_16_Timers_Array_Complex**](examples/ISR_16_Timers_Array_Complex)
2. Bump up version to sync with other TimerInterrupt Libraries. Modify Version String.

### Releases v1.0.1

1. Add complicated example [ISR_16_Timers_Array](examples/ISR_16_Timers_Array) utilizing and demonstrating the full usage of 16 independent ISR Timers.

### Releases v1.0.0

1. Permit up to 16 super-long-time, super-accurate ISR-based timers to avoid being blocked
2. Using cpp code besides Impl.h code to use if Multiple-Definition linker error.

---
---

## Prerequisites

 1. [`Arduino IDE 1.8.13+` for Arduino](https://www.arduino.cc/en/Main/Software)
 2. [`Arduino SAMD core v1.8.11+`](https://www.arduino.cc/en/Guide/ArduinoM0) for SAMD ARM Cortex-M0+ boards (Nano 33 IoT, etc.).
 3. [`Adafruit SAMD core v1.6.4+`](https://www.adafruit.com/) for SAMD ARM Cortex-M0+ and M4 boards (Itsy-Bitsy M4, etc.)
 4. [`Seeeduino SAMD core v1.8.1+`](https://www.seeedstudio.com/) for SAMD21/SAMD51 boards (XIAO M0, Wio Terminal, etc.)
 5. [`Blynk library 0.6.1+`](https://github.com/blynkkk/blynk-library) to use with certain example.
 6. To use with certain example, depending on which Ethernet card you're using:
   - [`Ethernet library v2.0.0+`](https://www.arduino.cc/en/Reference/Ethernet) for W5100, W5200 and W5500.
   - [`Ethernet2 library v1.0.4+`](https://github.com/khoih-prog/Ethernet2) for W5500 (Deprecated, use Arduino Ethernet library).
   - [`Ethernet3 library v1.5.3+`](https://github.com/sstaub/Ethernet3) for W5500/WIZ550io/WIZ850io/USR-ES1 with Wiznet W5500 chip.
   - [`EthernetLarge library v2.0.0+`](https://github.com/OPEnSLab-OSU/EthernetLarge) for W5100, W5200 and W5500. ***Ready*** from v1.0.1.
   - [`UIPEthernet library v2.0.9+`](https://github.com/UIPEthernet/UIPEthernet) for ENC28J60.
 7. To use with certain example
   - [`SimpleTimer library`](https://github.com/schinken/SimpleTimer) for [ISR_16_Timers_Array example](examples/ISR_16_Timers_Array).
---
---

## Installation

### Use Arduino Library Manager

The best and easiest way is to use `Arduino Library Manager`. Search for [**SAMD_TimerInterrupt**](https://github.com/khoih-prog/SAMD_TimerInterrupt), then select / install the latest version.
You can also use this link [![arduino-library-badge](https://www.ardu-badge.com/badge/SAMD_TimerInterrupt.svg?)](https://www.ardu-badge.com/SAMD_TimerInterrupt) for more detailed instructions.

### Manual Install

Another way to install is to:

1. Navigate to [**SAMD_TimerInterrupt**](https://github.com/khoih-prog/SAMD_TimerInterrupt) page.
2. Download the latest release `SAMD_TimerInterrupt-master.zip`.
3. Extract the zip file to `SAMD_TimerInterrupt-master` directory 
4. Copy whole `SAMD_TimerInterrupt-master` folder to Arduino libraries' directory such as `~/Arduino/libraries/`.

### VS Code & PlatformIO

1. Install [VS Code](https://code.visualstudio.com/)
2. Install [PlatformIO](https://platformio.org/platformio-ide)
3. Install [**SAMD_TimerInterrupt** library](https://platformio.org/lib/show/11396/SAMD_TimerInterrupt) or [**SAMD_TimerInterrupt** library](https://platformio.org/lib/show/11412/SAMD_TimerInterrupt) by using [Library Manager](https://platformio.org/lib/show/11412/SAMD_TimerInterrupt/installation). Search for **SAMD_TimerInterrupt** in [Platform.io Author's Libraries](https://platformio.org/lib/search?query=author:%22Khoi%20Hoang%22)
4. Use included [platformio.ini](platformio/platformio.ini) file from examples to ensure that all dependent libraries will installed automatically. Please visit documentation for the other options and examples at [Project Configuration File](https://docs.platformio.org/page/projectconf.html)

---
---

### Packages' Patches

#### 1. For Arduino SAMD boards
 
 ***To be able to compile without error and automatically detect and display BOARD_NAME on Arduino SAMD (Nano-33-IoT, etc) boards***, you have to copy the whole [Arduino SAMD cores 1.8.11](Packages_Patches/arduino/hardware/samd/1.8.11) directory into Arduino SAMD directory (~/.arduino15/packages/arduino/hardware/samd/1.8.11).
 
#### For core version v1.8.10+

Supposing the Arduino SAMD version is 1.8.11. Now only one file must be copied into the directory:

- `~/.arduino15/packages/arduino/hardware/samd/1.8.11/platform.txt`

Whenever a new version is installed, remember to copy this files into the new version directory. For example, new version is x.yy.zz

This file must be copied into the directory:

- `~/.arduino15/packages/arduino/hardware/samd/x.yy.zz/platform.txt`
 
#### For core version v1.8.9-

Supposing the Arduino SAMD version is 1.8.9. These files must be copied into the directory:

- `~/.arduino15/packages/arduino/hardware/samd/1.8.9/platform.txt`
- ***`~/.arduino15/packages/arduino/hardware/samd/1.8.9/cores/arduino/Arduino.h`***

Whenever a new version is installed, remember to copy these files into the new version directory. For example, new version is x.yy.z

These files must be copied into the directory:

- `~/.arduino15/packages/arduino/hardware/samd/x.yy.z/platform.txt`
- ***`~/.arduino15/packages/arduino/hardware/samd/x.yy.z/cores/arduino/Arduino.h`***
 
 This is mandatory to fix the ***notorious Arduino SAMD compiler error***. See [Improve Arduino compatibility with the STL (min and max macro)](https://github.com/arduino/ArduinoCore-samd/pull/399)
 
```
 ...\arm-none-eabi\include\c++\7.2.1\bits\stl_algobase.h:243:56: error: macro "min" passed 3 arguments, but takes just 2
     min(const _Tp& __a, const _Tp& __b, _Compare __comp)
```

Whenever the above-mentioned compiler error issue is fixed with the new Arduino SAMD release, you don't need to copy the `Arduino.h` file anymore.

#### 2. For Adafruit SAMD boards
 
 ***To be able to automatically detect and display BOARD_NAME on Adafruit SAMD (Itsy-Bitsy M4, etc) boards***, you have to copy the file [Adafruit SAMD platform.txt](Packages_Patches/adafruit/hardware/samd/1.6.4) into Adafruit samd directory (~/.arduino15/packages/adafruit/hardware/samd/1.6.4). 

Supposing the Adafruit SAMD core version is 1.6.4. This file must be copied into the directory:

- `~/.arduino15/packages/adafruit/hardware/samd/1.6.4/platform.txt`

Whenever a new version is installed, remember to copy this file into the new version directory. For example, new version is x.yy.zz
This file must be copied into the directory:

- `~/.arduino15/packages/adafruit/hardware/samd/x.yy.zz/platform.txt`

#### 3. For Seeeduino SAMD boards
 
 ***To be able to automatically detect and display BOARD_NAME on Seeeduino SAMD (XIAO M0, Wio Terminal, etc) boards***, you have to copy the file [Seeeduino SAMD platform.txt](Packages_Patches/Seeeduino/hardware/samd/1.8.1) into Adafruit samd directory (~/.arduino15/packages/Seeeduino/hardware/samd/1.8.1). 

Supposing the Seeeduino SAMD core version is 1.8.1. This file must be copied into the directory:

- `~/.arduino15/packages/Seeeduino/hardware/samd/1.8.1/platform.txt`

Whenever a new version is installed, remember to copy this file into the new version directory. For example, new version is x.yy.zz
This file must be copied into the directory:

- `~/.arduino15/packages/Seeeduino/hardware/samd/x.yy.zz/platform.txt`

---
---

### Libraries' Patches

#### Notes: These patches are totally optional and necessary only when you use the related Ethernet library and get certain error or issues.

#### 1. For application requiring 2K+ HTML page

If your application requires 2K+ HTML page, the current [`Ethernet library`](https://www.arduino.cc/en/Reference/Ethernet) must be modified if you are using W5200/W5500 Ethernet shields. W5100 is not supported for 2K+ buffer. If you use boards requiring different CS/SS pin for W5x00 Ethernet shield, for example ESP32, ESP8266, nRF52, etc., you also have to modify the following libraries to be able to specify the CS/SS pin correctly.

#### 2. For Ethernet library

To fix [`Ethernet library`](https://www.arduino.cc/en/Reference/Ethernet), just copy these following files into the [`Ethernet library`](https://www.arduino.cc/en/Reference/Ethernet) directory to overwrite the old files:
- [Ethernet.h](LibraryPatches/Ethernet/src/Ethernet.h)
- [Ethernet.cpp](LibraryPatches/Ethernet/src/Ethernet.cpp)
- [EthernetServer.cpp](LibraryPatches/Ethernet/src/EthernetServer.cpp)
- [w5100.h](LibraryPatches/Ethernet/src/utility/w5100.h)
- [w5100.cpp](LibraryPatches/Ethernet/src/utility/w5100.cpp)

#### 3. For EthernetLarge library

To fix [`EthernetLarge library`](https://github.com/OPEnSLab-OSU/EthernetLarge), just copy these following files into the [`EthernetLarge library`](https://github.com/OPEnSLab-OSU/EthernetLarge) directory to overwrite the old files:
- [EthernetLarge.h](LibraryPatches/EthernetLarge/src/EthernetLarge.h)
- [EthernetLarge.cpp](LibraryPatches/EthernetLarge/src/EthernetLarge.cpp)
- [EthernetServer.cpp](LibraryPatches/EthernetLarge/src/EthernetServer.cpp)
- [w5100.h](LibraryPatches/EthernetLarge/src/utility/w5100.h)
- [w5100.cpp](LibraryPatches/EthernetLarge/src/utility/w5100.cpp)


#### 4. For Ethernet2 library

To fix [`Ethernet2 library`](https://github.com/khoih-prog/Ethernet2), just copy these following files into the [`Ethernet2 library`](https://github.com/khoih-prog/Ethernet2) directory to overwrite the old files:

- [Ethernet2.h](LibraryPatches/Ethernet2/src/Ethernet2.h)
- [Ethernet2.cpp](LibraryPatches/Ethernet2/src/Ethernet2.cpp)

To add UDP Multicast support, necessary for the [**UPnP_Generic library**](https://github.com/khoih-prog/UPnP_Generic):

- [EthernetUdp2.h](LibraryPatches/Ethernet2/src/EthernetUdp2.h)
- [EthernetUdp2.cpp](LibraryPatches/Ethernet2/src/EthernetUdp2.cpp)

#### 5. For Ethernet3 library

5. To fix [`Ethernet3 library`](https://github.com/sstaub/Ethernet3), just copy these following files into the [`Ethernet3 library`](https://github.com/sstaub/Ethernet3) directory to overwrite the old files:
- [Ethernet3.h](LibraryPatches/Ethernet3/src/Ethernet3.h)
- [Ethernet3.cpp](LibraryPatches/Ethernet3/src/Ethernet3.cpp)

#### 6. For UIPEthernet library

***To be able to compile and run on nRF52 boards with ENC28J60 using UIPEthernet library***, you have to copy these following files into the UIPEthernet `utility` directory to overwrite the old files:

- [UIPEthernet.h](LibraryPatches/UIPEthernet/UIPEthernet.h)
- [UIPEthernet.cpp](LibraryPatches/UIPEthernet/UIPEthernet.cpp)
- [Enc28J60Network.h](LibraryPatches/UIPEthernet/utility/Enc28J60Network.h)
- [Enc28J60Network.cpp](LibraryPatches/UIPEthernet/utility/Enc28J60Network.cpp)

#### 7. For fixing ESP32 compile error

To fix [`ESP32 compile error`](https://github.com/espressif/arduino-esp32), just copy the following file into the [`ESP32`](https://github.com/espressif/arduino-esp32) cores/esp32 directory (e.g. ./arduino-1.8.12/hardware/espressif/cores/esp32) to overwrite the old file:
- [Server.h](LibraryPatches/esp32/cores/esp32/Server.h)

---
---

### HOWTO Fix `Multiple Definitions` Linker Error

The current library implementation, using **xyz-Impl.h instead of standard xyz.cpp**, possibly creates certain `Multiple Definitions` Linker error in certain use cases. Although it's simple to just modify several lines of code, either in the library or in the application, the library is adding 2 more source directories

1. **scr_h** for new h-only files
2. **src_cpp** for standard h/cpp files

besides the standard **src** directory.

To use the **old standard cpp** way, locate this library' directory, then just 

1. **Delete the all the files in src directory.**
2. **Copy all the files in src_cpp directory into src.**
3. Close then reopen the application code in Arduino IDE, etc. to recompile from scratch.

To re-use the **new h-only** way, just 

1. **Delete the all the files in src directory.**
2. **Copy the files in src_h directory into src.**
3. Close then reopen the application code in Arduino IDE, etc. to recompile from scratch.

---
---


## New from v1.0.0

Now with these new `16 ISR-based timers` (while consuming only **1 hardware timer**), the maximum interval is practically unlimited (limited only by unsigned long miliseconds). The accuracy is nearly perfect compared to software timers. The most important feature is they're ISR-based timers Therefore, their executions are not blocked by bad-behaving functions / tasks.
This important feature is absolutely necessary for mission-critical tasks. 

The `ISR_Timer_Complex` example will demonstrate the nearly perfect accuracy compared to software timers by printing the actual elapsed millisecs of each type of timers.
Being ISR-based timers, their executions are not blocked by bad-behaving functions / tasks, such as connecting to WiFi, Internet and Blynk services. You can also have many `(up to 16)` timers to use.
This non-being-blocked important feature is absolutely necessary for mission-critical tasks. 
You'll see blynkTimer Software is blocked while system is connecting to WiFi / Internet / Blynk, as well as by blocking task 
in loop(), using delay() function as an example. The elapsed time then is very unaccurate


---
---

## Usage

Before using any Timer, you have to make sure the Timer has not been used by any other purpose.

### 1. Using only Hardware Timer directly

#### 1.1 Init Hardware Timer

```
// Depending on the board, you can select SAMD21 Hardware Timer from TC3-TCC
// SAMD21 Hardware Timer from TC3 or TCC
// SAMD51 Hardware Timer only TC3
SAMDTimer ITimer0(TIMER_TC3);
```

#### 1.2 Set Hardware Timer Interval and attach Timer Interrupt Handler function

```
void TimerHandler0(void)
{
  // Doing something here inside ISR
}

#define TIMER0_INTERVAL_MS        1000      // 1s = 1000ms
void setup()
{
  ....
  
  // Interval in microsecs
  if (ITimer0.attachInterruptInterval(TIMER0_INTERVAL_MS * 1000, TimerHandler0))
    Serial.println("Starting  ITimer0 OK, millis() = " + String(millis()));
  else
    Serial.println("Can't set ITimer0. Select another freq. or timer");
}  
```

### 2. Using 16 ISR_based Timers from 1 Hardware Timer


#### 2.1 Init Hardware Timer and ISR-based Timer

```
// Depending on the board, you can select SAMD21 Hardware Timer from TC3-TCC
// SAMD21 Hardware Timer from TC3 or TCC
// SAMD51 Hardware Timer only TC3
SAMDTimer ITimer0(TIMER_TC3);

// Init SAMD_ISR_Timer
// Each SAMD_ISR_Timer can service 16 different ISR-based timers
SAMD_ISR_Timer ISR_Timer;
```

#### 2.2 Set Hardware Timer Interval and attach Timer Interrupt Handler functions

```
void TimerHandler(void)
{
  ISR_Timer.run();
}

#define HW_TIMER_INTERVAL_MS          50L

#define TIMER_INTERVAL_2S             2000L
#define TIMER_INTERVAL_5S             5000L
#define TIMER_INTERVAL_11S            11000L
#define TIMER_INTERVAL_101S           101000L

// In SAMD, avoid doing something fancy in ISR, for example complex Serial.print with String() argument
// The pure simple Serial.prints here are just for demonstration and testing. Must be eliminate in working environment
// Or you can get this run-time error / crash
void doingSomething2s()
{
  // Doing something here inside ISR
}
  
void doingSomething5s()
{
  // Doing something here inside ISR
}

void doingSomething11s()
{
  // Doing something here inside ISR
}

void doingSomething101s()
{
  // Doing something here inside ISR
}

void setup()
{
  ....
  
  // Interval in microsecs
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    lastMillis = millis();
    Serial.println("Starting  ITimer OK, millis() = " + String(lastMillis));
  }
  else
    Serial.println("Can't set ITimer correctly. Select another freq. or interval");

  // Just to demonstrate, don't use too many ISR Timers if not absolutely necessary
  // You can use up to 16 timer for each ISR_Timer
  ISR_Timer.setInterval(TIMER_INTERVAL_2S, doingSomething2s);
  ISR_Timer.setInterval(TIMER_INTERVAL_5S, doingSomething5s);
  ISR_Timer.setInterval(TIMER_INTERVAL_11S, doingSomething11s);
  ISR_Timer.setInterval(TIMER_INTERVAL_101S, doingSomething101s);
}  
```


---
---

### Examples: 

 1. [Argument_None](examples/Argument_None)
 2. [ISR_16_Timers_Array](examples/ISR_16_Timers_Array)
 3. [ISR_RPM_Measure](examples/ISR_RPM_Measure)
 4. [ISR_Timer_Complex_Ethernet](examples/ISR_Timer_Complex_Ethernet)
 5. [ISR_Timer_Complex_WiFiNINA](examples/ISR_Timer_Complex_WiFiNINA)
 6. [RPM_Measure](examples/RPM_Measure)
 7. [SwitchDebounce](examples/SwitchDebounce)
 8. [TimerInterruptTest](examples/TimerInterruptTest)
 9. [TimerInterruptLEDDemo](examples/TimerInterruptLEDDemo)
10. [**Change_Interval**](examples/Change_Interval). New
11. [**ISR_16_Timers_Array_Complex**](examples/ISR_16_Timers_Array_Complex). New
 

---
---

### Example [ISR_Timer_Complex_WiFiNINA](examples/ISR_Timer_Complex_WiFiNINA)

```
#if !( defined(ARDUINO_SAMD_ZERO) || defined(ARDUINO_SAMD_MKR1000) || defined(ARDUINO_SAMD_MKRWIFI1010) \
    || defined(ARDUINO_SAMD_NANO_33_IOT) || defined(ARDUINO_SAMD_MKRFox1200) || defined(ARDUINO_SAMD_MKRWAN1300) || defined(ARDUINO_SAMD_MKRWAN1310) \
    || defined(ARDUINO_SAMD_MKRGSM1400) || defined(ARDUINO_SAMD_MKRNB1500) || defined(ARDUINO_SAMD_MKRVIDOR4000) || defined(__SAMD21G18A__) \
    || defined(ARDUINO_SAMD_CIRCUITPLAYGROUND_EXPRESS) || defined(__SAMD21E18A__) || defined(__SAMD51__) || defined(__SAMD51J20A__) || defined(__SAMD51J19A__) \
    || defined(__SAMD51G19A__) || defined(__SAMD51P19A__) || defined(__SAMD21G18A__) )
  #error This code is designed to run on SAMD21/SAMD51 platform! Please check your Tools->Board setting.
#endif

#define BLYNK_PRINT Serial

//#define BLYNK_DEBUG
#ifdef BLYNK_DEBUG
  #undef BLYNK_DEBUG
#endif

/* Comment this out to disable prints and save space */
#define BLYNK_PRINT Serial

#include <BlynkSimpleWiFiNINA_SAMD.h>

#define USE_LOCAL_SERVER      true

#if USE_LOCAL_SERVER
  char auth[] = "******";
  char server[] = "account.duckdns.org";
  //char server[] = "192.168.2.112";

#else
  char auth[] = "******";
  char server[] = "blynk-cloud.com";
#endif
  
  #define BLYNK_HARDWARE_PORT       8080

#if !(USE_BUILTIN_ETHERNET || USE_UIP_ETHERNET)
  #define W5100_CS  10
  #define SDCARD_CS 4
#endif

// Your WiFi credentials.
char ssid[] = "SSID";
char pass[] = "12345678";

// These define's must be placed at the beginning before #include "SAMDTimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
// Don't define TIMER_INTERRUPT_DEBUG > 2. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0

#include "SAMDTimerInterrupt.h"
#include "SAMD_ISR_Timer.h"

#define TIMER_INTERVAL_MS         100
#define HW_TIMER_INTERVAL_MS      50

volatile uint32_t lastMillis = 0;

// Depending on the board, you can select SAMD21 Hardware Timer from TC3-TCC
// SAMD21 Hardware Timer from TC3 or TCC
// SAMD51 Hardware Timer only TC3

// Init SAMD timer TIMER_TC3
SAMDTimer ITimer(TIMER_TC3);

#if (TIMER_INTERRUPT_USING_SAMD21)
// Init SAMD timer TIMER_TCC
//SAMDTimer ITimer(TIMER_TCC);
#endif

// Init SAMD_ISR_Timer
// Each SAMD_ISR_Timer can service 16 different ISR-based timers
SAMD_ISR_Timer ISR_Timer;

// Init Blynk Timer
BlynkTimer blynkTimer;

#define LED_TOGGLE_INTERVAL_MS        5000L

#define TIMER_INTERVAL_2S             2000L
#define TIMER_INTERVAL_5S             5000L
#define TIMER_INTERVAL_11S            11000L
#define TIMER_INTERVAL_101S           101000L

void TimerHandler()
{
  static bool toggle  = false;
  static bool started = false;
  static int timeRun  = 0;

  ISR_Timer.run();

  // Toggle LED every LED_TOGGLE_INTERVAL_MS = 5000ms = 5s
  if (++timeRun == (LED_TOGGLE_INTERVAL_MS / HW_TIMER_INTERVAL_MS) )
  {
    timeRun = 0;

    if (!started)
    {
      started = true;
      pinMode(LED_BUILTIN, OUTPUT);
    }

    //timer interrupt toggles pin LED_BUILTIN
    digitalWrite(LED_BUILTIN, toggle);
    toggle = !toggle;
  }
}

// In SAMD, avoid doing something fancy in ISR, for example complex Serial.print with String() argument
// The pure simple Serial.prints here are just for demonstration and testing. Must be eliminate in working environment
// Or you can get this run-time error / crash
void doingSomething2s()
{
#if (TIMER_INTERRUPT_DEBUG > 0)  
  static unsigned long previousMillis = lastMillis;
  unsigned long deltaMillis = millis() - previousMillis;


  if (previousMillis > TIMER_INTERVAL_2S)
  {
    Serial.print("2s: Delta ms = "); Serial.println(deltaMillis);
  }

  previousMillis = millis();
#endif
}

// In SAMD, avoid doing something fancy in ISR, for example complex Serial.print with String() argument
// The pure simple Serial.prints here are just for demonstration and testing. Must be eliminate in working environment
// Or you can get this run-time error / crash
void doingSomething5s()
{
#if (TIMER_INTERRUPT_DEBUG > 0)  
  static unsigned long previousMillis = lastMillis;
  unsigned long deltaMillis = millis() - previousMillis;


  if (previousMillis > TIMER_INTERVAL_5S)
  {
    Serial.print("5s: Delta ms = "); Serial.println(deltaMillis);
  }

  previousMillis = millis();
#endif
}

// In SAMD, avoid doing something fancy in ISR, for example complex Serial.print with String() argument
// The pure simple Serial.prints here are just for demonstration and testing. Must be eliminate in working environment
// Or you can get this run-time error / crash
void doingSomething11s()
{
#if (TIMER_INTERRUPT_DEBUG > 0)  
  static unsigned long previousMillis = lastMillis;
  unsigned long deltaMillis = millis() - previousMillis;


  if (previousMillis > TIMER_INTERVAL_11S)
  {
    Serial.print("11s: Delta ms = "); Serial.println(deltaMillis);
  }

  previousMillis = millis();
#endif
}

// In SAMD, avoid doing something fancy in ISR, for example complex Serial.print with String() argument
// The pure simple Serial.prints here are just for demonstration and testing. Must be eliminate in working environment
// Or you can get this run-time error / crash
void doingSomething101s()
{
#if (TIMER_INTERRUPT_DEBUG > 0)  
  static unsigned long previousMillis = lastMillis;
  unsigned long deltaMillis = millis() - previousMillis;


  if (previousMillis > TIMER_INTERVAL101S)
  {
    Serial.print("101s: Delta ms = "); Serial.println(deltaMillis);
  }

  previousMillis = millis();
#endif
}

#define BLYNK_TIMER_MS        2000L

// Here is software Timer, you can do somewhat fancy stuffs without many issues.
// But always avoid
// 1. Long delay() it just doing nothing and pain-without-gain wasting CPU power.Plan and design your code / strategy ahead
// 2. Very long "do", "while", "for" loops without predetermined exit time.
void blynkDoingSomething2s()
{
  static unsigned long previousMillis = lastMillis;
  
  Serial.print(F("blynkDoingSomething2s: Delta programmed ms = ")); Serial.print(BLYNK_TIMER_MS);
  Serial.print(F(", actual = ")); Serial.println(millis() - previousMillis);
  
  previousMillis = millis();
}

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  delay(100);

  Serial.print(F("\nStarting ISR_Timer_Complex_WiFiNINA on ")); Serial.println(BOARD_NAME);
  Serial.println(SAMD_TIMER_INTERRUPT_VERSION);
  Serial.print(F("CPU Frequency = ")); Serial.print(F_CPU / 1000000); Serial.println(F(" MHz"));

  // You need this timer for non-critical tasks. Avoid abusing ISR if not absolutely necessary.
  blynkTimer.setInterval(BLYNK_TIMER_MS, blynkDoingSomething2s);

  // Interval in microsecs
  if (ITimer.attachInterruptInterval(HW_TIMER_INTERVAL_MS * 1000, TimerHandler))
  {
    lastMillis = millis();
    Serial.print(F("Starting ITimer OK, millis() = ")); Serial.println(lastMillis);
  }
  else
    Serial.println(F("Can't set ITimer. Select another freq. or interval"));

  // Just to demonstrate, don't use too many ISR Timers if not absolutely necessary
  // You can use up to 16 timer for each ISR_Timer
  ISR_Timer.setInterval(TIMER_INTERVAL_2S, doingSomething2s);
  ISR_Timer.setInterval(TIMER_INTERVAL_5S, doingSomething5s);
  ISR_Timer.setInterval(TIMER_INTERVAL_11S, doingSomething11s);
  ISR_Timer.setInterval(TIMER_INTERVAL_101S, doingSomething101s);

#if !(USE_BUILTIN_ETHERNET || USE_UIP_ETHERNET)
  pinMode(SDCARD_CS, OUTPUT);
  digitalWrite(SDCARD_CS, HIGH); // Deselect the SD card
#endif

#if USE_LOCAL_SERVER
  //Blynk.begin(auth, server, BLYNK_HARDWARE_PORT);
  Serial.println(F("Start Blynk"));
  Blynk.begin(auth, ssid, pass, server, BLYNK_HARDWARE_PORT);
#else
  Blynk.begin(auth);
  // You can also specify server:
  //Blynk.begin(auth, server, BLYNK_HARDWARE_PORT);
#endif

  if (Blynk.connected())
  {
    Serial.print(F("IP = ")); Serial.println(WiFi.localIP());
  }
}

#define BLOCKING_TIME_MS      3000L

void loop()
{
  Blynk.run();

  // This unadvised blocking task is used to demonstrate the blocking effects onto the execution and accuracy to Software timer
  // You see the time elapse of ISR_Timer still accurate, whereas very unaccurate for Software Timer
  // The time elapse for 2000ms software timer now becomes 3000ms (BLOCKING_TIME_MS)
  // While that of ISR_Timer is still prefect.
  delay(BLOCKING_TIME_MS);

  // You need this Software timer for non-critical tasks. Avoid abusing ISR if not absolutely necessary
  // You don't need to and never call ISR_Timer.run() here in the loop(). It's already handled by ISR timer.
  blynkTimer.run();
}
```
---
---

### Debug Terminal Output Samples

### 1. ISR_Timer_Complex_WiFiNINA on Arduino SAMD21 SAMD_NANO_33_IOT using WiFiNINA

The following is the sample terminal output when running example [ISR_Timer_Complex_WiFiNINA](examples/ISR_Timer_Complex_WiFiNINA) on **SAMD_NANO_33_IOT using Built-in WiFiNINA** to demonstrate the accuracy of ISR Hardware Timer, **especially when system is very busy**.  The ISR timer is **programmed for 2s, is activated exactly after 2.000s !!!**

While software timer, **programmed for 2s, is activated after 7.937s !!!**. Then in loop(), it's also activated **every 3s**.

```
Starting ISR_Timer_Complex_WiFiNINA on SAMD_NANO_33_IOT
SAMDTimerInterrupt v1.2.0
CPU Frequency = 48 MHz
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Starting  ITimer OK, millis() = 810
Start Blynk
[1571] WiFiNINA Firmware Version: 1.4.1
[1572] Con2:HueNet1
2s: Delta ms = 2000
[5439] ConW OK
[5439] IP:192.168.2.98
[5439] 
    ___  __          __
   / _ )/ /_ _____  / /__
  / _  / / // / _ \/  '_/
 /____/_/\_, /_//_/_/\_\
        /___/ v0.6.1 on NANO_33_IOT

[5440] BlynkArduinoClient.connect: Connecting to account.duckdns.org:8080
[5673] Ready (ping: 6ms).
IP = 192.168.2.98
2s: Delta ms = 2000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 7937
2s: Delta ms = 2000
2s: Delta ms = 2000
5s: Delta ms = 5000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3002
2s: Delta ms = 2000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3002
2s: Delta ms = 2000
5s: Delta ms = 5000
2s: Delta ms = 2000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3002
2s: Delta ms = 2000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3004
2s: Delta ms = 2000
5s: Delta ms = 5000
2s: Delta ms = 2000
11s: Delta ms = 11000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3002
2s: Delta ms = 2000
5s: Delta ms = 5000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3003
2s: Delta ms = 2000
2s: Delta ms = 2000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3002
2s: Delta ms = 2000
5s: Delta ms = 5000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3004
2s: Delta ms = 2000
11s: Delta ms = 11000
2s: Delta ms = 2000
blynkDoingSomething2s: Delta programmed ms = 2000, actual = 3002
5s: Delta ms = 5000
2s: Delta ms = 2000
```

---

### 2. TimerInterruptTest on Adafruit SAMD51 ITSYBITSY_M4

The following is the sample terminal output when running example [**TimerInterruptTest**](examples/TimerInterruptTest) on **Adafruit SAMD51 ITSYBITSY_M4** to demonstrate how to start/stop Hardware Timers.

```
Starting TimerInterruptTest on ITSYBITSY_M4
SAMDTimerInterrupt v1.2.0
CPU Frequency = 48 MHz
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 120 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 0x4101c000 , TC3 = 0x 0x4101c000
Starting  ITimer1 OK, millis() = 1820
ITimer0: millis() = 2820, delta = 1000
ITimer0: millis() = 3820, delta = 1000
ITimer0: millis() = 4820, delta = 1000
Stop ITimer0, millis() = 5001
Start ITimer0, millis() = 10002
ITimer0: millis() = 11002, delta = 1000
ITimer0: millis() = 12002, delta = 1000
ITimer0: millis() = 13002, delta = 1000
ITimer0: millis() = 14002, delta = 1000
ITimer0: millis() = 15002, delta = 1000
Stop ITimer0, millis() = 15003
Start ITimer0, millis() = 20004
ITimer0: millis() = 21004, delta = 1000
ITimer0: millis() = 22004, delta = 1000
ITimer0: millis() = 23004, delta = 1000
ITimer0: millis() = 24004, delta = 1000
ITimer0: millis() = 25004, delta = 1000
Stop ITimer0, millis() = 25005
Start ITimer0, millis() = 30006
ITimer0: millis() = 31006, delta = 1000
ITimer0: millis() = 32006, delta = 1000
ITimer0: millis() = 33006, delta = 1000
ITimer0: millis() = 34006, delta = 1000
ITimer0: millis() = 35006, delta = 1000
Stop ITimer0, millis() = 35007
Start ITimer0, millis() = 40008
ITimer0: millis() = 41008, delta = 1000
ITimer0: millis() = 42008, delta = 1000
ITimer0: millis() = 43008, delta = 1000
ITimer0: millis() = 44008, delta = 1000
ITimer0: millis() = 45008, delta = 1000
Stop ITimer0, millis() = 45009
Start ITimer0, millis() = 50010
ITimer0: millis() = 51010, delta = 1000
ITimer0: millis() = 52010, delta = 1000
ITimer0: millis() = 53010, delta = 1000
ITimer0: millis() = 54010, delta = 1000
ITimer0: millis() = 55010, delta = 1000
Stop ITimer0, millis() = 55011
Start ITimer0, millis() = 60012
ITimer0: millis() = 61012, delta = 1000
ITimer0: millis() = 62012, delta = 1000
ITimer0: millis() = 63012, delta = 1000
ITimer0: millis() = 64012, delta = 1000
ITimer0: millis() = 65012, delta = 1000
Stop ITimer0, millis() = 65013
Start ITimer0, millis() = 70014
ITimer0: millis() = 71014, delta = 1000
ITimer0: millis() = 72014, delta = 1000
ITimer0: millis() = 73014, delta = 1000
ITimer0: millis() = 74014, delta = 1000
ITimer0: millis() = 75014, delta = 1000
Stop ITimer0, millis() = 75015
Start ITimer0, millis() = 80016
ITimer0: millis() = 81016, delta = 1000
ITimer0: millis() = 82016, delta = 1000
ITimer0: millis() = 83016, delta = 1000
ITimer0: millis() = 84016, delta = 1000
ITimer0: millis() = 85016, delta = 1000
Stop ITimer0, millis() = 85017
Start ITimer0, millis() = 90018
ITimer0: millis() = 91018, delta = 1000

```

---

### 3. Argument_None on Arduino SAMD21 SAMD_NANO_33_IOT

The following is the sample terminal output when running example [**Argument_None**](examples/Argument_None) on **Arduino SAMD21 SAMD_NANO_33_IOT** to demonstrate how to start/stop Multiple Hardware Timers.

```
Starting Argument_None on SAMD_NANO_33_IOT
SAMDTimerInterrupt v1.2.0
CPU Frequency = 48 MHz
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Starting  ITimer1 OK, millis() = 910
F_CPU (MHz) = 48
TC_Timer::startTimer _Timer = 0x42002000, TCC0 = 0x42002000
Starting  ITimer1 OK, millis() = 911
ITimer0: millis() = 1410, delta = 500
ITimer0: millis() = 1910, delta = 500
ITimer0: millis() = 2410, delta = 500
ITimer0: millis() = 2910, delta = 500
ITimer1: millis() = 2911, delta = 2000
ITimer0: millis() = 3410, delta = 500
ITimer0: millis() = 3910, delta = 500
ITimer0: millis() = 4410, delta = 500
ITimer0: millis() = 4910, delta = 500
ITimer1: millis() = 4911, delta = 2000
ITimer0: millis() = 5410, delta = 500
ITimer0: millis() = 5910, delta = 500
ITimer0: millis() = 6410, delta = 500
ITimer0: millis() = 6910, delta = 500
ITimer1: millis() = 6911, delta = 2000
ITimer0: millis() = 7410, delta = 500
ITimer0: millis() = 7910, delta = 500
ITimer0: millis() = 8410, delta = 500
ITimer0: millis() = 8910, delta = 500
ITimer1: millis() = 8911, delta = 2000
ITimer0: millis() = 9410, delta = 500
ITimer0: millis() = 9910, delta = 500
ITimer0: millis() = 10410, delta = 500
ITimer0: millis() = 10910, delta = 500
ITimer1: millis() = 10911, delta = 2000
ITimer0: millis() = 11410, delta = 500
ITimer0: millis() = 11910, delta = 500
```

---

### 4. ISR_16_Timers_Array on Arduino SAMD21 SAMD_NANO_33_IOT

The following is the sample terminal output when running example [ISR_16_Timers_Array](examples/ISR_16_Timers_Array) on **Arduino SAMD21 SAMD_NANO_33_IOT** to demonstrate the accuracy of ISR Hardware Timer, **especially when system is very busy or blocked**. The 16 independent ISR timers are **programmed to be activated repetitively after certain intervals, is activated exactly after that programmed interval !!!**

While software timer, **programmed for 2s, is activated after 10.000s in loop()!!!**.

In this example, 16 independent ISR Timers are used, yet utilized just one Hardware Timer. The Timer Intervals and Function Pointers are stored in arrays to facilitate the code modification.


```
Starting ISR_16_Timers_Array on SAMD_NANO_33_IOT
SAMDTimerInterrupt v1.2.0
CPU Frequency = 48 MHz
CPU Frequency = 48 MHz
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Starting  ITimer OK, millis() = 1421
1s: Delta ms = 1000, ms = 2421
1s: Delta ms = 1000, ms = 3421
2s: Delta ms = 2000, ms = 3421
1s: Delta ms = 1000, ms = 4421
3s: Delta ms = 3000, ms = 4421
1s: Delta ms = 1000, ms = 5421
2s: Delta ms = 2000, ms = 5421
4s: Delta ms = 4000, ms = 5421
1s: Delta ms = 1000, ms = 6421
5s: Delta ms = 5000, ms = 6421
1s: Delta ms = 1000, ms = 7421
2s: Delta ms = 2000, ms = 7421
3s: Delta ms = 3000, ms = 7421
6s: Delta ms = 6000, ms = 7421
1s: Delta ms = 1000, ms = 8421
7s: Delta ms = 7000, ms = 8421
1s: Delta ms = 1000, ms = 9421
2s: Delta ms = 2000, ms = 9421
4s: Delta ms = 4000, ms = 9421
8s: Delta ms = 8000, ms = 9421
1s: Delta ms = 1000, ms = 10421
3s: Delta ms = 3000, ms = 10421
9s: Delta ms = 9000, ms = 10421
1s: Delta ms = 1000, ms = 11421
2s: Delta ms = 2000, ms = 11421
5s: Delta ms = 5000, ms = 11421
10s: Delta ms = 10000, ms = 11421
simpleTimerDoingSomething2s: Delta programmed ms = 2000, actual = 10000
1s: Delta ms = 1000, ms = 12421
11s: Delta ms = 11000, ms = 12421
1s: Delta ms = 1000, ms = 13421
2s: Delta ms = 2000, ms = 13421
3s: Delta ms = 3000, ms = 13421
4s: Delta ms = 4000, ms = 13421
6s: Delta ms = 6000, ms = 13421
12s: Delta ms = 12000, ms = 13421
1s: Delta ms = 1000, ms = 14421
13s: Delta ms = 13000, ms = 14421
1s: Delta ms = 1000, ms = 15421
2s: Delta ms = 2000, ms = 15421
7s: Delta ms = 7000, ms = 15421
14s: Delta ms = 14000, ms = 15421
1s: Delta ms = 1000, ms = 16421
3s: Delta ms = 3000, ms = 16421
5s: Delta ms = 5000, ms = 16421
15s: Delta ms = 15000, ms = 16421
1s: Delta ms = 1000, ms = 17421
2s: Delta ms = 2000, ms = 17421
4s: Delta ms = 4000, ms = 17421
8s: Delta ms = 8000, ms = 17421
16s: Delta ms = 16000, ms = 17421
1s: Delta ms = 1000, ms = 18421
1s: Delta ms = 1000, ms = 19421
2s: Delta ms = 2000, ms = 19421
3s: Delta ms = 3000, ms = 19421
6s: Delta ms = 6000, ms = 19421
9s: Delta ms = 9000, ms = 19421
1s: Delta ms = 1000, ms = 20421
1s: Delta ms = 1000, ms = 21421
2s: Delta ms = 2000, ms = 21421
4s: Delta ms = 4000, ms = 21421
5s: Delta ms = 5000, ms = 21421
10s: Delta ms = 10000, ms = 21421
simpleTimerDoingSomething2s: Delta programmed ms = 2000, actual = 10000
1s: Delta ms = 1000, ms = 22421
3s: Delta ms = 3000, ms = 22421
7s: Delta ms = 7000, ms = 22421
1s: Delta ms = 1000, ms = 23421
2s: Delta ms = 2000, ms = 23421
11s: Delta ms = 11000, ms = 23421
1s: Delta ms = 1000, ms = 24421
1s: Delta ms = 1000, ms = 25421
2s: Delta ms = 2000, ms = 25421
3s: Delta ms = 3000, ms = 25421
4s: Delta ms = 4000, ms = 25421
6s: Delta ms = 6000, ms = 25421
8s: Delta ms = 8000, ms = 25421
12s: Delta ms = 12000, ms = 25421
1s: Delta ms = 1000, ms = 26421
5s: Delta ms = 5000, ms = 26421
1s: Delta ms = 1000, ms = 27421
2s: Delta ms = 2000, ms = 27421
13s: Delta ms = 13000, ms = 27421
1s: Delta ms = 1000, ms = 28421
3s: Delta ms = 3000, ms = 28421
9s: Delta ms = 9000, ms = 28421
1s: Delta ms = 1000, ms = 29421
2s: Delta ms = 2000, ms = 29421
4s: Delta ms = 4000, ms = 29421
7s: Delta ms = 7000, ms = 29421
14s: Delta ms = 14000, ms = 29421
1s: Delta ms = 1000, ms = 30421
1s: Delta ms = 1000, ms = 31421
2s: Delta ms = 2000, ms = 31421
3s: Delta ms = 3000, ms = 31421
5s: Delta ms = 5000, ms = 31421
6s: Delta ms = 6000, ms = 31421
10s: Delta ms = 10000, ms = 31421
15s: Delta ms = 15000, ms = 31421
simpleTimerDoingSomething2s: Delta programmed ms = 2000, actual = 10000
1s: Delta ms = 1000, ms = 32421
1s: Delta ms = 1000, ms = 33421
2s: Delta ms = 2000, ms = 33421
4s: Delta ms = 4000, ms = 33421
8s: Delta ms = 8000, ms = 33421
16s: Delta ms = 16000, ms = 33421
1s: Delta ms = 1000, ms = 34421
3s: Delta ms = 3000, ms = 34421
11s: Delta ms = 11000, ms = 34421
```

---

### 5. Change_Interval on Arduino SAMD21 SAMD_NANO_33_IOT

The following is the sample terminal output when running example [Change_Interval](examples/Change_Interval) on **Arduino SAMD21 SAMD_NANO_33_IOT** to demonstrate how to change Timer Interval on-the-fly

```
Starting Change_Interval on SAMD_NANO_33_IOT
SAMDTimerInterrupt v1.2.0
CPU Frequency = 48 MHz
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Starting  ITimer OK, millis() = 1131
Time = 10001, TimerCount = 17
Time = 20002, TimerCount = 37
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 1000
Time = 30003, TimerCount = 47
Time = 40004, TimerCount = 57
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 500
Time = 50005, TimerCount = 77
Time = 60006, TimerCount = 97
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 1000
Time = 70007, TimerCount = 107
Time = 80008, TimerCount = 117
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 500
Time = 90009, TimerCount = 137
Time = 100010, TimerCount = 157
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 1000
Time = 110011, TimerCount = 167
Time = 120012, TimerCount = 177
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 500
Time = 130013, TimerCount = 197
Time = 140014, TimerCount = 217
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 1000
Time = 150015, TimerCount = 227
Time = 160016, TimerCount = 237
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 500
Time = 170017, TimerCount = 257
Time = 180018, TimerCount = 277
[TISR] SAMDTimerInterrupt: F_CPU (MHz) = 48 , TIMER_HZ = 48
[TISR] TC_Timer::startTimer _Timer = 0x 42002c00 , TC3 = 0x 42002c00
Changing Interval, Timer = 1000
Time = 190019, TimerCount = 287
Time = 200020, TimerCount = 297

```

---
---

### Debug

Debug is enabled by default on Serial.

You can also change the debugging level (_TIMERINTERRUPT_LOGLEVEL_) from 0 to 4

```cpp
// These define's must be placed at the beginning before #include "SAMD_TimerInterrupt.h"
// _TIMERINTERRUPT_LOGLEVEL_ from 0 to 4
// Don't define _TIMERINTERRUPT_LOGLEVEL_ > 0. Only for special ISR debugging only. Can hang the system.
#define TIMER_INTERRUPT_DEBUG         0
#define _TIMERINTERRUPT_LOGLEVEL_     0
```

---

### Troubleshooting

If you get compilation errors, more often than not, you may need to install a newer version of the core for Arduino boards.

Sometimes, the library will only work if you update the board core to the latest version because I am using newly added functions.

---
---

## Releases

### Releases v1.2.0

1. Add better debug feature.
2. Optimize code and examples to reduce RAM usage
3. Add Table of Contents

### Releases v1.1.1

1. Add example [**Change_Interval**](examples/Change_Interval) and [**ISR_16_Timers_Array_Complex**](examples/ISR_16_Timers_Array_Complex)
2. Bump up version to sync with other TimerInterrupt Libraries. Modify Version String.

### Releases v1.0.1

1. Add complicated example [ISR_16_Timers_Array](examples/ISR_16_Timers_Array) utilizing and demonstrating the full usage of 16 independent ISR Timers.

### Releases v1.0.0

1. Permit up to 16 super-long-time, super-accurate ISR-based timers to avoid being blocked
2. Using cpp code besides Impl.h code to use if Multiple-Definition linker error.

#### Supported Boards

  - **Arduino SAMD21 (ZERO, MKR, NANO_33_IOT, etc.)**.
  - **Adafruit SAM21 (Itsy-Bitsy M0, Metro M0, Feather M0, Gemma M0, etc.)**.
  - **Adafruit SAM51 (Itsy-Bitsy M4, Metro M4, Grand Central M4, Feather M4 Express, etc.)**.
  - **Seeeduino SAMD21/SAMD51 boards (SEEED_WIO_TERMINAL, SEEED_FEMTO_M0, SEEED_XIAO_M0, Wio_Lite_MG126, WIO_GPS_BOARD, SEEEDUINO_ZERO, SEEEDUINO_LORAWAN, SEEED_GROVE_UI_WIRELESS, etc.)** 

---
---

### Issues

Submit issues to: [SAMD_TimerInterrupt issues](https://github.com/khoih-prog/SAMD_TimerInterrupt/issues)

---

## TO DO

1. Search for bug and improvement.
2. Similar features for remaining Arduino boards such as SAM-DUE


## DONE


1. Basic hardware timers for SAMD21 and SAMD51.
2. More hardware-initiated software-enabled timers
3. Longer time interval
4. Similar features for remaining Arduino boards such as ESP32, ESP8266, STM32, nRF52, mbed-nRF52, Teensy, etc.
5. Add Table of Contents


---
---

### Contributions and Thanks

Many thanks for everyone for bug reporting, new feature suggesting, testing and contributing to the development of this library.

1. Use some code from the [**Tamasa's ZeroTimer Library**](https://github.com/EHbtj/ZeroTimer).
2. Use some code from the [**Dennis van Gils' SAMD51_InterruptTimer Library**](https://github.com/Dennis-van-Gils/SAMD51_InterruptTimer).

<table>
  <tr>
    <td align="center"><a href="https://github.com/EHbtj"><img src="https://github.com/EHbtj.png" width="100px;" alt="EHbtj"/><br /><sub><b>⭐️ Tamasa</b></sub></a><br /></td>
    <td align="center"><a href="https://github.com/Dennis-van-Gils"><img src="https://github.com/Dennis-van-Gils.png" width="100px;" alt="Dennis-van-Gils"/><br /><sub><b> Dennis van Gils</b></sub></a><br /></td>
  </tr> 
</table>

---

### Contributing

If you want to contribute to this project:

- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library

---

### License

- The library is licensed under [MIT](https://github.com/khoih-prog/SAMD_TimerInterrupt/blob/master/LICENSE)

---

## Copyright

Copyright 2020- Khoi Hoang


