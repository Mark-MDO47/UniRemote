/* Author: https://github.com/Mark-MDO47  Dec. 21, 2023
 *  https://github.com/Mark-MDO47/UniRemote
 *  
 *  
 */

 // This code was developed after reading the Random Nerd Tutorial below.
 // There are significant differences in this code and that, but I want to give a
 //    tip of the hat to Rui Santos & Sara Santos for the wonderful work they do.
 // Below is the attribution from the Random Nerd Tutorial.
 /*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/


#include <esp_now.h>   // for ESP-NOW
#include <WiFi.h>      // for ESP-NOW
#include "wifi_key.h"  // WiFi secrets

#include <Wire.h>     // for QR sensor (Tiny Code Reader) and anything else

#include "tiny_code_reader/tiny_code_reader.h" // see https://github.com/usefulsensors/tiny_code_reader_arduino.git

// PIN definitions
#define PIN_MSG_RDY 13

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
#define MDO_ESP_NOW_MSEC_PER_MSG_MIN 500 // millisec between messages
static uint8_t g_rcvr_mac_addr[ESP_NOW_ETH_ALEN * ESP_NOW_MAX_TOTAL_PEER_NUM];
static uint8_t g_rcvr_peer_num = 0;
static uint8_t rcvr_msg_count = 0;

esp_now_peer_info_t rcvr_peer_info; // will be filled in later

static esp_now_send_status_t g_last_send_callback_status;
static uint8_t g_last_send_callback_msg_count;
static uint8_t g_last_send_msg_count_checked;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// esp_now_decode_dbgprint_error() - ESP-NOW print string with error
//
void esp_now_decode_dbgprint_error(uint16_t errcode) {
  DBG_SERIALPRINT(errcode);
  switch (errcode)
  {
    case ESP_ERR_ESPNOW_NOT_INIT:
      DBG_SERIALPRINT(" ESPNOW is not initialized");
      break;
    case ESP_ERR_ESPNOW_ARG:
      DBG_SERIALPRINT(" ESPNOW Invalid argument");
      break;
    case ESP_ERR_ESPNOW_NO_MEM:
      DBG_SERIALPRINT(" ESPNOW Out of memory");
      break;
    case ESP_ERR_ESPNOW_FULL:
      DBG_SERIALPRINT(" ESPNOW peer list is full");
      break;
    case ESP_ERR_ESPNOW_NOT_FOUND:
      DBG_SERIALPRINT(" ESPNOW peer is not found");
      break;
    case ESP_ERR_ESPNOW_INTERNAL:
      DBG_SERIALPRINT(" ESPNOW Internal error");
      break;
    case ESP_ERR_ESPNOW_EXIST:
      DBG_SERIALPRINT(" ESPNOW peer has existed");
      break;
    case ESP_ERR_ESPNOW_IF:
      DBG_SERIALPRINT(" ESPNOW Interface error");
      break;
    default:
      DBG_SERIALPRINT(" ESPNOW UNKNOWN ERROR CODE");
  }
} // end esp_now_decode_dbgprint_error()

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
// qr_decode_get_mac_addr_to_send()
//       returns: (uint8_t *) pointer to MAC address; pointer is zero if bad decode
//
// qr_code is string starting with MAC address; something like this
// 74:4d:bd:11:22:33|the-rest-is-the-command
// 000000000011111111
// 012345678901234567
//
static uint8_t qr_decode_mac_addr[ESP_NOW_ETH_ALEN];
uint8_t * qr_decode_get_mac_addr_to_send(char * qr_code) {
  uint8_t * ret_addr = qr_decode_mac_addr;
  uint8_t tmp;

  // make sure MAC address is of the correct form and decode piece by piece
  if ((3*ESP_NOW_ETH_ALEN+1) > strlen(qr_code)) {
    ret_addr = ((uint8_t *) 0);
  } else { // at least one character after MAC address
    for (int i = 0; i < 3*ESP_NOW_ETH_ALEN; i += 3) {
      if (!isHexadecimalDigit(qr_code[i]) || !isHexadecimalDigit(qr_code[i+1])) {
        ret_addr = ((uint8_t *) 0);
        break;
      }
      tmp = 0;
      if ( ((3*(ESP_NOW_ETH_ALEN-1) != i) && (':' != qr_code[i+2])) ||
           ((3*(ESP_NOW_ETH_ALEN-1) == i) && ('|' != qr_code[i+2])) ) {
        ret_addr = ((uint8_t *) 0);
        break;
      }
      // these two hex digits are good
      for (int j = 0; j < 2; j += 1) {
        tmp <<= 4;
        if      (qr_code[i+j] <= '9') tmp |= qr_code[i+j] - '0';
        else if (qr_code[i+j] <= 'F') tmp |= qr_code[i+j] - 'A' + 10;
        else                          tmp |= qr_code[i+j] - 'a' + 10;
      }
      ret_addr[i/3] = tmp;
    } // end check MAC address
  }

  // if ret_addr is not 0 then the address is good
  return(ret_addr);
} // qr_decode_get_mac_addr_to_send


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// esp_now_register_peer() - 
//       returns: index to peer
int16_t esp_now_register_peer(uint8_t * mac_addr) {
  
  esp_err_t reg_status = ESP_OK;
  int16_t  reg_index = -1;
  uint8_t found = 0;
  int8_t this_found;
  uint8_t i, j;
  for (i = 0; i < g_rcvr_peer_num; i++) {
    this_found = -1;
    for (j = 0; j < ESP_NOW_ETH_ALEN; j++) {
      if (mac_addr[j] != g_rcvr_mac_addr[j+i*ESP_NOW_ETH_ALEN]) {
        this_found = 0;
        break;
      }
    }
    if (0 != this_found) {
      reg_index = i;
      return(reg_index); // already registered
    }
  }
  // not yet registered
  if (g_rcvr_peer_num > (ESP_NOW_MAX_TOTAL_PEER_NUM-1)) {
    reg_status = ESP_ERR_ESPNOW_FULL;
    reg_index = -1;
  } else {
    reg_index = g_rcvr_peer_num;
    memcpy(&g_rcvr_mac_addr[g_rcvr_peer_num*ESP_NOW_ETH_ALEN], mac_addr, ESP_NOW_ETH_ALEN);
    g_rcvr_peer_num += 1;
    memcpy(rcvr_peer_info.peer_addr, mac_addr, ESP_NOW_ETH_ALEN);
    rcvr_peer_info.channel = 0;  
    rcvr_peer_info.encrypt = false;
    // Add peer
    reg_status = esp_now_add_peer(&rcvr_peer_info);
  }
  
  if (reg_status != ESP_OK){
    reg_index = -1;
    DBG_SERIALPRINT("ERROR: ESP-NOW reg/add peer error ");
    DBG_SERIALPRINTLN(reg_status);
  }
  return(reg_index);
} // end esp_now_register_peer

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// esp_now_send() - decipher QR code and send as needed
//       returns: status from call
//   sends a string up to length ESP_NOW_MAX_DATA_LEN; includes the zero termination of the string   
//
// FIXME TODO this is only for initial testing
// FIXME TODO WARNING this can modify qr_code
//
esp_err_t esp_now_send(char * qr_code) {
  esp_err_t send_status = ESP_OK;
  static char msg_data[ESP_NOW_MAX_DATA_LEN+1];
  static uint32_t msec_prev_send = 0;
  uint32_t msec_now = millis();

  // see if waited long enough to send another ESP-NOW message
  if (msec_now < (MDO_ESP_NOW_MSEC_PER_MSG_MIN+msec_prev_send)) {
    DBG_SERIALPRINTLN("ERROR: too soon to send");
    return(ESP_ERR_ESPNOW_INTERNAL); // too soon to send another message
  }
  msec_prev_send = msec_now;

  // see if we can obtain and register the MAC address for sending
  uint8_t * mac_addr_ptr = qr_decode_get_mac_addr_to_send(qr_code);
  int16_t mac_addr_index;
  if ((uint8_t *)0 != mac_addr_ptr) {
    mac_addr_index = esp_now_register_peer(mac_addr_ptr);
  } else {
    DBG_SERIALPRINTLN("ERROR: qr_decode_get_mac_addr_to_send returned 0");
    return(ESP_ERR_ESPNOW_INTERNAL); // could not decode QR code
  }
  if (mac_addr_index < 0) {
    DBG_SERIALPRINTLN("ERROR: could not register MAC");
    return(ESP_ERR_ESPNOW_FULL); // could not register the MAC address
  }

  digitalWrite(PIN_MSG_RDY, HIGH);

  // copy message over starting after the MAC address
  memset(msg_data, '\0', sizeof(msg_data));
  strncpy(msg_data, &qr_code[3*ESP_NOW_ETH_ALEN], ESP_NOW_MAX_DATA_LEN-1); // max ESP-NOW msg size
  int len = strlen(msg_data)+1; // length to send
  rcvr_msg_count += 1;
  send_status = esp_now_send(mac_addr_ptr, (uint8_t *) msg_data, strlen(msg_data)+1);
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

  pinMode(PIN_MSG_RDY,OUTPUT);
  digitalWrite(PIN_MSG_RDY, LOW);

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
      DBG_SERIALPRINT("ESP-NOW send success msg ");
      DBG_SERIALPRINTLN(rcvr_msg_count);
    }
    else {
      DBG_SERIALPRINT("ERROR: ESP-NOW sending error ");
      esp_now_decode_dbgprint_error(send_status);
      DBG_SERIALPRINTLN(" ");
    }
  }

  delay(QRsampleDelayMsec);
} // end loop()
