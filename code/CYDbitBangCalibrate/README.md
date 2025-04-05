# CYDbitBangCalibrate

**Table Of Contents**
* [Top](#cydbitbangcalibrate "Top")
* [Arduino IDE Board Selection](#arduino-ide-board-selection "Arduino IDE Board Selection")
* [The Idea](#the-idea "The Idea")
* [Attributions](#attributions "Attributions")

## Arduino IDE Board Selection
* [Top](#cydbitbangcalibrate "Top")<br>
In the Arduino IDE, select the board as "ESP32 Dev Module". The CYD is based on the ESP32 D0WDQ6.

## The Idea
[Top](#cydbitbangcalibrate "Top")<br>
I am using the CYD pins that connect to the MicroSD slot for an SPI interface, but the CYD "normally" uses all four hardware SPI interfaces from the ESP32.
I wanted to free up an ESP32 hardware SPI channel so I adapted some code to use the XPT2046_Bitbang software-based SPI library for the touchscreen interface instead of the TFT_eSPI library.
This frees up the ESP32 VSPI hardware SPI channel for other uses.

This code is an adaptation of the Random Nerds touchscreen calibration routine that uses this XPT2046_Bitbang library.

## Attributions
[Top](#cydbitbangcalibrate "Top")<br>
The version of the software for CYD (ESP32-2432S028R or Cheap Yellow Display) screen touch calibration that I started from was
- Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/touchscreen-calibration/

The XPT2046_Bitbang library can be installed from the library manager (search for "XPT2046 Slim").
- https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library
