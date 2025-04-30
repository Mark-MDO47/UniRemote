# mdo_use_ota_webupdater

**Table Of Contents**
* [Top](#mdo_use_ota_webupdater "Top")

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

With the other includes"<br>
```C
#include "mdo_use_ota_webupdater.h" // for commanded ESP32 Over-The-Air (OTA) software updates via a webserver
```

Inside the routine that respondes to the command to perform OTA, whether from ESP-NOW or other sources<br>
```C
  if ((NULL != strstr(g_my_message,"OTA:WEB")) && (NULL != strstr(g_my_message,WIFI_OTA_ESP_NOW_PWD))) {
    g_ota_state = MDO_USE_OTA_WEB_UPDATER_REQUESTED; // loop() will handle it without getting multi-tasking issues
  }
```

Inside "loop() or else in a routine called from loop()"<br>
```C
  // if using Over-The-Air software updates
  if (MDO_USE_OTA_WEB_UPDATER_REQUESTED == g_ota_state) {
    start_ota_webserver();
    g_ota_state = MDO_USE_OTA_WEB_UPDATER_INIT;
  }
  if (MDO_USE_OTA_WEB_UPDATER_INIT == g_ota_state) {
    g_ota_server.handleClient();
  } // end if MDO_USE_OTA_WEB_UPDATER_INIT
```
