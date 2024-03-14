#ifndef MQTT_INCLUDED
#define MQTT_INCLUDED

#include "../settings/settings.h"

void MQTTreconnect() 
{
  MQTTattempts = 0;
  while ((!MQTTclient.connected()) && (MQTTattempts < 3))                 // Loop until we're reconnected
  {   
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print("Attempting MQTT connection...");
    #endif

    String clientId = String(MQTTdeviceID);

//    if (MQTTclient.connect(clientId.c_str(), MQTTlogin, MQTTpassword))    // Attempt to connect
    if (MQTTclient.connect(clientId.c_str(), MQTTlogin, MQTTpassword, LWTTopic, 0, false, "Offline", true))    // Attempt to connect
    {                                                          
      MQTTclient.publish(LWTTopic, "Online");

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Connected");
      #endif

      MQTTclient.subscribe(CommandTopic);                                 // ... and resubscribe

      MQTTclient.publish(iptopic, (WiFi.localIP().toString().c_str()), true);
      snprintf (msg, MSG_BUFFER_SIZE, "%i", (WiFi.RSSI()));
      MQTTclient.publish(RSSI_Topic, msg);
    } 
    else 
    {
      MQTTattempts++;
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("failed, Attempts=");
        Serial.println(uint32(MQTTattempts));
      #endif
    }
  }
}
//                                                              void MQTTreconnect() 
//=============================================================================================================================
void callback(char* topic, byte* payload, unsigned int length)
{
  #if DEBUG_MODE && DEBUG_MQTT
    Serial.print("MQTT: Message arrived [");
    Serial.print(topic);
    Serial.print("] [");
  #endif

  String messageTemp;
  
  for (unsigned int i = 0; i < length; i++)
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print((char)payload[i]);
    #endif
    messageTemp += (char)payload[i];
  }

  #if DEBUG_MODE && DEBUG_MQTT
    Serial.println("]");
  #endif

  if (String(topic) == String(BuzzerCommandTopic)) 
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print("Changing buzzer to ");
    #endif
    if(messageTemp == "true")
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("ON");
      #endif

      buzzerSwitch = true;
      if (page == 0)
      {
        tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
        tft.drawBitmap(190, 208, buzzerOnBitmap, 45, 45, ILI9341_WHITE);
      }
      MQTTclient.publish(buzzertopic, "true");
    }
    else if(messageTemp == "false")
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("OFF");
      #endif
      buzzerSwitch = false;
      if (page == 0)
      {
        tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
        tft.drawBitmap(190, 208, buzzerOffBitmap, 45, 45, ILI9341_WHITE);
      }
      MQTTclient.publish(buzzertopic, "false");
    }
    else 
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Failed");
      #endif
    }
  }
  else if (String(topic) == String(LightCommandTopic)) 
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print("Changing light to ");
    #endif

    if(messageTemp == "true")
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("ON");
      #endif

      ledSwitch = true;
      if (page == 0)
      {
        tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
        tft.drawBitmap(190, 153, ledOnBitmap, 45, 45, ILI9341_WHITE);
      }
      MQTTclient.publish(lighttopic, "true");
    }
    else if(messageTemp == "false")
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("OFF");
      #endif
      ledSwitch = false;
      if (page == 0)
      {
        tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
        tft.drawBitmap(190, 153, ledOffBitmap, 45, 45, ILI9341_WHITE);
      }
      MQTTclient.publish(lighttopic, "false");
    }
    else 
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Error");
      #endif
    }
  } 
  else if (String(topic) == String(ConvFactorCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(conversionFactor))
    {
      conversionFactor = atoi(messageTemp.c_str());
      EEPROMWritelong(saveCalibration, conversionFactor);
      EEPROM.commit();

      snprintf (msg, MSG_BUFFER_SIZE, "%i", conversionFactor);
      MQTTclient.publish(ConvFactorTopic, msg);

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("Changing ConversionFactor to ");
        Serial.println(conversionFactor);
      #endif
     
      if (page == 4)     // if calibration page
      {
        tft.setFont();
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(161, 146);
        tft.println(conversionFactor);
        if (conversionFactor < 100)
          tft.fillRect(197, 146, 22, 22, ILI9341_BLACK);
      }
    }
  }
  else if (String(topic) == String(AlarmThresholdCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(alarmThreshold))
    {
      EEPROMWritelong(saveAlarmThreshold, alarmThreshold);
      EEPROM.commit();
      alarmThreshold = atoi(messageTemp.c_str());

      snprintf (msg, MSG_BUFFER_SIZE, "%i", alarmThreshold);
      MQTTclient.publish(AlarmThresholdTopic, msg, true);

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("Changing alarmThreshold to ");
        Serial.println(alarmThreshold);
      #endif
   
      if (page == 3)     // if alarmThreshold page
      {
        tft.setFont();
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(151, 146);
        tft.println(alarmThreshold);
        if (alarmThreshold < 10)
       tft.fillRect(169, 146, 22, 22, ILI9341_BLACK);
      }
    }
  }
  else if (String(topic) == String(IntTimeCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(integrationMode))
    {
//      EEPROMWritelong(saveIntegrationMode, integrationMode);
//      EEPROM.commit();
      if (atoi(messageTemp.c_str()) == 60) 
      {
        integrationMode = 0;
        //integrationMode = atoi(messageTemp.c_str());
      }
      if (atoi(messageTemp.c_str()) == 180) 
      {
        integrationMode = 2;
        //integrationMode = atoi(messageTemp.c_str());
      }
      if (atoi(messageTemp.c_str()) == 5) 
      {
        integrationMode = 1;
        //integrationMode = atoi(messageTemp.c_str());
      }
      //snprintf (msg, MSG_BUFFER_SIZE, "%i", integrationMode);
      //snprintf (msg, MSG_BUFFER_SIZE, "%i", messageTemp);
      snprintf (msg, MSG_BUFFER_SIZE, (messageTemp.c_str()));
      MQTTclient.publish(IntTimeTopic, msg, true);
      //MQTTclient.publish(IntTimeTopic, String(messageTemp), true);

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("Changing integrationMode to ");
        Serial.println(integrationMode);
      #endif
   
      if (page == 0)     //
      {
        if (integrationMode == 0) // change button based on touch and previous state
        {
          tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
          tft.setFont(&FreeSans12pt7b);
          tft.setTextSize(1);
          tft.setCursor(180, 283);
          tft.println("INT");
          tft.setCursor(177, 309);
          tft.println("60 s");
        }
        else if (integrationMode == 1)
        {
          tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
          tft.setFont(&FreeSans12pt7b);
          tft.setTextSize(1);
          tft.setCursor(180, 283);
          tft.println("INT");
          tft.setCursor(184, 309);
          tft.println("5 s");
        }
        else if (integrationMode == 2)
        {
          tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
          tft.setFont(&FreeSans12pt7b);
          tft.setTextSize(1);
          tft.setCursor(180, 283);
          tft.println("INT");
          tft.setCursor(169, 309);
          tft.println("180 s");
        }
      }
    }
  }
}                                                        //void callback
#endif
