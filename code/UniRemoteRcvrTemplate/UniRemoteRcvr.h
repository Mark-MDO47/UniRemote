/* Author: https://github.com/Mark-MDO47  Feb. 28, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 * UniRemoteRcvr - Code for receiving commands from UniRemote
 *
 * Status returns from these routines return an "expanded" esp_err_t code
 *    All but one are taken from the Espressif ESP32 library files esp_err.h or esp_now.h
 * Below is a list of the possible status codes
 * UNI_ESP_NOW_MSG_RCVD        UniRemote code for message received
 * ESP_FAIL                    Generic esp_err_t code indicating failure
 * ESP_OK                      value indicating success (no error)
 * ESP_ERR_ESPNOW_NOT_INIT     ESPNOW is not initialized.
 * ESP_ERR_ESPNOW_ARG          Invalid argument
 * ESP_ERR_ESPNOW_NO_MEM       Out of memory
 * ESP_ERR_ESPNOW_INTERNAL     Internal error
 * ESP_ERR_ESPNOW_IF           Interface error

 */

#ifndef UNI_REMOTE_RCVR_H 

// include Espressif ESP32 wifi and ESP-NOW 
#include <esp_now.h>  // for ESP-NOW
#include <WiFi.h>     // for ESP-NOW

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_init()
//       returns: esp_err_t status
//   WiFi   - set to WIFI_STA mode
esp_err_t uni_remote_rcvr_init();

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
esp_err_t uni_remote_rcvr_get_msg( uint16_t * rcvd_len_ptr, char * rcvd_msg_ptr, uint8_t * mac_addr_ptr, uint32_t * p_msg_num_ptr);

#endif // UNI_REMOTE_RCVR_H 
