/* Author: https://github.com/Mark-MDO47  Feb. 19, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *  
 * UniRemoteRcvrTemplate - Template code for receiving commands from UniRemoteCYD
 *     using the UniRemoteRcvr "library"
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
 // This code was developed after reading the Random Nerd Tutorial below.
 // There are significant differences in this code and that, but I want to give a
 //    tip of the hat to Rui Santos & Sara Santos for the wonderful work they do.
 /*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/


#include "UniRemoteRcvr.h" // my library for UniRemoteRcvr

#define MDO_USE_OTA 1   // zero to not use, non-zero to use OTA ESP32 Over-The-Air software updates

#if MDO_USE_OTA
#include "mdo_use_ota_webupdater.h"
#endif // MDO_USE_OTA

static char g_my_message[ESP_NOW_MAX_DATA_LEN];     // received message
static uint8_t g_sender_mac_addr[ESP_NOW_ETH_ALEN]; // sender MAC address
static uint32_t g_my_message_num = 0;               // increments for each msg received unless UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// print_error_status_info()
//       returns: nothing
//   prints info about the received error status from the call to uni_remote_rcvr_get_msg
//
void print_error_status_info(esp_err_t msg_status) {
  if (UNI_REMOTE_RCVR_OK == msg_status) {
    return;
  } else if (UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status) {
    Serial.print("ERROR: ESP-NOW recv cb error: recv msg but no room in FIFO, some message(s) dropped: msg ");
    Serial.println(g_my_message_num);
  } else if (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG == msg_status) {
    Serial.print("ERROR: ESP-NOW recv cb error: recv_len too big: msg ");
    Serial.println(g_my_message_num);
  } else {
    Serial.print("ERROR: ESP-NOW unknown error status ");
    Serial.print(msg_status);
    Serial.print(" msg ");
    Serial.println(g_my_message_num);
  } // end display error status returns
} // end print_error_status_info()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// handle_message()
//       returns: nothing
//   prints info about the received message
//   if MDO_USE_OTA and message is OTA:WEB then start ota_webupdater
//
void handle_message(uint16_t rcvd_len) {
  // start the print
  Serial.print("UniRemoteRcvrTemplate received ESP-NOW message number: ");
  Serial.print(g_my_message_num);

  // print originating MAC address
  Serial.print(" sending mac_addr");
  for (int i = 0; i < sizeof(g_sender_mac_addr); i++) {
    Serial.print(":");
    if (g_sender_mac_addr[i] < 16) { // is this stupid or what?
      Serial.print("0");
    }
    Serial.print(g_sender_mac_addr[i],HEX);
  }
  Serial.println(" ");

  // print info about message and the message itself (assumed to be zero-terminated ASCII string)
  Serial.print(" msglen ");
  Serial.print(rcvd_len);
  Serial.print(" '");
  Serial.print((char *)g_my_message);
  Serial.println("'");

#if MDO_USE_OTA // if using Over-The-Air software updates
  if ((NULL != strstr(g_my_message,"OTA:WEB")) && (NULL != strstr(g_my_message,WIFI_OTA_ESP_NOW_PWD))) {
    // This is the correct parameter for code that is using ESP-NOW but not connecting to router (already in WiFi STA mode but no IP address)
    mdo_ota_web_request(START_OTA_WEB_BEGIN_WIFI | START_OTA_WEB_INIT_MDNS | START_OTA_WEB_INIT_UPDATER_WEBPAGE); // loop() will handle it
  }
#endif // MDO_USE_OTA if using Over-The-Air software updates

} // end handle_message()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup() - initialize hardware and software
//       returns: nothing
//   Serial - initialized
//   WiFi   - init for ESP-NOW in WIFI_STA mode
//
void setup() {
  Serial.begin(115200); // basically for debugging...
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("\nStarting UniTestRcvrTemplate\n");

  // init UniRemoteRcvr - inits WiFi mode to WIFI_STA and inits ESP-NOW
  esp_err_t status_init_uni_remote_rcvr = uni_remote_rcvr_init();
  if (status_init_uni_remote_rcvr != ESP_OK) { // (== UNI_REMOTE_RCVR_OK)
    Serial.print("ERROR: ESP-NOW init error; status: ");
    Serial.println(status_init_uni_remote_rcvr);
    return;
  }
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
//
// see if there is a message to report
void loop() {
  uint16_t rcvd_len = 0; // the length of the message/command. If zero, no message.

  // get any message received. If 0 == rcvd_len, no message.
  esp_err_t msg_status = uni_remote_rcvr_get_msg(&rcvd_len, &g_my_message[0], &g_sender_mac_addr[0], &g_my_message_num);

  // we can get an error even if no message
  print_error_status_info(msg_status); // won't print if UNI_REMOTE_RCVR_OK (== ESP_OK)

  // these error codes come from set/clear flags; clear so can detect next time
  if ((UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status) || (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG == msg_status)) {
    uni_remote_rcvr_clear_extended_status_flags();
  }

  // we can get a message with or without an error; see above uni_remote_rcvr_clear_extended_status_flags()
  // If 0 == rcvd_len, no message.
  if (rcvd_len > 0) {
    handle_message(rcvd_len);
  }

#if MDO_USE_OTA // if using Over-The-Air software updates
  // if using Over-The-Air software updates
  mdo_ota_web_loop();
#endif // MDO_USE_OTA if using Over-The-Air software updates

  delay(200);
} // end loop()
