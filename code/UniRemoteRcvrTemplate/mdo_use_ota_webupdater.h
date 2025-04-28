#ifndef MDO_USE_OTA_WEBUPDATER_H

/* Author: https://github.com/Mark-MDO47  Apr. 27, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 * UniRemoteRcvrTemplate - Template code for receiving commands from UniRemoteCYD
 *     using the UniRemoteRcvr "library"
 * mdo_use_ota_webupdater - My adaptation of Over-The-Air ESP32 software updates.
 * It is based on the Arduino ESP32 example code OTAWebUpdater.ino
 *
 * The idea is that the OTA Web Updater is not started until we receive an ESP-NOW
 *    command to initialize and start it up. Prior to that, we do not connect to any
 *    WiFi SSID, using only ESP-NOW.
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

#include "../gitignore_wifi_key.h" // for OTA updating

#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>

extern const char* host;
extern const char* g_ssid;
extern const char* g_password;

#define MDO_USE_OTA_WEB_UPDATER_NOT_INIT  0 // OTA Web Server not initialized/started
#define MDO_USE_OTA_WEB_UPDATER_REQUESTED 1 // ESP-NOW requested to initialize/start OTA Web Server
#define MDO_USE_OTA_WEB_UPDATER_INIT      2 // OTA Web Server initialized/started; periodically call g_ota_server.handleClient()
extern uint16_t g_ota_state;

extern WebServer g_ota_server;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// start_ota_webserver() - 
//    returns nothing
extern void start_ota_webserver();

#endif // MDO_USE_OTA_WEBUPDATER_H
