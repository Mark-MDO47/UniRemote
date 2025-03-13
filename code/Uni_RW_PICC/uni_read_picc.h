/* Author: https://github.com/Mark-MDO47  Dec. 21, 2024
 *  https://github.com/Mark-MDO47/UniRemote
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

#ifndef UNI_READ_PICC_H
#define UNI_READ_PICC_H 1

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

/////////////////////////////////////////////////////////////////////////////////////////////////////////
// uni_read_picc() - get next PICC command
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
// p_picc_read will be filled with the command; zero-terminated string.
//
uint8_t uni_read_picc(char p_picc_read[]) {
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
  picc_cmd[0] = p_picc_read[0] = '\0';

  // don't do anything until next waitfor time
  if (msec_now < msec_waitfor) return(ret_value);

  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    msec_waitfor = msec_now + 500;
    return(ret_value);
  }

#if DEBUG_PRINT_PICC_INFO
  // Display card UID
  Serial.print("--READING---------\nCard UID: ");
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
    strncpy(p_picc_read, picc_cmd, ESP_NOW_MAX_DATA_LEN-1); // max ESP-NOW msg size -1 for the zero termination
  }
  return(ret_value);
} // end uni_read_picc()
#endif // UNI_READ_PICC_H
