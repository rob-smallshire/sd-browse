/*
 * clock.cpp
 *
 *  Created on: 10 Nov 2013
 *      Author: rjs
 */


#include <Arduino.h>

#include <I2C.h>
#include <DS1307.h>

void testClock() {
    I2c.begin();
    I2c.timeOut(30000);
    I2c.pullup(false);
    Serial.begin(9600);
    DS1307::setDatetime(DateTime(__DATE__, __TIME__));
    for (int i = 0; i < 56; ++i) {
    	DS1307::writeByte(i, i * 2);
    }
    DS1307::setOutput(false);
    DS1307::setSquareWave(false);
    DS1307::setRate(DS1307::RATE_1_HZ);

    DateTime now = DS1307::getDatetime();

    Serial.print(now.year(), DEC);
    Serial.print('/');
    Serial.print(now.month(), DEC);
    Serial.print('/');
    Serial.print(now.day(), DEC);
    Serial.print(' ');
    Serial.print(now.hour(), DEC);
    Serial.print(':');
    Serial.print(now.minute(), DEC);
    Serial.print(':');
    Serial.print(now.second(), DEC);
    Serial.println();

    Serial.print(" since midnight 1/1/1970 = ");
    Serial.print(now.unixtime());
    Serial.print("s = ");
    Serial.print(now.unixtime() / 86400L);
    Serial.println("d");

    for (int i = 0; i < 56 ; ++i) {
    	uint8_t data;
    	DS1307::readByte(i, data);
    	Serial.print(i);
    	Serial.print(" : ");
    	Serial.println(data);
    }

    bool output = DS1307::getOutput();
    Serial.println(output);

    bool square_wave = DS1307::getSquareWave();
    Serial.println(square_wave);

    DS1307::Rate rate = DS1307::getRate();
    Serial.println(rate);

    delay(1000);

}
