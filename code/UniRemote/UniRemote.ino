/* Author: https://github.com/Mark-MDO47  Dec. 21, 2023
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

#include <Wire.h>    // for QR sensor (Tiny Code Reader) and anything else

#include "tiny_code_reader/tiny_code_reader.h" // see https://github.com/usefulsensors/tiny_code_reader_arduino.git

// DEBUG definitions

#define DEBUG_SERIALPRINT 1 // print messages
#if DEBUG_SERIALPRINT
  #define DBG_SERIALPRINT   Serial.print
  #define DBG_SERIALPRINTLN Serial.println
#else  // not DEBUG_SERIALPRINT
#endif // DEBUG_SERIALPRINT

#define DEBUG_QR_INPUT 1 // set 1 to get debug messages from QR code sensor

// QR Code definitions
const int32_t QRsampleDelayMsec = 200;

// ESP-NOW definitions
static uint8_t rcvr_mac_addr[6] = {0x74, 0x4d, 0xbd, 0x98, 0x7f, 0x1c}; // FIXME TODO this may be dynamic
static uint8_t rcvr_msg_count = 0;
esp_now_peer_info_t rcvr_peer_info; // will be filled in later

static esp_now_send_status_t g_last_send_callback_status;
static uint8_t g_last_send_callback_msg_count;
static uint8_t g_last_send_msg_count_checked;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// esp_now_send_callback() - ESP-NOW sending callback function
//       returns: nothing
//
void esp_now_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // crude way to save last status - works OK if not sending to fast
  g_last_send_callback_status = status;
  g_last_send_callback_msg_count = rcvr_msg_count;

  // display in case some one is watching
  if (ESP_NOW_SEND_SUCCESS == status) {
    DBG_SERIALPRINT("esp_now_send_callback OK msg ");
    DBG_SERIALPRINTLN(g_last_send_callback_msg_count);
  } else {
    DBG_SERIALPRINT("ERROR: esp_now_send_callback msg status ");
    DBG_SERIALPRINT(status);
    DBG_SERIALPRINT(" msg ");
    DBG_SERIALPRINTLN(g_last_send_callback_msg_count);
  }
} // end esp_now_send_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// esp_now_send() - decipher QR code and send as needed
//    returns: status from call
// FIXME TODO this is only for initial testing
// FIXME TODO WARNING this can modify qr_code
//
esp_err_t esp_now_send(char * qr_code) {
  esp_err_t send_status = ESP_OK;
  uint16_t qr_len = strlen(qr_code)+1;
  if (qr_len >= 256) {
    qr_code[256] = 0;
    qr_len = strlen(qr_code)+1;
  }
  rcvr_msg_count += 1;
  send_status = esp_now_send(rcvr_mac_addr, (uint8_t *) qr_code, qr_len);
  return (send_status);
} // end esp_now_send()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup() - initialize hardware and software
//       returns: nothing
//   Serial - only if serial debugging enabled
//   Wire   - for I2C to QR code sensor (and others...)
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
  DBG_SERIALPRINTLN("\nStarting UniRemote\n");
#endif // DEBUG_SERIALPRINT

  Wire.begin(); // for the QR code sensor

  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // init ESP-NOW
  esp_err_t status_init_espnow = esp_now_init();
  if (status_init_espnow != ESP_OK) {
    DBG_SERIALPRINT("ERROR: ESP-NOW init error ");
    DBG_SERIALPRINTLN(status_init_espnow);
    return;
  }

  // register Send CallBack
  esp_err_t status_register_send_cb = esp_now_register_send_cb(esp_now_send_callback);
  if (status_register_send_cb != ESP_OK){
    DBG_SERIALPRINT("ERROR: ESP-NOW register send callback error ");
    DBG_SERIALPRINTLN(status_register_send_cb);
    return;
  }
  
  // register peer - FIXME TODO this may be dynamic
  memcpy(rcvr_peer_info.peer_addr, rcvr_mac_addr, 6);
  rcvr_peer_info.channel = 0;  
  rcvr_peer_info.encrypt = false;
  
  // Add peer
  esp_err_t status_add_peer = esp_now_add_peer(&rcvr_peer_info);
  if (status_add_peer != ESP_OK){
    DBG_SERIALPRINT("ERROR: ESP-NOW add peer error ");
    DBG_SERIALPRINTLN(status_add_peer);
    return;
  }
} // end setup()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop() - handles the loop
//       returns: nothing
//  if QR code seen
//    send to ESP-NOW destination
//  give a slight delay
//
void loop() {
  static int QRcodeSeen = 1; // 0 == no code found, 1 == code found
  tiny_code_reader_results_t QRresults = {};

  // Perform a read action on the I2C address of the sensor to get the
  // current face information detected.
  if (!tiny_code_reader_read(&QRresults)) {
    DBG_SERIALPRINTLN("No QR code results found on the i2c bus");
    delay(QRsampleDelayMsec);
    return;
  }

  if (0 == QRresults.content_length) {
    if (0 != QRcodeSeen) { Serial.println("No QR code found, waiting...\n"); }
    QRcodeSeen = 0; // found nothing
  } else {
#if (DEBUG_QR_INPUT)
    DBG_SERIALPRINT("Found '");
    DBG_SERIALPRINT((char*)QRresults.content_bytes);
    DBG_SERIALPRINTLN("'");
#endif // DEBUG_QR_INPUT
    QRcodeSeen = 1; // found something
    esp_err_t send_status = esp_now_send((char*)QRresults.content_bytes);
    if (send_status == ESP_OK) {
      DBG_SERIALPRINTLN("ESP-NOW send success");
    }
    else {
      DBG_SERIALPRINT("ERROR: ESP-NOW sending error ");
      DBG_SERIALPRINTLN(send_status);
    }
  }

  delay(QRsampleDelayMsec);
} // end loop()
