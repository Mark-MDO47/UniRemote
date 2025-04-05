# UniRemoteCYD software - one remote to rule them all!

**Table Of Contents**
* [Top](#uniremotecyd-software-\--one-remote-to-rule-them-all "Top")
* [Arduino IDE Board Selection](#arduino-ide-board-selection "Arduino IDE Board Selection")
* [Expected Flow for V1.0](#expected-flow-for-v10 "Expected Flow for V1.0")
* [Licensing](#licensing "Licensing")

## Arduino IDE Board Selection
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
In the Arduino IDE, select the board as "ESP32 Dev Module". The CYD is based on the ESP32 D0WDQ6.

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


## Licensing
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
This repository has a LICENSE file for Apache 2.0. There may be code included that I have modified from other open sources (such as Arduino, Espressif, SparkFun, Seeed Studio, DFRobot, RandomNerds, etc.). These other sources may possibly be licensed using a different license model. In such a case I will include some notation of this. Typically I will include verbatim the license in the included/modified source code, but alternatively there might be a LICENSE file in the source code area that points out exceptions to the Apache 2.0 license.

