/* Author: https://github.com/Mark-MDO47  Dec. 21, 2024
 *  https://github.com/Mark-MDO47/UniRemote
 *  
 *  This code will scan a command (QR code or RFID or ??) and send it to
 *     the appropriate MAC address using ESP-NOW point-to-point on WiFi.
 *  The command is sent in plain text, not encrypted.
 *
 * ESP-NOW is a protocol designed by Espressif Systems https://www.espressif.com/
 *     This UniRemote code expects to run on particular hardware as described below.
 *     The receiver(s) of the command(s) can be based on just about any ESP32 that includes WiFi.
 *
 * UniRemote runs on the "Cheap Yellow Display" ESP32-2432S028R based on ESP32-D0WDQ6.
 *     I used this one: https://www.aliexpress.us/item/3256805697430313.html
 *
 * The QR code reader used is the Tiny Code Reader from Useful Sensors
 *     I used this one: https://www.sparkfun.com/products/23352
 *     See https://github.com/usefulsensors/tiny_code_reader_arduino.git
 *
 * The RFID reader used is (as far as I can tell) pretty common.
 *     I used this one: https://www.amazon.com/dp/B07VLDSYRW
 *     It is capable of reading and/or writing RFID Smart Cards.
 *     The controller chip could support UART or I2C or SPI interfaces,
 *        but the only interface supported without board modification is the
 *        SPI interface. Since the CYD didn't have enough pins available without board
 *        modification, I am using a "sniffer" that takes the signals from the CYD MicroSD slot.
 *     I used this sniffer: https://www.sparkfun.com/sparkfun-microsd-sniffer.html
 *        See also https://learn.sparkfun.com/tutorials/microsd-sniffer-hookup-guide/introduction
 *     I used these RFID Smart Cards ISO14443A: https://www.amazon.com/dp/B07S63VT7X
 *
 * The scanned command contains the MAC address and the command string. This code will
 *     dynamically register the MAC addresses but ESP_NOW has a limit of 20 MAC
 *     addresses that can be registered at once.
 *   
 *  The scanned command should be a text tab-separated-variable text file of the following form:
 *  <MAC ADDRESS><TAB><COMMAND STRING><TAB><DESCRIPTION STRING>
 *  
 *  <MAC ADDRESS> is a string of the following exact form:
 *      ##:##:##:##:##:##
 *    This is the MAC Address that will be used to send the ESP-NOW message;
 *      the MAC address of the target system.
 *    Note that this is a six-part MAC address in hexadecimal. Each hex number
 *    is exactly two digits long. If you need to start it with a zero, do so.
 *    Because I am a lazy coder, formatting the string properly is up to you.
 *  
 *  <COMMAND STRING> is a short (maximum 249 characters + zero termination) command
 *    The receiving MAC address will receive it as a zero-terminated string (including
 *    the zero terminator).
 *
 * <DESCRIPTION STRING> can be zero length or more, but for consistency
 *    the <TAB> prior to the description string is required.
 *    The description is just for your purposes; it is not sent to the ESP-NOW target.
 *
 * QRcode.py in https://github.com/Mark-MDO47/MDOpythonUtils
 *    will create such a QR code from text input. It requires that you
 *    install the "qrcode" package, using conda or pip or whatever.
 *    I use the command line "python QRcode.py -s intructions.txt"
 *    Each QR code is generated with a short *.html to allow printing.
 *
 * Future plans - perhaps include voice commands and a few buttons.
 */

 // This code was developed after reading the Random Nerd Tutorials below.
 // There are significant differences in this code and the tutorials, but I want to give a
 //    tip of the hat to Rui Santos & Sara Santos for the wonderful work they do.
 // Below is the attribution from the Random Nerd Tutorial on ESP-NOW.
 /*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp-now-esp32-arduino-ide/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/
// Below is the attribution from the Random Nerd Tutorial on LVGL.
/*  Rui Santos & Sara Santos - Random Nerd Tutorials - https://RandomNerdTutorials.com/esp32-lvgl-ebook/
    THIS EXAMPLE WAS TESTED WITH THE FOLLOWING HARDWARE:
    1) ESP32-2432S028R 2.8 inch 240×320 also known as the Cheap Yellow Display (CYD): https://makeradvisor.com/tools/cyd-cheap-yellow-display-esp32-2432s028r/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/cyd-lvgl/
    2) REGULAR ESP32 Dev Board + 2.8 inch 240x320 TFT Display: https://makeradvisor.com/tools/2-8-inch-ili9341-tft-240x320/ and https://makeradvisor.com/tools/esp32-dev-board-wi-fi-bluetooth/
      SET UP INSTRUCTIONS: https://RandomNerdTutorials.com/esp32-tft-lvgl/
    Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
    The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

// Also a giant tip of the hat to the folks at LVGL
//   https://lvgl.io/
//   https://github.com/lvgl/lvgl/tree/master
//   https://docs.lvgl.io/master/examples.html

// and this info on using I2C and SPI with the "Cheap Yellow Display" ESP32-2432S028R GPIO pins
//   https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/discussions/3
//   https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/ADDONS.md#wiring
//   https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/155f70e485994c9423c3af1035bb5ce72899ff01/Examples/InputTests/NunchuckTest/NunchuckTest.ino#L78

// Below is the attribution from the Random Nerd Tutorial on ESP32 with MFRC522 RFID Reader/Writer (Arduino IDE).
/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-mfrc522-rfid-reader-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

// Also a giant tip of the hat to the folks at https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display
//    and https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library and
//    for pioneering the method of using
//    "bit banging" (software-controlled) SPI on the touchscreen and freeing up the hardware VSPI channel
//    for use on the RC522-based RFID reader/writer via the MicroSD Sniffer. See 
//    https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/TROUBLESHOOTING.md#display-touch-and-sd-card-are-not-working-at-the-same-time

// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

/*  Install the "lvgl" library version 9.2 by kisvegabor to interface with the TFT Display - https://lvgl.io/
    *** IMPORTANT: lv_conf.h available on the internet will probably NOT work with the examples available at Random Nerd Tutorials ***
    *** YOU MUST USE THE lv_conf.h FILE PROVIDED IN THE LINK BELOW IN ORDER TO USE THE EXAMPLES FROM RANDOM NERD TUTORIALS ***
    FULL INSTRUCTIONS AVAILABLE ON HOW CONFIGURE THE LIBRARY: https://RandomNerdTutorials.com/cyd-lvgl/ or https://RandomNerdTutorials.com/esp32-tft-lvgl/   */
// https://github.com/lvgl/lvgl.git
#include <lvgl.h>

// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI
// A library for interfacing with the touch screen "bit banging" (software-controlled) SPI
// Can be installed from the library manager (Search for "XPT2046 Slim")
// https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library
#include <XPT2046_Bitbang.h>

// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI
// A library for interfacing with LCD displays
// Can be installed from the library manager (Search for "TFT_eSPI")
// https://github.com/Bodmer/TFT_eSPI
// #include <TFT_eSPI.h>
// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI

#define INCLUDE_RFID_SENSOR 1  // set to 1 to include RFID Sensor scan for commands
#define INCLUDE_QR_SENSOR   1  // set to 1 to include QR Cbode Reader scan for commands

#if INCLUDE_RFID_SENSOR
// A library for interfacing with the RC522-based RFID reader either via I2C or SPI
// Can be installed from the library manager (Search for "MFRC522v2" by GithubCommunity)
// https://github.com/OSSLibraries/Arduino_MFRC522v2
#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(5);

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

MFRC522::MIFARE_Key key;

byte blockAddress = 2;
// byte newBlockData[17] = {"Rui Santos - RNT"};
byte newBlockData[17] = {"_MDO_MDO_MDO_MDO"};
//byte newBlockData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   // CLEAR DATA
byte bufferblocksize = 18;
byte blockDataRead[18];
#endif // INCLUDE_RFID_SENSOR

#include <esp_now.h>   // for ESP-NOW
#include <WiFi.h>      // for ESP-NOW
#include "../wifi_key.h"  // WiFi secrets


#if INCLUDE_QR_SENSOR
  #include <Wire.h>     // for QR sensor (Tiny Code Reader) and anything else
  #include "../tiny_code_reader/tiny_code_reader.h" // see https://github.com/usefulsensors/tiny_code_reader_arduino.git
#endif // INCLUDE_QR_SENSOR

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

// DEBUG definitions
#define DBG_SERIALPRINT Serial.print
#define DBG_SERIALPRINTLN Serial.println
#define DEBUG_PRINT_TOUCHSCREEN_INFO 0

// PIN definitions

// Connector P1 - 
// Connector P3 - GND,35,22,21 (note 21 is shared, don't mess with it)

// Connector CN1 - GND,22,27,3V3
// these definitions have the QR code reader wire colors matching the CYD wire colors
//    yellow = SCL
//    blue   = SDA
#define CYD_CN1_SDA 22 // for "Cheap Yellow Display" ESP32-2432S028R
#define CYD_CN1_SCL 27 // for "Cheap Yellow Display" ESP32-2432S028R

// internal CYD RGB LED Pins
#define CYD_LED_RED    4
#define CYD_LED_GREEN 16
#define CYD_LED_BLUE  17
#define CYD_LED_ON  LOW  // inverted
#define CYD_LED_OFF HIGH // inverted

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// define touchscreen and LVGL definitions

#define USE_LV_TICK_SET_CB 1 // 1 to use lv_tick_set_cb() in setup; 0 to use lv_tick_inc() in loop

// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI
      // SPIClass touchscreenSPI = SPIClass(VSPI);
XPT2046_Bitbang ts(XPT2046_MOSI, XPT2046_MISO, XPT2046_CLK, XPT2046_CS); // create software-controlled SPI for touchscreen
// TFT_eSPI tft = TFT_eSPI();
// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);


#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 320
#define SCREEN_LSCAPE_WIDTH  SCREEN_HEIGHT
#define SCREEN_LSCAPE_HEIGHT SCREEN_WIDTH

// Touchscreen coordinates: (x, y) and pressure (z)
int x, y, z;

#define DRAW_BUF_SIZE (SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8))
uint32_t draw_buf[DRAW_BUF_SIZE / 4];

// coloration
lv_style_t g_style_blue, g_style_yellow, g_style_red, g_style_green, g_style_grey, g_style_ghost;

// size
lv_style_t g_style_screen_width_btn_height;
lv_style_t g_style_screen_width_comm_height;

const int32_t CYDsampleDelayMsec = 5;

// ESP-NOW definitions
static uint8_t g_rcvr_mac_addr[ESP_NOW_ETH_ALEN * ESP_NOW_MAX_TOTAL_PEER_NUM];
static uint8_t g_rcvr_peer_num = 0; // count how many peers we put into our table (20 max)
static uint8_t g_last_scanned_cmd_count = 0; // count how many we scan and possibly process

esp_now_peer_info_t rcvr_peer_info; // will be filled in later

typedef int32_t uni_esp_now_status;
#define UNI_ESP_NOW_CB_NEVER_HAPPENED -1 // my own status
static uni_esp_now_status g_last_send_callback_status = UNI_ESP_NOW_CB_NEVER_HAPPENED; // -1 means never happened

#define UNI_CMD_QNUM_NOW    0 // 0==sending now, 1==next up
#define UNI_CMD_QNUM_NEXT   1 // 0==sending now, 1==next up
#define UNI_CMD_QNUM_NUM    2 // 0==sending now, 1==next up
typedef struct {
  char scanned_cmd[ESP_NOW_MAX_DATA_LEN+2];
  uint16_t scanned_cmd_len;
} uni_cmd_queue_t;
static uni_cmd_queue_t g_cmd_queue[UNI_CMD_QNUM_NUM]; // queue for msgs; 0==sending now, 1==next up

// UNI REMOTE definitions
#define UNI_ESP_NOW_MSEC_PER_MSG_MIN 500 // minimum millisec between sending messages

#define UNI_STATE_WAIT_CMD     0    // last cmd all done, wait for next cmd (any source OK)
#define UNI_STATE_CMD_SEEN      1    // command in queue, waiting for GO or CLEAR
#define UNI_STATE_SENDING_CMD  2    // command being sent (very short state)
#define UNI_STATE_WAIT_CB      3    // waiting for send callback (very short state)
#define UNI_STATE_SHOW_STAT    4    // show error status and allow abort
#define UNI_STATE_NUM    5    // number of states
uint8_t g_uni_state = UNI_STATE_WAIT_CMD;

#define UNI_STATE_NO_ERROR 0  // this UNI_STATE is in error
#define UNI_STATE_IN_ERROR 1  // this UNI_STATE is in error
uint8_t g_uni_state_error = UNI_STATE_NO_ERROR;

// some error codes that can be displayed just as if ESP_ERR_ESPNOW_ code
#define UNI_ERR_TOO_SOON        501 // too soon to send another ESP-NOW message
#define UNI_ERR_CMD_DECODE_FAIL 502 // could not decode MAC from CMD



uint32_t g_uni_state_times[UNI_STATE_NUM];


#define ACTION_BUTTON_LEFT  0   // left top
#define ACTION_BUTTON_MID   1   // middle top
#define ACTION_BUTTON_RIGHT 2   // right top
#define ACTION_BUTTON_NUM 3     // number of action buttons at top of screen
typedef struct {
  lv_obj_t * button;            // object pointer for button itself
  lv_obj_t * button_label;      // for text on the button
  lv_obj_t * button_text_label; // object pointer for external button text
  uint16_t btn_idx;             // index to the action button
} action_button_t;
action_button_t g_action_buttons[ACTION_BUTTON_NUM]; // for three buttons on top screen

typedef struct {
  uint8_t  pressed;             // non-zero for new button press
  uint8_t  btn_idx;             // index to the action button
  uint8_t  uni_state;           // g_uni_state at time of button press
  uint8_t  uni_state_error;     // g_uni_state_error at time of button press
} button_press_t;
button_press_t g_button_press;

typedef struct {
  lv_obj_t * label_obj;
  lv_obj_t * label_text;
} styled_label_t;
styled_label_t g_styled_label_last_status; // 2 lines storage for last error on bottom screen
styled_label_t g_styled_label_opr_comm;    // 5 lines storage for opr communication in middle screen

char g_msg[1025]; // for generating text strings

#define DBG_FAKE_CMD 1 // special fake command button
uint8_t g_do_dbg_fake_cmd = 0;

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_lv_button_text_style - set button texts and style
//    p_btn_idx - within g_action_buttons[]
//    p_label   - label within button
//    p_text    - text under button
//    p_style   - button style to use
//    
void uni_lv_button_text_style(uint8_t p_btn_idx, char * p_label, char * p_text, lv_style_t * p_style) {
  lv_obj_add_style(g_action_buttons[p_btn_idx].button, p_style, 0);
  lv_label_set_text(g_action_buttons[p_btn_idx].button_label, p_label);
  lv_label_set_text(g_action_buttons[p_btn_idx].button_text_label, p_text);
} // end uni_lv_button_text_style()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_wait_new_cmd
//
void uni_alert_4_wait_new_cmd() {
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "LED ON", "Lights on", &g_style_blue);
#if DBG_FAKE_CMD
  uni_lv_button_text_style(ACTION_BUTTON_MID, "Dbg FakeCmd", "Debug Fake", &g_style_grey);
#else // not DBG_FAKE_CMD
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
#endif // not DBG_FAKE_CMD
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "LED OFF", "Lights off", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Scan Command");
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_wait_new_cmd()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_ok_new_cmd
//
void uni_alert_4_ok_new_cmd() {
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_ok_new_cmd()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_wait_send_or_clear_cmd
//
void uni_alert_4_wait_send_or_clear_cmd() {
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "SEND", "Send Command", &g_style_blue);
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "CLEAR", "Clear Command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Scan Command");
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_wait_send_or_clear_cmd()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_invalid_cmd
//
void uni_alert_4_invalid_cmd() {
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_invalid_cmd()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_ignore_cmd
void uni_alert_4_ignore_cmd() {
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_ignore_cmd()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_sending_cmd
//
void uni_alert_4_sending_cmd() {
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "ABORT", "Abort send and\nClear Command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Sending Command\n  please wait...");
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_sending_cmd()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_wait_callback
//
void uni_alert_4_wait_callback() {
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "ABORT", "Abort send and\nClear Command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Waiting callback from Command\n  please wait...");
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_wait_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_alert_4_rcvd_callback
//
void uni_alert_4_rcvd_callback() {
  // callback routine already showed error status
  // TODO sprintf(g_msg, "ESP-NOW ERROR: sending msg %d\n  %s", g_last_scanned_cmd_count, g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd, uni_esp_now_decode_error(g_last_send_callback_status));
  // lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "SEND", "Send again", &g_style_blue);
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "ABORT", "Abort send and\nClear Command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Command Send callback failed\n  SEND again or ABORT this command...");
  if (UNI_STATE_NO_ERROR == g_uni_state_error) {
    // NO ERROR
  } else {
    // with error
  }
} // end uni_alert_4_rcvd_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_display_state
//
void uni_display_state() {
  switch (g_uni_state) {
    case UNI_STATE_WAIT_CMD:    // last cmd all done, wait for next cmd
      uni_alert_4_wait_new_cmd();
      break;
    case UNI_STATE_CMD_SEEN:   // command in queue, waiting for GO or CLEAR
      uni_alert_4_wait_send_or_clear_cmd();
      break;
    case UNI_STATE_SENDING_CMD: // command being sent (very short state)
      uni_alert_4_sending_cmd();
      break;
    case UNI_STATE_WAIT_CB:     // waiting for send callback (very short state)
      uni_alert_4_wait_callback();
      break;
    case UNI_STATE_SHOW_STAT:      // show error status and allow abort
      uni_alert_4_rcvd_callback();
      break;
    default:
      // FIXME TODO should never get here
      break;
  }
} // end uni_display_state()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// static void cyd_input_read(lv_indev_t * indev, lv_indev_data_t * data)
//
#include "cyd_input_read.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// button_event_callback()
//
static void button_event_callback(lv_event_t * e) {
  static uint8_t counter = 0;
  static lv_event_code_t code;
  static lv_obj_t * button;
  static action_button_t * action_button_ptr;

  code = lv_event_get_code(e);
  if ((LV_EVENT_CLICKED == code) && (0 == g_button_press.pressed)) {
    button = (lv_obj_t*) lv_event_get_target(e);
    action_button_ptr = (action_button_t*) lv_event_get_user_data(e); // &action_buttons[idx]
    counter++;
    g_button_press.btn_idx = (uint8_t) action_button_ptr->btn_idx;
    g_button_press.uni_state = g_uni_state;
    g_button_press.uni_state_error = g_uni_state_error;
    g_button_press.pressed = (uint8_t) 1; // handle_button_press() clears it
    // LV_LOG_USER("Counter: %d", counter);
    DBG_SERIALPRINT("button_event_callback() ");
    DBG_SERIALPRINT(g_button_press.btn_idx);
    DBG_SERIALPRINT(" state ");
    DBG_SERIALPRINTLN(g_uni_state);
  }
} // end button_event_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// handle_button_press - 
int32_t handle_button_press() {
  int32_t ret_val = 0;
  g_button_press.pressed = (uint8_t) 0;
  DBG_SERIALPRINTLN("handle_button_press() 0");
  switch (g_button_press.uni_state) {
    case UNI_STATE_WAIT_CMD:    // last cmd all done, wait for next cmd
      if (ACTION_BUTTON_LEFT == g_button_press.btn_idx) {
        // turn on LEDs
        digitalWrite(CYD_LED_RED, CYD_LED_ON);
        digitalWrite(CYD_LED_GREEN, CYD_LED_ON);
        digitalWrite(CYD_LED_BLUE, CYD_LED_ON);
      } else if (ACTION_BUTTON_RIGHT == g_button_press.btn_idx) {
        // turn off LEDs
        digitalWrite(CYD_LED_RED, CYD_LED_OFF);
        digitalWrite(CYD_LED_GREEN, CYD_LED_OFF);
        digitalWrite(CYD_LED_BLUE, CYD_LED_OFF);
#if DBG_FAKE_CMD
      } else if (ACTION_BUTTON_MID == g_button_press.btn_idx) {
        // fake command
        g_do_dbg_fake_cmd = 1;
      }
#else // not DBG_FAKE_CMD
      }
#endif // DBG_FAKE_CMD
      break;
    case UNI_STATE_CMD_SEEN:   // command in queue, waiting for GO or CLEAR
      if (ACTION_BUTTON_LEFT == g_button_press.btn_idx) {
        // send ESP_NOW command
        g_uni_state = UNI_STATE_SENDING_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_STATE_SENDING_CMD");
      } else if (ACTION_BUTTON_RIGHT == g_button_press.btn_idx) {
        // clear ESP_NOW command
        g_uni_state = UNI_STATE_WAIT_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_STATE_WAIT_CMD");
      }
      break;
    case UNI_STATE_SENDING_CMD: // command being sent (very short state)
      break;
    case UNI_STATE_WAIT_CB:     // waiting for send callback (very short state)
      break;
    case UNI_STATE_SHOW_STAT:      // show error status and allow abort
      if (ACTION_BUTTON_LEFT == g_button_press.btn_idx) {
        // send ESP_NOW command
        g_uni_state = UNI_STATE_SENDING_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_STATE_SENDING_CMD");
      } else if (ACTION_BUTTON_RIGHT == g_button_press.btn_idx) {
        // clear ESP_NOW command
        g_uni_state = UNI_STATE_WAIT_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_STATE_WAIT_CMD");
      }
      break;
    default:
      // FIXME TODO should never get here
      break;
  }
  return(ret_val);
} // end handle_button_press()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_lv_button_create()
//    p_btn_idx - within g_action_buttons[]
//    p_align   - alignment (ex: LV_ALIGN_CENTER)
//    p_label   - initial label within button
//    p_text    - initial text under button
//    p_style   - initial button style to use
//    
void uni_lv_button_create(uint8_t p_btn_idx, lv_align_t p_align, char * p_label, char * p_text, lv_style_t * p_style) {
  // Create a Button
  g_action_buttons[p_btn_idx].btn_idx = p_btn_idx;
  g_action_buttons[p_btn_idx].button = lv_button_create(lv_screen_active());    
  lv_obj_set_size(g_action_buttons[p_btn_idx].button, 90, 50); // Set the button size
  lv_obj_align(g_action_buttons[p_btn_idx].button, p_align, 0, 0);

  // Add label object inside the button
  g_action_buttons[p_btn_idx].button_label = lv_label_create(g_action_buttons[p_btn_idx].button);     
  lv_obj_center(g_action_buttons[p_btn_idx].button_label);

  // Add button text object below the button
  g_action_buttons[p_btn_idx].button_text_label = lv_label_create(lv_screen_active());
  lv_obj_align_to(g_action_buttons[p_btn_idx].button_text_label, g_action_buttons[p_btn_idx].button, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

  // Put text and set styles
  uni_lv_button_text_style(p_btn_idx, p_label, p_text, p_style);

  // Set the callback function and set the "data" to the g_action_buttons[] index
  lv_obj_add_event_cb(g_action_buttons[p_btn_idx].button, button_event_callback, LV_EVENT_ALL, (void*)&g_action_buttons[p_btn_idx]);  // Assign a callback to the button
} // end uni_lv_button_create()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_lv_style_init - initialize all the button styles
//
void uni_lv_style_init() {
  lv_style_init(&g_style_blue);
  lv_style_set_bg_color(&g_style_blue, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
  lv_style_set_text_color(&g_style_blue, lv_palette_darken(LV_PALETTE_GREY, 4));

  lv_style_init(&g_style_yellow);
  lv_style_set_bg_color(&g_style_yellow, lv_palette_main(LV_PALETTE_YELLOW));
  lv_style_set_text_color(&g_style_yellow, lv_palette_darken(LV_PALETTE_GREY, 4));

  lv_style_init(&g_style_red);
  lv_style_set_bg_color(&g_style_red, lv_palette_main(LV_PALETTE_RED));
  lv_style_set_text_color(&g_style_red, lv_palette_darken(LV_PALETTE_GREY, 4));

  lv_style_init(&g_style_green);
  lv_style_set_bg_color(&g_style_green, lv_palette_main(LV_PALETTE_GREEN));
  lv_style_set_text_color(&g_style_green, lv_palette_darken(LV_PALETTE_GREY, 4));

  lv_style_init(&g_style_grey);
  lv_style_set_bg_color(&g_style_grey,  lv_palette_lighten(LV_PALETTE_GREY, 3));
  lv_style_set_text_color(&g_style_grey, lv_palette_darken(LV_PALETTE_GREY, 4));

  lv_style_init(&g_style_ghost);
  lv_style_set_bg_color(&g_style_ghost,  lv_palette_lighten(LV_PALETTE_GREY, 3));
  lv_style_set_text_color(&g_style_ghost, lv_palette_lighten(LV_PALETTE_GREY, 3));

  lv_style_init(&g_style_screen_width_btn_height);
  lv_style_set_width(&g_style_screen_width_btn_height, SCREEN_LSCAPE_WIDTH);
  lv_style_set_height(&g_style_screen_width_btn_height, 50);

  lv_style_init(&g_style_screen_width_comm_height);
  lv_style_set_width(&g_style_screen_width_comm_height, SCREEN_LSCAPE_WIDTH);
  lv_style_set_height(&g_style_screen_width_comm_height, 80);
} // end uni_lv_style_init()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
void lv_create_main_gui(void) {
  // Initialize the styles
  uni_lv_style_init();
  // Create the Buttons
  uni_lv_button_create(0, LV_ALIGN_TOP_LEFT,  "1234567890\n12345678901\n1234", "Some Text 1\nMore and\n   ... more", &g_style_blue);
  uni_lv_button_create(1, LV_ALIGN_TOP_MID,   "2 Label", "Some Text 2\nMore and\n   ... more", &g_style_yellow);
  uni_lv_button_create(2, LV_ALIGN_TOP_RIGHT, "3 Label", "Some Text 3\nMore and\n   ... more", &g_style_red);

  g_styled_label_last_status.label_obj = lv_obj_create(lv_screen_active());
  lv_obj_add_style(g_styled_label_last_status.label_obj, &g_style_green, 0);
  lv_obj_add_style(g_styled_label_last_status.label_obj, &g_style_screen_width_btn_height, 0);
  lv_obj_align(g_styled_label_last_status.label_obj, LV_ALIGN_BOTTOM_LEFT, 0, 0);
  g_styled_label_last_status.label_text = lv_label_create(g_styled_label_last_status.label_obj);
  lv_obj_align_to(g_styled_label_last_status.label_text, g_styled_label_last_status.label_obj, LV_ALIGN_TOP_LEFT, 0, -8);
  lv_label_set_text(g_styled_label_last_status.label_text, "Waiting for\nCommand scan");

  g_styled_label_opr_comm.label_obj = lv_obj_create(lv_screen_active());
  lv_obj_add_style(g_styled_label_opr_comm.label_obj, &g_style_grey, 0);
  lv_obj_add_style(g_styled_label_opr_comm.label_obj, &g_style_screen_width_comm_height, 0);
  lv_obj_align_to(g_styled_label_opr_comm.label_obj, g_styled_label_last_status.label_obj, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
  g_styled_label_opr_comm.label_text = lv_label_create(g_styled_label_opr_comm.label_obj);
  lv_obj_align_to(g_styled_label_opr_comm.label_text, g_styled_label_opr_comm.label_obj, LV_ALIGN_TOP_LEFT, 0, -14);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "No Instructions Yet\nNext 2\nNext  3\nNext   4\nNext    5");
} // end lv_create_main_gui()


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_cmd_decode_get_mac_addr()
//       returns: (uint8_t *) pointer to MAC address; pointer is zero if bad decode
//
// p_cmd is string starting with MAC address; something like this
// 74:4d:bd:11:22:33|the-rest-is-the-command
// 000000000011111111
// 012345678901234567
//
static uint8_t cmd_decode_mac_addr[ESP_NOW_ETH_ALEN];
uint8_t * uni_cmd_decode_get_mac_addr(char * p_cmd) {
  uint8_t * ret_addr = cmd_decode_mac_addr;
  uint8_t tmp;

  // make sure MAC address is of the correct form and decode piece by piece
  if ((3*ESP_NOW_ETH_ALEN+1) > strlen(p_cmd)) {
    ret_addr = ((uint8_t *) 0);
  } else { // at least one character after MAC address
    for (int i = 0; i < 3*ESP_NOW_ETH_ALEN; i += 3) {
      if (!isHexadecimalDigit(p_cmd[i]) || !isHexadecimalDigit(p_cmd[i+1])) {
        ret_addr = ((uint8_t *) 0);
        break;
      }
      tmp = 0;
      if ( ((3*(ESP_NOW_ETH_ALEN-1) != i) && (':' != p_cmd[i+2])) ||
           ((3*(ESP_NOW_ETH_ALEN-1) == i) && ('|' != p_cmd[i+2])) ) {
        ret_addr = ((uint8_t *) 0);
        break;
      }
      // these two hex digits are good
      for (int j = 0; j < 2; j += 1) {
        tmp <<= 4;
        if      (p_cmd[i+j] <= '9') tmp |= p_cmd[i+j] - '0';
        else if (p_cmd[i+j] <= 'F') tmp |= p_cmd[i+j] - 'A' + 10;
        else                          tmp |= p_cmd[i+j] - 'a' + 10;
      }
      ret_addr[i/3] = tmp;
    } // end check MAC address
  }

  // if ret_addr is not 0 then the address is good
  return(ret_addr);
} // uni_cmd_decode_get_mac_addr

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_esp_now_decode_error() - return string with ESP-NOW error
//
char * uni_esp_now_decode_error(uint16_t errcode) {
  char * str;
  switch (errcode)
  {
    case ESP_ERR_ESPNOW_NOT_INIT:
      str = " ESPNOW is not initialized";
      break;
    case ESP_ERR_ESPNOW_ARG:
      str = " ESPNOW Invalid argument";
      break;
    case ESP_ERR_ESPNOW_NO_MEM:
      str = " ESPNOW Out of memory";
      break;
    case ESP_ERR_ESPNOW_FULL:
      str = " ESPNOW peer list is full";
      break;
    case ESP_ERR_ESPNOW_NOT_FOUND:
      str = " ESPNOW peer is not found";
      break;
    case ESP_ERR_ESPNOW_INTERNAL:
      str = " ESPNOW Internal error";
      break;
    case ESP_ERR_ESPNOW_EXIST:
      str = " ESPNOW peer has existed";
      break;
    case ESP_ERR_ESPNOW_IF:
      str = " ESPNOW Interface error";
      break;
    case ESP_OK:
      str = " ESPNOW call OK";
      break;
    // my own internal error codes
    case UNI_ERR_TOO_SOON:
      str = " too soon to send ESP-NOW message";
      break;
    case UNI_ERR_CMD_DECODE_FAIL:
      str = " could not decode MAC from CMD";
      break;
    default:
      str = " ESPNOW UNKNOWN ERROR CODE";
  }
  return(str);
} // end uni_esp_now_decode_error()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_esp_now_cmd_send_callback() - ESP-NOW sending callback function
//       returns: nothing
//
void uni_esp_now_cmd_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // crude way to save last status - works OK if not sending too fast
  g_last_send_callback_status = (uni_esp_now_status)status;

  // display status and transition states
  if (ESP_NOW_SEND_SUCCESS == g_last_send_callback_status) {
    sprintf(g_msg, "ESP-Now callback OK CMD #%d", g_last_scanned_cmd_count);
    lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
    g_uni_state_error = UNI_STATE_NO_ERROR;
    g_uni_state = UNI_STATE_WAIT_CMD;
  } else { // ESP_NOW_SEND_FAIL
    sprintf(g_msg, "ESP-Now callback FAIL CMD #%d", g_last_scanned_cmd_count);
    lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
    g_uni_state_error = UNI_STATE_IN_ERROR;
    g_uni_state = UNI_STATE_SHOW_STAT; // show error status and allow abort
  }
} // end uni_esp_now_cmd_send_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_esp_now_register_peer() - 
//       returns: index to peer or -1 for failure
int16_t uni_esp_now_register_peer(uint8_t * mac_addr) {
  
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
} // end uni_esp_now_register_peer

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_esp_now_cmd_send() - decipher QR code and send as needed
//       returns: status from call
//   sends a string up to length ESP_NOW_MAX_DATA_LEN; includes the zero termination of the string   
//
// FIXME TODO this is only for initial testing
// FIXME TODO WARNING this can modify p_cmd
//
esp_err_t uni_esp_now_cmd_send(char * p_cmd) {
  esp_err_t send_status = ESP_OK;
  static char esp_now_msg_data[ESP_NOW_MAX_DATA_LEN+1];
  static uint32_t msec_prev_send = 0;
  uint32_t msec_now = millis();

  // see if waited long enough to send another ESP-NOW message
  if (msec_now < (UNI_ESP_NOW_MSEC_PER_MSG_MIN+msec_prev_send)) {
    DBG_SERIALPRINTLN("ERROR: too soon to send");
    return(UNI_ERR_TOO_SOON); // too soon to send another message
  }
  msec_prev_send = msec_now;

  // see if we can obtain and register the MAC address for sending
  uint8_t * mac_addr_ptr = uni_cmd_decode_get_mac_addr(p_cmd);
  int16_t mac_addr_index;
  if ((uint8_t *)0 != mac_addr_ptr) {
    mac_addr_index = uni_esp_now_register_peer(mac_addr_ptr);
  } else {
    DBG_SERIALPRINTLN("ERROR: uni_cmd_decode_get_mac_addr returned 0");
    return(UNI_ERR_CMD_DECODE_FAIL); // could not decode MAC from CMD
  }
  if (mac_addr_index < 0) {
    DBG_SERIALPRINTLN("ERROR: could not register MAC");
    return(ESP_ERR_ESPNOW_FULL); // could not register the MAC address
  }

  // copy message over starting after the MAC address
  memset(esp_now_msg_data, '\0', sizeof(esp_now_msg_data));
  strncpy(esp_now_msg_data, &p_cmd[3*ESP_NOW_ETH_ALEN], ESP_NOW_MAX_DATA_LEN-1); // max ESP-NOW msg size
  int len = strlen(esp_now_msg_data)+1; // length to send
  send_status = esp_now_send(mac_addr_ptr, (uint8_t *) esp_now_msg_data, strlen(esp_now_msg_data)+1);
  return (send_status);
} // end uni_esp_now_cmd_send()

#if INCLUDE_RFID_SENSOR
/////////////////////////////////////////////////////////////////////////////////////////////////////////
// read_PICC() - get next PICC command - DEBUGGING just detect, read, write, read
//   PICC = Proximity Integrated Circuit Card (Contactless Card) - the RFID card we are reading
//      At this time we plan to use the MIFARE Classic EV1 1K
//   Currently just puts info into serial printout
//   Eventually would read command from PICC and return command as zero-terminated ASCII string
char * read_PICC() {
  static uint32_t msec_prev = 0;
  static uint32_t msec_waitfor = 0;
  uint32_t msec_now = millis();
  char PICC_cmd[1025];
  
  // make command zero length
  PICC_cmd[0] = '\0';

  // don't do anything until next waitfor time
  if (msec_now < msec_waitfor)
    return(PICC_cmd);

  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    msec_waitfor = msec_now + 500;
    return(PICC_cmd);
  }

  // Display card UID
  Serial.print("----------------\nCard UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  // Authenticate the specified block using KEY_A == command 0x60
  if (mfrc522.PCD_Authenticate(MFRC522Constants::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid)) != 0) {
    Serial.println("Authentication failed.");
    return(PICC_cmd);
  }

  // Read data from the specified block
  if (mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferblocksize) != 0) {
    Serial.println("Read BEFORE failed.");
  } else {
    Serial.println("Read BEFORE successful!");
    Serial.print("Data BEFORE in block ");
    Serial.print(blockAddress);
    Serial.print(": ");
    for (byte i = 0; i < 16; i++) {
      Serial.print((char)blockDataRead[i]);  // Print as character
    }
    Serial.println();
  }
  // Authenticate the specified block using KEY_A = command 0x60
  if (mfrc522.PCD_Authenticate(MFRC522Constants::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid)) != MFRC522Constants::StatusCode::STATUS_OK) {
    Serial.println("Authentication failed.");
    return(PICC_cmd);
  }
  
  // Write data to the specified block
  if (mfrc522.MIFARE_Write(blockAddress, newBlockData, 16) != 0) {
    Serial.println("Write failed.");
  } else {
    Serial.print("Data written successfully in block: ");
    Serial.println(blockAddress);
  }

  // Authenticate the specified block using KEY_A = command 0x60
  if (mfrc522.PCD_Authenticate(MFRC522Constants::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid)) != MFRC522Constants::StatusCode::STATUS_OK) {
    Serial.println("Authentication failed.");
    return(PICC_cmd);
  }

  // Read data from the specified block
  if (mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferblocksize) != MFRC522Constants::StatusCode::STATUS_OK) {
    Serial.println("Read AFTER failed.");
  } else {
    Serial.println("Read AFTER successful!");
    Serial.print("Data AFTER in block ");
    Serial.print(blockAddress);
    Serial.print(": ");
    for (byte i = 0; i < 16; i++) {
      Serial.print((char)blockDataRead[i]);  // Print as character
    }
    Serial.println();
  }
  
  // Halt communication with the card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  msec_waitfor = msec_now + 2000; // Delay for readability
  return(PICC_cmd);
} // end read_PICC()
#endif // INCLUDE_RFID_SENSOR

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_get_command() - get next scanned command
//     p_msec_now is time stamp for start of process
// returns 0 if no command scanned and 1 if a command was scanned
// command would be copied into g_cmd_queue[UNI_CMD_QNUM_NOW]
//
uint16_t uni_get_command(uint32_t p_msec_now) {
  static uint8_t first_time = 0;      // 0 on first time through uni_get_command()
  static uint32_t next_rfid_msec = 0; // p_msec_now must be >= this to do another RFID action

  uint16_t num_cmds_scanned = 0;

  static char * fake_cmd[] = { 
    "74:4d:bd:11:11:11|dbg non-existing",
    "74:4d:bd:11:11:1|dbg address too short",
    "74:4d:bd:11:11:11c|dbg address too long",
    "74:4d:bd:98:7f:1c|dbg XIAO ESP32-Sense"
   };
#define FAKE_CMD_NUM 4
   static uint8_t fake_cmd_idx = 0;
   static uint8_t tmp;

  if (0 == first_time) { DBG_SERIALPRINTLN("first_time fake command code"); }
  if (0 != g_do_dbg_fake_cmd) {
    // do next fake command
    DBG_SERIALPRINTLN("Doing fake command");
    num_cmds_scanned = 1;
    g_do_dbg_fake_cmd = 0;
    g_last_scanned_cmd_count += 1;
    g_uni_state_times[g_uni_state] = p_msec_now;
    strncpy(g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd, fake_cmd[fake_cmd_idx], sizeof(g_cmd_queue[0].scanned_cmd));
    tmp = fake_cmd_idx + 1;
    if (tmp < FAKE_CMD_NUM) {
      fake_cmd_idx = tmp;
    } else { // not really required here but this way fake_cmd_idx is NEVER out of bounds
      fake_cmd_idx = 0;
    }
    g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd_len = strlen(g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
    sprintf(g_msg, "DBG Fake CMD #%d:\n %s", g_last_scanned_cmd_count, g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
  }
#if INCLUDE_RFID_SENSOR
  if (0 == first_time) { DBG_SERIALPRINTLN("first_time RFID code"); }
  if ((0 == num_cmds_scanned) && (next_rfid_msec <= p_msec_now)) {
    // try RFID scanner
    read_PICC();
/*  FIXME TODO WILL NEED SOMETHING SIMILAR TO THIS
    if (QRresults.content_length > 0) {
      DBG_SERIALPRINTLN("Doing QR Code");
      num_cmds_scanned = 1;
      g_last_scanned_cmd_count += 1;
      g_uni_state_times[g_uni_state] = p_msec_now;
      strncpy(g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd, (char *)QRresults.content_bytes, sizeof(g_cmd_queue[0].scanned_cmd));
      g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd_len = strlen(g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
      sprintf(g_msg, "QR CMD #%d scanned:\n %s", g_last_scanned_cmd_count, g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
    FIXME TODO WILL NEED SOMETHING SIMILAR TO THIS
 */
  } // end if got PICC result
#endif // INCLUDE_RFID_SENSOR
#if INCLUDE_QR_SENSOR
  static tiny_code_reader_results_t QRresults = {};
  if (0 == first_time) { DBG_SERIALPRINTLN("first_time QR code"); }
  if (0 == num_cmds_scanned) {
    // try QR code reader
    if (!tiny_code_reader_read(&QRresults)) { // Perform a read action on the I2C address of the sensor
      lv_label_set_text(g_styled_label_last_status.label_text, "I2C bus QR code sensor no response");
    } else if (QRresults.content_length > 0) {
      DBG_SERIALPRINTLN("Doing QR Code");
      num_cmds_scanned = 1;
      g_last_scanned_cmd_count += 1;
      g_uni_state_times[g_uni_state] = p_msec_now;
      strncpy(g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd, (char *)QRresults.content_bytes, sizeof(g_cmd_queue[0].scanned_cmd));
      g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd_len = strlen(g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
      sprintf(g_msg, "QR CMD #%d scanned:\n %s", g_last_scanned_cmd_count, g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
    }
    QRresults.content_length = 0;
  }
#endif // INCLUDE_QR_SENSOR

  if (0 != num_cmds_scanned) {
    // Show new status and change state
    lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
    g_uni_state = UNI_STATE_CMD_SEEN;
    DBG_SERIALPRINTLN("Change state to UNI_STATE_CMD_SEEN");
  }

  first_time = 1;
  return(num_cmds_scanned);
} // end uni_get_command()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_tick() - use Arduinos millis() as tick source for LVGL
static uint32_t uni_tick(void)
{
    return millis();
} // end uni_tick()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup() - initialize hardware and software
//       returns: nothing
//   Serial - only if serial debugging enabled
//   Wire   - for I2C to QR code sensor (and others...)
//   WiFi   - for ESP-NOW communications
//
void setup() {
  // turn off LEDs
  pinMode(CYD_LED_RED, OUTPUT);
  digitalWrite(CYD_LED_RED, CYD_LED_OFF);
  pinMode(CYD_LED_GREEN, OUTPUT);
  digitalWrite(CYD_LED_GREEN, CYD_LED_OFF);
  pinMode(CYD_LED_BLUE, OUTPUT);
  digitalWrite(CYD_LED_BLUE, CYD_LED_OFF);

  Serial.begin(115200); // basically for debugging...
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("\nStarting UniRemote\n");

#if INCLUDE_QR_SENSOR
  Wire.begin(CYD_CN1_SDA, CYD_CN1_SCL); // for the QR code sensor
#endif // INCLUDE_QR_SENSOR

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
  esp_err_t status_register_send_cb = esp_now_register_send_cb(uni_esp_now_cmd_send_callback);
  if (status_register_send_cb != ESP_OK){
    DBG_SERIALPRINT("ERROR: ESP-NOW register send callback error ");
    DBG_SERIALPRINTLN(status_register_send_cb);
    return;
  }

#if INCLUDE_RFID_SENSOR
  // init RFID sensor
  mfrc522.PCD_Init();    // Init MFRC522 board.
  // Prepare key - all keys are set to FFFFFFFFFFFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
#endif // INCLUDE_RFID_SENSOR

  // Start LVGL
  lv_init();

#if USE_LV_TICK_SET_CB // if using lv_tick_set_cb()
  // Set a tick source so that LVGL can calculate how much time elapsed.
  lv_tick_set_cb(uni_tick);
#endif // end if using lv_tick_set_cb(); otherwise lv_tick_inc() done at end of loop()


// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI
  // Start the SPI for the touchscreen and init the touchscreen
      // touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
      // touchscreen.begin(touchscreenSPI);
      // Set the Touchscreen rotation in landscape mode
      // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
      // touchscreen.setRotation(2);
// NOTE WE ARE USING XPT2046_Bitbang INSTEAD OF TFT_eSPI
  // Start the SPI for the touch screen and init the TS library
  ts.begin();
  //    I have not yet done the experiment of changing rotation with XPT2046_Bitbang
  //ts.setRotation(1);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_90); // landscape
  
  // Initialize an LVGL input device object (Touchscreen)
  lv_indev_t * indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, cyd_input_read);

  // Function to draw the GUI
  lv_create_main_gui();
} // end setup()


/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop() - handles the loop
//       returns: nothing
//  if command (QR code or RFID) seen
//    send to ESP-NOW destination
//  give a slight delay
//
void loop() {
  uint32_t msec_now = millis();
  esp_err_t send_status;

  if (0 != g_button_press.pressed) { handle_button_press(); }
  else switch (g_uni_state) {
    case UNI_STATE_WAIT_CMD:    // last cmd all done, wait for next cmd
      if (0 == uni_get_command(msec_now)) {
        lv_label_set_text(g_styled_label_last_status.label_text, "No scanned command found, waiting...\n");
      }
      break;
    case UNI_STATE_CMD_SEEN:   // command in queue, waiting for GO or CLEAR
      break;
    case UNI_STATE_SENDING_CMD: // command being sent (very short state)
      send_status = uni_esp_now_cmd_send((char *)g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
      if (send_status == ESP_OK) {
        sprintf(g_msg, "ESP-NOW send success CMD #%d %s", g_last_scanned_cmd_count, g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd);
        lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
        g_uni_state = UNI_STATE_WAIT_CB;
        DBG_SERIALPRINTLN("Change state to UNI_STATE_WAIT_CB");
      }
      else {
        sprintf(g_msg, "ESP-NOW ERROR: sending CMD #%d: %s\n  %s", g_last_scanned_cmd_count, g_cmd_queue[UNI_CMD_QNUM_NOW].scanned_cmd, uni_esp_now_decode_error(send_status));
        lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
        g_uni_state = UNI_STATE_SHOW_STAT; // show error status and allow abort
        DBG_SERIALPRINTLN("Change state to UNI_STATE_WAIT_CMD");
      }
      break;
    case UNI_STATE_WAIT_CB:     // waiting for send callback
      break;
    case UNI_STATE_SHOW_STAT:   // show error status and allow abort
      break;
    default:
      // FIXME TODO should never get here
      break;
  }
  uni_display_state();

  lv_task_handler();  // let the GUI do its work

#if (1-USE_LV_TICK_SET_CB) // if not using lv_tick_set_cb()
  // not using lv_tick_set_cb(); do with manual estimate
  lv_tick_inc(CYDsampleDelayMsec); // tell LVGL how much time has passed
#endif // (USE_LV_TICK_SET_CB) end if not using lv_tick_set_cb(); otherwise lv_tick_set_cb() done in setup() after lv_init()
  delay(CYDsampleDelayMsec);
  // delay(QRsampleDelayMsec); FIXME TODO wait between intervals
} // end loop()
