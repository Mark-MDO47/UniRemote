/* Author: https://github.com/Mark-MDO47  Dec. 21, 2023
 *  https://github.com/Mark-MDO47/UniRemote
 *  
 *  This code will read a command from a QR code and send it to
 *     the appropriate MAC address using ESP-NOW point-to-point on WiFi.
 *     It is sent in plain text, not encrypted.
 *
 *  The QR code reader for this code is Tiny Code Reader from Useful Sensors
 *     see https://github.com/usefulsensors/tiny_code_reader_arduino.git
 *
 *  Uses the "Cheap Yellow Display" ESP32-2432S028R. A little fun getting two
 *     GPIO pins to use the QR code reader.
 *
 * ESP-NOW is a protocol designed by Espressif Systems https://www.espressif.com/
 *     This code should run on just about any ESP32 device that includes WiFi. 
 *
 * The QR code contains the MAC address and the command string. This code will
 *     dynamically register the MAC addresses but ESP_NOW has a limit of 20 MAC
 *     addresses that can be registered at once.
 *   
 *  The QR code should be a text tab-separated-variable text file of the following form:
 *  <MAC ADDRESS><TAB><COMMAND STRING><TAB><DESCRIPTION STRING>
 *  
 *  <MAC ADDRESS> is a string of the following exact form:
 *      ##:##:##:##:##:##
 *    This is the MAC Address that will be used to send the ESP-NOW message;
 *      the MAC address of the target system.
 *    Note that this is a six-part MAC address in hexadecimal. Each hex number
 *    is exactly two digits long. If you need to start it with a zero, do so.
 *    Because I am a lazy coder.
 *  
 *  <COMMAND STRING> is a short (maximum 249 characters + zero termination) command
 *    The receiving MAC address will receive it as a zero-terminated string (including
 *    the zero terminator).
 *
 * <DESCRIPTION STRING> can be zero length or more, but for QR code consistency
 *    checking the <TAB> prior to the string is required. The description is
 *    just for your purposes; it is not sent to the ESP-NOW target.
 *
 *    QRcode.py in https://github.com/Mark-MDO47/MDOpythonUtils
 *     will create such a QR code from text input. It requires that you
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

// and this info on using I2C with the "Cheap Yellow Display" ESP32-2432S028R GPIO pins
//   https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/discussions/3
//   https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/main/ADDONS.md#wiring
//   https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display/blob/155f70e485994c9423c3af1035bb5ce72899ff01/Examples/InputTests/NunchuckTest/NunchuckTest.ino#L78


#include <esp_now.h>   // for ESP-NOW
#include <WiFi.h>      // for ESP-NOW
#include "../wifi_key.h"  // WiFi secrets

#include <Wire.h>     // for QR sensor (Tiny Code Reader) and anything else

#include "../tiny_code_reader/tiny_code_reader.h" // see https://github.com/usefulsensors/tiny_code_reader_arduino.git

#include <lvgl.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>

// PIN definitions

// Install the "XPT2046_Touchscreen" library by Paul Stoffregen to use the Touchscreen - https://github.com/PaulStoffregen/XPT2046_Touchscreen - Note: this library doesn't require further configuration
#include <XPT2046_Touchscreen.h>

// Touchscreen pins
#define XPT2046_IRQ 36   // T_IRQ
#define XPT2046_MOSI 32  // T_DIN
#define XPT2046_MISO 39  // T_OUT
#define XPT2046_CLK 25   // T_CLK
#define XPT2046_CS 33    // T_CS

// these definitions have the QR code reader wire colors matching the CYD wire colors
//    yellow = SCL
//    blue   = SDA
#define CYD_SDA 22 // for "Cheap Yellow Display" ESP32-2432S028R
#define CYD_SCL 27 // for "Cheap Yellow Display" ESP32-2432S028R

// define touchscreen and LVGL definitions

SPIClass touchscreenSPI = SPIClass(VSPI);
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

// DEBUG definitions

#define DEBUG_SERIALPRINT 1 // print messages
#if DEBUG_SERIALPRINT
  #define DBG_SERIALPRINT   Serial.print
  #define DBG_SERIALPRINTLN Serial.println
#else  // not DEBUG_SERIALPRINT
#endif // DEBUG_SERIALPRINT

#define DEBUG_QR_INPUT 1 // set 1 to get debug messages from QR code sensor

// ESP-NOW definitions
static uint8_t g_rcvr_mac_addr[ESP_NOW_ETH_ALEN * ESP_NOW_MAX_TOTAL_PEER_NUM];
static uint8_t g_rcvr_peer_num = 0; // count how many peers we put into our table (20 max)
static uint8_t g_last_scanned_cmd_count = 0; // count how many we scan and possibly process

esp_now_peer_info_t rcvr_peer_info; // will be filled in later

typedef int32_t uni_esp_now_status;
#define UNI_ESP_NOW_CB_NEVER_HAPPENED -1 // my own status
static uni_esp_now_status g_last_send_callback_status = UNI_ESP_NOW_CB_NEVER_HAPPENED; // -1 means never happened

#define UNI_QR_CODE_QNUM_NOW    0 // 0==sending now, 1==next up
#define UNI_QR_CODE_QNUM_NEXT   1 // 0==sending now, 1==next up
#define UNI_QR_CODE_QNUM_NUM    2 // 0==sending now, 1==next up
typedef struct {
  char qr_msg[ESP_NOW_MAX_DATA_LEN+2];
  uint16_t qr_msg_len;
} uni_qr_code_queue_t;
static uni_qr_code_queue_t g_qr_code_queue[UNI_QR_CODE_QNUM_NUM]; // queue for msgs; 0==sending now, 1==next up

// UNI REMOTE definitions
#define UNI_ESP_NOW_MSEC_PER_MSG_MIN 500 // minimum millisec between sending messages

#define UNI_WAIT_CMD     0    // last cmd all done, wait for next cmd (probably QR but any source OK)
#define UNI_QR_SEEN      1    // command in queue, waiting for GO or CLEAR
#define UNI_SENDING_CMD  2    // command being sent (very short state)
#define UNI_WAIT_CB      3    // waiting for send callback (very short state)
#define UNI_RCVD_CB      4    // Received CB; show status
#define UNI_STATE_NUM    5    // number of states
uint8_t g_uni_state = UNI_WAIT_CMD;

#define UNI_STATE_NO_ERROR 0  // this UNI_STATE is in error
#define UNI_STATE_IN_ERROR 1  // this UNI_STATE is in error
uint8_t g_uni_state_error = UNI_STATE_NO_ERROR;

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

#define DBG_FAKE_QR 1 // special fake QR button
uint8_t g_do_dbg_fake_qr = 0;

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
#if DBG_FAKE_QR
  uni_lv_button_text_style(ACTION_BUTTON_MID, "Dbg FakeQR", "Debug Fake", &g_style_grey);
#else // not DBG_FAKE_QR
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
#endif // not DBG_FAKE_QR
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "LED OFF", "Lights off", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Scan QR Code");
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
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "SEND", "Send QR command", &g_style_blue);
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "CLEAR", "Clear QR command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Scan QR Code");
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
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "ABORT", "Abort send and\nClear QR command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Sending QR Command\n  please wait...");
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
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "ABORT", "Abort send and\nClear QR command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "Waiting callback from QR Command\n  please wait...");
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
  // TODO sprintf(g_msg, "ESP-NOW ERROR: sending msg %d\n  %s", g_last_scanned_cmd_count, g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg, uni_esp_now_decode_error(g_last_send_callback_status));
  // lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
  uni_lv_button_text_style(ACTION_BUTTON_LEFT, "SEND", "Send again", &g_style_blue);
  uni_lv_button_text_style(ACTION_BUTTON_MID, "", "", &g_style_ghost);
  uni_lv_button_text_style(ACTION_BUTTON_RIGHT, "ABORT", "Abort send and\nClear QR command", &g_style_red);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "QR Send callback failed\n  SEND again or ABORT this command...");
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
    case UNI_WAIT_CMD:    // last cmd all done, wait for next cmd (probably QR but any source OK)
      uni_alert_4_wait_new_cmd();
      break;
    case UNI_QR_SEEN:   // command in queue, waiting for GO or CLEAR
      uni_alert_4_wait_send_or_clear_cmd();
      break;
    case UNI_SENDING_CMD: // command being sent (very short state)
      uni_alert_4_sending_cmd();
      break;
    case UNI_WAIT_CB:     // waiting for send callback (very short state)
      uni_alert_4_wait_callback();
      break;
    case UNI_RCVD_CB:      // got callback; show status (show error)
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
    case UNI_WAIT_CMD:    // last cmd all done, wait for next cmd (probably QR but any source OK)
      if (ACTION_BUTTON_LEFT == g_button_press.btn_idx) {
        // turn on LEDs
      } else if (ACTION_BUTTON_RIGHT == g_button_press.btn_idx) {
        // turn off LEDs
#if DBG_FAKE_QR
      } else if (ACTION_BUTTON_MID == g_button_press.btn_idx) {
        // fake QR
        g_do_dbg_fake_qr = 1;
      }
#else // not DBG_FAKE_QR
      }
#endif // DBG_FAKE_QR
      break;
    case UNI_QR_SEEN:   // command in queue, waiting for GO or CLEAR
      if (ACTION_BUTTON_LEFT == g_button_press.btn_idx) {
        // send ESP_NOW command
        g_uni_state = UNI_SENDING_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_SENDING_CMD");
      } else if (ACTION_BUTTON_RIGHT == g_button_press.btn_idx) {
        // clear ESP_NOW command
        g_uni_state = UNI_WAIT_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_WAIT_CMD");
      }
      break;
    case UNI_SENDING_CMD: // command being sent (very short state)
      break;
    case UNI_WAIT_CB:     // waiting for send callback (very short state)
      break;
    case UNI_RCVD_CB:      // got callback; show status (displaying status)
      if (ACTION_BUTTON_LEFT == g_button_press.btn_idx) {
        // send ESP_NOW command
        g_uni_state = UNI_SENDING_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_SENDING_CMD");
      } else if (ACTION_BUTTON_RIGHT == g_button_press.btn_idx) {
        // clear ESP_NOW command
        g_uni_state = UNI_WAIT_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_WAIT_CMD");
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
  lv_label_set_text(g_styled_label_last_status.label_text, "Waiting for\nQR Code Command");

  g_styled_label_opr_comm.label_obj = lv_obj_create(lv_screen_active());
  lv_obj_add_style(g_styled_label_opr_comm.label_obj, &g_style_grey, 0);
  lv_obj_add_style(g_styled_label_opr_comm.label_obj, &g_style_screen_width_comm_height, 0);
  lv_obj_align_to(g_styled_label_opr_comm.label_obj, g_styled_label_last_status.label_obj, LV_ALIGN_OUT_TOP_LEFT, 0, 0);
  g_styled_label_opr_comm.label_text = lv_label_create(g_styled_label_opr_comm.label_obj);
  lv_obj_align_to(g_styled_label_opr_comm.label_text, g_styled_label_opr_comm.label_obj, LV_ALIGN_TOP_LEFT, 0, -14);
  lv_label_set_text(g_styled_label_opr_comm.label_text, "No Instructions Yet\nNext 2\nNext  3\nNext   4\nNext    5");
} // end lv_create_main_gui()


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
    case ESP_NOW_SEND_SUCCESS:
      str = " ESPNOW Send Callback Good";
      break;
    case ESP_NOW_SEND_FAIL:
      str = " ESPNOW Send Callback FAIL";
      break;
    default:
      str = " ESPNOW UNKNOWN ERROR CODE";
  }
  return(str);
} // end uni_esp_now_decode_error()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_esp_now_msg_send_callback() - ESP-NOW sending callback function
//       returns: nothing
//
void uni_esp_now_msg_send_callback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // crude way to save last status - works OK if not sending too fast
  g_last_send_callback_status = (uni_esp_now_status)status;

  // display status and transition states
  if (ESP_NOW_SEND_SUCCESS == g_last_send_callback_status) {
    sprintf(g_msg, "ESP-Now callback OK CMD #%d", g_last_scanned_cmd_count);
    lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
    g_uni_state_error = UNI_STATE_NO_ERROR;
    g_uni_state = UNI_WAIT_CMD;
  } else { // ESP_NOW_SEND_FAIL
    sprintf(g_msg, "ESP-Now callback FAIL CMD #%d", g_last_scanned_cmd_count);
    lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
    g_uni_state_error = UNI_STATE_IN_ERROR;
    g_uni_state = UNI_RCVD_CB;
  }
} // end uni_esp_now_msg_send_callback()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_esp_now_register_peer() - 
//       returns: index to peer
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
// uni_esp_now_msg_send() - decipher QR code and send as needed
//       returns: status from call
//   sends a string up to length ESP_NOW_MAX_DATA_LEN; includes the zero termination of the string   
//
// FIXME TODO this is only for initial testing
// FIXME TODO WARNING this can modify qr_code
//
esp_err_t uni_esp_now_msg_send(char * qr_code) {
  esp_err_t send_status = ESP_OK;
  static char esp_now_msg_data[ESP_NOW_MAX_DATA_LEN+1];
  static uint32_t msec_prev_send = 0;
  uint32_t msec_now = millis();

  // see if waited long enough to send another ESP-NOW message
  if (msec_now < (UNI_ESP_NOW_MSEC_PER_MSG_MIN+msec_prev_send)) {
    DBG_SERIALPRINTLN("ERROR: too soon to send");
    return(ESP_ERR_ESPNOW_INTERNAL); // too soon to send another message
  }
  msec_prev_send = msec_now;

  // see if we can obtain and register the MAC address for sending
  uint8_t * mac_addr_ptr = qr_decode_get_mac_addr_to_send(qr_code);
  int16_t mac_addr_index;
  if ((uint8_t *)0 != mac_addr_ptr) {
    mac_addr_index = uni_esp_now_register_peer(mac_addr_ptr);
  } else {
    DBG_SERIALPRINTLN("ERROR: qr_decode_get_mac_addr_to_send returned 0");
    return(ESP_ERR_ESPNOW_INTERNAL); // could not decode QR code
  }
  if (mac_addr_index < 0) {
    DBG_SERIALPRINTLN("ERROR: could not register MAC");
    return(ESP_ERR_ESPNOW_FULL); // could not register the MAC address
  }

  // copy message over starting after the MAC address
  memset(esp_now_msg_data, '\0', sizeof(esp_now_msg_data));
  strncpy(esp_now_msg_data, &qr_code[3*ESP_NOW_ETH_ALEN], ESP_NOW_MAX_DATA_LEN-1); // max ESP-NOW msg size
  int len = strlen(esp_now_msg_data)+1; // length to send
  send_status = esp_now_send(mac_addr_ptr, (uint8_t *) esp_now_msg_data, strlen(esp_now_msg_data)+1);
  return (send_status);
} // end uni_esp_now_msg_send()

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

  Wire.begin(CYD_SDA, CYD_SCL); // for the QR code sensor

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
  esp_err_t status_register_send_cb = esp_now_register_send_cb(uni_esp_now_msg_send_callback);
  if (status_register_send_cb != ESP_OK){
    DBG_SERIALPRINT("ERROR: ESP-NOW register send callback error ");
    DBG_SERIALPRINTLN(status_register_send_cb);
    return;
  }

  // Start LVGL
  lv_init();

  // Start the SPI for the touchscreen and init the touchscreen
  touchscreenSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  touchscreen.begin(touchscreenSPI);
  // Set the Touchscreen rotation in landscape mode
  // Note: in some displays, the touchscreen might be upside down, so you might need to set the rotation to 0: touchscreen.setRotation(0);
  touchscreen.setRotation(2);

  // Create a display object
  lv_display_t * disp;
  // Initialize the TFT display using the TFT_eSPI library
  disp = lv_tft_espi_create(SCREEN_WIDTH, SCREEN_HEIGHT, draw_buf, sizeof(draw_buf));
  lv_display_set_rotation(disp, LV_DISPLAY_ROTATION_270); // landscape
  
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
//  if QR code seen
//    send to ESP-NOW destination
//  give a slight delay
//
void loop() {
  static int QRcodeSeen = 1; // 0 == no code found, 1 == code found
  static tiny_code_reader_results_t QRresults = {};
  uint32_t msec_now = millis();
  esp_err_t send_status;
  static char * fake_qr_code[] = { 
    "74:4d:bd:11:11:11|dbg non-existing",
    "74:4d:bd:11:11:1|dbg address too short",
    "74:4d:bd:11:11:11c|dbg address too long",
    "74:4d:bd:98:7f:1c|dbg XIAO ESP32-Sense"
   };
#define FAKE_QR_CODE_NUM 4
   static uint8_t fake_qr_code_idx = 0;
   static uint8_t tmp;

  if (0 != g_button_press.pressed) { handle_button_press(); }
  else switch (g_uni_state) {
    case UNI_WAIT_CMD:    // last cmd all done, wait for next cmd (probably QR but any source OK)
      if (0 != g_do_dbg_fake_qr) {
        g_do_dbg_fake_qr = 0;
        g_last_scanned_cmd_count += 1;
        g_uni_state_times[g_uni_state] = msec_now;
        strncpy(g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg, fake_qr_code[fake_qr_code_idx], sizeof(g_qr_code_queue[0].qr_msg));
        tmp = fake_qr_code_idx + 1;
        if (tmp < FAKE_QR_CODE_NUM) {
          fake_qr_code_idx = tmp;
        } else { // not really required here but this way fake_qr_code_idx is NEVER out of bounds
          fake_qr_code_idx = 0;
        }
        g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg_len = QRresults.content_length = strlen(g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg);
        sprintf(g_msg, "DBG Fake QR CMD #%d:\n %s", g_last_scanned_cmd_count, g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg);
        lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
        g_uni_state = UNI_QR_SEEN;
        DBG_SERIALPRINTLN("Change state to UNI_QR_SEEN");
      } else if (!tiny_code_reader_read(&QRresults)) { // Perform a read action on the I2C address of the sensor
        lv_label_set_text(g_styled_label_last_status.label_text, "I2C bus QR code sensor no response");
        QRresults.content_length = 0;
      } else {
        if (0 == QRresults.content_length) {
          lv_label_set_text(g_styled_label_last_status.label_text, "No QR code found, waiting...\n");
        } else {
          g_last_scanned_cmd_count += 1;
          g_uni_state_times[g_uni_state] = msec_now;
          strncpy(g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg, (char *)QRresults.content_bytes, sizeof(g_qr_code_queue[0].qr_msg));
          g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg_len = strlen(g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg);
          sprintf(g_msg, "QR CMD #%d scanned:\n %s", g_last_scanned_cmd_count, g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg);
          lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
          g_uni_state = UNI_QR_SEEN;
          DBG_SERIALPRINTLN("Change state to UNI_QR_SEEN");
        }
      }
      break;
    case UNI_QR_SEEN:   // command in queue, waiting for GO or CLEAR
      break;
    case UNI_SENDING_CMD: // command being sent (very short state)
      send_status = uni_esp_now_msg_send((char *)g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg);
      if (send_status == ESP_OK) {
        sprintf(g_msg, "ESP-NOW send success CMD #%d %s", g_last_scanned_cmd_count, g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg);
        lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
        QRresults.content_length = 0;
        g_uni_state = UNI_WAIT_CB;
        DBG_SERIALPRINTLN("Change state to UNI_WAIT_CB");
      }
      else {
        sprintf(g_msg, "ESP-NOW ERROR: sending CMD #%d: %s\n  %s", g_last_scanned_cmd_count, g_qr_code_queue[UNI_QR_CODE_QNUM_NOW].qr_msg, uni_esp_now_decode_error(send_status));
        lv_label_set_text(g_styled_label_last_status.label_text, g_msg);
        QRresults.content_length = 0;
        g_uni_state = UNI_WAIT_CMD;
        DBG_SERIALPRINTLN("Change state to UNI_WAIT_CMD");
      }
      break;
    case UNI_WAIT_CB:     // waiting for send callback
      break;
    case UNI_RCVD_CB:      // got callback; show status (very short state)
      break;
    default:
      // FIXME TODO should never get here
      break;
  }
  uni_display_state();

  lv_task_handler();  // let the GUI do its work
  lv_tick_inc(CYDsampleDelayMsec); // tell LVGL how much time has passed
  delay(CYDsampleDelayMsec);
  // delay(QRsampleDelayMsec); FIXME TODO wait between intervals
} // end loop()
