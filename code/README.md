# Guide to Code
 
| One Remote to Rule Them All | I can bend countless devices to my will |
| --- | --- |
| <img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/LegendaryArtifactsClub_1.png" width="300" alt="Legendary Artifacts Club by Elle Cordova"> | <img src="https://github.com/Mark-MDO47/UniRemote/blob/master/resources/images/LegendaryArtifactsClub_2.png" width="300" alt="Legendary Artifacts Club by Elle Cordova"> |
| Link | Description |
| --- | --- |
| --- | **MAINLINE CODE** |
| [code/UniRemoteCYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD "UniRemoteCYD") | "Cheap Yellow Display"-based UniRemote code sending ESP-NOW commands - attributions in the code.<br>Commands are input with either the QR code reader (I2C) or the RFID Reader (SPI via MicroSD sniffer).<br>To free up access to the CYD hardware VSPI port for the RFID Reader, the code uses the XPT2046_Bitbang software-based SPI library instead of the TFT_eSPI hardware based SPI library for the touchscreen.<br>This technique of using XPT2046_Bitbang for the touchscreen could alternatively be used to access the MicroSD card on the CYD if the MicroSD pins weren't being used for the RFID Reader. |
| [code/UniRemoteRcvrTemplate](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteRcvrTemplate "UniRemoteRcvrTemplate") | UniRemote code **template** for generic receiver of the commands - attributions in the code.<br>UniRemoteRcvrTemplate.ino shows an example of using UniRemoteRcvr.h and UniRemoteRcvr.cpp to receive ESP-NOW commands from UniRemoteCYD |
| [code/Uni_RW_PICC](https://github.com/Mark-MDO47/UniRemote/tree/master/code/Uni_RW_PICC "Uni PICC Rd/Wr") | UniRemote PICC read/write routines - attribution in the README |
| [code/tiny_code_reader](https://github.com/Mark-MDO47/UniRemote/tree/master/code/tiny_code_reader "tiny_code_reader") | *.h file for using the QR code reader - attribution in its README |
| --- | **UTILITIES** |
| [code/readMacAddress](https://github.com/Mark-MDO47/UniRemote/tree/master/code/readMacAddress "readMacAddress") | Code to read the WiFi MAC address of pretty much any ESP32 - used on remotes to get info - attribution in its README |
| [code/CYDbitBangCalibrate](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDbitBangCalibrate "CYDbitBangCalibrate") | Modified Random Nerds CYD Calibrate routine that uses the XPT2046_Bitbang software-based SPI library instead of the TFT_eSPI hardware based SPI library for the touchscreen - attributions in the code |
| [code/WriteRFID_CYD](https://github.com/Mark-MDO47/UniRemote/tree/master/code/WriteRFID_CYD "WriteRFID_CYD") | Code with same hardware setup as UniRemoteCYD to write RFID cards using input text strings in same format as input to QRCode.py - attributions in the code |
| [MDOpythonUtils QRCode.py](https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode "QRCode.py") | Python routine to generate QR code based on directions in a text file |
| --- | **DOCUMENTATION** |
| [code/CYDtest](https://github.com/Mark-MDO47/UniRemote/tree/master/code/CYDtest "CYDtest") | ESP32-2432S028R (Cheap Yellow Display or CYD) pointers to tutorials and hardware information |
| [code/RFIDRC522test](https://github.com/Mark-MDO47/UniRemote/tree/master/code/RFIDRC522test "RFIDRC522test")  | RFID RC522 pointers to tutorials and hardware information |
| [code/UniRemote](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemote "UniRemote") | UniRemote code with QR code reader and **generic ESP32** module - attributions in the code. This is now **unused** and **deprecated**. I am switching to the CYD and the code in **UniRemoteCYD**. |
| [code/UniTestRcvr](https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniTestRcvr "UniTestRcvr") | UniRemote code testbench for receiver of the commands - attributions in the code. This is now **unused** and **deprecated**. All further development for the receiver will go to the generic **UniRemoteRcvrTemplate**. |
| --- end of table --- | --- end of table --- |
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
| Routine | Description |
| --- | --- |
| void lv_indev_set_long_press_time(lv_indev_t * indev, uint16_t long_press_time); | Setting value to 65,535 after creating indev did not help |
| void lv_indev_reset_long_press(lv_indev_t * indev); | Calling within indev read_cb function (cyd_input_read() in my code) when .tirqTouched() or .touched() caused buttons to not operate |
