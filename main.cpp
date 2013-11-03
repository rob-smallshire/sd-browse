#include <Arduino.h>

#include "temperature.hpp"
#include "humidity.hpp"


void setup() {
	Serial.begin(9600);
}


void loop() {
	reportTemperatures();
	reportHumidity();
	delay(1000);
}
