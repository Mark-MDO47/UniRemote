# WriteRFID

**Table Of Contents**
* [Top](#writerfid "Top")
* [Arduino IDE Board Selection](#arduino-ide-board-selection "Arduino IDE Board Selection")
  * [Do-It-Yourself Layout Creator](#do\-it\-yourself-layout-creator "Do-It-Yourself Layout Creator")
* [Description](#description "Description")
* [Example Run of WriteRFID](#example-run-of-writerfid "Example Run of WriteRFID")

## Arduino IDE Board Selection
[Top](#WriteRFID "Top")<br>
The same setup works for both hardware platforms. In the Arduino IDE, select the board as "ESP32 Dev Module".

WriteRFID has been run on two different hardware platforms
- UniRemote_CYD
  - Based on ESP32-2432S028R (Cheap Yellow Display or CYD). The CYD is based on the ESP32 D0WDQ6.
  - UniRemoteCYD includes other hardware, especially a "sniffer" on the MicroSD slot connecting the VSPI port to the Mifare RC522 RF IC Card Sensor Module.
  - https://github.com/Mark-MDO47/UniRemote/blob/master/README.md
- Special purpose ESP32D hardware
  - Based on an ESP32D processor embedded in a module that seems to be based on the ESP32-WROOM-32D Devkit C
    - https://docs.espressif.com/projects/esp-idf/en/latest/esp32/hw-reference/esp32/get-started-devkitc.html
  - Also connects the VSPI port to the Mifare RC522 RF IC Card Sensor Module.
  - https://github.com/Mark-MDO47/UniRemote/blob/master/code/WriteRFID/ESP32D_HW.md

| UniRemoteCYD | ESP32D HW |
| --- | --- |
| <img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/UniRemoteCYD_glamour.png" width="400" alt="The UniRemoteCYD"> | <img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/RFID_ESP32D_top.png" width="400" alt="ESP32D Special Hardware"> |

This diagram shows the pin connections to the RC522 RFID reader/writer for both hardware configurations. In the case of UniRemoteCYD, the connections go through the MicroSD slot and a sniffer card. The actual GPIO pin connections are the same in both cases.

<img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/RFIDWriterESP32D_schem.png" width="600" alt="ESP32D Special Hardware schematic">

### Do-It-Yourself Layout Creator
[Top](#uniremote-\--one-remote-to-rule-them-all "Top")<br>
I didn't know about this when I did my perfboard design for the Special purpose ESP32D hardware for WriteRFID but I wish I had used DIYLC instead of hacking away at my perfboard!
- https://github.com/bancika/diy-layout-creator
- https://github.com/bancika/diy-layout-creator/releases

Here is a nice YouTube showing how to use it
- Design Circuits on Stripboard or Veroboard for FREE with DIY Layout Creator by TekSparkz - https://www.youtube.com/watch?v=mzje3eHqi2E

## Description
[Top](#WriteRFID "Top")<br>
This routine writes and checks command info to PICC (RFID) cards.
It is intended to write PICC cards to be used by UniRemoteCYD.
- https://github.com/Mark-MDO47/UniRemote
- https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD

This routine runs on ESP32-2432S028R (Cheap Yellow Display or CYD) using
the same hardware setup as UniRemoteCYD, with a "sniffer" on the MicroSD
slot connecting the VSPI port to the Mifare RC522 RF IC Card Sensor Module.

This unmodified routine also runs on the specialized ESP32D hardware described in [Arduino IDE Board Selection](#arduino-ide-board-selection "Arduino IDE Board Selection")

The dialog with the operator is done using the serial port, not the display
and touchscreen. Because I am lazy and also because the specialized ESP32D hardware doesn't have a display.

The PICC command info is entered in array write_strings[] in the code before running WriteRFID.
The text in write_strings[] is the same format as the input file to QRCode.py
- https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode

The command is a one-line zero-terminated ASCII string (without line end) of the following form
```
Example: "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; MUSIC:NEXT ignore ; EYES:PATTERN OPPOSITE/64/BLINK"
<destination>\t<description>\t<WiFiMACaddress>|<destination> ; <command, perhaps broken up with colon> <parameters, perhaps broken up with forward slash> { optional further  "; <command> <parameters>" triplets }
   <destination>: a human readable name for the target
   <description>: a human readable summary of the command(s)
   <WiFiMACaddress>: the ESP-NOW WiFi MAC address of the destination - 6 hexadecimal bytes separated with colon, case insensitive
   <destination>: a human readable name for the target that will display on UniRemoteCYD
   ; - semicolon surrounded by spaces starts each command
   <commmand>: a command for the destination. Optionally broken up with colon ":"
   <parameters>: one or more parameters for the command. If more than one parameter, broken up with forward slash "/". If no parameters are needed, still one must be provided - see "ignore" above.
```

## Example Run of WriteRFID
[Top](#WriteRFID "Top")<br>
```
starting WriteRFID - writes to a PICC RFID card
   see https://github.com/Mark-MDO47/UniRemote

Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with r or R to start looking for card and read/display it
   Enter anything starting with w or W to start looking for card and writing below write string
   Enter anything starting with s or S to skip below write string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Message for BANJO	ac:67:b2:2c:c9:c0|BANJO ; MUSIC:SONG A440"
Enter your command to process this String >  
You Entered "w"
 --Write--> "ac:67:b2:2c:c9:c0|BANJO ; MUSIC:SONG A440"
Please place card on writer
--READING---------
Card UID:  41 84 A5 82
PICC Write final MFRC522 status 0
uni_write_picc() succesful! Please remove card, short wait, replace card for reading
--READING---------
Card UID:  41 84 A5 82
PICC read final MFRC522 status 0
 --Read---> "ac:67:b2:2c:c9:c0|BANJO ; MUSIC:SONG A440"
Comparison GOOD - Read data matches Write data
Please remove PICC card

Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with r or R to start looking for card and read/display it
   Enter anything starting with w or W to start looking for card and writing below write string
   Enter anything starting with s or S to skip below write string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Message for BANJO	ac:67:b2:2c:c9:c0|BANJO ; VOLUME:GSCALE 50"
Enter your command to process this String > 
You Entered "r"
Please place card to read on reader (AKA writer)
--READING---------
Card UID:  72 1C EE AC
PICC read final MFRC522 status 0
 --Read---> "ac:67:b2:2c:c9:c0|BANJO ; MUSIC:NEXT ignore ; EYES:PATTERN OPPOSITE/64/BLINK"
Please remove PICC card
   and enter R or r to read another
    or enter M or m to go to main menu
Enter your command >  
You Entered "r"
Please place card to read on reader (AKA writer)
--READING---------
Card UID:  F1 9B A4 82
PICC read final MFRC522 status 0
 --Read---> "ac:67:b2:2c:c9:c0|BANJO ; MUSIC:TYPE CHRISTMAS"
Please remove PICC card
   and enter R or r to read another
    or enter M or m to go to main menu
Enter your command >  
You Entered "m"
Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with r or R to start looking for card and read/display it
   Enter anything starting with w or W to start looking for card and writing below write string
   Enter anything starting with s or S to skip below write string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Message for BANJO	ac:67:b2:2c:c9:c0|BANJO ; VOLUME:GSCALE 50"
Enter your command to process this String >  
You Entered "s"
Skipping to next string

Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with r or R to start looking for card and read/display it
   Enter anything starting with w or W to start looking for card and writing below write string
   Enter anything starting with s or S to skip below write string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Message for BANJO	ac:67:b2:2c:c9:c0|BANJO ; VOLUME:GSCALE 150"
Enter your command to process this String >  
You Entered "a"
Aborting; done
```
