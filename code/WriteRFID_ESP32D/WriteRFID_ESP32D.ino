/* Author: https://github.com/Mark-MDO47  Jan 23, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 */

/*
   Copyright 2024, 2025 Mark Olson

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
 *  This routine writes and checks command info to PICC (RFID) cards.
 *  It is intended to write PICC cards to be used by UniRemoteCYD.
 *     https://github.com/Mark-MDO47/UniRemote
 *     https://github.com/Mark-MDO47/UniRemote/tree/master/code/UniRemoteCYD
 *  This routine runs on ESP32-WROOM-32D Devkit C using the Mifare RC522 RF IC Card Sensor Module.
 *  The dialog with the operator is done using the serial port, not the display
 *     and touchscreen. Because I am lazy.
 *  The PICC command info is entered in array write_strings[]. The input and output
 *     formats are described below.
 *     The text in write_strings[] is the same format as the input file to QRCode.py
 *     https://github.com/Mark-MDO47/MDOpythonUtils/tree/master/QRCode
 *
 *########################## INPUT ###########################
 * The input QR code file should be a tab-separated-variable text file
 *     of the following form. The input text array for the WriteRFID_ESP32D program
 *     should be of the same form for ease of transcription, but the *.png
 *     filename is ignored in WriteRFID_ESP32D.
 * <*.png filename><TAB><DESCRIPTION STRING><TAB><MAC ADDRESS><"|"><COMMAND STRING>
 *
 * Most of those fields are described in the output section.
 * The ones for input only are here.
 *
 * <*.png filename> is a string giving a unique filename for the generated QR-code.
 *     The *.png will be created containing a QR code in the output format below.
 *     There will also be a *.html file to display the QR code in a browser
 *      example: bad_test_lg_msg.png
 *
 * <"|"> is the single character of a vertical bar (the bash pipe character).
 *
 * <TAB> is the single character Tab (ASCII Horizontal Tab, '\t', hex 0x09)
 *
 *########################## OUTPUT ###########################
 * The output QR code or MIFARE Classic EV1 1K card should be a
 *     tab-separated-variable text file of the following form:
 * <MAC ADDRESS><TAB><COMMAND STRING><TAB><DESCRIPTION STRING>
 * 
 * <MAC ADDRESS> is a string of the following exact form:
 *     ##:##:##:##:##:##
 *   This is the MAC Address that will be used to send the ESP-NOW message;
 *     the MAC address of the target system.
 *   Note that this is a six-part MAC address in hexadecimal. Each hex number
 *   is exactly two digits long. If you need to start it with a zero, do so.
 *   Because I am a lazy coder, formatting the string properly is up to you.
 * 
 * <COMMAND STRING> is a short (maximum 249 characters + zero termination) command
 *   The receiving MAC address will receive it as a zero-terminated string (including
 *   the zero terminator).
 *
 * <DESCRIPTION STRING> can be zero length or more, but for consistency
 *   the <TAB> prior to the description string is required.
 *   The description is just for your purposes; it is not sent to the ESP-NOW target.
 *
 */

/*
 * The RFID reader used is (as far as I can tell) pretty common.
 *     I used this one: https://www.amazon.com/dp/B07VLDSYRW
 *     It is capable of reading and/or writing RFID Smart Cards.
 *     The controller chip could support UART or I2C or SPI interfaces,
 *        but the only interface supported without board modification is the
 *        SPI interface.
 */

/*
 * This code was developed after reading the Random Nerd Tutorials below.
 * There are significant differences in this code and the tutorials,
 *    but I want to give a tip of the hat to Rui Santos & Sara Santos for the
 *    wonderful work they do.
 * Below is the attribution from the Random Nerd Tutorial esp32-mfrc522-rfid-reader-arduino.
 */
/*
  Rui Santos & Sara Santos - Random Nerd Tutorials
  Complete project details at https://RandomNerdTutorials.com/esp32-mfrc522-rfid-reader-arduino/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.  
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*/

/*
 * I am using this MicroSD sniffer https://www.sparkfun.com/sparkfun-microsd-sniffer.html to get access to pins to
 *    use for the SPI interface of the RFID reader. In order to free up a hardware SPI channel (VSPI), I am using the
 *    XPT2046_Bitbang library for the touchscreen so I can use VSPI through the MicroSD interface to control the
 *    RFID reader.
 */

/* 
 * NOTE: I have not trimmed the includes down to the minimum yet. TODO FIXME
 *    Not using display or touchscreen, probably do not need XPT2046_Bitbang.
 *    Believe it or not, it does need esp_now.h to get max ESP-NOW msg length.
 */

// ----------------------------
// Standard Libraries
// ----------------------------

#include <SPI.h>

// ----------------------------
// Additional Libraries - each one of these will need to be installed.
// ----------------------------

#include <XPT2046_Bitbang.h>
// A library for interfacing with the touch screen
//
// Can be installed from the library manager (Search for "XPT2046 Slim")
// https://github.com/TheNitek/XPT2046_Bitbang_Arduino_Library

#include <TFT_eSPI.h>
// A library for interfacing with LCD displays
//
// Can be installed from the library manager (Search for "TFT_eSPI")
// https://github.com/Bodmer/TFT_eSPI

#include <esp_now.h>   // for ESP-NOW; get ESP_NOW_MAX_DATA_LEN

#include <MFRC522v2.h>
#include <MFRC522DriverSPI.h>
//#include <MFRC522DriverI2C.h>
#include <MFRC522DriverPinSimple.h>
#include <MFRC522Debug.h>

// Learn more about using SPI/I2C or check the pin assigment for your board: https://github.com/OSSLibraries/Arduino_MFRC522v2#pin-layout
MFRC522DriverPinSimple ss_pin(5);

MFRC522DriverSPI driver{ss_pin}; // Create SPI driver
//MFRC522DriverI2C driver{};     // Create I2C driver
MFRC522 mfrc522{driver};         // Create MFRC522 instance

MFRC522::MIFARE_Key key;

// DEBUG definitions
#define DBG_SERIALPRINT Serial.print
#define DBG_SERIALPRINTLN Serial.println
#define DEBUG_PRINT_TOUCHSCREEN_INFO 0    // Print Touchscreen info about X, Y and Pressure (Z) on the Serial Monitor
#define DEBUG_PRINT_PICC_INFO 1           // Print UID and other info when PICC RFID card detection on the Serial Monitor
#define DEBUG_PRINT_PICC_DATA_FINAL 1     // Print the data we read in ASCII after all reads
#define DEBUG_PRINT_PICC_DATA_EACH  0     // Print the data we read in ASCII after each read

#define TRUE  1 // in a LOGICAL statement, anything non-zero is true
#define FALSE 0 // in a LOGICAL statement, only zero is false
#define NUMOF(x) (sizeof((x)) / sizeof((x[0]))) // calculates the size of an array

// PICC definitions for RFID reader
#define PICC_EV1_1K_NUM_SECTORS         16 // 16 sectors each with 4 blocks of 16 bytes
#define PICC_EV1_1K_SECTOR_NUM_BLOCKS   4  // each sector has 4 blocks of 16 bytes
#define PICC_EV1_1K_BLOCK_NUM_BYTES     16 // each block has 16 bytes
#define PICC_EV1_1K_BLOCK_SECTOR_AVOID  3  // avoid blockAddress 0 and block 3 within each sector
#define PICC_EV1_1K_START_BLOCKADDR     1  // do not use blockAddress 0
#define PICC_EV1_1K_END_BLOCKADDR ((PICC_EV1_1K_SECTOR_NUM_BLOCKS) * PICC_EV1_1K_NUM_SECTORS - 1)

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// parse_string - parse input string into string for PICC writing
//    strptr_desc  - input string starting at <DESCRIPTION STRING><TAB><MAC ADDRESS><"|"><COMMAND STRING>
//    build_string - PICC writing string <MAC ADDRESS><"|"><COMMAND STRING><TAB><DESCRIPTION STRING>
//    output_desc  - non-zero to output <TAB><DESCRIPTION STRING>
//
// returns 0 if success; else non-zero
// 
// assumes the input is well formed
//
uint8_t parse_string(char *build_string, char *strptr_desc, uint8_t output_desc) {
  uint8_t ret_val = 0xFF;
  char * strptr_mac = strstr(strptr_desc,"\t");

  if ((ESP_NOW_MAX_DATA_LEN-1) > strlen(strptr_desc)) {
    build_string[0] = '\0';
    strcat(build_string,1+strptr_mac); // copy MAC address and Command String
    if (output_desc) {
      strcat(build_string,"\t");
      strncat(build_string,strptr_desc,strptr_mac-strptr_desc);
    }
    ret_val = 0;
  }
  return(ret_val);
} // end parse_string()

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_write_picc() - write a PICC command
//   PICC = Proximity Integrated Circuit Card (Contactless Card) - the RFID card we are writing
//      At this time we plan to use the MIFARE Classic EV1 1K
//   Writes write_cmd to PICC and return zero if succesful
//      write_cmd is parameter: pointer to zero-terminated string.
//      Maximum length is 519 chars plus zero because we will be using it on ESP-NOW.
//      Maximum data length on MIFARE Classic EV1 1K is 752 bytes
//
#include "../Uni_RW_PICC/uni_write_picc.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_read_picc(char my_picc_read[]) - get next PICC command
//   PICC = Proximity Integrated Circuit Card (Contactless Card) - the RFID card we are reading
//      At this time we plan to use the MIFARE Classic EV1 1K
// Returns zero if got a command; else non-zero
// param my_picc_read[] will be filled with the command; zero-terminated string.
//
#include "../Uni_RW_PICC/uni_read_picc.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// get_ascii_string() - get one string (no leading or trailing ' ' or '\t') then flush to '\n'
//
// THIS IS THE STRING CLASS VERSION
//
//    parameters: none
//    returns:    char * pointer to the string
//
// Reads from Serial, gets one string then flushes to '\n'.
// Does not return until the flush is complete.
// Maximum string length is MAX_STRING_LENGTH
//
// Restriction: if you want to get two different strings, you need to copy
//    them out of our string buffer when we return and store the strings in
//    your own separate buffers.
// Another way to say this is that this routine only has one string buffer.
//    The second time you call it, it will overwrite the string buffer from
//    the first call.
//
#define MAX_STRING_LENGTH 20
#define DO_DEBUG_INPUT FALSE // TRUE=debug, FALSE=no debug
#if DO_DEBUG_INPUT
  // NOTE: these are not complex enough to cover all the cases
  #define DEBUG_INPUT_PRINT(x) Serial.print((x))
  #define DEBUG_INPUT_PRINTLN(x) Serial.println((x))
#else
  #define DEBUG_INPUT_PRINT(x)
  #define DEBUG_INPUT_PRINTLN(x)
#endif // DO_DEBUG_INPUT

char * get_ascii_string() {
  static char ascii_string[MAX_STRING_LENGTH+1]; // only this routine knows the name, but we return a pointer to it
  String my_string_object = "Hello!";
  int16_t tmp1 = 0;
  int16_t tmp2 = 0;
  uint8_t found = FALSE;

  Serial.setTimeout(10000); // 10,000 milliseconds is 10 seconds
  memset((void *)ascii_string, 0, NUMOF(ascii_string)); // clear buffer; good idea for zero-terminated strings

  while (!found) {
    while (!Serial.available()) ; // wait for typing to start
    my_string_object = Serial.readStringUntil('\n'); // get a line
    DEBUG_INPUT_PRINT(F("DBGIN Entire String object |")); DEBUG_INPUT_PRINT(my_string_object); DEBUG_INPUT_PRINTLN(F("|"));
    my_string_object.trim(); // trim off spaces/tabs front and back
    DEBUG_INPUT_PRINT(F("DBGIN trimmed String object |")); DEBUG_INPUT_PRINT(my_string_object); DEBUG_INPUT_PRINTLN(F("|"));
    if (0 != my_string_object.length()) {
      found = TRUE;
      tmp1 = my_string_object.indexOf(' ');
      tmp2 = my_string_object.indexOf('\t');
      DEBUG_INPUT_PRINT(F("DBGIN tmp1=")); DEBUG_INPUT_PRINT(tmp1); DEBUG_INPUT_PRINT(F(", tmp2=")); DEBUG_INPUT_PRINTLN(tmp2);
      if (-1 == tmp1) tmp1 = 2*MAX_STRING_LENGTH; // make it big and positive
      if (-1 == tmp2) tmp2 = 2*MAX_STRING_LENGTH;
      // if no ' ' or '\t' found, will copy entire string
      //string2ascii_ncopy(my_string_object, ascii_string, 0, min(tmp1,tmp2), MAX_STRING_LENGTH);
      tmp1 = min(tmp1,tmp2);
      tmp1 = min(tmp1,(int16_t) my_string_object.length());
      tmp1 = min(tmp1,(int16_t) MAX_STRING_LENGTH);
      for (int i = 0; i < tmp1; i++) {
        ascii_string[i] = my_string_object[i];
      }
      DEBUG_INPUT_PRINT(F("DBGIN Entire ASCII string result |")); DEBUG_INPUT_PRINT(ascii_string); DEBUG_INPUT_PRINTLN(F("|"));
    } // end if my_string_object has at least one character
  } // end while (!found)

  return(ascii_string);
} // end get_ascii_string() - STRING CLASS version

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// setup() - Initialize CYD hardware: serial port and VSPI to MFRC522 via the MicroSD card and a sniffer.
//
void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this

  mfrc522.PCD_Init();    // Init MFRC522 board.
  Serial.println(F("\nstarting WriteRFID_ESP32D - writes to a PICC RFID card"));
  Serial.println(F("   see https://github.com/Mark-MDO47/UniRemote\n"));
 
  // Prepare key - all keys are set to FFFFFFFFFFFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// loop() - send write_strings to PICC cards
//
// Runs on a CYD but communicates through the serial port.
//
// Thus the UniRemoteCYD hardware is used to write the PICC cards
//    but when used as a remote control there is no code to write a PICC card.
//
char * write_strings[] = {
  /*
  "XIAO_test_msg.png\tTest Message to XIAO ESP32-Sense\t74:4d:bd:98:7f:1c|TestMessage XIAO ESP32-Sense",
  "bad_test_ne_msg.png\tTest Message to non-existing destination\t74:4d:bd:11:11:11|TestMessage non-existing",
  "bad_test_sh_msg.png\tTest Message is too short\t74:4d:bd:11:11:1|TestMessage address too short",
  "bad_test_lg_msg.png\tTest Message is too long\t74:4d:bd:11:11:11c|TestMessage address too long" };
   */
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; MUSIC:SONG MERRY-GENTLEMEN",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; MUSIC:SONG SILENCE",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; MUSIC:TYPE CHRISTMAS",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; MUSIC:NEXT ignore ; EYES:PATTERN OPPOSITE/64/BLINK" };
  /*
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN TOGETHER/64/SINELON",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN TOGETHER/64/BLINK",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN OPPOSITE/64/BLINK",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN TOGETHER/64/BL-OPN",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN TOGETHER/64/OPEN",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN OPPOSITE/64/OPEN",
  "BANJO\tMessage for BANJO\tac:67:b2:2c:c9:c0|BANJO ; EYES:PATTERN TOGETHER/64/OFF" };
  */

// #define STATE_READ_1 0
#define STATE_DESCRIBE 0
#define STATE_WRITE    1
#define STATE_WRITE    1
#define STATE_READ     2
#define STATE_DONE     3
void loop() {
  static uint8_t state = STATE_DESCRIBE;
  static char build_string[ESP_NOW_MAX_DATA_LEN];
  static uint16_t input_idx = 0;
  static char my_picc_read[PICC_EV1_1K_NUM_SECTORS*(PICC_EV1_1K_SECTOR_NUM_BLOCKS-1)*PICC_EV1_1K_BLOCK_NUM_BYTES]; // 16 extra bytes
  uint16_t the_status;
  char * strptr;
  char * opr_input;

  if (STATE_DESCRIBE == state) {
    if (NUMOF(write_strings) > input_idx) {
      Serial.println("Prepare to write MIFARE Classic EV1 1K card");
      Serial.println("   Enter anything starting with w or W to start looking for card and writing");
      Serial.println("   Enter anything starting with s or S to skip this string and go to the next");
      Serial.println("   Enter anything starting with a or A to abort this process and stop writing RFID cards");
      // print without the first field (unused)
      Serial.print(" --String-To-Process--> \""); Serial.print(strptr = 1+strstr(write_strings[input_idx],"\t")); Serial.println("\"");
      Serial.print("Enter your command to process this String > ");
      opr_input = get_ascii_string();
      Serial.println(" "); Serial.print("You Entered \""); Serial.print(opr_input); Serial.println("\"");
      if (('w' == *opr_input) || ('W' == *opr_input)) {
        if (0 == parse_string(build_string,strptr,0)) {
          Serial.print(" --Write--> \""); Serial.print(build_string); Serial.println("\"");
          state = STATE_WRITE;
          Serial.println("Please place card on writer");
        } else {
          Serial.println("ERROR parsing input string, skipping to next string\n");
          input_idx += 1;
        }
      } else if (('s' == *opr_input) || ('S' == *opr_input)) {
          Serial.println("Skipping to next string\n");
          input_idx += 1;
      } else if (('a' == *opr_input) || ('A' == *opr_input)) {
        Serial.println("Aborting; done");
        state = STATE_DONE;
      }
    } else {
      Serial.println("All input is processed; done");
      state = STATE_DONE;
    }
  } else if (STATE_WRITE == state) {
    if (0 == (the_status = uni_write_picc(build_string))) {
      Serial.println("uni_write_picc() succesful! Please remove card, short wait, replace card for reading");
      input_idx += 1;
      state = STATE_READ;
      delay(1000);
    }
  } else if (STATE_READ == state) {
    if (0 == (the_status = uni_read_picc(my_picc_read))) {
      Serial.print(" --Read---> \""); Serial.print(my_picc_read); Serial.println("\"");
      if (0 == strcmp(build_string, my_picc_read)) {
        Serial.println("Comparison GOOD - Read data matches Write data");
      } else {
        Serial.println("ERROR: Comparison BAD - read data does not match what was written");
      }
      Serial.println("Please remove PICC card\n");
      delay(1000);
      state = STATE_DESCRIBE;
    }
  }
} // end loop()
