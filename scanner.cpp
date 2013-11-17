/*
 * scanner.cpp
 *
 *  Created on: 4 Nov 2013
 *      Author: rjs
 */

#include <Arduino.h>

#include <I2C.h>

#include "scanner.hpp"

// Found device at address -  0x28 - Servo controller
// Found device at address -  0x48 - Fan Controller B
// Found device at address -  0x4B - Fan Controller A
// Found device at address -  0x68 - DS1307 RTC

void scanI2C() {
	Serial.begin(9600);
	Serial.println("Scanning...");
	I2c.begin();
	I2c.timeOut(100);
	I2c.scan();
	Serial.println("done!");
}


