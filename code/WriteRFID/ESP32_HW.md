# WriteRFID

Schematic for WriteRFID<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/RFIDWriterESP32D_schem.png" width="500" alt="Schematic for WriteRFID"><br>
Top View of WriteRFID<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/RFID_ESP32D_top.png" width="500" alt="Top View of WriteRFID"><br>
Bottom View of WriteRFID<br>
<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/RFID_ESP32D_bottom.png" width="500" alt="Bottom View of WriteRFID">

**Table Of Contents**
* [Top](#WriteRFID "Top")
* [Arduino IDE Board Selection](#arduino-ide-board-selection "Arduino IDE Board Selection")
* [Description](#description "Description")

## Arduino IDE Board Selection
[Top](#WriteRFID "Top")<br>
I am using this board - the **32D** variant.
- https://www.aliexpress.com/item/3256806553795646.html

I think this board is based on the ESP32-WROOM-32D Devkit C.
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html

If using this board in the Arduino IDE, select the board as **"ESP32 Dev Module"**.

## Description
[Top](#WriteRFID "Top")<br>
This routine writes and checks command info to PICC (RFID) cards.
It is intended to write PICC cards to be used by UniRemoteCYD.
- https://github.com/Mark-MDO47/UniRemote
- https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD

This routine runs on ESP32-WROOM-32D Devkit C using the Mifare RC522 RF IC Card Sensor Module.
- https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html
- https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html (ESP32-DevKitC V4)
- https://www.espressif.com/sites/default/files/documentation/esp32-wroom-32d_esp32-wroom-32u_datasheet_en.pdf

Exactly the same code is used on the UniRemoteCYD hardware and on this specialized hardware.
- I got weary of re-programming the UniRemoteCYD back and forth and so I made this stand-alone programmer hardware.

The dialog with the operator is done using the serial port.

For details on the command format and how to run the code, see https://github.com/Mark-MDO47/UniRemote/edit/master/code/WriteRFID/README.md
