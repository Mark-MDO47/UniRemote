# RFID RC522 test

**Table Of Contents**
* [Top](#rfid-rc522-test "Top")
* [Hardware](#hardware "Hardware")
* [Software](#software "Software")

## Hardware
[Top](#rfid-rc522-test "Top")<br>
| Link | Description |
| --- | --- |
| https://www.amazon.com/dp/B07VLDSYRW | **READER/WRITER** HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card + Key Ring |
| https://www.amazon.com/dp/B07S63VT7X | **CARDS** Meikuler 13.56MHz MIFARE Classic 1K, RFID Smart Cards / M1 Cards, ISO14443A Printable Blank RFID PVC Cards |

- This RFID reader doesn't support I2C or UART without board modifications; just SPI
  - even though the IC itself supports I2C and UART, the board pins and strapping are just for SPI interface
  - look for Adrianotiger post #4 on Mar 2023 on https://forum.arduino.cc/t/esp32-rfid-rc522-i2c/1100200/3
- On the other hand, the CYD board doesn't have enough SPI pins coming out natively
  - requires 5 lines in addition to GND and 3.3V
  - theoretically the MicroSD card reader uses SPI; maybe I can pick up the signals with a "sniffer" card and use that

## Software
[Top](#rfid-rc522-test "Top")<br>
The original plan was to store the command string directly in the RFID card and have the RFID reader/writer on the CYD I2C bus along with the QR reader. See the [Hardware](#hardware "Hardware") section; this is not feasible.
Options:
- use MicroSD sniffer on the CYD and control both devices that way
  - https://www.sparkfun.com/sparkfun-microsd-sniffer.html
  - https://learn.sparkfun.com/tutorials/microsd-sniffer-hookup-guide/introduction
  - https://github.com/sparkfun/MicroSD_Sniffer.git
- have another ESP32 or 3.3V microcontroller that is on the I2C bus and use it to control the SPI bus to the RFID reader/writer.

### Multiple SPI Devices on Same SPI Bus
[Top](#rfid-rc522-test "Top")<br>
Some resources
- https://docs.arduino.cc/learn/communication/spi/
- https://forum.arduino.cc/t/multiple-spi-chip-select-question/101140
- https://forum.arduino.cc/t/how-to-use-spi-for-multiple-devices/427739

It turns out that the MicroSD card and the Touchscreen are not set up for this purpose. They do not share all the pins except for Chip Select (CS).

Need to use software bit banging

### All ESP32 Hardware SPI Ports Used Up - Need Software Bit Banging
[Top](#rfid-rc522-test "Top")<br>
This general Arduino usage doc for LVGL says "To get started it's recommended to use TFT_eSPI library as a TFT driver to simplify testing."
- https://docs.lvgl.io/master/details/integration/framework/arduino.html

However, in the case of the Cheap Yellow Display, there are not enough available hardware SPI resources on the ESP32 to handle the display (HSPI), the touchscreen (VSPI), and the MicroSD card slot (out of luck) at the same time (along with the other HW SPI ports used for general purpose stuff like FLASH memory).
  - https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
  - Using bit-banging so touch and SD can work in same program	https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/TROUBLESHOOTING.md#display-touch-and-sd-card-are-not-working-at-the-same-time

For that reason I am using the XPT2046_Bitbang library instead of the TFT_eSPI library for the touchscreen. This seems to work OK with some changes to the code.

I made a version of the Random Nerds CYDCalibrate.ino
- Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/touchscreen-calibration/

... that I call  CYDbitBangCalibrate.ino here. For some reason I don't understand, the calibration constants are quite different between the two SPI libraries. You can see that by looking at the two calibration text files, also in the following directory.
- https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDbitBangCalibrate

If you use the XPT2046_Bitbang library and want to calibrate the touchscreen, you might want to use something similar to my CYDbitBangCalibrate program to generate the calibration constants.

https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD contains a program that uses
- lvgl library
- XPT2046_Bitbang library
- MicroSD Sniffer and HW VSPI for RC522 RF IC Card Sensor Module
- I2C for QR Code reader

I used these CYDbitBangCalibrate calibration constants in the cyd_input_read() routine found in https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD/cyd_input_read.h<br>
I will probably rework this UniRemoteCYD program further but I don't expect to change the routine name cyd_input_read so you should be able to find it.

## Documentation
[Top](#rfid-rc522-test "Top")<br>
Documentation of the MIFARE Classic 1K card
- https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf

Documentation of the MFRC522v2 library
- https://osslibraries.github.io/Arduino_MFRC522v2/

I used the Random Nerds tutorials to get up to speed.
- https://randomnerdtutorials.com/esp32-spi-communication-arduino/
- https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
- https://randomnerdtutorials.com/esp32-i2c-master-slave-arduino/
- https://randomnerdtutorials.com/esp32-spi-communication-arduino/
- https://randomnerdtutorials.com/esp32-mfrc522-rfid-reader-arduino/

More documentation on the CYD internal pinouts and the connector types.
- https://debugdiaries.co.uk/esp32-cheap-display-cyd-pinouts/ - compact, useful format
- https://macsbug.wordpress.com/2022/08/17/esp32-2432s028/ - need to Google-translate from Japanese but quite good

## SPI Sniffer Info
[Top](#rfid-rc522-test "Top")<br>
I am using the Arduino library RFID_MFRC522v2 by GithubCommunity. An important point when using this library in the CYD is that the reset pin RST (ESP32 pin 21) is unused since 25 Jun 2020 v2.0.0. That is fortunate, since in the CYD pin 21 is used for other purposes. I tried an experiment leaving it floating and that seemed to work.

Work In Progress - Here is my best guess so far as to I/O channels and SD card sniffer
| Sniffer | Alt | ESP32 pin | RC522 | Color | Comment |
| --- | --- | --- | --- | --- | --- |
| DAT2 |  |  |  | N/C | unused |
| CD | CS | 5 | SDA | Green | TF_CS |
| CMD | MOSI | 23 | MOSI | Yellow |  |
| GND | GND |  | GND | Black | ground |
| VCC | VDD |  | 3.3V | Red | 3.3V |
| CLK | CLX | 18 | SCK | White | TF_CLK |
| DAT0 | MISO | 19 | MISO | Blue |  |
| DAT1 |  |  |  | N/C | unused |
|  |  | 21 | RST | N/C | unused |
|  |  |  | IRQ | N/C | unused |
