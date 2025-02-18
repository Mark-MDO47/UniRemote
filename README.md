# UniRemote - one remote to rule them all!
I wanted a way to control all the projects I make.
- Note: it is not a remote for A/V systems. It is for my projects.

I wanted this to work not just at home in my WiFi zone but also at remote locations like a Maker Faire.<br>
It communicates with my projects via ESP-NOW WiFi
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

Current state of software and tools: **v0.9** "basics work: CYD, Tiny Code Reader for QRcodes, Mifare RC522 RF IC via MicroSD sniffer for 13.56MHz MIFARE Classic 1K cards, XPT2046_Bitbang lib to free up VSPI"

Here is my breadboard setup with the Cheap Yellow Display, the QR code reader and the RFID reader.<br>
![alt text](https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/CYD_PICC_RFID_1024_768.jpg "breadboard setup with the Cheap Yellow Display, the QR code reader and the RFID reader")

**Table Of Contents**
* [Top](#uniremote-\--one-remote-to-rule-them-all "Top")
* [The Plan](#the-plan "The Plan")
* [Guide to the Code](#guide-to-the-code "Guide to the Code")
* [Expected Flow for V1.0](#expected-flow-for-v10 "Expected Flow for V1.0")
* [Interesting Considerations](#interesting-considerations "Interesting Considerations")
  * [General CYD and LVGL and Related Info](#general-cyd-and-lvgl-and-related-info "General CYD and LVGL and Related Info")
  * [DRAM_STR - Move Constant Strings to RAM instead of Program Storage](#dram_str-\--move-constant-strings-to-ram-instead-of-program-storage "DRAM_STR - Move Constant Strings to RAM instead of Program Storage")
  * [Entire Screen Pans or Scrolls](#entire-screen-pans-or-scrolls "Entire Screen Pans or Scrolls")
  * [LVGL Timeout and Crash if Call LVGL Routine inside ESP-NOW callback](#lvgl-timeout-and-crash-if-call-lvgl-routine-inside-esp\-now-callback "LVGL Timeout and Crash if Call LVGL Routine inside ESP-NOW callback")
* [Licensing](#licensing "Licensing")

## The Plan
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I have settled on the following hardware for the Universal Remote Control:
- CYD - Cheap Yellow Display
  - I am using the ESP32-2432S028R (Cheap Yellow Display or CYD) based on ESP32-D0WDQ6 controller.
  - I purchased this one: https://www.aliexpress.us/item/3256805697430313.html
  - This schematic might reflect the CYD: https://oshwlab.com/mariusmym/esp32_cyd_cheap_yellow_display
- QR code reader
  - I am using the Tiny Code Reader from Useful Sensors
  - I purchased this one: https://www.sparkfun.com/products/23352
  - https://cdn.sparkfun.com/assets/e/9/d/9/f/TCR_Datasheet.pdf
  - https://github.com/usefulsensors/tiny_code_reader_docs/blob/main/README.md
  - https://github.com/usefulsensors/tiny_code_reader_arduino
- RFID Reader
  - I am using the HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card + Key Ring
    - I purchased this one: https://www.amazon.com/dp/B07VLDSYRW
    - This doesn't support I2C or UART without board modifications; just SPI
      - look for Adrianotiger post #4 on Mar 2023 on https://forum.arduino.cc/t/esp32-rfid-rc522-i2c/1100200/3
  - I am using the Meikuler 13.56MHz MIFARE Classic 1K, RFID Smart Cards / M1 Cards, ISO14443A Printable Blank RFID PVC Cards
    - I purchased this one: https://www.amazon.com/dp/B07S63VT7X
    - https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf
    - A.K.A. PICC = Proximity Integrated Circuit Card (Contactless Card)
  - See following for more RFID Reader details:
    - https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test
  - A bit of effort to use this. Got a "sniffer" card to use the SPI pins in the MicroSD slot, needed to do "bit banging" for touchscreen so could use hardware SPI for SD card.
    - I purchased this one: https://www.sparkfun.com/sparkfun-microsd-sniffer.html
    - https://github.com/sparkfun/MicroSD_Sniffer
    - https://github.com/Mark-MDO47/UniRemote/blob/master/code/RFIDRC522test/README.md

Will be trying these to see if they can be a more compact MicroSD sniffer:
- Extender cable 1: https://www.aliexpress.com/item/3256804754907124.html
- Extender cable 2: https://www.aliexpress.com/item/3256805782352551.html
- Alternative MicroSD sniffer to JTAG: https://shop.blinkinlabs.com/products/microsd-to-jtag-adapter-for-esp32

May use one or more of the following
- Alternative QR code reader
  - https://github.com/IndustrialArduino/NORVI-ESP32-CAMERA/tree/main/ESP32-CAMERA-QR_CODE_SCANNER_WITH_DISPLAY
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
| [code/UniRemoteCYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD "UniRemoteCYD") | UniRemote code with QR code reader, sniffer to RFID Reader, and CYD that uses the XPT2046_Bitbang software-based SPI library instead of the TFT_eSPI hardware based SPI library for the touchscreen - attributions in the code |
| [code/UniTestRcvr](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniTestRcvr "UniTestRcvr") | UniRemote code template for generic receiver of the commands - attributions in the code |
| [code/Uni_RW_PICC](https://github.com/Mark-MDO47/UniRemote/tree/master/code/Uni_RW_PICC "Uni PICC Rd/Wr") | UniRemote PICC read/write routines - attribution in the README |
| [code/tiny_code_reader](https://github.com/Mark-MDO47/UniRemote/tree/master/code/tiny_code_reader "tiny_code_reader") | *.h file for using the QR code reader - attribution in its README |
| --- | --- |
| [code/readMacAddress](https://github.com/Mark-MDO47/UniRemote/tree/master/code/readMacAddress "readMacAddress") | Code to read the WiFi MAC address of pretty much any ESP32 - used on remotes to get info - attribution in its README |
| [MDOpythonUtils QRCode.py](https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode "QRCode.py") | Python routine to generate QR code based on directions in a text file |
| [code/CYDbitBangCalibrate](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDbitBangCalibrate "CYDbitBangCalibrate") | Modified Random Nerds CYD Calibrate routine that uses the XPT2046_Bitbang software-based SPI library instead of the TFT_eSPI hardware based SPI library for the touchscreen - attributions in the code |
| [code/WriteRFID_CYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/WriteRFID_CYD "WriteRFID_CYD") | Code with same hardware setup as UniRemoteCYD to write RFID cards using input text strings in same format as input to QRCode.py - attributions in the code |
| --- | --- |
| [code/CYDtest](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDtest "CYDtest") | ESP32-2432S028R (Cheap Yellow Display or CYD) pointers to tutorials and hardware information |
| [code/RFIDRC522test](https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test "RFIDRC522test")  | RFID RC522 pointers to tutorials and hardware information |
| [code/UniRemote](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemote "UniRemote") | UniRemote code with QR code reader and **generic ESP32** module - attributions in the code. This is now **unused** and **deprecated**. I am switching to the CYD and the code in **UniRemoteCYD**. |

## Expected Flow for V1.0
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>

**UNI_STATE_WAIT_CMD**
- *last cmd all done, wait for next cmd (probably QR but any source OK)*
- alert OK for next CMD
- wait for CMD
  - if not "too soon"
    - scan CMD code into RAM

**UNI_STATE_CMD_SEEN**
- *command in queue, waiting for GO or CLEAR*
- alert wait for send
- wait for SEND or CLEAR
  - if receive CLEAR, clear cmd and go to WAIT_CMD
  - if receive SEND, go to SENDING

**UNI_STATE_SENDING_CMD**
- *command being sent (very short state)*
- alert that SENDING
- send command
  - check if too soon, go to UNI_STATE_SHOW_STAT
  - check MAC addr validity & able to register MAC peer; if error go to SHOW_STAT
  - call send ESP-NOW routine
    - if OK go to WAIT_CB
    - if error go to SHOW_STAT

**UNI_STATE_WAIT_CB**
- *waiting for send callback (very short state)*
- alert that WAIT_CB
- wait for callback
  - if timeout or bad, go to SHOW_STAT
  - if OK, go to WAIT_CMD

**UNI_STATE_SHOW_STAT**
- *show error status and allow cmd abort*
- alert that SHOW_STAT
- wait for SEND or ABORT
  - if receive ABORT, clear cmd and go to WAIT_CMD
  - if receive SEND, go to SENDING

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
| LVGL library I use | https://docs.arduino.cc/libraries/rfid_mfrc522v2 |
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
Since I am using an ESP-32 CYD with WiFi and LVGL, there is a lot of code and I quickly ran to the limits of program storage versus RAM.
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
- If I did 2 or sometimes up to 3 ESP-NOW messages with no receiver, even if done quite slowly, I would get an LVGL watchdog timer timeout and then it would crash and reboot.

It was a little mysterious because the processing was the same for both of them, only the status message itself was different.

My **guess** at the mechanism is:
- If the receiver was present, the response was so quick that the LVGL routines were not doing anything when the callback routine was called.
- If the receiver was absent, the response was a timeout and this could happen when the loop and/or LVGL refresh was happening so problem if calling non-reentrant LVGL routine.

## Licensing
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
This repository has a LICENSE file for Apache 2.0. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 license.

