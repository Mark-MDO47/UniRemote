# mdo_use_ota_webupdater

**Table Of Contents**
* [Top](#mdo_use_ota_webupdater "Top")

The story of how to use this capability in my DuelWithBanjos project is here
- https://github.com/Mark-MDO47/DuelWithBanjos/blob/master/code/DuelWithBanjos/OTA_story.md

## Minimum Implementation
The minimum implementation of mdo_use_ota_webupdater inside your *.ino program.

With the other includes"<br>
```C
#include "mdo_use_ota_webupdater.h" // for commanded OTA ESP32 Over-The-Air (OTA) software updates
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
