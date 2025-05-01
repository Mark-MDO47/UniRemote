# mdo_use_ota_webupdater

**Table Of Contents**
* [Top](#mdo_use_ota_webupdater "Top")
* [mdo_use_ota_webupdater - My Adaptation of the ESP32 Example OTAWebUpdater.ino](#mdo_use_ota_webupdater-\--my-adaptation-of-the-esp32-example-otawebupdaterino "mdo_use_ota_webupdater - My Adaptation of the ESP32 Example OTAWebUpdater.ino")
* [How to Use mdo_use_ota_webupdater](#how-to-use-mdo_use_ota_webupdater "How to Use mdo_use_ota_webupdater")
* [Minimum Implementation](#minimum-implementation "Minimum Implementation")
* [Detailed Calling Sequence](#detailed-calling-sequence "Detailed Calling Sequence")
  * [start_ota_webserver function](#start_ota_webserver-function "start_ota_webserver function")
  * [g_ota_state global and associated states](#g_ota_state-global-and-associated-states "g_ota_state global and associated states")

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

Do this first:<br>

Copy the files mdo_use_ota_webupdater.cpp and mdo_use_ota_webupdater.h
   into the directory  containing your *.ino program.
Edit the mdo_use_ota_webupdater.cpp line below to include your Wifi and etc. credentials. I placed the file in the .. directory but you can place it wherever you want as long as your "#include" statement can find it.
- #include "../gitignore_wifi_key.h" // WiFi and WebUpdate credentials

The file gitignore_wifi_key.h I use defines 5 things. The first four are mandatory, the fifth one is an optional password on the command to run the webserver.
```C
#define WIFI_PWD "<your password to your WiFi>"
#define WIFI_SSID "<your WiFi SSID>"
#define WIFI_OTA_WEB_USR "<your login name to ESP32 OTA web server>"
#define WIFI_OTA_WEB_PWD "<your password for ESP32 OTA web server>"
#define WIFI_OTA_ESP_NOW_PWD "<your password for comand to start ESP32 OTA web server>"
```

Inside your *.ino file, near the other includes<br>
```C
#include "mdo_use_ota_webupdater.h" // for commanded ESP32 Over-The-Air (OTA) software updates via a webserver
```

Inside the routine that responds to the command to perform OTA, whether from ESP-NOW or other sources
- NOTE that the implementation shown checks for a string command that includes "OTA:WEB" and the optional password that allows the command to succeed
```C
  if ((NULL != strstr(g_my_message,"OTA:WEB")) && (NULL != strstr(g_my_message,WIFI_OTA_ESP_NOW_PWD))) {
    g_ota_state = MDO_USE_OTA_WEB_UPDATER_REQUESTED; // loop() will handle it without getting multi-tasking issues
  }
```

Inside "loop()" or else in a routine called from "loop()" (NOTE: this assumes we are using ESP-NOW already; see after code for other options)<br>
```C
  if (MDO_USE_OTA_WEB_UPDATER_REQUESTED == g_ota_state) {
    start_ota_webserver(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
    g_ota_state = MDO_USE_OTA_WEB_UPDATER_INIT;
  }
  if (MDO_USE_OTA_WEB_UPDATER_INIT == g_ota_state) {
    g_ota_server.handleClient();
  } // end if MDO_USE_OTA_WEB_UPDATER_INIT
```

**OTHER OPTIONS for start_ota_webserver**
- example if not using WiFi at all and not connecting to router and also not using ESP-NOW:
  - start_ota_webserver(START_OTA_WEB_INIT_WIFI_STA | START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
- example if using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address):
  - start_ota_webserver(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
- example if already connected to router and have IP address:
  - start_ota_webserver(START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);

## Detailed Calling Sequence
[Top](#mdo_use_ota_webupdater "Top")<br>

### start_ota_webserver function
[Top](#mdo_use_ota_webupdater "Top")<br>
```C
// start_ota_webserver() -  connects to the WiFi router (using the built-in SSID and credentials) and then starts the web page.
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
//         start_ota_webserver(START_OTA_WEB_INIT_WIFI_STA | START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
//       example if using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address):
//         start_ota_webserver(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
//       example if already connected to router and have IP address:
//         start_ota_webserver(START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE);
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
//    start_ota_webserver() all the time but only when commanded to actually do an update.
//    The weakness cannot be exploited until after start_ota_webserver() is called, and also
//    goes away with the automatic reboot after the update completes.
//
void start_ota_webserver(uint16_t p_init_flags);
```

### g_ota_server global and its usage
[Top](#mdo_use_ota_webupdater "Top")<br>
```C
extern WebServer g_ota_server; // call g_ota_server.handleClient() periodically once MDO_USE_OTA_WEB_UPDATER_INIT
```

### g_ota_state global and associated states
[Top](#mdo_use_ota_webupdater "Top")<br>
```C
#define MDO_USE_OTA_WEB_UPDATER_NOT_INIT  0 // OTA Web Server not initialized/started
#define MDO_USE_OTA_WEB_UPDATER_REQUESTED 1 // We are requested to initialize/start OTA Web Server from loop()
#define MDO_USE_OTA_WEB_UPDATER_INIT      2 // OTA Web Server initialized/started; periodically call g_ota_server.handleClient()
extern uint16_t g_ota_state;
```
