# Mega Blaster 2

Demo video can be found here: https://www.youtube.com/watch?v=4sKwGWfikdc

[Instruction Manual](https://github.com/AidanHockey5/MegaBlaster2/raw/master/man/MegaBlaster2InstructionManual.pdf)

[Store Link](https://www.aidanlawrence.com/product/mega-blaster-2/)


This project contains the source material for the Mega Blaster 2, a hardware Sega Genesis/Mega Drive video game music (VGM) player that utilizes the real sound chips. Play any Genesis track on-demand with brilliant sound quality on the actual YM2612/YM3438 and SN76489 PSG synthesizer chips that were found in the real Genesis consoles(1). The Mega Blaster 2 is compatible with both .vgm and .vgz files and can support both the YM2612 and YM3438 simply by changing the DIP switch values. This unit does not use any emulated sound - these are the real chips playing back logged data from the vgm files. 

# VGM Files

VGM files are logged datasets of the data that would have been sent to the real sound chips of whatever console or arcade machine was being sampled. The Mega Blaster 2 simply reads-in this data and sends it to the sound chips at the exact rate specified in the VGM spec, accurately recreating the original sound.

You can find Genesis VGM files here: https://project2612.org/list.php

You can find more VGM files here: https://vgmrips.net/packs/

Please make sure to remove any non-vgm/vgz files before adding them to your Mega Blaster 2's SD card, such as album art images, playlist files, and text files.

# VGM vs. VGZ

VGZ files are gzip compressed versions of VGM files. The Mega Blaster 2 is capable of automatically decompressing these vgz files on-the-fly at a slight performance penalty. If you'd like faster load times for each track, consider decompressing the VGZ files in advance by opening them with a program like [7zip](https://www.7-zip.org/)
The Mega Blaster 2 does not require file extensions in order to operate as it will verify files based on their internal headers.

# Building Your Own Unit

Here are some quick tips for building your own board.

1) You can find an interactive Bill of Materials (BOM) within the `schematic->bom` folder. Open the HTML file in any browser for an easy-to-use part position guide.
2) I use [KiCAD](https://kicad.org/) as my EDA suite.
4) I buy most of my components from [LCSC](https://lcsc.com/). I buy my PCBs from [JLCPCB](https://jlcpcb.com/). If you buy your PCBs from JLCPCB, you can often get a shipping discount on your LCSC parts order.
5) You will need an [Atmel ICE](https://www.microchip.com/DevelopmentTools/ProductDetails/ATATMEL-ICE) or [Jlink](https://www.segger.com/products/debug-probes/j-link/) to flash the bootloader. Once you have flashed the bootloader, all subsequent programming can be done via the on-board USB port. A cheaper option would be to grab a [Jlink EDU Mini](https://shop-us.segger.com/J_Link_EDU_mini_p/8.08.91.htm) as they feature the full functionality of a professional Jlink Unit with the stipulation that it is ONLY to be used for educational purposes. I personally use an Atmel ICE unit. You can find the bootloader bin file in the `bootloader` folder of the repository. [You can find an example of me programming the bootloader using Microchip Studio here.](https://youtu.be/FjPftGuLnGg?t=9259)
6) Once you've flashed the bootloader, you can drag the [.uf2 firmware file found on the releases page](https://github.com/AidanHockey5/MegaBlaster2/releases) on to the "drive" that pops up on your computer, or build from source. If you can't see any "drive," make sure the bootloader mentioned above is flashed correctly, then try double-tapping the RESET button. 
7) If your OS reports a USB fault, you may have a short somewhere on your USB IO lines, power lines, or your 32.786KHz crystal is not hooked up correctly.
8) I use [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/install/ide?install=vscode) extension.
9) Make sure your SN76489 PSG chip is orientated correctly. On this board, the PSG is upside-down relative to the rest of the board. The Flash IC is also upside-down. This was done for routing purposes. A small star on the silk-screen indicates the pin-1 position of every IC.
10) For general usage instructions, [please refer to the manual](https://github.com/AidanHockey5/MegaBlaster2/raw/master/man/MegaBlaster2InstructionManual.pdf).

# Reporting Bugs

As you can imagine, this is quite a complex project. There are bound to be bugs to squish, so if you happen to find any, [please report them over on the issues page](https://github.com/AidanHockey5/MegaBlaster2/issues) so I can tkae a look at them! Please try to document the steps it takes to recreate the bug if you happen to know them.

# Schematic

![Schematic](https://github.com/AidanHockey5/MegaBlaster2/raw/master/schematic/MegaBlaster2.png)

# License
The source code to this project is licensed under the AGPL 3.0 license.

The printed circuit board design files for the Mega Blaster 2, including it’s schematic, gerber files, KiCAD files, bill-of-materials, and 3D models are licensed under the Creative Commons Attribution-NonCommercial-ShareAlike 4.0 license.

You are permitted to build your own Mega Blaster 2 units for personal use. You are not permitted to sell said units.

(1) Pedantic note: Discrete YM2612 ICs were indeed included on some models of the Genesis, but the SN76489 PSG and YM3438 were found baked into ASIC chips on the actual consoles. The PSG's noise is slightly different on the ASIC version.
