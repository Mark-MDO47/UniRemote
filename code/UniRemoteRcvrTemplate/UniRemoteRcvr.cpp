/* Author: https://github.com/Mark-MDO47  Feb. 28, 2025
 *  https://github.com/Mark-MDO47/UniRemote
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

/*
 * UniRemoteRcvr - Code for receiving commands from UniRemote
 *
 * Status returns from these routines return an "expanded" esp_err_t code
 * It is also possible to receive an error code from Espressif ESP32 library files esp_err.h or esp_now.h
 * I tried to give my codes different values than the ESP-NOW codes (except for ESP_OK)
 *
 * Below is a list of the codes specific to UniRemoteRcvr
 * UNI_REMOTE_RCVR_OK                   same as ESP_OK
 * UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED circular buffer _put() called but no room in circular buffer; message dropped
 * UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG      ESP-NOW rcvr callback message bigger than ESP-NOW allows (cannot happen)
 * UNI_REMOTE_RCVR_INFO_NO_MSG_2_GET    circular buffer _get() called but circular buffer is empty
 *                                      NOTE: this status only used internally, not returned to callers
 *
 * Below is a list of some of the possible ESP-NOW status codes
 * ESP_FAIL                    Generic esp_err_t code indicating failure
 * ESP_OK                      value indicating success (no error)
 * ESP_ERR_ESPNOW_NOT_INIT     ESPNOW is not initialized.
 * ESP_ERR_ESPNOW_ARG          Invalid argument
 * ESP_ERR_ESPNOW_NO_MEM       Out of memory
 * ESP_ERR_ESPNOW_INTERNAL     Internal error
 * ESP_ERR_ESPNOW_IF           Interface error
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

#include <UniRemoteRcvr.h>  // for UniRemoteRcvr "library"

// definitions to support ESP-NOW
#define UNI_ESP_NOW_HDR_MAC_OFFSET 12 // This is where the MAC address is on my system

// private definitions for circular buffer of ESP-NOW messages
#define UNI_REMOTE_RCVR_NUM_BUFR 3 // number of buffers for messages
typedef struct {
  char msg[ESP_NOW_MAX_DATA_LEN];     // received message
  uint8_t mac_addr[ESP_NOW_ETH_ALEN]; // sender MAC address
  uint16_t msg_len;                   // length including trailing zero byte
  uint16_t msg_num;                   // msg num; may skip if messages discarded
  int16_t msg_status;                 // status for this individual message. Almost certainly ESP_OK
} uni_remote_rcvr_circular_buffer_entry_t;

typedef struct {
  uni_remote_rcvr_cbuf_extended_status_t  info;
  uni_remote_rcvr_circular_buffer_entry_t entries[UNI_REMOTE_RCVR_NUM_BUFR];
} uni_remote_rcvr_circular_buffer_t;
static uni_remote_rcvr_circular_buffer_t g_circ_buf; // 

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_circ_buf_inc_idx() - return value of incremented index - doesn't check for full
static uint16_t uni_remote_rcvr_circ_buf_inc_idx(uint16_t p_idx) {
  // this could be shorter but I wanted it to be clearer
  uint16_t new_idx = p_idx + 1;
  if (new_idx >= g_circ_buf.info.idx_num) {
    new_idx = 0;
  }
  return(new_idx);
} // end uni_remote_rcvr_circ_buf_inc_idx()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_circ_buf_get() - get data from circular buffer if data is available
//    note: only one thread may call put and only one thread may call get
static int16_t uni_remote_rcvr_circ_buf_get(char * p_msg_ptr, uint8_t * p_mac_addr_ptr, uint16_t * p_msg_len_ptr, uint32_t * p_msg_num_ptr, esp_err_t * p_msg_stat_ptr) {
  int16_t status = ESP_OK;
  uint16_t new_idx = uni_remote_rcvr_circ_buf_inc_idx(g_circ_buf.info.idx_out);
  uni_remote_rcvr_circular_buffer_entry_t * out_entry_ptr = &g_circ_buf.entries[g_circ_buf.info.idx_out];

  if (g_circ_buf.info.idx_in == g_circ_buf.info.idx_out) { // empty
    status = UNI_REMOTE_RCVR_INFO_NO_MSG_2_GET; // INFO - no data to get 
  } else { // get data from out_entry_ptr
    status = ESP_OK;
    *p_msg_stat_ptr = out_entry_ptr->msg_status;
    *p_msg_num_ptr =  out_entry_ptr->msg_num;
    *p_msg_len_ptr =  out_entry_ptr->msg_len;
    memset(p_msg_ptr, '\0', *p_msg_len_ptr);
    strncpy(p_msg_ptr, &out_entry_ptr->msg[0], *p_msg_len_ptr);
    memcpy(p_mac_addr_ptr, &out_entry_ptr->mac_addr[0], ESP_NOW_ETH_ALEN);
    g_circ_buf.info.idx_out = new_idx;  // MUST be last manipulation of circular buffer
  } // end if data to transfer
  return(status);
} // end uni_remote_rcvr_circ_buf_get()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_circ_buf_put() - put data into circular buffer if room is available
//    note: only one thread may call put and only one thread may call get
static int16_t uni_remote_rcvr_circ_buf_put(const char * p_msg_ptr, const uint8_t * p_mac_addr_ptr, int p_msg_len) {
  int16_t status = ESP_OK;
  uint16_t new_idx = uni_remote_rcvr_circ_buf_inc_idx(g_circ_buf.info.idx_in);
  uni_remote_rcvr_circular_buffer_entry_t * in_entry_ptr = &g_circ_buf.entries[g_circ_buf.info.idx_in];

  if (new_idx == g_circ_buf.info.idx_out) { // no room
    status = ESP_ERR_ESPNOW_FULL;
    g_circ_buf.info.flag_circ_buf_full = 1;
  } else {
    status = ESP_OK;
    in_entry_ptr->msg_status = ESP_OK;
    in_entry_ptr->msg_num = g_circ_buf.info.msg_callback_num;
    in_entry_ptr->msg_len = (uint16_t) p_msg_len;
    memset(&in_entry_ptr->msg[0], '\0', p_msg_len);
    strncpy(&in_entry_ptr->msg[0], (char *)p_msg_ptr, p_msg_len);
    memcpy(&in_entry_ptr->mac_addr[0], &p_mac_addr_ptr[0], ESP_NOW_ETH_ALEN);
    g_circ_buf.info.idx_in = new_idx; // MUST be last manipulation of circular buffer
  } // end if room available
  return(status);
} // end uni_remote_rcvr_circ_buf_put()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_callback() - callback function that will be executed when data is received
static void uni_remote_rcvr_callback(const uint8_t * p_mac_addr, const uint8_t *p_recv_data, int p_recv_len) {
  g_circ_buf.info.msg_callback_num += 1;
  if (p_recv_len >= ESP_NOW_MAX_DATA_LEN) { // cannot happen - data too big
    g_circ_buf.info.flag_data_too_big = 1;
  } else { // put data into buffer; buf_put() reports if it cannot do it
    uni_remote_rcvr_circ_buf_put((char *)p_recv_data, &p_mac_addr[UNI_ESP_NOW_HDR_MAC_OFFSET], p_recv_len);
  }
  return;
} // end uni_remote_rcvr_callback()


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_get_extended_status()
//       returns: nothing for status
//
// uni_remote_rcvr_cbuf_extended_status_t - returned by uni_remote_rcvr_get_extended_status()
// The idx_* items are for internal usage by UniRemoteRcvr.
// The msg_callback_num is the number of times that the ESP-NOW rcvr callback routine was called.
//    It gets stored into the circular buffer each time a message is stored, and returned via p_msg_num_ptr.
//    The *p_msg_num_ptr returned by uni_remote_rcvr_get_msg() will normally increment by one
//    each time a message is returned.
//      If it skips a number, that means there was no room in the circular buffer to store it.
//      See the description about flag_circ_buf_full, UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED,
//      and uni_remote_rcvr_clear_extended_status_flags() 
// Currently there are two flags and associated status returns from uni_remote_rcvr_get_msg()
//    flag_circ_buf_full (UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED) - circular buffer got full and message lost
//         This means that you should either
//            call uni_remote_rcvr_get_msg() more frequently
//            increase UNI_REMOTE_RCVR_NUM_BUFR to allow more buffering
//    flag_data_too_big  (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG) - some bug in ESP-NOW or a bad actor generated
//         condition that didn't cause a buffer overflow because we checked.
//         Honestly I don't expect to ever see this one.
//
void uni_remote_rcvr_get_extended_status(uni_remote_rcvr_cbuf_extended_status_t * extended_status_ptr) {
  memcpy(extended_status_ptr, &g_circ_buf.info, sizeof(uni_remote_rcvr_cbuf_extended_status_t));
} // end uni_remote_rcvr_get_extended_status()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_clear_extended_status_flags()
//       returns: nothing for status
//
// The "flag" entries are global status for uni_remote_rcvr. The code in this file will
//    autonomously set the flags to show that something unusual happened.
//   Your calling code can notice these events and take any desired actions. After any other
//    actions your code performs, it can use uni_remote_rcvr_clear_extended_status_flags()
//    to clear the flags. That way you can tell if the event happened again.
//
// Currently there are two flags and associated status returns from uni_remote_rcvr_get_msg()
//    flag_circ_buf_full (UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED) - circular buffer got full and message lost
//         This means that you should either
//            call uni_remote_rcvr_get_msg() more frequently
//            increase UNI_REMOTE_RCVR_NUM_BUFR to allow more buffering
//    flag_data_too_big  (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG) - some bug in ESP-NOW or a bad actor generated
//         condition that didn't cause a buffer overflow because we checked.
//         Honestly I don't expect to ever see this one.
//
void uni_remote_rcvr_clear_extended_status_flags() {
  g_circ_buf.info.flag_circ_buf_full = 0;
  g_circ_buf.info.flag_data_too_big  = 0;
} // end uni_remote_rcvr_clear_extended_status_flags()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_init()
//       returns: esp_err_t status
//
//   Circular Buffer inited
//   WiFi    - inited and set to WIFI_STA mode
//   ESP-NOW - inited
//      ESP-NOW receive callback installed
//
esp_err_t uni_remote_rcvr_init() {

  // initialize our circular buffer data struct
  g_circ_buf.info.idx_num = UNI_REMOTE_RCVR_NUM_BUFR;
  g_circ_buf.info.idx_in = g_circ_buf.info.idx_out = 0; // when in == out, circ_buf is empty
  g_circ_buf.info.msg_callback_num = 0;   // number of times ESP-NOW rcvr callback is called
  g_circ_buf.info.flag_circ_buf_full = 0; // non-zero == flag that circular buffer was full in ESP-NOW rcvr callback
  g_circ_buf.info.flag_data_too_big = 0;  // non-zero == flag that ESP-NOW rcvr callback with too much data for ESP-NOW

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
//          either ESP_OK, UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED, or UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG
//
//    Parameters:
//      p_rcvd_len_ptr - output - pointer to number of chars in received message (not including zero termination)
//      p_rcvd_msg_ptr - output - pointer to area to store the received message
//      p_mac_addr_ptr - output - pointer to array of length ESP_NOW_ETH_ALEN (6) uint8_t to receive MAC address of source of message
//      p_msg_num_ptr  - output - pointer to message number == count of callbacks at time of this message receive
//
// NOTE: p_rcvd_len and status return are independent.
//          If p_rcvd_len is > 0 then a message was returned, whether status return is ESP_OK or not
//          If status return is not ESP_OK, there might or might not be a message returned
//
// p_rcvd_len will be zero if no message or the number of bytes returned not counting the zero termination
//     p_rcvd_len will always be less than max_len and will always be less than ESP_NOW_MAX_DATA_LEN (currently  250)
//   following entries are only changed if p_rcvd_len is > 0
// p_rcvd_msg will have the zero-terminated message
// p_mac_addr will have the array of bytes (uint8_t mac_addr[6] or [ESP_NOW_ETH_ALEN]) filled with the MAC address of the sending node
// p_msg_num  will have the number of callbacks associated with this message
//    The *p_msg_num_ptr returned by uni_remote_rcvr_get_msg() will normally increment by one
//    each time a message is returned.
//      If it skips a number, that means there was no room in the circular buffer to store it.
//      See the description about flag_circ_buf_full, UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED,
//      and uni_remote_rcvr_clear_extended_status_flags() 
//
esp_err_t uni_remote_rcvr_get_msg(uint16_t * p_rcvd_len_ptr, char * p_rcvd_msg_ptr, uint8_t * p_mac_addr_ptr, uint32_t * p_msg_num_ptr) {
  static esp_err_t msg_status;
  static esp_err_t circ_buf_status;
  static esp_err_t my_status;

  // get the next message if there is one
  circ_buf_status = uni_remote_rcvr_circ_buf_get(p_rcvd_msg_ptr, p_mac_addr_ptr, p_rcvd_len_ptr, p_msg_num_ptr, &msg_status);
  if (UNI_REMOTE_RCVR_INFO_NO_MSG_2_GET == circ_buf_status) {
    // there is no data to return
    *p_rcvd_len_ptr = 0;
  }
  if (0 != g_circ_buf.info.flag_circ_buf_full)     my_status = UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED;
  else if (0 != g_circ_buf.info.flag_data_too_big) my_status = UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG;
  else                                             my_status = ESP_OK;
  return(my_status);
} // end uni_remote_rcvr_get_msg()
