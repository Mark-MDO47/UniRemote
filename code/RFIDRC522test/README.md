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
- have another ESP32 or 3.3V microcontroller that is on the I2C bus and use it to control the SPI bus to the RFID reader/writer.
- use MicroSD sniffer on the CYD and control both devices that way
  - https://www.sparkfun.com/sparkfun-microsd-sniffer.html
  - https://learn.sparkfun.com/tutorials/microsd-sniffer-hookup-guide/introduction
  - https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
  - may be a problem with using the SD card interface: "I'll have another play and try and figure out the conflicting pin, I've identified that it only happens after adding XPT2046_Touchscreen" in https://forum.arduino.cc/t/cheap-yellow-display-touch-and-sd/1279772/7

## Documentation
[Top](#rfid-rc522-test "Top")<br>
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
Work In Progress - Here is my best guess so far as to I/O channels and SD card sniffer
| Sniffer | Alt | ESP32 pin | RC522 | Color | Comment |
| --- | --- | --- | --- | --- | --- |
| DAT2 |  |  |  |  | unused |
| CD | CS | 5 | SDA | Green | TF_CS |
| CMD | MOSI | 23 | MOSI | Yellow |  |
| GND | GND |  | GND | Black | ground |
| VCC | VDD |  | 3.3V | Red | 3.3V |
| CLK | CLX | 18 | SCK | Orange | TF_CLK |
| DAT0 | MISO | 19 | MISO | Blue |  |
| DAT1 |  |  |  |  | unused |
|  |  | ??? | RST | White | reset |
|  |  |  | IRQ | N/C | unused |
