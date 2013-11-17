#include <Arduino.h>

#include <SdFat.h>

#include "sdcard.hpp"

namespace {
    const uint8_t SLAVE_SELECT = 53;
    const uint8_t SD_CHIP_SELECT = 4;

    Sd2Card card;
    SdVolume volume;
    SdFile root;


    // store error strings in flash to save RAM
    #define error(s) error_P(PSTR(s))

    void error_P(const char* str) {
      Serial.println("error: ");
      Serial.println(str);
      if (card.errorCode()) {
        Serial.println("SD error: ");
        Serial.print(card.errorCode(), HEX);
        Serial.print(',');
        Serial.println(card.errorData(), HEX);
      }
      while(1);
    }
}

namespace sd {

void initialize() {
    pinMode(SLAVE_SELECT, OUTPUT);     // change this to 53 on a mega
    digitalWrite(SLAVE_SELECT, HIGH);  // Disable W5100 Ethernet

    if (!card.init(SPI_HALF_SPEED, SD_CHIP_SELECT)) {
        error("card.init failed!");
    }

    if (!volume.init(&card)) error("vol.init failed!");

    if (!root().openRoot(&volume)) error("openRoot failed");
}

SdFile& root() {
    return ::root;
}

} // namespace sd

//
//
//void setup() {
//    Serial.begin(9600);
//    // On the Ethernet Shield, CS is pin 4. It's set as an output by default.
//	// Note that even if it's not used as the CS pin, the hardware SS pin
//	// (10 on most Arduino boards, 53 on the Mega) must be left as an output
//	// or the SD library functions will not work.
//	pinMode(SLAVE_SELECT, OUTPUT);     // change this to 53 on a mega
//	digitalWrite(SLAVE_SELECT, HIGH);  // Disable W5100 Ethernet
//
//    Serial.println("Starting...");
//    // Initialize SdFat or print a detailed error message and halt
//    // Use half speed like the native library.
//    // change to SPI_FULL_SPEED for more performance.
//    if (!sd.begin(SD_CHIP_SELECT, SPI_HALF_SPEED)) sd.initErrorHalt();
//
//    // open the file for write at end like the Native SD library
//    if (!myFile.open("test.txt", O_RDWR | O_CREAT | O_AT_END)) {
//      sd.errorHalt("opening test.txt for write failed");
//    }
//    // if the file opened okay, write to it:
//    Serial.print("Writing to test.txt...");
//    myFile.println("testing 1, 2, 3.");
//
//    // close the file:
//    myFile.close();
//    Serial.println("done.");
//}
//
//
//
//void loop() {
//}
//
//
