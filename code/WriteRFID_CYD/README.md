# WriteRFID_CYD

**Table Of Contents**
* [Top](#writerfid_cyd "Top")
* [Description](#description "Description")
* [Example Run of WriteRFID_CYD](#example-run-of-writerfid_cyd "Example Run of WriteRFID_CYD")

## Description
This routine writes and checks command info to PICC (RFID) cards.
It is intended to write PICC cards to be used by UniRemoteCYD.
- https://github.com/Mark-MDO47/UniRemote
- https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD

This routine runs on ESP32-2432S028R (Cheap Yellow Display or CYD) using
the same hardware setup as UniRemoteCYD, with a "sniffer" on the MicroSD
slot connecting the VSPI port to the Mifare RC522 RF IC Card Sensor Module.

The dialog with the operator is done using the serial port, not the display
and touchscreen. Because I am lazy.

The PICC command info is entered in array write_strings[] in the code before running.
The text in write_strings[] is the same format as the input file to QRCode.py
- https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode

## Example Run of WriteRFID_CYD
```
starting WriteRFID_CYD - writes to a PICC RFID card
   see https://github.com/Mark-MDO47/UniRemote

Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with w or W to start looking for card and writing
   Enter anything starting with s or S to skip this string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Test Message to XIAO ESP32-Sense	74:4d:bd:98:7f:1c|TestMessage XIAO ESP32-Sense"
Enter your command to process this String >  
You Entered "Skip"
Skipping to next string

Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with w or W to start looking for card and writing
   Enter anything starting with s or S to skip this string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Test Message to non-existing destination	74:4d:bd:11:11:11|TestMessage non-existing"
Enter your command to process this String >  
You Entered "write"
 --Write--> "74:4d:bd:11:11:11	TestMessage non-existing	Test Message to non-existing destination"
Please place card on writer
--READING---------
Card UID:  13 6A 98 20
PICC Write final MFRC522 status 0
uni_write_picc() succesful! Please remove card, short wait, replace card for reading
--READING---------
Card UID:  13 6A 98 20
PICC read final MFRC522 status 0
 --Read---> "74:4d:bd:11:11:11	TestMessage non-existing	Test Message to non-existing destination"
Comparison GOOD - Read data matches Write data
Please remove PICC card

Prepare to write MIFARE Classic EV1 1K card
   Enter anything starting with w or W to start looking for card and writing
   Enter anything starting with s or S to skip this string and go to the next
   Enter anything starting with a or A to abort this process and stop writing RFID cards
 --String-To-Process--> "Test Message is too short	74:4d:bd:11:11:1|TestMessage address too short"
Enter your command to process this String >  
You Entered "abort"
Aborting; done
```
