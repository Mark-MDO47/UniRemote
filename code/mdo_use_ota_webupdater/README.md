# mdo_use_ota_webupdater

**Table Of Contents**
* [Top](#mdo_use_ota_webupdater "Top")
* [mdo_use_ota_webupdater - My Adaptation of the ESP32 Example OTAWebUpdater.ino](#mdo_use_ota_webupdater-\--my-adaptation-of-the-esp32-example-otawebupdaterino "mdo_use_ota_webupdater - My Adaptation of the ESP32 Example OTAWebUpdater.ino")
* [How to Use mdo_use_ota_webupdater](#how-to-use-mdo_use_ota_webupdater "How to Use mdo_use_ota_webupdater")
* [Minimum Implementation](#minimum-implementation "Minimum Implementation")
* [Detailed Calling Sequence](#detailed-calling-sequence "Detailed Calling Sequence")
  * [start_ota_webserver function](#start_ota_webserver-function "start_ota_webserver function")

## mdo_use_ota_webupdater - My Adaptation of the ESP32 Example OTAWebUpdater.ino
[Top](#mdo_use_ota_webupdater "Top")<br>
This code is designed for use with an ESP32 processor.

The idea is to allow commanded Over-The-Air (OTA) software updates via a webserver.

The OTA Web Updater is not started until the ESP32 receives some sort of command to do so,
either from ESP-NOW or other means.
- "Not started" means the ESP32 has not connected to any WiFi router, does not have an IP address, and has not served the web page to allow updating.
- ESP-NOW can operate in this "not started" state.
- This hides the OTA capability except for the brief times it is in use.

Once the command to serve the OTA web page is received, the ESP32 connects to the WiFi router (using the built-in SSID and credentials) and then starts the web page.
- In this implementation, it will probably hang if it cannot connect to the specified WiFi SSID.

After the upload and update is complete, the ESP32 automatically reboots and the system returns to the "not started" site for the OTA capability.

## How to Use mdo_use_ota_webupdater
[Top](#mdo_use_ota_webupdater "Top")<br>
The story showing how to use this capability (in my DuelWithBanjos project using ESP-NOW commanding) is here:
- https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/code/DuelWithBanjos/OTA_story.md

## Minimum Implementation
[Top](#mdo_use_ota_webupdater "Top")<br>
The minimum implementation of mdo_use_ota_webupdater inside your *.ino program to obtain this capability is shown below.

**Do this first:**<br>
- Copy the files mdo_use_ota_webupdater.cpp and mdo_use_ota_webupdater.h into the directory  containing your *.ino program.
- Edit the mdo_use_ota_webupdater.cpp line below to include your Wifi and etc. credentials. I placed the file in the .. directory but you can place it wherever you want as long as your "#include" statement can find it.
  - #include "../gitignore_wifi_key.h" // WiFi and WebUpdate credentials

The file gitignore_wifi_key.h I use defines 5 things. The first four are mandatory, the fifth one is an optional password on the command to run the webserver.
```C
#define WIFI_PWD "<your password to your WiFi>"
#define WIFI_SSID "<your WiFi SSID>"
#define WIFI_OTA_WEB_USR "<your login name to ESP32 OTA web server>"
#define WIFI_OTA_WEB_PWD "<your password for ESP32 OTA web server>"
#define WIFI_OTA_ESP_NOW_PWD "<your password for comand to start ESP32 OTA web server>"
```

**Do this near the other #includes**<br>
```C
#include "mdo_use_ota_webupdater.h" // for commanded ESP32 Over-The-Air (OTA) software updates via a webserver
```

**Do this inside the routine that responds to the command to perform OTA, whether from ESP-NOW or other sources**
- NOTE that
  - the implementation shown uses example two from the "Detailed Calling Sequence" for mdo_ota_web_request(). See just after this for other options.
  - the implementation shown checks an ESP-NOW command for a string that includes "OTA:WEB" and the optional password that allows the command to succeed.
    - your code might use a button for the command or other means instead of this string comparison
```C
  if ((NULL != strstr(g_my_message,"OTA:WEB")) && (NULL != strstr(g_my_message,WIFI_OTA_ESP_NOW_PWD))) {
    // This is the correct parameter for code that is using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address)
    mdo_ota_web_request(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE); // loop() will handle it
  }
```
**OTHER OPTIONS for mdo_ota_web_request**
- example if not using WiFi at all and not connecting to router and also not using ESP-NOW:
  - mdo_ota_web_request(START_OTA_WEB_INIT_WIFI_STA | START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
- example if using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address):
  - mdo_ota_web_request(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
- example if already connected to router and have IP address:
  - mdo_ota_web_request(START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);

**Do this inside "loop()" (called periodically) or else in a routine called periodically from "loop()"**<br>
```C
    // if using Over-The-Air software updates
    mdo_ota_web_loop();
```

## Detailed Calling Sequence
[Top](#mdo_use_ota_webupdater "Top")<br>

### mdo_ota_web_request function
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// mdo_ota_web_request() -  Requests that mdo_ota_web_loop() starts the ota_webserver
//    returns nothing
//
//    Parameters:
//      p_init_flags - input  - set bit-flags to specify what to do; "bitwise-OR" them together
//         START_OTA_WEB_INIT_WIFI_STA        - init WiFi to STA mode (do not set if already init ESP-NOW)
//         START_OTA_WEB_BEGIN_WIFI           - connect to router using known SSID and Password and get IP address
//         START_OTA_WEB_INIT_MDNS            - init mdns so can route http://esp32.local to the ESP32
//         START_OTA_WEB_INIT_UPDATER_WEBPAGE - init and start the updater webpage
//
//       example if not using WiFi at all and not connecting to router and also not using ESP-NOW:
//         mdo_ota_web_request(START_OTA_WEB_INIT_WIFI_STA | START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
//       example if using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address):
//         mdo_ota_web_request(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
//       example if already connected to router and have IP address:
//         mdo_ota_web_request(START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
//
// Rationale: if the command to do ota_webserver is an interrupt service routine or a callback routine,
//    we don't want to do the actual process at that time. Instead we set a flag so the next time through
//    loop() we start it with no multitasking problems.
//
// Restriction:
//    It will probably hang if it cannot connect to the specified WiFi SSID.
//
// Results:
//    The web browser address http://esp32.local will find the webpage.
//       Restriction: only one at a time webpage with this name per WiFi router SSID
//    Otherwise if you know the IP address (w.x.y.z) you can just enter http://w.x.y.z in the browser
//
// The Web Page allows a user to login and launch the OTA upload/update page.
// There is a weakness that allows the OTA upload/update web page to be entered without loging in.
//    I have not looked into fixing this. The problem is somewhat mitigated by not calling
//    mdo_ota_web_request() all the time but only when commanded to actually do an update.
//    The weakness cannot be exploited until after mdo_ota_web_request() is called, and also
//    goes away with the automatic reboot after the update completes.
//
void mdo_ota_web_request(uint16_t p_init_flags);
```

### mdo_ota_web_loop function
```C
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// mdo_ota_web_loop() -  handles the OTA webserver through its various states. Call periodically from loop()
//    returns nothing
//
//    Parameters:
//      None.
// 
void mdo_ota_web_loop();
```
