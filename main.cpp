#include <Arduino.h>

#include <OneWire.h>
#include <DallasTemperature.h>

// Data wire is plugged into pin 2 on the Arduino
#define ONE_WIRE_BUS 17

// Setup a oneWire instance to communicate with any OneWire devices (not just Maxim/Dallas temperature ICs)
OneWire oneWire(ONE_WIRE_BUS);

// Pass our oneWire reference to Dallas Temperature.
DallasTemperature sensors(&oneWire);

DeviceAddress insideThermometer  = {0x28, 0xA7, 0x81, 0xEB, 0x03, 0x00, 0x00, 0xF8}; // yellow
DeviceAddress cabinetThermometer = {0x28, 0x70, 0x5A, 0x71, 0x03, 0x00, 0x00, 0x6D}; // red
DeviceAddress outsideThermometer = {0x28, 0xEB, 0x51, 0x71, 0x03, 0x00, 0x00, 0x54}; // blue

void setup() {
	Serial.begin(9600);
}

void reportTemperatures() {
	sensors.requestTemperatures();
	float outsideCelsiusTemperature = sensors.getTempC(outsideThermometer);
	float cabinetCelsiusTemperature = sensors.getTempC(cabinetThermometer);
	float insideCelsiusTemperature = sensors.getTempC(insideThermometer);

	Serial.print("outside = ");
	Serial.println(outsideCelsiusTemperature);

	Serial.print("cabinet = ");
	Serial.println(cabinetCelsiusTemperature);

	Serial.print("inside = ");
	Serial.println(insideCelsiusTemperature);
}

void loop() {
	reportTemperatures();
	delay(1000);
}
