# RFID RC522 test

**Table Of Contents**
* [Top](#rfid-rc522-test "Top")
* [Hardware](#hardware "Hardware")
* [Software](#software "Software")
  * [Multiple SPI Devices on Same SPI Bus](#multiple-spi-devices-on-same-spi-bus "Multiple SPI Devices on Same SPI Bus")
  * [All ESP32 Hardware SPI Ports Used Up - Need Software Bit Banging](#all-esp32-hardware-spi-ports-used-up-\--need-software-bit-banging "All ESP32 Hardware SPI Ports Used Up - Need Software Bit Banging")
    * [Touchscreen Calibration with XPT2046_Bitbang](#touchscreen-calibration-with-xpt2046_bitbang "Touchscreen Calibration with XPT2046_Bitbang")
    * [All Together Now](#all-together-now "All Together Now")
* [Documentation](#documentation "Documentation")
* [MicroSD Sniffer Cards](#microsd-sniffer-cards "MicroSD Sniffer Cards")
  * [SPI Sparkfun Sniffer Info](#spi-sparkfun-sniffer-info "SPI Sparkfun Sniffer Info")
  * [Blinken Labs Sniffer Info](#blinken-labs-sniffer-info "Blinken Labs Sniffer Info")

## Hardware
[Top](#rfid-rc522-test "Top")<br>
| Link | Description |
| --- | --- |
| https://www.amazon.com/dp/B07VLDSYRW | **READER/WRITER** HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card + Key Ring |
| https://www.amazon.com/dp/B07S63VT7X | **CARDS** Meikuler 13.56MHz MIFARE Classic 1K, RFID Smart Cards / M1 Cards, ISO14443A Printable Blank RFID PVC Cards |

- This RC522 RFID reader doesn't support I2C or UART without board modifications; just SPI
  - even though the IC itself supports I2C and UART, the board pins and strapping are just for SPI interface
  - look for Adrianotiger post #4 on Mar 2023 on https://forum.arduino.cc/t/esp32-rfid-rc522-i2c/1100200/3
  - here is webpage for the module including schematic https://www.sunrom.com/p/rfid-readerwriter-1356mhz-rc522-spi-module-with-cardkeychain
- On the other hand, the CYD board doesn't have enough unused GPIO pins coming out natively to use SPI
  - nominally requires 5 lines in addition to GND and 3.3V - see below for discussion of the "RESET" or "RST" line
  - the CYD MicroSD card reader uses SPI; a "sniffer" card can be used to access the GPIO lines from that

- RESET
  - Looking at the code for the "RFID_MFRC522v2 by Github Community" library, it appears that in 25 Jun 2020, v2.0.0 the library quit using the hardware reset pin and now uses only the software reset
  - You can still find the commented-out hardware reset code near Arduino/libraries/RFID_MFRC522v2/src/MFRC522v2.cpp line 81
  - In my experience this library works without connecting the hardware RST line
  - Thus the number of lines (and GPIO pins) needed is 4 in addition to GND and 3.3V

## Software
[Top](#rfid-rc522-test "Top")<br>
The original plan was to store the command string directly in the RFID card and have the RFID reader/writer on the CYD I2C bus along with the QR reader. See the [Hardware](#hardware "Hardware") section; this is not feasible.
Options:
- use MicroSD sniffer on the CYD and control both devices that way
  - https://www.sparkfun.com/sparkfun-microsd-sniffer.html
  - https://learn.sparkfun.com/tutorials/microsd-sniffer-hookup-guide/introduction
  - https://github.com/sparkfun/MicroSD_Sniffer.git
- have another ESP32 or 3.3V microcontroller that is on the I2C bus and use it to control the SPI bus to the RFID reader/writer.

Using the MicroSD sniffer on the CYD works.

### Multiple SPI Devices on Same SPI Bus
[Top](#rfid-rc522-test "Top")<br>
Some resources
- https://docs.arduino.cc/learn/communication/spi/
- https://forum.arduino.cc/t/multiple-spi-chip-select-question/101140
- https://forum.arduino.cc/t/how-to-use-spi-for-multiple-devices/427739

It turns out that the MicroSD card and the Touchscreen are not set up for this purpose.
- To use this technique they would need to share all the GPIO SPI pins except for SPI Chip Select (CS).
- These two SPI interfaces on the CYD share no GPIO pins.

Because of this we need to use software bit banging for one SPI port to free up the hardware SPI port for use on the MicroSD card. It is common to use the software bit banging for interfacing with the touch screen since it has a low amount of actual data to transfer and the software SPI interface is slower than the hardware SPI interface.

### All ESP32 Hardware SPI Ports Used Up - Need Software Bit Banging
[Top](#rfid-rc522-test "Top")<br>
This general Arduino usage doc for LVGL says "To get started it's recommended to use TFT_eSPI library as a TFT driver to simplify testing."
- https://docs.lvgl.io/master/details/integration/framework/arduino.html

However, in the case of the Cheap Yellow Display, there are not enough available hardware SPI resources on the ESP32 to handle the display (HSPI), the touchscreen (VSPI), and the MicroSD card slot (out of luck) at the same time (along with the other HW SPI ports used for general purpose stuff like FLASH memory).
  - https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
  - Using bit-banging so touch and SD can work in same program - https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/TROUBLESHOOTING.md#display-touch-and-sd-card-are-not-working-at-the-same-time

For that reason I am using the XPT2046_Bitbang library instead of the TFT_eSPI library for the touchscreen. This seems to work OK with some changes to the code.
- This library can be installed from the Arduino IDE library manager (Search for "XPT2046 Slim")

#### Touchscreen Calibration with XPT2046_Bitbang
[Top](#rfid-rc522-test "Top")<br>
I made a version of the Random Nerd CYDCalibrate.ino
- Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/touchscreen-calibration/

... that I call  CYDbitBangCalibrate.ino here:
- https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDbitBangCalibrate

This is a good way to see how to change from CYD touchscreen operation using the XPT2046_Bitbang library instead of the TFT_eSPI library: compare the code from the following two sources
- TFT_eSPI - https://RandomNerdTutorials.com/touchscreen-calibration/
- XPT2046_Bitbang - https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDbitBangCalibrate

For some reason I don't understand, the calibration constants generated are fairly different between the two calibration routines using different SPI libraries.

If you use the XPT2046_Bitbang library and want to calibrate the touchscreen, you might want to use something similar to my CYDbitBangCalibrate program to generate the calibration constants.

#### All Together Now
[Top](#rfid-rc522-test "Top")<br>
https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD contains a program that uses
- lvgl library
- XPT2046_Bitbang library
- MicroSD Sniffer and HW VSPI for RC522 RF IC Card Sensor Module
- I2C for QR Code reader

At this time that is still a work in progress, but it does operate all the devices.

I used my CYDbitBangCalibrate calibration constants in the cyd_input_read() routine found in https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD/cyd_input_read.h<br>
I will probably rework this UniRemoteCYD program further but I don't expect to change the routine name cyd_input_read so you should be able to find it.

## Documentation
[Top](#rfid-rc522-test "Top")<br>
Documentation of the MIFARE Classic 1K card
- https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf

Documentation of the MFRC522v2 library
- https://osslibraries.github.io/Arduino_MFRC522v2/

I used the Random Nerd tutorials to get up to speed.
- https://randomnerdtutorials.com/esp32-spi-communication-arduino/
- https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
- https://randomnerdtutorials.com/esp32-i2c-master-slave-arduino/
- https://randomnerdtutorials.com/esp32-spi-communication-arduino/
- https://randomnerdtutorials.com/esp32-mfrc522-rfid-reader-arduino/

More documentation on the CYD internal pinouts and the connector types.
- https://debugdiaries.co.uk/esp32-cheap-display-cyd-pinouts/ - compact, useful format
- https://macsbug.wordpress.com/2022/08/17/esp32-2432s028/ - need to Google-translate from Japanese but quite good

## MicroSD Sniffer Cards
[Top](#rfid-rc522-test "Top")<br>
I am using the Arduino library RFID_MFRC522v2 by GithubCommunity. An important point when using this library in the CYD is that the reset pin RST (ESP32 pin 21) is unused since 25 Jun 2020 v2.0.0. That is fortunate, since in the CYD pin 21 is used for other purposes. I tried an experiment leaving it floating and that seemed to work.

### SPI Sparkfun Sniffer Info
[Top](#rfid-rc522-test "Top")<br>
The Sparkfun MicroSD card sniffer carried all the signals needed for using the RC522 RFID card reader.
- https://www.sparkfun.com/sparkfun-microsd-sniffer.html

GPIO channels for Sparkfun MicroSD card sniffer on the CYD are shown below.
- NOTE: "-" (single dash) means No Connect. Had to do this since on Blinken Labs JTAG sniffer with Treedix JTAG breakout board NC is used as a label.

| Sparkfun | Alt | ESP32 GPIO pin | RC522 | Color | Comment |
| --- | --- | --- | --- | --- | --- |
| DAT2 |  |  |  | - | unused |
| CD | CS | 5 | SDA | Green | TF_CS |
| CMD | MOSI | 23 | MOSI | Yellow |  |
| GND | GND |  | GND | Black | ground |
| VCC | VDD |  | 3.3V | Red | 3.3V |
| CLK | CLX | 18 | SCK | White | TF_CLK |
| DAT0 | MISO | 19 | MISO | Blue |  |
| DAT1 |  |  |  | - | unused |
|  |  | 21 | RST | - | unused |
|  |  |  | IRQ | - | unused |

### Blinken Labs Sniffer Info
[Top](#rfid-rc522-test "Top")<br>
The Blinken Labs MicroSD to JTAG adapter for ESP32 carries most of the signals needed for using the RC522 RFID card reader. This card was not designed as a general purpose MicroSD sniffer but rather as a JTAG interface; it has all the signals needed for the JTAG interface.
- https://shop.blinkinlabs.com/products/microsd-to-jtag-adapter-for-esp32

I connected the Blinken Labs board via JTAG cable to a Treedix JTAG breakout card.
- https://www.amazon.com/dp/B09DKDG7XN

As before, "-" (single dash) means No Connect.

| BLINKIN | Brkt-Bd | Alt | ESP32 GPIO pin | RC522 | Color  | Comment |
| --- | --- | --- | --- | --- | --- | --- |
| DATA2/TDI | NC | - | - |  | unused |
| DATA3/TCK | CLK | CS | 5 | SDA | Green | TF_CS |
| CMD/TD0 | SW0 | MOSI | 23 | MOSI | Yellow |  |
| 3.3V-jumper | VCC | 3.3V | 3.3V | 3.3V | Red | 3.3V |
| CLK/TMS | SWIO | CLX | 18 | SCK | White | TF_CLK |
| GND | GNDd | GND | GND | GND | Black | ground |
| DATA3 | - | - | - | - |  | 19 MISO - not wired |
| DATA2 | - | - | - |  |  | unused |
| GND | GND(1) | GND | GND | GND |  | connected internally |
| GND | GND(2) | GND | GND | GND |  | connected internally |
| - | /RST | - | - | - |  | unused |
| - | KEY | - | - | - |  | unused |
