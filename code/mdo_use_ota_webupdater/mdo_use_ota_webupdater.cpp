/* Author: https://github.com/Mark-MDO47  Apr. 27, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 * UniRemoteRcvrTemplate - Template code for receiving commands from UniRemoteCYD
 *     using the UniRemoteRcvr "library"
 * mdo_use_ota_webupdater - My adaptation of Over-The-Air ESP32 software updates.
 *
 * It is based on the Arduino ESP32 example code OTAWebUpdater.ino
 *    Obviously any code from that file is under its own license terms not mine.
 *    I am not sure what its license is, but it must be pretty permissive to be an example.
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

#include "mdo_use_ota_webupdater.h"

const char* host = "esp32";
const char* g_ssid = WIFI_SSID;
const char* g_password = WIFI_PWD;

uint16_t g_ota_state = MDO_USE_OTA_WEB_UPDATER_NOT_INIT;

WebServer g_ota_server(80);

/*
 * Login page
 */

const char* loginIndex =
 "<form name='loginForm'>"
    "<table width='20%' bgcolor='A09F9F' align='center'>"
        "<tr>"
            "<td colspan=2>"
                "<center><font size=4><b>ESP32 Login Page</b></font></center>"
                "<br>"
            "</td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
             "<td>Username:</td>"
             "<td><input type='text' size=25 name='userid'><br></td>"
        "</tr>"
        "<br>"
        "<br>"
        "<tr>"
            "<td>Password:</td>"
            "<td><input type='Password' size=25 name='pwd'><br></td>"
            "<br>"
            "<br>"
        "</tr>"
        "<tr>"
            "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
        "</tr>"
    "</table>"
"</form>"
"<script>"
    "function check(form)"
    "{"
    "if(form.userid.value=='" WIFI_OTA_WEB_USR "' && form.pwd.value=='" WIFI_OTA_WEB_PWD "')"
    "{"
    "window.open('/serverIndex')"
    "}"
    "else"
    "{"
    " alert('Error Password or Username')/*displays error message*/"
    "}"
    "}"
"</script>";

/*
 * Server Index Page
 */

const char* serverIndex =
"<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
"<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
   "<input type='file' name='update'>"
        "<input type='submit' value='Update'>"
    "</form>"
 "<div id='prg'>progress: 0%</div>"
 "<script>"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
 "},"
 "error: function (a, b, c) {"
 "}"
 "});"
 "});"
 "</script>";

/////////////////////////////////////////////////////////////////////////////////////////////////////////
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
void start_ota_webserver(uint16_t p_init_flags) {

  if (p_init_flags & START_OTA_WEB_INIT_WIFI_STA) {
    // Set device as a Wi-Fi Station
    WiFi.mode(WIFI_STA);
  } // end if START_OTA_WEB_INIT_WIFI_STA

  if (p_init_flags & START_OTA_WEB_BEGIN_WIFI) {
    // Connect to WiFi network
    WiFi.begin(g_ssid, g_password);
    Serial.println("");

    // Wait for connection
    while (WiFi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(g_ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } // end if START_OTA_WEB_BEGIN_WIFI

  if (p_init_flags & START_OTA_WEB_INIT_MDNS) {
    /*use mdns for host name resolution*/
    if (!MDNS.begin(host)) { //http://esp32.local
      Serial.println("Error setting up MDNS responder!");
      while (1) {
        delay(1000);
      }
    }
    Serial.println("mDNS responder started");
  } // end if START_OTA_WEB_INIT_MDNS


  if (p_init_flags & START_OTA_WEB_INIT_UPDATER_WEBPAGE) {
    /*return index page which is stored in serverIndex */
    g_ota_server.on("/", HTTP_GET, []() {
      g_ota_server.sendHeader("Connection", "close");
      g_ota_server.send(200, "text/html", loginIndex);
    });
    g_ota_server.on("/serverIndex", HTTP_GET, []() {
      g_ota_server.sendHeader("Connection", "close");
      g_ota_server.send(200, "text/html", serverIndex);
    });
    /*handling uploading firmware file */
    g_ota_server.on("/update", HTTP_POST, []() {
      g_ota_server.sendHeader("Connection", "close");
      g_ota_server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
      ESP.restart();
    }, []() {
      HTTPUpload& upload = g_ota_server.upload();
      if (upload.status == UPLOAD_FILE_START) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
        if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_WRITE) {
        /* flashing firmware to ESP*/
        if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
          Update.printError(Serial);
        }
      } else if (upload.status == UPLOAD_FILE_END) {
        if (Update.end(true)) { //true to set the size to the current progress
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        } else {
          Update.printError(Serial);
        }
      }
    });
    g_ota_server.begin();
  } // end if START_OTA_WEB_INIT_UPDATER_WEBPAGE

} // end start_ota_webserver()
