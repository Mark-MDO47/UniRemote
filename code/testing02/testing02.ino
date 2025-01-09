/* Author: https://github.com/Mark-MDO47  Dec. 21, 2023
 *  https://github.com/Mark-MDO47/UniRemote
 *  
 *  
 */

// check array indexing

#include <esp_now.h>   // for ESP-NOW

static char g_qr_code_queue[2][ESP_NOW_MAX_DATA_LEN+2];

void setup() {
  Serial.begin(115200); // basically for debugging...
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(1000); // 1 second delay - XIAO ESP32S3 Sense and others need this
  Serial.println(""); // print a blank line in case there is some junk from power-on
  Serial.println("\nStarting UniRemote\n");

  strncpy(&g_qr_code_queue[0][0], "0123456789 this is 0-0", ESP_NOW_MAX_DATA_LEN);
  strncpy(&g_qr_code_queue[1][0], "abcdefghij this is 1-0", ESP_NOW_MAX_DATA_LEN);

  for (int i = 0; i < 2; i += 1) {
    // for (int j = 0; (j < 80) && ('\0' != g_qr_code_queue[i][j]); j += 1) {
    Serial.print(i); Serial.print(" "); Serial.println(&g_qr_code_queue[i][0]);
  }
} // end setup()

void loop() {
  
} // end loop()
