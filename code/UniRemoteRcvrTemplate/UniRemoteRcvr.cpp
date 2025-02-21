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

// definitions to support ESP-NOW
#define UNI_ESP_NOW_HDR_MAC_OFFSET 12 // This is where the MAC address is on my system
#define UNI_REMOTE_RCVR_NUM_BUFR 3 // number of buffers for messages

typedef struct {
  uint16_t idx_in;
  uint16_t idx_out;
  uint16_t idx_num;
  uint8_t mac_addr[UNI_REMOTE_RCVR_NUM_BUFR][ESP_NOW_ETH_ALEN]; // 
  char msg[UNI_REMOTE_RCVR_NUM_BUFR][ESP_NOW_MAX_DATA_LEN];     // 
  uint16_t msg_len[UNI_REMOTE_RCVR_NUM_BUFR];                   // length including trailing zero byte
  uint16_t msg_num[UNI_REMOTE_RCVR_NUM_BUFR];                   // msg num; may skip if messages discarded
  int16_t msg_status[UNI_REMOTE_RCVR_NUM_BUFR];                 // 
} circular_buffer_t;
static circular_buffer_t g_circ_buf; // 

static uint32_t g_msg_callback_num = 0;   // number of time ESP-NOW callback is called
static uint16_t g_flag_circ_buf_full = 0; // non-zero == flag that circular buffer was full in ESP-NOW callback
static uint16_t g_flag_data_too_big = 0;  // non-zero == flag that ESP-NOW callback with too much data for ESP-NOW

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_circ_buf_inc_idx() - return value of incremented index - doesn't check for full
static uint16_t uni_remote_rcvr_circ_buf_inc_idx(uint16_t p_idx) {
  // this could be shorter but I wanted it to be clearer
  uint16_t new_idx = p_idx + 1;
  if (new_idx >= g_circ_buf.idx_num) {
    new_idx = 0;
  }
  return(new_idx);
} // end uni_remote_rcvr_circ_buf_inc_idx()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_circ_buf_get() - get data from circular buffer if data is available
//    note: only one thread may call put and only one thread may call get
static int16_t uni_remote_rcvr_circ_buf_get(char * p_msg_ptr, uint8_t * p_mac_addr_ptr, uint16_t * p_msg_len_ptr, uint32_t * p_msg_num_ptr, esp_err_t * p_msg_stat_ptr) {
  int16_t status = ESP_OK;
  uint16_t new_idx = uni_remote_rcvr_circ_buf_inc_idx(g_circ_buf.idx_out);
  if (g_circ_buf.idx_in == g_circ_buf.idx_out) { // empty
    status = ESP_ERR_ESPNOW_NO_MEM; // probably not a good idea but I will keep this inside UniRemoteRcvr
  } else {
    *p_msg_stat_ptr = g_circ_buf.msg_status[g_circ_buf.idx_out];
    *p_msg_num_ptr = g_circ_buf.msg_num[g_circ_buf.idx_out];
    *p_msg_len_ptr = g_circ_buf.msg_len[g_circ_buf.idx_out];
    memset(p_msg_ptr, '\0', *p_msg_len_ptr);
    strncpy(p_msg_ptr, &g_circ_buf.msg[g_circ_buf.idx_out][0], *p_msg_len_ptr);
    memcpy(p_mac_addr_ptr, &g_circ_buf.mac_addr[g_circ_buf.idx_out][0], ESP_NOW_ETH_ALEN);
    g_circ_buf.idx_out = new_idx;  // MUST be last manipulation of circular buffer
  } // end if data to transfer
  return(status);
} // end uni_remote_rcvr_circ_buf_get()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_circ_buf_put() - put data into circular buffer if room is available
//    note: only one thread may call put and only one thread may call get
static int16_t uni_remote_rcvr_circ_buf_put(const char * p_msg_ptr, const uint8_t * p_mac_addr_ptr, int p_msg_len, uint32_t p_msg_num) {
  int16_t status = ESP_OK;
  uint16_t new_idx = uni_remote_rcvr_circ_buf_inc_idx(g_circ_buf.idx_in);
  if (new_idx == g_circ_buf.idx_out) { // no room
    g_flag_circ_buf_full = 1;
    status = ESP_ERR_ESPNOW_FULL;
  } else {
    g_circ_buf.msg_status[g_circ_buf.idx_in] = ESP_OK;
    g_circ_buf.msg_num[g_circ_buf.idx_in] = p_msg_num;
    g_circ_buf.msg_len[g_circ_buf.idx_in] = (uint16_t) p_msg_len;
    memset(&g_circ_buf.msg[g_circ_buf.idx_in][0], '\0', p_msg_len);
    strncpy(&g_circ_buf.msg[g_circ_buf.idx_in][0], (char *)p_msg_ptr, p_msg_len);
    memcpy(&g_circ_buf.mac_addr[g_circ_buf.idx_in][0], &p_mac_addr_ptr[0], ESP_NOW_ETH_ALEN);
    g_circ_buf.idx_in = new_idx; // MUST be last manipulation of circular buffer
  } // end if room available
  return(status);
} // end uni_remote_rcvr_circ_buf_put()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_callback() - callback function that will be executed when data is received
static void uni_remote_rcvr_callback(const uint8_t * p_mac_addr, const uint8_t *p_recv_data, int p_recv_len) {
  g_msg_callback_num += 1;
  if (p_recv_len >= ESP_NOW_MAX_DATA_LEN) { // cannot happen - data too big
    g_flag_data_too_big = 1;
  } else { // put data into buffer; buf_put() reports if it cannot do it
    uni_remote_rcvr_circ_buf_put((char *)p_recv_data, &p_mac_addr[UNI_ESP_NOW_HDR_MAC_OFFSET], p_recv_len, g_msg_callback_num);
  }
  return;
} // end uni_remote_rcvr_callback()


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_get_extended_status
//       returns: nothing for status
void uni_remote_rcvr_get_extended_status(uint32_t * p_msg_callback_num_ptr, uint16_t * p_flag_circ_buf_full_ptr, uint16_t * p_flag_data_too_big_ptr,
                                         uint16_t * p_idx_num, uint16_t * p_idx_in, uint16_t * p_idx_out) {
  *p_idx_in  = g_circ_buf.idx_in;
  *p_idx_out = g_circ_buf.idx_out;
  *p_idx_num = g_circ_buf.idx_num;
  *p_msg_callback_num_ptr = g_msg_callback_num;
  *p_flag_circ_buf_full_ptr = g_flag_circ_buf_full;
  *p_flag_data_too_big_ptr = g_flag_data_too_big;
} // end uni_remote_rcvr_get_extended_status()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_init()
//       returns: esp_err_t status
//   WiFi    - set to WIFI_STA mode
//   ESP-NOW - inited
//
esp_err_t uni_remote_rcvr_init() {

  // initialize our circular buffer data struct
  g_circ_buf.idx_num = UNI_REMOTE_RCVR_NUM_BUFR;
  g_circ_buf.idx_in = g_circ_buf.idx_out = 0; // when in == out, circ_buf is empty

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
//      p_rcvd_len_ptr - output - pointer to number of chars in received message (not including zero termination)
//      p_rcvd_msg_ptr - output - pointer to area to store the received message
//      p_mac_addr_ptr - output - pointer to array of length ESP_NOW_ETH_ALEN (6) uint8_t to receive MAC address of source of message
//      p_msg_num_ptr  - output - pointer to message number == count of callbacks at time of this message receive
//
// status return will always be ESP_OK
//
// p_rcvd_len will be zero if no message or the number of bytes returned not counting the zero termination
//     p_rcvd_len will always be less than max_len and will always be less than ESP_NOW_MAX_DATA_LEN (currently  250)
//   following entries are only changed if p_rcvd_len is > 0
// p_rcvd_msg will have the zero-terminated message
// p_mac_addr will have the array of bytes (uint8_t mac_addr[6] or [ESP_NOW_ETH_ALEN]) filled with the MAC address of the sending node
// p_msg_num  will have the number of callbacks associated with this message
//
esp_err_t uni_remote_rcvr_get_msg(uint16_t * p_rcvd_len_ptr, char * p_rcvd_msg_ptr, uint8_t * p_mac_addr_ptr, uint32_t * p_msg_num_ptr) {
  static esp_err_t msg_status;
  static esp_err_t circ_buf_status;

  // get the next message if there is one
  circ_buf_status = uni_remote_rcvr_circ_buf_get(p_rcvd_msg_ptr, p_mac_addr_ptr, p_rcvd_len_ptr, p_msg_num_ptr, &msg_status);
  if (ESP_ERR_ESPNOW_NO_MEM == circ_buf_status) {
    // there is no data to return
    *p_rcvd_len_ptr = 0;
  }
  return(ESP_OK);
} // end uni_remote_rcvr_get_msg()
