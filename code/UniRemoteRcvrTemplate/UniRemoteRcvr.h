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

#ifndef UNI_REMOTE_RCVR_H

// include Espressif ESP32 wifi and ESP-NOW 
#include <esp_now.h>  // for ESP-NOW
#include <WiFi.h>     // for ESP-NOW

// public definitions for circular buffer of ESP-NOW messages

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
typedef struct {
  uint16_t idx_in;             // next entry index for circ_buf_put
  uint16_t idx_out;            // next entry index for circ_buf_get
  uint16_t idx_num;            // number of entries in circ_buf
  uint32_t msg_callback_num;   // number of times ESP-NOW rcvr callback is called
  uint16_t flag_circ_buf_full; // non-zero == flag that circular buffer was full in ESP-NOW rcvr callback
  uint16_t flag_data_too_big;  // non-zero == flag that ESP-NOW rcvr callback with too much data for ESP-NOW
} uni_remote_rcvr_cbuf_extended_status_t;

#define UNI_REMOTE_RCVR_OK                  ESP_OK // success
#define UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED  -101 // circular buffer _put() called but no room in circular buffer; message dropped
#define UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG       -102 // ESP-NOW rcvr callback message bigger than ESP-NOW allows (cannot happen)
#define UNI_REMOTE_RCVR_INFO_NO_MSG_2_GET     -201 // circular buffer _get() called but circular buffer is empty

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
void uni_remote_rcvr_get_extended_status(uni_remote_rcvr_cbuf_extended_status_t * extended_status_ptr);

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
void uni_remote_rcvr_clear_extended_status_flags();

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_init()
//       returns: esp_err_t status
//
//   Circular Buffer inited
//   WiFi    - inited and set to WIFI_STA mode
//   ESP-NOW - inited
//      ESP-NOW receive callback installed
//
esp_err_t uni_remote_rcvr_init();

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
esp_err_t uni_remote_rcvr_get_msg(uint16_t * rcvd_len_ptr, char * rcvd_msg_ptr, uint8_t * mac_addr_ptr, uint32_t * p_msg_num_ptr);

#endif // UNI_REMOTE_RCVR_H 
