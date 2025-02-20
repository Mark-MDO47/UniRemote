/* Author: https://github.com/Mark-MDO47  Feb. 28, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 * UniRemoteRcvr - Code for receiving commands from UniRemote
 *
 * Status returns from these routines return an esp_err_t code
 *    taken from the Espressif ESP32 library files esp_err.h or esp_now.h
 * Below is a list of the possible status codes
 * ESP_FAIL                    Generic esp_err_t code indicating failure
 * ESP_OK                      value indicating success (no error)
 * ESP_ERR_ESPNOW_NOT_INIT     ESPNOW is not initialized.
 * ESP_ERR_ESPNOW_ARG          Invalid argument
 * ESP_ERR_ESPNOW_NO_MEM       Out of memory
 * ESP_ERR_ESPNOW_INTERNAL     Internal error
 * ESP_ERR_ESPNOW_IF           Interface error

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

#include <UniRemoteRcvr.h>  // for UniRemoteRcvr "library"


// ESP-NOW definitions
static char g_uni_remote_rcvr_data[ESP_NOW_MAX_DATA_LEN];
static uint16_t g_uni_remote_rcvr_msglen = 0;
static uint16_t g_uni_remote_rcvr_msgnum = 0;
static uint16_t g_uni_remote_rcvr_prev_msgnum = 0;
static esp_err_t g_uni_remote_rcvr_status = ESP_OK;
#define UNI_ESP_NOW_HDR_MAC_OFFSET 12 // This is where the MAC address is on my system
static char g_uni_remote_rcvr_header[ESP_NOW_ETH_ALEN];

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_callback() - callback function that will be executed when data is received
void uni_remote_rcvr_callback(const uint8_t * mac_addr, const uint8_t *recv_data, int recv_len) {
  Serial.println("cb 01");
  if (recv_len >= sizeof(g_uni_remote_rcvr_data)) {
    Serial.print("cb 02 skip recv_len:"); Serial.print(recv_len); 
    Serial.print("sizeof:"); Serial.println(sizeof(g_uni_remote_rcvr_data)); 
    g_uni_remote_rcvr_status = ESP_ERR_ESPNOW_ARG;
    g_uni_remote_rcvr_msglen = recv_len;
    g_uni_remote_rcvr_msgnum += 1;
    return;
  }
  Serial.println("cb 03");
  g_uni_remote_rcvr_status = ESP_OK;
  memset(g_uni_remote_rcvr_data, '\0', recv_len+1);
  strncpy(g_uni_remote_rcvr_data, (char *)recv_data, recv_len);
  memcpy(g_uni_remote_rcvr_header, &mac_addr[UNI_ESP_NOW_HDR_MAC_OFFSET], ESP_NOW_ETH_ALEN);
  g_uni_remote_rcvr_msglen = recv_len;
  g_uni_remote_rcvr_msgnum += 1;
  Serial.println("cb 04");
} // end uni_remote_rcvr_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_init()
//       returns: esp_err_t status
//   WiFi   - set to WIFI_STA mode
//
esp_err_t uni_remote_rcvr_init() {
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // init ESP-NOW
  esp_err_t status_init_espnow = esp_now_init();
  if (status_init_espnow != ESP_OK) {
    return(status_init_espnow);
  }

  // register ESP-NOW receiver callback
  return(esp_now_register_recv_cb(esp_now_recv_cb_t(uni_remote_rcvr_callback)));
} // end uni_remote_rcvr_init()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_get_msg()
//       returns: esp_err_t status
//
//    Parameters:
//      max_len     - input  - max number of chars (including zero termination) to return
//
//      rcvd_len_ptr - output - pointer to number of chars returned (not including zero termination)
//      rcvd_msg_ptr - output - pointer to area to store the received message
//      mac_addr_ptr - output - pointer to array of length ESP_NOW_ETH_ALEN (6) uint8_t to receive MAC address of source of message
//
// status return will always be ESP_OK or ESP_ERR_ESPNOW_ARG
//
// rcvd_len will be zero if no message or the number of bytes returned not counting the zero termination
//     rcvd_len will always be less than max_len and will always be less than ESP_NOW_MAX_DATA_LEN (currently  250)
// rcvd_msg will have the zero-terminated message if rcvd_len is > 0
// mac_addr will have the array of bytes (uint8_t mac_addr[6]) filled with the MAC address of the sending node
//
esp_err_t uni_remote_rcvr_get_msg(uint16_t max_len, uint16_t * rcvd_len_ptr, char * rcvd_msg_ptr, uint8_t * mac_addr_ptr) {
  esp_err_t status = ESP_OK;
  if (g_uni_remote_rcvr_prev_msgnum != g_uni_remote_rcvr_msgnum) {
    // TODO FIXME should be just one message with no skips
    g_uni_remote_rcvr_prev_msgnum = g_uni_remote_rcvr_msgnum;
    *rcvd_len_ptr = 0;
    if (ESP_OK == g_uni_remote_rcvr_status) {
      *rcvd_len_ptr = g_uni_remote_rcvr_msglen;
      memset(rcvd_msg_ptr, '\0', max_len);
      strncpy(rcvd_msg_ptr, (char *)g_uni_remote_rcvr_data, max_len-1);
      memcpy(mac_addr_ptr, g_uni_remote_rcvr_header, ESP_NOW_ETH_ALEN);
    }
    if (max_len < (g_uni_remote_rcvr_msglen+1)) {
      status = ESP_ERR_ESPNOW_ARG;
    }
  }
  return(status);
} // end uni_remote_rcvr_get_msg()
