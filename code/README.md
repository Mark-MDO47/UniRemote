# Guide to Code
 
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
| [code/WriteRFID](https://github.com/Mark-MDO47/UniRemote/tree/master/code/WriteRFID "WriteRFID") | Used to write RFID cards using input text strings in same format as input to QRCode.py - attributions in the code.<BR>Works on both UniRemoteCYD hardware and EPS32D special purpose hardware. |
| [MDOpythonUtils QRCode.py](https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode "QRCode.py") | Python routine to generate QR code based on directions in a text file |
| --- | **DOCUMENTATION** |
| [code/CYDtest](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDtest "CYDtest") | ESP32-2432S028R (Cheap Yellow Display or CYD) pointers to tutorials and hardware information |
| [code/RFIDRC522test](https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test "RFIDRC522test")  | RFID RC522 pointers to tutorials and hardware information |
| --- | **DEPRECATED** |
| [code/UniRemote](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemote "UniRemote") | UniRemote code with QR code reader and **generic ESP32** module - attributions in the code. This is now **unused** and **deprecated**. I am switching to the CYD and the code in **UniRemoteCYD**. |
| [code/UniTestRcvr](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniTestRcvr "UniTestRcvr") | UniRemote code testbench for receiver of the commands - attributions in the code. This is now **unused** and **deprecated**. All further development for the receiver will go to the generic **UniRemoteRcvrTemplate**. |
