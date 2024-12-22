/* Author: https://github.com/Mark-MDO47  Dec. 22, 2023
 *  https://github.com/Mark-MDO47/UniRemote
 *  
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


#include <esp_now.h>  // for ESP-NOW
#include <WiFi.h>     // for ESP-NOW

// DEBUG definitions

#define DEBUG_SERIALPRINT 1 // print messages
#if DEBUG_SERIALPRINT
  #define DBG_SERIALPRINT   Serial.print
  #define DBG_SERIALPRINTLN Serial.println
#else  // not DEBUG_SERIALPRINT
#endif // DEBUG_SERIALPRINT

// ESP-NOW definitions
static char g_esp_now_recv_data[ESP_NOW_MAX_DATA_LEN+1];
static uint16_t g_esp_now_recv_msglen = 0;
static uint16_t g_esp_now_recv_msgnum = 0;
static uint16_t g_esp_now_prev_msgnum = 0;
static esp_err_t g_esp_now_recv_status = ESP_OK;
#define MDO_ESP_NOW_HEADER_NUM 46 // basically for debugging
static char g_esp_now_header[MDO_ESP_NOW_HEADER_NUM];

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// esp_now_recv_callback() - callback function that will be executed when data is received
void esp_now_recv_callback(const uint8_t * mac_addr, const uint8_t *recv_data, int recv_len) {
  g_esp_now_recv_msgnum += 1;
  g_esp_now_recv_msglen = recv_len;
  if (recv_len >= sizeof(g_esp_now_recv_data)) {
    g_esp_now_recv_status = ESP_ERR_ESPNOW_ARG;
    return;
  }
  g_esp_now_recv_status = ESP_OK;
  memset(g_esp_now_recv_data, '\0', recv_len+1);
  strncpy(g_esp_now_recv_data, (char *)recv_data, recv_len);
  memcpy(g_esp_now_header, mac_addr, MDO_ESP_NOW_HEADER_NUM);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup() - initialize hardware and software
//       returns: nothing
//   Serial - only if serial debugging enabled
//   WiFi   - for ESP-NOW communications
//
void setup() {
#if DEBUG_SERIALPRINT
  Serial.begin(115200); // basically for debugging...
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this
  DBG_SERIALPRINTLN(""); // print a blank line in case there is some junk from power-on
  DBG_SERIALPRINTLN("\nStarting UniTestRcvr\n");
#endif // DEBUG_SERIALPRINT

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // init ESP-NOW
  esp_err_t status_init_espnow = esp_now_init();
  if (status_init_espnow != ESP_OK) {
    DBG_SERIALPRINT("ERROR: ESP-NOW init error ");
    DBG_SERIALPRINTLN(status_init_espnow);
    return;
  }

  // register ESP-NOW receiver callback
  esp_err_t status_init_recv_cb = esp_now_register_recv_cb(esp_now_recv_cb_t(esp_now_recv_callback));

} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop()
//
void loop() {

  if (g_esp_now_prev_msgnum != g_esp_now_recv_msgnum) {
    g_esp_now_prev_msgnum = g_esp_now_recv_msgnum;
    if (ESP_OK == g_esp_now_recv_status) {
      DBG_SERIALPRINT("ESP-NOW recv cb msg: ");
      DBG_SERIALPRINT(g_esp_now_recv_msgnum);
      DBG_SERIALPRINT(" mac_addr");
      for (int i = 0; i < 40; i++) {
        DBG_SERIALPRINT(":");
        if (g_esp_now_header[i] < 16) { // is this stupid or what?
          DBG_SERIALPRINT("0");
        }
        DBG_SERIALPRINT(g_esp_now_header[i],HEX);
      }
      DBG_SERIALPRINT(" msglen ");
      DBG_SERIALPRINT(g_esp_now_recv_msglen);
      DBG_SERIALPRINT(" '");
      DBG_SERIALPRINT((char *)g_esp_now_recv_data);
      DBG_SERIALPRINTLN("'");
    } else if (ESP_ERR_ESPNOW_ARG == g_esp_now_recv_status) {
      DBG_SERIALPRINT("ERROR: ESP-NOW recv cb error: recv_len too big: ");
      DBG_SERIALPRINT(g_esp_now_recv_msglen);
      DBG_SERIALPRINT(" msg ");
      DBG_SERIALPRINTLN(g_esp_now_recv_msgnum);
    } else {
      DBG_SERIALPRINT("ERROR: ESP_NOW unknown error status ");
      DBG_SERIALPRINT(g_esp_now_recv_status);
      DBG_SERIALPRINT(" msg ");
      DBG_SERIALPRINTLN(g_esp_now_recv_msgnum);
    }
  }
} // end loop()
