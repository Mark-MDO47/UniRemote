# UniRemoteRcvr and UniRemoteRcvrTemplate

**Table Of Contents**
* [Top](#uniremotercvr-and-uniremotercvrtemplate "Top")
* [The Pattern for Using UniRemoteCYD and ESP-NOW Commands](#the-pattern-for-using-uniremotecyd-and-esp\-now-commands "The Pattern for Using UniRemoteCYD and ESP-NOW Commands")

## The Pattern for Using UniRemoteCYD and ESP-NOW Commands
[Top](#uniremotercvr-and-uniremotercvrtemplate "Top")<br>

In order to use **UniRemoteCYD** to send ESP-NOW commands to your receiver code, your receiver code must run on an ESP-32 that includes WiFi.

**UniRemoteRcvr.cpp** and **UniRemoteRcvr.h** are the pattern for interfacing with **UniRemoteCYD** and receiving the ESP-NOW commands.<br>
In the simplest form, your receiver code ***.ino**  program does the following:

```c
#include "UniRemoteRcvr.h"
static char g_my_message[ESP_NOW_MAX_DATA_LEN];
static uint8_t g_sender_mac_addr[ESP_NOW_ETH_ALEN];
static uint32_t g_my_message_num = 0;

void setup() {
  // init UniRemoteRcvr - inits WiFi mode to WIFI_STA and inits ESP-NOW
  esp_err_t status_init_uni_remote_rcvr = uni_remote_rcvr_init();
  if (status_init_uni_remote_rcvr != ESP_OK) {
    // handle error status
    return;
  }

  // Initialize everything else

} //

void loop() {
  uint16_t rcvd_len = 0; // the length of the message/command. If zero, no message.

  // get any message received. If 0 == rcvd_len, no message.
  esp_err_t msg_status = uni_remote_rcvr_get_msg(&rcvd_len, &g_my_message[0], &g_sender_mac_addr[0], &g_my_message_num);

  // we can get an error even if no message
  if (msg_status != UNI_REMOTE_RCVR_OK) { // (== ESP_OK)
    // handle error status here

    // these error codes come from set/clear flags; clear so can detect next time
    if ((UNI_REMOTE_RCVR_ERR_CBUF_MSG_DROPPED == msg_status) || (UNI_REMOTE_RCVR_ERR_MSG_TOO_BIG == msg_status)) {
      uni_remote_rcvr_clear_extended_status_flags();
    }
  }

  // we can get a message with or without an error; see above uni_remote_rcvr_clear_extended_status_flags()
  // If 0 == rcvd_len, no message.
  if (rcvd_len > 0) {
    // process command
  }

  // do everything else the program does
}
```

**UniRemoteRcvrTemplate.ino** is an example program that illustrates one way to follow this pattern.
