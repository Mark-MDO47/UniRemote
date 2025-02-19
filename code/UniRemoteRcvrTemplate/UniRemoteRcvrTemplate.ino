/* Author: https://github.com/Mark-MDO47  Dec. 22, 2024
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

static uint8_t g_my_mac_addr[ESP_NOW_ETH_ALEN];
static char g_my_message[ESP_NOW_MAX_DATA_LEN];
static uint32_t g_my_message_num = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// print_error_status_info()
//       returns: nothing
//   prints info about the received error status from the call to uni_remote_rcvr_get_msg
//
void print_error_status_info(esp_err_t msg_status) {
  if (ESP_ERR_ESPNOW_ARG == msg_status) {
    Serial.print("ERROR: ESP-NOW recv cb error: recv_len too big: ");
    Serial.print(" msg ");
    Serial.println(g_my_message_num);
  } else {
    Serial.print("ERROR: ESP_NOW unknown error status ");
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
  Serial.print("UniRemoteRcvr received ESP-NOW message number: ");
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
  Serial.println("\nStarting UniTestRcvr\n");

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

  esp_err_t msg_status = uni_remote_rcvr_get_msg((uint16_t) sizeof(g_my_message), &rcvd_len, &g_my_message[0], &g_my_mac_addr[0]);
  if ((ESP_OK == msg_status) && (rcvd_len > 0)) {
    g_my_message_num += 1;
    print_message_info(rcvd_len);
    // end if received a message with good status
  } else if (ESP_OK != msg_status) {
    // print out error status information
    print_error_status_info(msg_status);
  } // end if status good and length > 0

  // Notice that if status is ESP_OK and rcvd_len == 0 then no error, just no message yet
} // end loop()
