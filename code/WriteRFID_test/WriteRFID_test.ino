/* Author: https://github.com/Mark-MDO47  Jan 23, 2025
 *  https://github.com/Mark-MDO47/UniRemote
 *
 * This test code is to alter the Random Nerd Tutorials esp32-mfrc522-rfid-reader-arduino
 *  code to a form that fits in my uniRemoteCYD or uniRemote code.
 *
 * Mostly I want to avoid delay() statements so I can use multiple devices in the same
 *  code.
 *
 * The RFID reader used is (as far as I can tell) pretty common.
 *     I used this one: https://www.amazon.com/dp/B07VLDSYRW
 *     It is capable of reading and/or writing RFID Smart Cards.
 *     The controller chip could support UART or I2C or SPI interfaces,
 *        but the only interface supported without board modification is the
 *        SPI interface.
 *
 * This code was developed after reading the Random Nerd Tutorials below.
 * There are (or will be) significant differences in this code and the tutorials,
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
 *    
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

#include <esp_now.h>   // for ESP-NOW; get ESP_NOW_MAX_DATA_LENs

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
#define DEBUG_PRINT_PICC_DATA_EACH  1     // Print the data we read in ASCII after each read

// PICC definitions for RFID reader
#define PICC_EV1_1K_NUM_SECTORS         16 // 16 sectors each with 4 blocks of 16 bytes
#define PICC_EV1_1K_SECTOR_NUM_BLOCKS   4  // each sector has 4 blocks of 16 bytes
#define PICC_EV1_1K_BLOCK_NUM_BYTES     16 // each block has 16 bytes
#define PICC_EV1_1K_BLOCK_SECTOR_AVOID  3  // avoid blockAddress 0 and block 3 within each sector
#define PICC_EV1_1K_START_BLOCKADDR     1  // do not use blockAddress 0
#define PICC_EV1_1K_END_BLOCKADDR ((PICC_EV1_1K_SECTOR_NUM_BLOCKS) * PICC_EV1_1K_NUM_SECTORS - 1)

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_write_picc() - write a PICC command
//   PICC = Proximity Integrated Circuit Card (Contactless Card) - the RFID card we are writing
//      At this time we plan to use the MIFARE Classic EV1 1K
//   Writes write_cmd to PICC and return zero if succesful
//      write_cmd is parameter: pointer to zero-terminated string.
//      Maximum length is 519 chars plus zero because we will be using it on ESP-NOW.
//      Maximum data length on MIFARE Classic EV1 1K is 752 bytes (see below)
//
// NOTE: the blockAddress is the combination of sector and block: blockAddress = _NUM_SECTORS*sector + block
//   For PICC EV1 1K the blockAddress can range from 0 (sector 0 block 0) to 63 (sector 15 block 3)
//   We can write in all blockAddress EXCEPT:
//       blockAddress = 0 - never ever
//       any 3rd block in any sector - never ever
//   So there are a total of 47 sectors we can use; (16*3-1)*16 bytes = 752 bytes
//
// Returns zero if succesfully wrote command; else non-zero
//
uint8_t uni_write_picc(char * write_cmd) {
  // variables to keep track of timing of our actions
  static uint32_t msec_prev = 0;
  static uint32_t msec_waitfor = 0;
  uint32_t msec_now = millis();

  // variables to help with reading/writing the PICC card
  byte blockAddress;
  byte bufferblocksize = PICC_EV1_1K_BLOCK_NUM_BYTES;  // number to write; no slack
  MFRC522Constants::StatusCode picc_status;

  static char picc_msg[1026];            // temp area to build strings for messages
  static char picc_cmd[PICC_EV1_1K_NUM_SECTORS*(PICC_EV1_1K_SECTOR_NUM_BLOCKS-1)*PICC_EV1_1K_BLOCK_NUM_BYTES]; // 16 extra bytes; place the command for the card here
  char * picc_cmd_ptr = picc_cmd;       // pointer to the command to write

  uint8_t ret_value = 0xFF; // did not get a command yet

  // thoroughly init buffer for zero-terminated command to write to PICC
  memset(picc_cmd_ptr, 0, sizeof(picc_cmd));
  strncpy(picc_cmd_ptr, write_cmd, ESP_NOW_MAX_DATA_LEN-1); // max ESP-NOW msg size -1 for the zero termination

  // don't do anything until next waitfor time
  if (msec_now < msec_waitfor) return(ret_value);

  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    msec_waitfor = msec_now + 500;
    return(ret_value);
  }

#if DEBUG_PRINT_PICC_INFO
  // Display card UID
  Serial.print("----------------\nCard UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();
#endif // DEBUG_PRINT_PICC_INFO

  // check if card is our expected type
  MFRC522Constants::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (MFRC522Constants::PICC_Type::PICC_TYPE_MIFARE_1K != piccType) {
    // FIXME TODO set display status to wrong card type
#if DEBUG_PRINT_PICC_INFO
    sprintf(picc_msg, "ERROR: PICC Type %d not PICC_TYPE_MIFARE_1K %d", piccType, MFRC522Constants::PICC_Type::PICC_TYPE_MIFARE_1K);
    Serial.println(picc_msg); 
#endif // DEBUG_PRINT_PICC_INFO
    msec_waitfor = msec_now + 500;
    return(ret_value);
  }

  // Write data to all data blocks
  picc_status = MFRC522Constants::StatusCode::STATUS_OK;
  for (blockAddress = PICC_EV1_1K_START_BLOCKADDR; (blockAddress <= PICC_EV1_1K_END_BLOCKADDR) && (picc_status == MFRC522Constants::StatusCode::STATUS_OK); blockAddress += 1) {

    // skip the blocks we should not use
    if (PICC_EV1_1K_BLOCK_SECTOR_AVOID == (blockAddress % PICC_EV1_1K_SECTOR_NUM_BLOCKS)) continue;

    // Authenticate the specified block using KEY_A == command 0x60
    // MF1S50YYX_V1 Rev. 3.2 — 23 May 2018 says "The HLTA command needs to be sent encrypted to the PICC after a successful authentication in order to be accepted"
    if ((picc_status = mfrc522.PCD_Authenticate(MFRC522Constants::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid))) != MFRC522Constants::StatusCode::STATUS_OK) {
#if DEBUG_PRINT_PICC_INFO
      sprintf(picc_msg, "ERROR: PICC Write Authentication failed, status %d", picc_status);
      Serial.println(picc_msg);
#endif // DEBUG_PRINT_PICC_INFO
      // FIXME TODO set display status to authentication failed
      break; // do the halt and stop
    }

    if ((picc_status = mfrc522.MIFARE_Write(blockAddress, (byte *)picc_cmd_ptr, bufferblocksize)) != MFRC522Constants::StatusCode::STATUS_OK) {
#if DEBUG_PRINT_PICC_INFO
      sprintf(picc_msg, "ERROR: PICC Write failed, status %d", picc_status);
      Serial.println(picc_msg);
#endif // DEBUG_PRINT_PICC_INFO
      // FIXME TODO set display status to Write failed
      break; // do the halt and stop
    } else {
      picc_cmd_ptr += PICC_EV1_1K_BLOCK_NUM_BYTES;
    } // end one block Write successful
  } // end for all blockAddress

  // Halt communication with the card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

#if DEBUG_PRINT_PICC_DATA_FINAL
  Serial.print("PICC Write final MFRC522 status "); Serial.println(picc_status);
#endif // DEBUG_PRINT_PICC_DATA_FINAL
  if (MFRC522Constants::StatusCode::STATUS_OK == picc_status) ret_value = 0;
  msec_waitfor = msec_now + 2000; // Delay for readability
  return(ret_value);
} // end uni_write_picc()

char g_picc_read[PICC_EV1_1K_NUM_SECTORS*(PICC_EV1_1K_SECTOR_NUM_BLOCKS-1)*PICC_EV1_1K_BLOCK_NUM_BYTES]; // 16 extra bytes

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_read_picc() - get next PICC command - DEBUGGING just detect, read, write, read
//   PICC = Proximity Integrated Circuit Card (Contactless Card) - the RFID card we are reading
//      At this time we plan to use the MIFARE Classic EV1 1K
//   FIXME TODO Currently just puts info into serial printout
//   Eventually would read command from PICC and return command as zero-terminated ASCII string
//
// NOTE: the blockAddress is the combination of sector and block: blockAddress = _NUM_SECTORS*sector + block
//   For PICC EV1 1K the blockAddress can range from 0 (sector 0 block 0) to 63 (sector 15 block 3)
//   We can write in all blockAddress EXCEPT:
//       blockAddress = 0 - never ever
//       any 3rd block in any sector - never ever
//   So there are a total of 47 sectors we can use; (16*3-1)*16 bytes = 752 bytes
//
// Returns zero if got a command; else non-zero
// g_picc_read will be filled with the command; zero-terminated string.
//
uint8_t uni_read_picc() {
  // variables to keep track of timing of our actions
  static uint32_t msec_prev = 0;
  static uint32_t msec_waitfor = 0;
  uint32_t msec_now = millis();

  // variables to help with reading/writing the PICC card
  byte blockAddress;
  byte bufferblocksize = PICC_EV1_1K_BLOCK_NUM_BYTES+2;  // need this number in RAM; leaving some slack
  byte blockDataRead[PICC_EV1_1K_BLOCK_NUM_BYTES+2];
  MFRC522Constants::StatusCode picc_status;

  static char picc_msg[1026];            // temp area to build strings for messages
  static char picc_cmd[PICC_EV1_1K_NUM_SECTORS*(PICC_EV1_1K_SECTOR_NUM_BLOCKS-1)*PICC_EV1_1K_BLOCK_NUM_BYTES]; // 16 extra bytes; assemble the command from the card here
  char * picc_cmd_ptr = picc_cmd; // pointer to the output buffer

  uint8_t ret_value = 0xFF; // did not get a command yet

  // init zero-terminated command read from PICC in case of no card or early error
  picc_cmd[0] = g_picc_read[0] = '\0';

  // don't do anything until next waitfor time
  if (msec_now < msec_waitfor) return(ret_value);

  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    msec_waitfor = msec_now + 500;
    return(ret_value);
  }

#if DEBUG_PRINT_PICC_INFO
  // Display card UID
  Serial.print("----------------\nCard UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();
#endif // DEBUG_PRINT_PICC_INFO

  // check if card is our expected type
  MFRC522Constants::PICC_Type piccType = mfrc522.PICC_GetType(mfrc522.uid.sak);
  if (MFRC522Constants::PICC_Type::PICC_TYPE_MIFARE_1K != piccType) {
    // FIXME TODO set display status to wrong card type
#if DEBUG_PRINT_PICC_INFO
    sprintf(picc_msg, "ERROR: PICC Type %d not PICC_TYPE_MIFARE_1K %d", piccType, MFRC522Constants::PICC_Type::PICC_TYPE_MIFARE_1K);
    Serial.println(picc_msg); 
#endif // DEBUG_PRINT_PICC_INFO
    msec_waitfor = msec_now + 500;
    return(ret_value);
  }

  // thoroughly init zero-terminated command read from PICC; zero length as we build it up
  memset(picc_cmd_ptr, 0, sizeof(picc_cmd));

  // Read data from all blocks
  picc_status = MFRC522Constants::StatusCode::STATUS_OK;
  for (blockAddress = PICC_EV1_1K_START_BLOCKADDR; (blockAddress <= PICC_EV1_1K_END_BLOCKADDR) && (picc_status == MFRC522Constants::StatusCode::STATUS_OK); blockAddress += 1) {

    // skip the blocks we should not use
    if (PICC_EV1_1K_BLOCK_SECTOR_AVOID == (blockAddress % PICC_EV1_1K_SECTOR_NUM_BLOCKS)) continue;

    // Authenticate the specified block using KEY_A == command 0x60
    // MF1S50YYX_V1 Rev. 3.2 — 23 May 2018 says "The HLTA command needs to be sent encrypted to the PICC after a successful authentication in order to be accepted"
    if ((picc_status = mfrc522.PCD_Authenticate(MFRC522Constants::PICC_Command::PICC_CMD_MF_AUTH_KEY_A, blockAddress, &key, &(mfrc522.uid))) != MFRC522Constants::StatusCode::STATUS_OK) {
#if DEBUG_PRINT_PICC_INFO
      sprintf(picc_msg, "ERROR: PICC Authentication failed, status %d", picc_status);
      Serial.println(picc_msg);
#endif // DEBUG_PRINT_PICC_INFO
      // FIXME TODO set display status to authentication failed
      picc_cmd[0] = '\0';
      break; // do the halt and stop
    }

    if ((picc_status = mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferblocksize)) != MFRC522Constants::StatusCode::STATUS_OK) {
#if DEBUG_PRINT_PICC_INFO
      sprintf(picc_msg, "ERROR: PICC Read failed, status %d", picc_status);
      Serial.println(picc_msg);
#endif // DEBUG_PRINT_PICC_INFO
      // FIXME TODO set display status to read failed
      picc_cmd[0] = '\0';
      break; // do the halt and stop
    } else {
      memcpy(picc_cmd_ptr, blockDataRead, PICC_EV1_1K_BLOCK_NUM_BYTES);
      picc_cmd_ptr += PICC_EV1_1K_BLOCK_NUM_BYTES;
#if DEBUG_PRINT_PICC_DATA_EACH
      Serial.println("PICC Read successful!");
      Serial.print("Data in block ");
      Serial.print(blockAddress);
      Serial.print(": ");
      for (byte i = 0; i < 16; i++) {
        Serial.print((char)blockDataRead[i]);  // Print as character
      }
      Serial.println();
#endif // DEBUG_PRINT_PICC_DATA_EACH
    } // end one block read successful
  } // end for all blockAddress

  // Halt communication with the card
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();

  msec_waitfor = msec_now + 2000; // Delay for readability
#if DEBUG_PRINT_PICC_DATA_FINAL
  Serial.print("PICC read final MFRC522 status "); Serial.println(picc_status);
#endif // DEBUG_PRINT_PICC_DATA_FINAL
  if (MFRC522Constants::StatusCode::STATUS_OK == picc_status) {
    ret_value = 0;
    strncpy(g_picc_read, picc_cmd, ESP_NOW_MAX_DATA_LEN-1); // max ESP-NOW msg size -1 for the zero termination
  }
  return(ret_value);
} // end uni_read_picc()

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this

  mfrc522.PCD_Init();    // Init MFRC522 board.
  Serial.println(F("starting WriteRFID_test - writes to a PICC RFID card"));
 
  // Prepare key - all keys are set to FFFFFFFFFFFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

char * write_string = "This is WriteRFID_test - writes to a PICC RFID card";
#define STATE_READ_1 0
#define STATE_WRITE  1
#define STATE_READ_2 2
#define STATE_DONE   3
void loop() {
  static uint8_t state = STATE_READ_1;
  uint8_t the_status;

  if ((STATE_READ_1 == state) || (STATE_READ_2 == state)) {
    if (0 == (the_status = uni_read_picc())) {
      Serial.println(g_picc_read);
      delay(2000);
      state += 1;
    }
  } else if (STATE_WRITE == state) {
    if (0 == (the_status = uni_write_picc(write_string))) {
      Serial.println("uni_write_picc() succesful!");
      delay(2000);
      state += 1;
    }
  }
}