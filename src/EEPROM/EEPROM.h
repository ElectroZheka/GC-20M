#ifndef EEPROM_INCLUDED
#define EEPROM_INCLUDED

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include "../settings/settings.h"

long EEPROMReadlong(int address);                         // EEPRON Read
void EEPROMWritelong(int address, long value);            // EEPRON Write

//=============================================================================================================================

#endif