# UniRemote - one remote to rule them all!
I wanted a way to control all the projects I make. It is not a remote for A/V systems.<br>
I wanted this to work not just at home in my WiFi zone but also at remote locations like a Maker Faire.<br>
Plan is to communicate with my projects via ESP-NOW WiFi
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

**Table Of Contents**
* [Top](#uniremote-\--one-remote-to-rule-them-all "Top")
* [The Plan](#the-plan "The Plan")
* [Expected Flow for V1.0](#expected-flow-for-v10 "Expected Flow for V1.0")
* [Licensing](#licensing "Licensing")

## The Plan
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I may include one or more of these ways to input my desires:
- CYD - Cheap Yellow Display - perhaps this one
  - https://www.aliexpress.us/item/3256805697430313.html
- QR code reader - perhaps this one
  - https://www.sparkfun.com/products/21231
- Voice Input - perhaps one of these
  - https://www.seeedstudio.com/XIAO-ESP32S3-Sense-p-5639.html
  - https://www.dfrobot.com/product-2665.html
- Joystick or rotary encoder, buttons
- BlueTooth keyboard

## Guide to the Code
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
Here is what the code is:<br>
| Link | Description |
| --- | --- |
| [code/CYDtest](https://github.com/Mark-MDO47/UniRemote/code/CYDtest "CYDtest") | ESP32-2432S028R (Cheap Yellow Display or CYD) throwaway test code |
| [code/UniRemote](https://github.com/Mark-MDO47/UniRemote/code/UniRemote "UniRemote") | UniRemote code with QR code reader above and generic ESP32 module |
| [code/UniRemoteCYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD "UniRemoteCYD") | UniRemote code with QR code reader above and CYD |
| [code/UniTestRcvr](https://github.com/Mark-MDO47/UniRemote/code/UniTestRcvr "UniTestRcvr") | UniRemote code template for generic receiver of the commands |
| [code/readMacAddress](https://github.com/Mark-MDO47/UniRemote/code/readMacAddress "readMacAddress") | Code from RandomNerds to read the WiFi MAC address of an ESP32 - attribution in its README |
| [code/tiny_code_reader](https://github.com/Mark-MDO47/UniRemote/code/tiny_code_reader "tiny_code_reader") | *.h file for using the QR code reader above |

## Expected Flow for V1.0
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>

**UNI_WAIT_CMD**
- *last cmd all done, wait for next cmd (probably QR but any source OK)*
- alert OK for next QR
- wait for QR
  - if not "too soon"
    - scan QR code into RAM
    - check validity & if valid register peer and alert that have VALID_CMD

**UNI_VALID_CMD**
- *command validated and in queue, waiting for GO or CLEAR*
- alert wait for send
- wait for SEND
  - if receive another QR, alert that ignoring
  - if receive CLEAR, clear cmd and go to WAITING
  - if receive SEND, go to SENDING

**UNI_SENDING_CMD**
- *command being sent (very short state)*
- alert that SENDING
- send command, set timer for "too soon", go to WAIT_CALLBACK

**UNI_WAIT_CB**
- *waiting for send callback*
- alert that WAIT_CALLBACK
- wait for callback
  - if timeout or bad, go to VALID_CMD
  - if OK, go to WAIT_QR

## Licensing
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
This repository has a LICENSE file for Apache 2.0. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 license.

