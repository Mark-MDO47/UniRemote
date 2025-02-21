/* Author: https://github.com/Mark-MDO47  Feb. 19, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *  
 * UniRemoteRcvrTemplate - Template code for receiving commands from UniRemoteCYD
 *     using the UniRemoteRcvr "library"
 *  
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

static char g_my_message[ESP_NOW_MAX_DATA_LEN];
static uint8_t g_my_mac_addr[ESP_NOW_ETH_ALEN];
static uint32_t g_my_message_num = 0;

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
// print_message_info()
//       returns: nothing
//   prints info about the received message
//
void print_message_info(uint16_t rcvd_len) {
  // start the print
  Serial.print("UniRemoteRcvrTemplate received ESP-NOW message number: ");
  Serial.print(g_my_message_num);

  // print originating MAC address
  Serial.print(" sending mac_addr");
  for (int i = 0; i < sizeof(g_my_mac_addr); i++) {
    Serial.print(":");
    if (g_my_mac_addr[i] < 16) { // is this stupid or what?
      Serial.print("0");
    }
    Serial.print(g_my_mac_addr[i],HEX);
  }
  Serial.println(" ");

  // print info about message and the message itself (assumed to be zero-terminated ASCII string)
  Serial.print(" msglen ");
  Serial.print(rcvd_len);
  Serial.print(" '");
  Serial.print((char *)g_my_message);
  Serial.println("'");
} // end print_message_info()

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

  // init UniRemoteRcvr
  esp_err_t status_init_espnow = uni_remote_rcvr_init();
  if (status_init_espnow != ESP_OK) {
    Serial.print("ERROR: ESP-NOW init error; status: ");
    Serial.println(status_init_espnow);
    return;
  }
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
//
// see if there is a message to report
void loop() {
  uint16_t rcvd_len = 0;

  // get any message received. If 0 == rcvd_len, no message.
  esp_err_t msg_status = uni_remote_rcvr_get_msg(&rcvd_len, &g_my_message[0], &g_my_mac_addr[0], &g_my_message_num);

  // we can get an error even if no message
  print_error_status_info(msg_status); // won't print if UNI_REMOTE_RCVR_OK (== ESP_OK)

  // these error codes come from set/clear flags; clear so can detect next time
  if ((UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status) || (UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status)) {
    uni_remote_rcvr_clear_extended_status_flags();
  }

  // If 0 == rcvd_len, no message.
  if (rcvd_len > 0) {
    print_message_info(rcvd_len);
  }

  delay(200);
} // end loop()
