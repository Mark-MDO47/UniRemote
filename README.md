# UniRemote - one remote to rule them all!
I wanted a way to control all the projects I make. It is not a remote for A/V systems.<br>
I wanted this to work not just at home in my WiFi zone but also at remote locations like a Maker Faire.<br>
Plan is to communicate with my projects via ESP-NOW WiFi
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

Here is my breadboard setup with the Cheap Yellow Display, the QR code reader and the RFID reader.<br>
![alt text](https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/CYD_QR_RFID_1024_768.jpg "breadboard setup with the Cheap Yellow Display, the QR code reader and the RFID reader")

**Table Of Contents**
* [Top](#uniremote-\--one-remote-to-rule-them-all "Top")
* [The Plan](#the-plan "The Plan")
* [Guide to the Code](#guide-to-the-code "Guide to the Code")
* [Expected Flow for V1.0](#expected-flow-for-v10 "Expected Flow for V1.0")
* [Interesting Considerations](#interesting-considerations "Interesting Considerations")
  * [DRAM_STR - Move Constant Strings to RAM instead of Program Storage](#dram_str-\--move-constant-strings-to-ram-instead-of-program-storage "DRAM_STR - Move Constant Strings to RAM instead of Program Storage")
  * [Entire Screen Pans or Scrolls](#entire-screen-pans-or-scrolls "Entire Screen Pans or Scrolls")
* [Licensing](#licensing "Licensing")

## The Plan
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I have settled on the following hardware for the Universal Remote Control:
- CYD - Cheap Yellow Display
  - I am using the ESP32-2432S028R (Cheap Yellow Display or CYD) based on ESP32-D0WDQ6 controller.
  - I purchased this one: https://www.aliexpress.us/item/3256805697430313.html
- QR code reader
  - I am using the Tiny Code Reader from Useful Sensors
  - I purchased this one: https://www.sparkfun.com/products/23352
- RFID reader
  - I am using the HiLetgo 3pcs RFID Kit - Mifare RC522 RF IC Card Sensor Module + S50 Blank Card + Key Ring
    - https://www.amazon.com/dp/B07VLDSYRW
    - This doesn't support I2C or UART without board modifications; just SPI
      - look for Adrianotiger post #4 on Mar 2023 on https://forum.arduino.cc/t/esp32-rfid-rc522-i2c/1100200/3
  - I am using the Meikuler 13.56MHz MIFARE Classic 1K, RFID Smart Cards / M1 Cards, ISO14443A Printable Blank RFID PVC Cards
    - https://www.amazon.com/dp/B07S63VT7X
    - https://www.nxp.com/docs/en/data-sheet/MF1S50YYX_V1.pdf
  - this option looking good; see following for more details:
    - https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test

May use one or more of the following
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
| [code/CYDtest](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDtest "CYDtest") | ESP32-2432S028R (Cheap Yellow Display or CYD) pointers to tutorials and hardware information |
| [code/RFIDRC522test](https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test "RFIDRC522test")  | RFID RC522 pointers to tutorials and hardware information |
| [code/UniRemote](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemote "UniRemote") | UniRemote code with QR code reader above and **generic ESP32** module - attributions in the code. This is now **unused** and **deprecated**. I am switching to the CYD and the code in **UniRemoteCYD**. |
| [code/UniRemoteCYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD "UniRemoteCYD") | UniRemote code with QR code reader above and CYD - attributions in the code |
| [code/UniTestRcvr](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniTestRcvr "UniTestRcvr") | UniRemote code template for generic receiver of the commands - attributions in the code |
| [code/readMacAddress](https://github.com/Mark-MDO47/UniRemote/tree/master/code/readMacAddress "readMacAddress") | Code to read the WiFi MAC address of an ESP32 - attribution in its README |
| [code/tiny_code_reader](https://github.com/Mark-MDO47/UniRemote/tree/master/code/tiny_code_reader "tiny_code_reader") | *.h file for using the QR code reader above - attribution in its README |
| [MDOpythonUtils QRCode.py](https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode "QRCode.py") | Python routine to generate QR code based on directions in a text file |

## Expected Flow for V1.0
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>

**UNI_STATE_WAIT_CMD**
- *last cmd all done, wait for next cmd (probably QR but any source OK)*
- alert OK for next QR
- wait for QR
  - if not "too soon"
    - scan QR code into RAM

**UNI_STATE_QR_SEEN**
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

### DRAM_STR - Move Constant Strings to RAM instead of Program Storage
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

## Licensing
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
This repository has a LICENSE file for Apache 2.0. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 license.

