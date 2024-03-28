#include "battery.h"

float getVoltage()
{
//  batteryInput = analogRead(BATT_PIN);
//  batteryVoltage = (batteryInput / 1024.0) * 5.3;
  float batteryVoltage = (analogRead(BATT_PIN) / 1024.0) * 5.3; 
  batteryVoltage = constrain(batteryVoltage, 2.5, 4.3);

  #if DEBUG_MODE && DEBUG_BATT
//    Serial.println("BATT: ADC: " + String(batteryInput));
    Serial.println("BATT: Voltage: " + String(batteryVoltage) + "V");
  #endif

  return batteryVoltage;
}

float getPercent()
{
  float batteryVoltage = (analogRead(BATT_PIN) / 1024.0) * 5.3; 
  batteryVoltage = constrain(batteryVoltage, 2.5, 4.3);
  float batteryPercent = map(batteryVoltage, 2.8, 4.2, 0, 100);

  #if DEBUG_MODE && DEBUG_BATT
    Serial.println("BATT: " + String(batteryVoltage) + "V  " + String(batteryPercent) + "%");
  #endif

  return batteryPercent;
}