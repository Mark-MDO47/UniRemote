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

byte blockAddress = 2;
// byte newBlockData[17] = {"Rui Santos - RNT"};
byte newBlockData[17] = {"_MDO_MDO_MDO_MDO"};
//byte newBlockData[16] = {0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};   // CLEAR DATA
byte bufferblocksize = 18;
byte blockDataRead[18];

void setup() {
  Serial.begin(115200);  // Initialize serial communication
  while (!Serial);       // Do nothing if no serial port is opened (added for Arduinos based on ATMEGA32U4).
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this

  mfrc522.PCD_Init();    // Init MFRC522 board.
  Serial.println(F("Warning: this example overwrites a block in your card, use with care!"));
 
  // Prepare key - all keys are set to FFFFFFFFFFFF at chip delivery from the factory.
  for (byte i = 0; i < 6; i++) {
    key.keyByte[i] = 0xFF;
  }
}

void loop() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    delay(500);
    return;
  }

  // Display card UID
  Serial.print("----------------\nCard UID: ");
  MFRC522Debug::PrintUID(Serial, (mfrc522.uid));
  Serial.println();

  // Authenticate the specified block using KEY_A = 0x60
  if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) {
    Serial.println("Authentication failed.");
    return;
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
  // Authenticate the specified block using KEY_A = 0x60
  if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) {
    Serial.println("Authentication failed.");
    return;
  }
  
  // Write data to the specified block
  if (mfrc522.MIFARE_Write(blockAddress, newBlockData, 16) != 0) {
    Serial.println("Write failed.");
  } else {
    Serial.print("Data written successfully in block: ");
    Serial.println(blockAddress);
  }

  // Authenticate the specified block using KEY_A = 0x60
  if (mfrc522.PCD_Authenticate(0x60, blockAddress, &key, &(mfrc522.uid)) != 0) {
    Serial.println("Authentication failed.");
    return;
  }

  // Read data from the specified block
  if (mfrc522.MIFARE_Read(blockAddress, blockDataRead, &bufferblocksize) != 0) {
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

  delay(2000);  // Delay for readability
}