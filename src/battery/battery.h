#ifndef BATTERY_INCLUDED
#define BATTERY_INCLUDED

#include <Arduino.h>
//#include <ESP8266WiFi.h>
#include "../settings/settings.h"

float getVoltage();
float getPercent();

#endif