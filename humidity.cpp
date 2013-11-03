/*
 * humidity.cpp
 *
 *  Created on: 3 Nov 2013
 *      Author: rjs
 */

#include "humidity.hpp"

#include <DHT.h>

namespace
{

#define DHTPIN 16
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

// dewPoint function NOAA
// reference: http://wahiduddin.net/calc/density_algorithms.htm
double dewPoint(double celsius, double humidity)
{
	double A0= 373.15/(273.15 + celsius);
	double SUM = -7.90298 * (A0-1);
	SUM += 5.02808 * log10(A0);
	SUM += -1.3816e-7 * (pow(10, (11.344*(1-1/A0)))-1) ;
	SUM += 8.1328e-3 * (pow(10,(-3.49149*(A0-1)))-1) ;
	SUM += log10(1013.246);
	double VP = pow(10, SUM-3) * humidity;
	double T = log(VP/0.61078);   // temp var
	return (241.88 * T) / (17.558-T);
}

} // namespace

void reportHumidity() {
	float humidity = dht.readHumidity();
	float temperature = dht.readTemperature();

	Serial.print("cabinet humidity = ");
	Serial.println(humidity);

	Serial.print("cabinet temperature = ");
	Serial.println(temperature);


	Serial.print("dew point temperature = ");
	Serial.println(dewPoint(temperature, humidity));
}
