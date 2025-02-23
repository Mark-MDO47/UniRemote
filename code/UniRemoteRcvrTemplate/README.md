# UniRemoteRcvr and UniRemoteRcvrTemplate

**Table Of Contents**
* [Top](#uniremotercvr-and-uniremotercvrtemplate "Top")
* [The Simplest Pattern for Using UniRemoteCYD and ESP-NOW Commands](#the-simplest-pattern-for-using-uniremotecyd-and-esp\-now-commands "The Simplest Pattern for Using UniRemoteCYD and ESP-NOW Commands")
* [What are all the routines I might call](#what-are-all-the-routines-i-might-call "What are all the routines I might call")
* [Detailed Calling Sequence](#detailed-calling-sequence "Detailed Calling Sequence")
  * [uni_remote_rcvr_init](#uni_remote_rcvr_init "uni_remote_rcvr_init")
  * [uni_remote_rcvr_get_msg](#uni_remote_rcvr_get_msg "uni_remote_rcvr_get_msg")
  * [uni_remote_rcvr_get_extended_status](#uni_remote_rcvr_get_extended_status "uni_remote_rcvr_get_extended_status")
  * [uni_remote_rcvr_clear_extended_status_flags](#uni_remote_rcvr_clear_extended_status_flags "uni_remote_rcvr_clear_extended_status_flags")
* [What Error Codes Might I Receive](#what-error-codes-might-i-receive "What Error Codes Might I Receive")
* [TLDR Why Call uni_remote_rcvr_clear_extended_status_flags](#tldr-why-call-uni_remote_rcvr_clear_extended_status_flags "TLDR Why Call uni_remote_rcvr_clear_extended_status_flags")

## The Simplest Pattern for Using UniRemoteCYD and ESP-NOW Commands
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
In order to use **UniRemoteCYD** to send ESP-NOW commands to your receiver code, your receiver code must run on an ESP-32 that includes WiFi.
- https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/network/esp_now.html

**UniRemoteRcvr.cpp** and **UniRemoteRcvr.h** are the pattern for interfacing with **UniRemoteCYD** and receiving the ESP-NOW commands.<br>
In the simplest complete form, your receiver code ***.ino** program does the following.
- Note that I included a section inside the #ifdef/#endif for **HANDLE_CERTAIN_UNLIKELY_ERRORS**.
- This is optional code but makes the processing complete.

```c
#include "UniRemoteRcvr.h"

void setup() {
  // init UniRemoteRcvr - inits WiFi mode to WIFI_STA and inits ESP-NOW
  esp_err_t status_init_uni_remote_rcvr = uni_remote_rcvr_init();
  if (status_init_uni_remote_rcvr != ESP_OK) { // (== UNI_REMOTE_RCVR_OK)
    // handle error status
    return;
  }

  // Initialize everything else

} // end setup()

void loop() {
  static char my_message[ESP_NOW_MAX_DATA_LEN];     // received message
  static uint8_t sender_mac_addr[ESP_NOW_ETH_ALEN]; // sender MAC address
  static uint32_t my_message_num = 0;               // increments for each msg received unless UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED
  uint16_t rcvd_len = 0; // the length of the message/command. If zero, no message.

  // get any message received. If 0 == rcvd_len, no message.
  esp_err_t msg_status = uni_remote_rcvr_get_msg(&rcvd_len, &my_message[0], &sender_mac_addr[0], &my_message_num);

#ifdef HANDLE_CERTAIN_UNLIKELY_ERRORS
  // we can get an error even if no message
  if (msg_status != UNI_REMOTE_RCVR_OK) { // (== ESP_OK)
    // handle error status here

    // these error codes come from set/clear flags; clear so can detect next time
    if ((UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status) || (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG == msg_status)) {
      uni_remote_rcvr_clear_extended_status_flags();
    }
  }
#endif // HANDLE_CERTAIN_UNLIKELY_ERRORS

  // we can get a message with or without an error; see above uni_remote_rcvr_clear_extended_status_flags()
  // If 0 == rcvd_len, no message.
  if (rcvd_len > 0) {
    // process command
  }

  // do everything else the program does
} // end loop()
```

**UniRemoteRcvrTemplate.ino** is an example program that illustrates one way to follow this pattern.

## What are all the routines I might call
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
There are four routines that can be called from UniRemoteRcvr; listed in the table below.
- The first two are those necessary for absolutely minimum functionality.
- The last two routines are used to assist with conditions that are not expected to be seen by the average user.
- Parameters are omitted in this table to give an overview without too much detail.

| Routine | Type | Description |
| --- | --- | --- |
| esp_err_t uni_remote_rcvr_init() | necessary | initialization; call inside setup() |
| esp_err_t uni_remote_rcvr_get_msg() | necessary | returns message if one is ready; also returns deeper uni_remote_rcvr error codes |
| void uni_remote_rcvr_get_extended_status() | optional | returns extended status for conditions that are not expected to be seen by the average user |
| void uni_remote_rcvr_clear_extended_status_flags() | optional | clears flags from extended status so further events can be detected |

## Detailed Calling Sequence
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
Here is a description of the detiled calling sequence for the above routines, taken from UniRemoteRcvr.h.

### uni_remote_rcvr_init
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
```c
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
```

### uni_remote_rcvr_get_msg
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
```c
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
```

### uni_remote_rcvr_get_extended_status
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
```c
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_remote_rcvr_get_extended_status()
//       returns: nothing for status
//
// uni_remote_rcvr_cbuf_extended_status_t - returned by uni_remote_rcvr_get_extended_status()
// The idx_* items are for internal usage by UniRemoteRcvr.
// The msg_callback_num is the number of times that the ESP-NOW callback routine was called.
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
void uni_remote_rcvr_get_extended_status(uni_remote_rcvr_cbuf_extended_status_t * extended_status);
```

### uni_remote_rcvr_clear_extended_status_flags
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
```c
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
```

## What Error Codes Might I Receive
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
The "esp_err_t" returned from the "necessary" routines above denotes a slightly extended range compared to the ESP32 WiFi routines.
| Routine | Status Description |
| --- | --- |
| esp_err_t uni_remote_rcvr_init() | all return codes are from the Espressif ESP32 WiFi routines |
| esp_err_t uni_remote_rcvr_get_msg() | returns include my extended range |

Here is a description of the status returns that might be received from calling the above two routines, taken from UniRemoteRcvr.h.
```
Status returns from these routines return an "expanded" esp_err_t code
It is also possible to receive an error code from Espressif ESP32 library files esp_err.h or esp_now.h
I tried to give my codes different values than the ESP-NOW codes (except for ESP_OK)

Below is a list of the codes specific to UniRemoteRcvr
UNI_REMOTE_RCVR_OK                   same as ESP_OK
UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED circular buffer _put() called but no room in circular buffer; message dropped
UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG      ESP-NOW callback message bigger than ESP-NOW allows (cannot happen)
UNI_REMOTE_RCVR_INFO_NO_MSG_2_GET    circular buffer _get() called but circular buffer is empty
                                     NOTE: this status only used internally, not returned to callers

Below is a list of some of the possible ESP-NOW status codes
ESP_FAIL                    Generic esp_err_t code indicating failure
ESP_OK                      value indicating success (no error)
ESP_ERR_ESPNOW_NOT_INIT     ESPNOW is not initialized.
ESP_ERR_ESPNOW_ARG          Invalid argument
ESP_ERR_ESPNOW_NO_MEM       Out of memory
ESP_ERR_ESPNOW_INTERNAL     Internal error
ESP_ERR_ESPNOW_IF           Interface error
```

## TLDR Why Call uni_remote_rcvr_clear_extended_status_flags
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>
This is best explained in the comments for the calling sequence of uni_remote_rcvr_clear_extended_status_flags().
- [uni_remote_rcvr_clear_extended_status_flags](#uni_remote_rcvr_clear_extended_status_flags "uni_remote_rcvr_clear_extended_status_flags")
