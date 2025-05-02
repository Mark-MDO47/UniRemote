#ifndef MDO_USE_OTA_WEBUPDATER_H

/* Author: https://github.com/Mark-MDO47  Apr. 27, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 * UniRemoteRcvrTemplate - Template code for receiving commands from UniRemoteCYD
 *     using the UniRemoteRcvr "library"
 * mdo_use_ota_webupdater - My adaptation of Over-The-Air ESP32 software updates.
 * It is based on the Arduino ESP32 example code OTAWebUpdater.ino
 *
 * The idea is that the OTA Web Updater is not started until we receive a command
 *    (ESP-NOW or otherwise) to initialize and start it up.
 *    Prior to that, we do not connect to any WiFi SSID nor get an IP address.
 *    Note: using ESP-NOW does not connect to any WiFi SSID nor get an IP address.
 *
 * In this implementation, it will probably hang if it cannot connect to the specified
 *    WiFi SSID.
 *
 */

/*
   Copyright 2025 Mark Olson

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
 */
#define MDO_USE_OTA_WEBUPDATER_H

#include "../gitignore_wifi_key.h" // WiFi and WebUpdate credentials

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

extern const char* host;
extern const char* g_ssid;
extern const char* g_password;

#define START_OTA_WEB_INIT_WIFI_STA        0x0001 // init WiFi from beginning (do not set if already init ESP-NOW)
#define START_OTA_WEB_BEGIN_WIFI           0x0002 // connect to router using known SSID and Password and get IP address
#define START_OTA_WEB_INIT_MDNS            0x0004 // init mdns so can route http://esp32.local to the ESP32
#define START_OTA_WEB_INIT_UPDATER_WEBPAGE 0x0008 // init and start the updater webpage


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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// mdo_ota_web_loop() -  handles the OTA webserver through its various states. Call periodically from loop()
//    returns nothing
//
//    Parameters:
//      None.
// 
void mdo_ota_web_loop();

#endif // MDO_USE_OTA_WEBUPDATER_H
