# UniRemote - one remote to rule them all!
I wanted a way to control all the projects I make.
- Note: it is not a remote for A/V systems. It is for my projects.

I wanted this to work not just at home in my WiFi zone but also at remote locations like a Maker Faire.<br>
It communicates with my projects via ESP-NOW WiFi
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

Current state of software and tools: **v0.9** "basics work: CYD, Tiny Code Reader for QRcodes, Mifare RC522 RF IC via MicroSD sniffer for 13.56MHz MIFARE Classic 1K cards, XPT2046_Bitbang lib to free up VSPI"

Here is a short YouTube demonstration of the latest version
- https://youtu.be/a9atNTkQOKo

This is the UniRemoteCYD breadboard setup with "drone style" batteries.
- I removed the QR code reader since the RFID reader was so easy to use
- The contactless RFID PICC cards (Proximity Integrated Circuit Card) are to the left
- The "drone style" battery charger and charging cable is at the top
- The XAIO ESP32-Sense target processor is below that, attached to a USB cable
- The Cheap Yellow Display (CYD) and its RFID sensor and batteries are on the bottom right
  - The red board sticking out at the bottom is the MicroSD Sniffer card used to communicate with the RFID sensor
- You can see more photos at https://github.com/Mark-MDO47/UniRemote/blob/master/photos.md
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemote_overview.jpg" width="1000" alt="Image of UniRemote breadboard overview">

A little comic relief - the hilarious **Legendary Artifacts Club** by Elle Cordova https://www.youtube.com/shorts/GEJ_KPZY4n4

| One Remote to Rule Them All | I can bend countless devices to my will |
| --- | --- |
| <img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/LegendaryArtifactsClub_1.png" width="300" alt="Legendary Artifacts Club by Elle Cordova"> | <img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/LegendaryArtifactsClub_2.png" width="300" alt="Legendary Artifacts Club by Elle Cordova"> |


**Table Of Contents**
* [Top](#uniremote-\--one-remote-to-rule-them-all "Top")
* [The Plan](#the-plan "The Plan")
* [Guide to the Code](#guide-to-the-code "Guide to the Code")
* [Schematic](#schematic "Schematic")
  * [Battery Harness](#battery-harness "Battery Harness")
* [Interesting Considerations](#interesting-considerations "Interesting Considerations")
  * [General CYD and LVGL and Related Info](#general-cyd-and-lvgl-and-related-info "General CYD and LVGL and Related Info")
  * [DRAM_STR - Move Constant Strings to RAM instead of Program Storage](#dram_str-\--move-constant-strings-to-ram-instead-of-program-storage "DRAM_STR - Move Constant Strings to RAM instead of Program Storage")
  * [Entire Screen Pans or Scrolls](#entire-screen-pans-or-scrolls "Entire Screen Pans or Scrolls")
  * [LVGL Timeout and Crash if Call LVGL Routine inside ESP-NOW callback](#lvgl-timeout-and-crash-if-call-lvgl-routine-inside-esp\-now-callback "LVGL Timeout and Crash if Call LVGL Routine inside ESP-NOW callback")
  * [Do-It-Yourself Layout Creator](#do\-it\-yourself-layout-creator "Do-It-Yourself Layout Creator")
* [Licensing](#licensing "Licensing")

## The Plan
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
My current thinking is to just use the RFID reader and leave out the QR code reader. Maybe add it back in later.

Because the MicroSD sniffer sticks out so far, I had planned to use removable 18650 batteries mounted on the side of the CYD to give it some protection. However, putting the 18650 batteries on the outside was trickier than I thought due to some connectors and components getting in the way. It also didn't provide much protection for the sniffer. I went ahead and put the Lipo "drone style" batteries in for now. There is a connector that allows me to charge the Lipo batteries without removing them.
- More details in the section [Battery Harness](#battery-harness "Battery Harness")

I have settled on the following hardware for the Universal Remote Control:
- CYD - Cheap Yellow Display
  - I am using the ESP32-2432S028R (Cheap Yellow Display or CYD) based on ESP32-D0WDQ6 controller.
  - I purchased this one: https://www.aliexpress.us/item/3256805697430313.html
  - This schematic might reflect the CYD: https://oshwlab.com/mariusmym/esp32_cyd_cheap_yellow_display
  - This gives an excellent CYD introduction: https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r
- RFID Reader
  - I am using the HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card + Key Ring
    - I purchased this one: https://www.amazon.com/dp/B07VLDSYRW
    - This doesn't support I2C or UART without board modifications; just SPI
      - look for Adrianotiger post #4 on Mar 2023 on https://forum.arduino.cc/t/esp32-rfid-rc522-i2c/1100200/3
      - here is webpage for the module including schematic https://www.sunrom.com/p/rfid-readerwriter-1356mhz-rc522-spi-module-with-cardkeychain
  - I am using the Meikuler 13.56MHz MIFARE Classic 1K, RFID Smart Cards / M1 Cards, ISO14443A Printable Blank RFID PVC Cards
    - I purchased this one: https://www.amazon.com/dp/B07S63VT7X
    - https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf
    - A.K.A. PICC = Proximity Integrated Circuit Card (Contactless Card)
  - See following for more RFID Reader details:
    - https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test
  - It took a bit of effort to use this with the CYD. I got a "sniffer" card to use the SPI pins in the CYD MicroSD slot, needed to do "bit banging" for touchscreen so could use hardware SPI for SD card.
    - I purchased this one: https://www.sparkfun.com/sparkfun-microsd-sniffer.html
    - https://github.com/sparkfun/MicroSD_Sniffer
    - Schematic https://cdn.sparkfun.com/datasheets/Tools/SparkFun_MicroSD_Sniffer_v10.pdf
    - https://github.com/Mark-MDO47/UniRemote/blob/master/code/RFIDRC522test/README.md

Will be trying these to see if they can be a more compact MicroSD sniffer:
- Nope: Extender cable 1: https://www.aliexpress.com/item/3256804754907124.html
- Nope: Extender cable 2: https://www.aliexpress.com/item/3256805782352551.html
- Nope: Alternative MicroSD sniffer to JTAG: https://shop.blinkinlabs.com/products/microsd-to-jtag-adapter-for-esp32

May use one or more of the following
- QR code reader
  - Tiny Code Reader from Useful Sensors
  - I purchased this one: https://www.sparkfun.com/products/23352
  - https://cdn.sparkfun.com/assets/e/9/d/9/f/TCR_Datasheet.pdf
  - https://github.com/usefulsensors/tiny_code_reader_docs/blob/main/README.md
  - https://github.com/usefulsensors/tiny_code_reader_arduino
- Alternative QR code readers
  - https://github.com/IndustrialArduino/NORVI-ESP32-CAMERA/tree/main/ESP32-CAMERA-QR_CODE_SCANNER_WITH_DISPLAY
  - https://randomnerdtutorials.com/esp32-cam-qr-code-reader-scanner-arduino/
- Voice Input - perhaps one of these
  - https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html
  - https://www.dfrobot.com/product-2665.html
- Joystick or rotary encoder, buttons
- BlueTooth keyboard

The hardware that receives the commands can use basically any ESP32 that includes WiFi.

## Guide to the Code
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
Here is what the code is:<br>
| Link | Description |
| --- | --- |
| --- | **UNIREMOTE CODE** |
| [code/UniRemoteCYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD "UniRemoteCYD") | "Cheap Yellow Display"-based UniRemote code sending ESP-NOW commands - attributions in the code.<br>Commands are input with either the QR code reader (I2C) or the RFID Reader (SPI via MicroSD sniffer).<br>To free up access to the CYD hardware VSPI port for the RFID Reader, the code uses the XPT2046_Bitbang software-based SPI library instead of the TFT_eSPI hardware based SPI library for the touchscreen.<br>This technique of using XPT2046_Bitbang for the touchscreen could alternatively be used to access the MicroSD card on the CYD if the MicroSD pins weren't being used for the RFID Reader. |
| [code/Uni_RW_PICC](https://github.com/Mark-MDO47/UniRemote/tree/master/code/Uni_RW_PICC "Uni PICC Rd/Wr") | UniRemote PICC read/write routines - attribution in the README |
| [code/CYDbitBangCalibrate](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDbitBangCalibrate "CYDbitBangCalibrate") | Modified Random Nerds CYD Calibrate routine that uses the XPT2046_Bitbang software-based SPI library instead of the TFT_eSPI hardware based SPI library for the touchscreen - attributions in the code |
| [code/tiny_code_reader](https://github.com/Mark-MDO47/UniRemote/tree/master/code/tiny_code_reader "tiny_code_reader") | *.h file for using the QR code reader - attribution in its README<br>NOTE: not currently used in UniRemoteCYD |
| --- | **RECEIVER CODE** |
| [code/UniRemoteRcvrTemplate](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteRcvrTemplate "UniRemoteRcvrTemplate") | UniRemote code **template** for generic receiver of the commands - attributions in the code.<br>UniRemoteRcvrTemplate.ino shows an example of using UniRemoteRcvr.h and UniRemoteRcvr.cpp to receive ESP-NOW commands from UniRemoteCYD.<br>Also includes my adaptation of the ESP32 example **OTAWebUpdater.ino** Over-The-Air web updater **mdo_use_ota_webupdater**. |
| [code/mdo_use_ota_webupdater](https://github.com/Mark-MDO47/UniRemote/blob/master/code/mdo_use_ota_webupdater "mdo_use_ota_webupdater") |  My adaptation of the ESP32 example **OTAWebUpdater.ino** Over-The-Air web updater **mdo_use_ota_webupdater**. |
| [code/readMacAddress](https://github.com/Mark-MDO47/UniRemote/tree/master/code/readMacAddress "readMacAddress") | Code to read the WiFi MAC address of pretty much any ESP32 - used on remotes to get info - attribution in its README |
| --- | **UTILITIES** |
| [code/WriteRFID](https://github.com/Mark-MDO47/UniRemote/tree/master/code/WriteRFID "WriteRFID") | Used to **write** RFID cards using input text strings in same format as input to QRCode.py or **read** RFID cards - attributions in the code.<BR>Works on both UniRemoteCYD hardware and EPS32D special purpose hardware. |
| [MDOpythonUtils QRCode.py](https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode "QRCode.py") | Python routine to generate QR code based on directions in a text file |
| --- | **DOCUMENTATION** |
| [code/CYDtest](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDtest "CYDtest") | ESP32-2432S028R (Cheap Yellow Display or CYD) pointers to tutorials and hardware information |
| [code/RFIDRC522test](https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test "RFIDRC522test")  | RFID RC522 pointers to tutorials and hardware information |
| --- | **DEPRECATED** |
| [code/UniRemote](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemote "UniRemote") | UniRemote code with QR code reader and **generic ESP32** module - attributions in the code. This is now **unused** and **deprecated**. I am switching to the CYD and the code in **UniRemoteCYD**. |
| [code/UniTestRcvr](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniTestRcvr "UniTestRcvr") | UniRemote code testbench for receiver of the commands - attributions in the code. This is now **unused** and **deprecated**. All further development for the receiver will go to the generic **UniRemoteRcvrTemplate**. |
| --- end of table --- | --- end of table --- |

## Schematic
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
Connector P1 is some sort of JST connector. My best info on figuring this out is
- https://www.mattmillman.com/info/crimpconnectors/common-jst-connector-types/

This shows the connections to the CYD<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemoteCYD_schem_1.png" width="800" alt="Schematic of UniRemoteCYD showing CYD connections">

This shows the connections to the batteries<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemoteCYD_schem_2.png" width="800" alt="Schematic of UniRemoteCYD showing Battery connections">

### Battery Harness
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
As mentioned above, I would like to come up with a more compact MicroSD sniffer arrangement. The fact that this sticks out so far has driven me to consider multiple ways to handle it in packaging, including the posibility of placing 18650 batteries on the sides to give it some protection.
- Alternatively I might use the two-wire interface to connect to another Arduino that connects to the RFID scanner, although that seems somewhat complicated. When I compare it to the time spent trying other options for MicroSD sniffers, the additional Arduino is looking better and better.

I chose to implement the fixed Lipo "drone style" batteries. There is a connector that can be used to disconnect the Lipo batteries from the UniRemote and connect to a charger without removing the batteries. Below is a diagram of the circuit.<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemote_BatteryHarness_FixedLipo.png" width="500" alt="Diagram for Fixed Lipo Battery Harness and Charger">

Alternatively I did create a removable 18650 battery harness. Because these would be mounted on the sides of the CYD, the theory was that they would provide some protection for the MicroSD sniffer which sticks out on one side. In practice this did not seem as helpful as I hoped. Below is a diagram of the circuit.<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemote_BatteryHarness_Removable18650.png" width="500" alt="Diagram for Removable 18650 Battery Harness">

## Parts List
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I finally got around to adding the parts list.

| Price (each) | Num | Description | URL |
| --- | --- | --- | --- |
| $10.00 | 1 | CYD ESP32 WiFi Bluetooth Development Board 2.8 Inch 240X320 TFT | https://www.aliexpress.us/item/3256805697430313.html |
| $3.75 | 1 | CYD Case Only | https://www.aliexpress.us/item/3256806316808825.html |
| $3.33 | 1 | HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card | https://www.amazon.com/dp/B07VLDSYRW |
| $12.00 | 1 | SparkFun microSD Sniffer | https://www.sparkfun.com/sparkfun-microsd-sniffer.html |
| $0.20 | 1 | ON/Off Switch Self-Lock Micro Push Button Switch DC 30V 1A | https://www.amazon.com/gp/product/B086L2GPGX | $0.20 |
| $0.90 | 2 pair | 20 PCS JST MX 1.25 4-pin connector & wire (male/female) | https://www.amazon.com/dp/B09DYLY95R?th=1 |
| $0.30 | 2 pair | mxuteuk 20 Pairs 22 AWG JST SM Plug & wire (male/female) 150mm | https://www.amazon.com/dp/B083GR7FQF |
| ??? | 2 | drone batteries and charger from my old parts bin | old parts bin |
| 0.10 | 8 | from 220 Pieces M3 Male Female Hex Brass Standoffs Spacers Screws Nuts Kit, Kindroufly Brass Standoffs Assortment Kit | https://www.amazon.com/dp/B0B3Y6WF2Y?th=1 |

## Interesting Considerations
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
Following are things I wanted to document and remember.

### General CYD and LVGL and Related Info
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>

One key thing I found out was that all four of the ESP-32 hardware SPI ports are in use in the "standard" LVGL configuration. In order to use a hardware SPI port for the MicroSD slot, I had to use the XPT2046_Bitbang library instead of the TFT_eSPI library for the touchscreen.
- See https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test for details.

| Description | URL |
| --- | --- |
| CYD Pinouts & Connectors | https://randomnerdtutorials.com/esp32-cheap-yellow-display-cyd-pinout-esp32-2432s028r |
| More CYD Pinouts in useful format | https://debugdiaries.co.uk/esp32-cheap-display-cyd-pinouts |
| CYD info in Japanese; need Google Translate but info looks quite good | https://macsbug.wordpress.com/2022/08/17/esp32-2432s028 |
| Info/Code for CYD and many variants | https://github.com/rzeldent/esp32-smartdisplay |
| --- | --- |
| RFID library I use | https://docs.arduino.cc/libraries/rfid_mfrc522v2 |
| LVGL and CYD | https://randomnerdtutorials.com/lvgl-cheap-yellow-display-esp32-2432s028r |
| LVGL and CYD | https://rntlab.com/module-1/esp32-lvgl-ebook |
| CYD and alternative library with lots of good info | https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display.git|
| CYD LVGL interesting project | https://github.com/bitbank2/CYD_Projects.git<br>https://github.com/TheNitek/CYD-LVGL-Template.git |
| YouTube Ralph S. Bacon "#203 SPIFFS vs LITTLEFS for ESP32 & ESP8266 (not Arduino UNO)" | https://www.youtube.com/watch?v=4r6YZlLfKfw |
| --- | --- |
| Espressif ESP32 example using non-standard SPI pins | https://docs.espressif.com/projects/arduino-esp32/en/latest/api/spi.html |
| Using bit-banging so touch and SD can work in same program | https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/TROUBLESHOOTING.md#display-touch-and-sd-card-are-not-working-at-the-same-time |

### DRAM_STR - Move Constant Strings to RAM instead of Program Storage
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
Since I am using an ESP-32 CYD with WiFi and LVGL, there is a lot of code included in libraries and I quickly ran to the limits of program storage versus RAM.
```
Sketch uses 1208557 bytes (92%) of program storage space. Maximum is 1310720 bytes.
Global variables use 61544 bytes (18%) of dynamic memory, leaving 266136 bytes for local variables. Maximum is 327680 bytes.
```
I discovered that I need to do the opposite of what I do with the Arduino Nano. I need use the "DRAM_STR" macro to move constant strings (easiest to find) from program storage into dynamic memory. On the Arduino Nano it was often helpful to use the "F" macro to move constant strings in the other direction since the dynamic memory was so limited.

### Entire Screen Pans or Scrolls
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I didn't find a good way to prevent the entire screen from "panning" or "scrolling" during a "long press". I tried the following without success.

| Routine | Description |
| --- | --- |
| void lv_indev_set_long_press_time(lv_indev_t * indev, uint16_t long_press_time); | Setting value to 65,535 after creating indev did not help |
| void lv_indev_reset_long_press(lv_indev_t * indev); | Calling within indev read_cb function (cyd_input_read() in my code) when .tirqTouched() or .touched() caused buttons to not operate |

### LVGL Timeout and Crash if Call LVGL Routine inside ESP-NOW callback
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I guess I should have predicted that a callback, which is somewhat similar to an interrupt routine, shouldn't assume it can call a lot of routines that might not be re-entrant.
- Result - just set flags in callback routines, do the work of the flags in loop() and its routines.

The symptom was:
- If I did ESP-NOW messages (using PICC card as the source) for which the receiver was present and receiving, I could seemingly send as many as I wanted with the physical scanning of a PICC card giving a timing interval.
- If I did 2 or sometimes up to 3 ESP-NOW messages with no receiver, even if done quite slowly, the CYD would get an LVGL watchdog timer timeout and then it would crash and reboot.

It was a little mysterious because the processing was the same for both of them, only the status message itself was different.

My **guess** at the mechanism is:
- If the receiver was present, the response was so quick that the LVGL routines were not doing anything when the ESP-NOW send callback routine was called.
- If the receiver was absent, the response was a timeout and this could happen when the loop and/or LVGL refresh was happening so problem if calling non-reentrant LVGL routine from ESP-NOW send callback.

### Do-It-Yourself Layout Creator
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I didn't know about this when I did my perfboard design for [code/WriteRFID](https://github.com/Mark-MDO47/UniRemote/tree/master/code/WriteRFID "WriteRFID") but I wish I had used DIYLC instead of hacking away at my perfboard!
- https://github.com/bancika/diy-layout-creator
- https://github.com/bancika/diy-layout-creator/releases

Here is a nice YouTube showing how to use it
- Design Circuits on Stripboard or Veroboard for FREE with DIY Layout Creator by TekSparkz - https://www.youtube.com/watch?v=mzje3eHqi2E

## Licensing
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
This repository has a LICENSE file for Apache 2.0. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 license.

