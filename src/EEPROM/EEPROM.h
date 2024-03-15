#ifndef EEPROM_INCLUDED
#define EEPROM_INCLUDED

#include "../settings/settings.h"

//long EEPROMReadlong(long address);                        // EEPRON Read
long EEPROMReadlong(int address);                        // EEPRON Read
//unsigned int EEPROMReadlong(int address);
void EEPROMWritelong(int address, long value);            // EEPRON Write
//void EEPROMWritelong(int address, unsigned int value);           // EEPRON Write
//=============================================================================================================================
long EEPROMReadlong(int address) 
//unsigned int EEPROMReadlong(int address)
{ 
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);

//  #if DEBUG_MODE && DEBUG_EEPROM
//    Serial.println("EEPROM: Read address: "+ String(address) + " Value: "+ String((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF));
//  #endif

  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}
//=============================================================================================================================
void EEPROMWritelong(int address, long value) 
//void EEPROMWritelong(int address, unsigned int value) 
{
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
 
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);

//  #if DEBUG_MODE && DEBUG_EEPROM
//    Serial.println("EEPROM: Write address: "+ String(address) + " Value: "+ String(value));
//  #endif
}
//=============================================================================================================================
#endif