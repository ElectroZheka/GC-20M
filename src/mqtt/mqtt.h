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

    if (MQTTclient.connect(MQTTdeviceID, MQTTlogin, MQTTpassword, LWTTopic, 0, false, "Offline", true))    // Attempt to connect
    {                                                          
      MQTTclient.publish(LWTTopic, "Online");

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Connected!");
      #endif

      MQTTsend = 1;

      MQTTclient.subscribe(CommandTopic);                                 // ... and resubscribe

      MQTTclient.publish(iptopic, (WiFi.localIP().toString().c_str()), true);
      snprintf (msg, MSG_BUFFER_SIZE, "%i", (WiFi.RSSI()));
      MQTTclient.publish(RSSI_Topic, msg);

      snprintf (msg, MSG_BUFFER_SIZE, "%li", conversionFactor);                             
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("MQTT >: "+ String(ConvFactorTopic) + (": ") + msg);
      #endif
      MQTTclient.publish(ConvFactorTopic, msg, true);

      switch(integrationMode)
      {
        case 0: 
          value = 60;
          break;
        case 1: 
          value = 5;
          break;
        case 2: 
          value = 180;
          break;
        default:
          break;
      }

      snprintf (msg, MSG_BUFFER_SIZE, "%i", int(value));
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("MQTT >: " + String(IntTimeTopic) + ": " + msg);
      #endif
      MQTTclient.publish(IntTimeTopic, msg, true);

      snprintf (msg, MSG_BUFFER_SIZE, "%i", alarmThreshold);                             
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("MQTT >: "+ String(AlarmThresholdTopic) + ": " + msg);
      #endif
      MQTTclient.publish(AlarmThresholdTopic, msg, true);

      snprintf (msg, MSG_BUFFER_SIZE, "%i", MQTTUpdateTime);                             
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("MQTT >: "+ String(MQTTUpdateTimeTopic) + ": " + msg);
      #endif
      MQTTclient.publish(MQTTUpdateTimeTopic, msg, true);

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("MQTT >: " + String(buzzertopic) + ": " + buzzerSwitch);
      #endif
      if (buzzerSwitch)
      {
      //  MQTTclient.publish(lighttopic, ((char)buzzerSwitch), true);
        MQTTclient.publish(lighttopic, "true", true);
      }
      else
      {
        MQTTclient.publish(lighttopic, "false", true);
      }

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("MQTT >: " + String(lighttopic) + ": " + ledSwitch);
      #endif
      if (ledSwitch)
      {
        MQTTclient.publish(lighttopic, "true", true);
      }
      else
      {
        MQTTclient.publish(lighttopic, "false", true);
      }
    } 
    else 
    {
      MQTTattempts++;
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("failed, Attempts=");
        Serial.println(uint32(MQTTattempts));
      #endif

      MQTTsend = 0;
    }
  }
}
//                                                              void MQTTreconnect() 
//=============================================================================================================================
void callback(char* topic, byte* payload, unsigned int length)
{
  #if DEBUG_MODE && DEBUG_MQTT
    Serial.print("MQTT <: [");
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
//------------------------------------------------------------------
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
//------------------------------------------------------------------
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
//------------------------------------------------------------------
  else if (String(topic) == String(ConvFactorCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(conversionFactor))
    {
      conversionFactor = atoi(messageTemp.c_str());
      EEPROMWritelong(saveCalibration, conversionFactor);
      EEPROM.commit();

      snprintf (msg, MSG_BUFFER_SIZE, "%li", conversionFactor);
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
//------------------------------------------------------------------
  else if (String(topic) == String(AlarmThresholdCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(alarmThreshold))
    {
      alarmThreshold = atoi(messageTemp.c_str());
      EEPROM.write(saveAlarmThreshold, alarmThreshold);
      EEPROM.commit();

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
//------------------------------------------------------------------
  else if (String(topic) == String(IntTimeCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(integrationMode))
    {
      switch(atoi(messageTemp.c_str()))
      {
        case 60: 
          integrationMode = 0;
          break;
        case 180: 
          integrationMode = 2;
          break;
        case 5: 
          integrationMode = 1;
          break;
        default:
          break;
      }

//      EEPROMWritelong(saveIntegrationMode, integrationMode);
//      EEPROM.commit();

      //snprintf (msg, MSG_BUFFER_SIZE, "%i", integrationMode);
      //snprintf (msg, MSG_BUFFER_SIZE, "%i", messageTemp);
      snprintf (msg, MSG_BUFFER_SIZE, (messageTemp.c_str()));
      MQTTclient.publish(IntTimeTopic, msg, true);

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("Changing integrationMode to ");
        Serial.println(integrationMode);
      #endif
   
      if (page == 0)     //
      {
        tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
        tft.setFont(&FreeSans12pt7b);
        tft.setTextSize(1);
        tft.setCursor(180, 283);
        tft.println("INT");

        switch(integrationMode)
        {
          case 0: 
            tft.setCursor(177, 309);
            tft.println("60 s");
            value = 60;
            break;
          case 1: 
            tft.setCursor(184, 309);
            tft.println("5 s");
            value = 5;
            break;
          case 2: 
            tft.setCursor(169, 309);
            tft.println("180 s");
            value = 180;
            break;
          default:
            break;
        }
      }
    }
  }
//------------------------------------------------------------------
  else if (String(topic) == String(MQTTUpdateTimeCommandTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(MQTTUpdateTime))
    {
      MQTTUpdateTime = atoi(messageTemp.c_str());
      EEPROM.write(saveMQTTUpdateTime, MQTTUpdateTime);
      EEPROM.commit();

      snprintf (msg, MSG_BUFFER_SIZE, "%i", MQTTUpdateTime);
      MQTTclient.publish(MQTTUpdateTimeTopic, msg, true);

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.print("Changing MQTTUpdateTime to ");
        Serial.println(MQTTUpdateTime);
      #endif
   
      // if (page == 3)     // if alarmThreshold page
      // {
      //   tft.setFont();
      //   tft.setTextSize(3);
      //   tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
      //   tft.setCursor(151, 146);
      //   tft.println(MQTTUpdateTime);
      //   if (MQTTUpdateTime < 10)
      //  tft.fillRect(169, 146, 22, 22, ILI9341_BLACK);
      // }
    }
  }
}                                                        //void callback
#endif
