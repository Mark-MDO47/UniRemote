#ifndef UNI_WRITE_PICC_H
#define UNI_WRITE_PICC_H 1

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
#endif // UNI_WRITE_PICC_H
