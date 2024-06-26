#ifndef MQTT_INCLUDED
#define MQTT_INCLUDED

#include "../settings/settings.h"

void updateMqttTopic();
void MQTTreconnect();
void callback(char* topic, byte* payload, unsigned int length);

//=============================================================================================================================
void updateMqttTopic()
{
  snprintf (RSSI_Topic, MSG_BUFFER_SIZE, "%s/System/RSSI", MQTTdeviceID);
  snprintf (batterytopic, MSG_BUFFER_SIZE, "%s/System/Battery", MQTTdeviceID);
  snprintf (iptopic, MSG_BUFFER_SIZE, "%s/System/IP", MQTTdeviceID);

  snprintf (buzzertopic, MSG_BUFFER_SIZE, "%s/System/Buzzer", MQTTdeviceID);
  snprintf (lighttopic, MSG_BUFFER_SIZE, "%s/System/Light", MQTTdeviceID);
  snprintf (AlarmThresholdTopic, MSG_BUFFER_SIZE, "%s/System/AlarmThreshold", MQTTdeviceID);
  snprintf (ConvFactorTopic, MSG_BUFFER_SIZE, "%s/System/ConversionFactor", MQTTdeviceID);
  snprintf (IntTimeTopic, MSG_BUFFER_SIZE, "%s/System/Integration_Time", MQTTdeviceID);
  snprintf (MQTTUpdateTimeTopic, MSG_BUFFER_SIZE, "%s/System/MQTTUpdate_Time", MQTTdeviceID);

  snprintf (CPMTopic, MSG_BUFFER_SIZE, "%s/Doserate/CPM", MQTTdeviceID);
  snprintf (Doserate_uSv_Topic, MSG_BUFFER_SIZE, "%s/Doserate/uSv_hr", MQTTdeviceID);
  snprintf (Doserate_uR_Topic, MSG_BUFFER_SIZE, "%s/Doserate/uR_hr", MQTTdeviceID);
  snprintf (DoseLevelTopic, MSG_BUFFER_SIZE, "%s/Doserate/DoseLevel", MQTTdeviceID);

  snprintf (BuzzerCommandTopic, MSG_BUFFER_SIZE, "%s/Control/Buzzer", MQTTdeviceID);
  snprintf (LightCommandTopic, MSG_BUFFER_SIZE, "%s/Control/Light", MQTTdeviceID);
  snprintf (AlarmThresholdCommandTopic, MSG_BUFFER_SIZE, "%s/Control/AlarmThreshold", MQTTdeviceID);
  snprintf (ConvFactorCommandTopic, MSG_BUFFER_SIZE, "%s/Control/ConversionFactor", MQTTdeviceID);
  snprintf (IntTimeCommandTopic, MSG_BUFFER_SIZE, "%s/Control/Integration_Time", MQTTdeviceID);
  snprintf (MQTTUpdateTimeCommandTopic, MSG_BUFFER_SIZE, "%s/Control/MQTTUpdate_Time", MQTTdeviceID);

  snprintf (CommandTopic, MSG_BUFFER_SIZE, "%s/Control/#", MQTTdeviceID );
  snprintf (LWTTopic, MSG_BUFFER_SIZE, "%s/System/LWT", MQTTdeviceID );
}

void MQTTreconnect() 
{
  #if DEBUG_MODE && DEBUG_MQTT
   Serial.print("Attempting MQTT connection.");
  #endif

  MQTTattempts = 0;

  while ((!MQTTclient.connected()) && (MQTTattempts < 3))                 // Loop until we're reconnected
  {   
    MQTTclient.connect(MQTTdeviceID, MQTTlogin, MQTTpassword, LWTTopic, 0, false, "Offline", true);
    MQTTattempts ++;
    #if DEBUG_MODE && DEBUG_WiFi
      Serial.print(".");
    delay(10);
    #endif 
  }

  if (MQTTclient.connected())    // Attempt to connect
  {                                                          
    MQTTclient.publish(LWTTopic, "Online");

    #if DEBUG_MODE && DEBUG_MQTT
      Serial.println(" Connected!");
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
      MQTTclient.publish(buzzertopic, "1", true);
    }
    else
    {
      MQTTclient.publish(buzzertopic, "0", true);
    }

    #if DEBUG_MODE && DEBUG_MQTT
      Serial.println("MQTT >: " + String(lighttopic) + ": " + ledSwitch);
    #endif
    if (ledSwitch)
    {
      MQTTclient.publish(lighttopic, "1", true);
    }
    else
    {
      MQTTclient.publish(lighttopic, "0", true);
    }
  } 
  else 
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print(" failed, Attempts=");
      Serial.println(uint32(MQTTattempts));
    #endif

    MQTTsend = 0;
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

    if(messageTemp == "1")
    {
      buzzerSwitch = 1;
      MQTTclient.publish(buzzertopic, "1");
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("ON");
      #endif
    }
    else if(messageTemp == "0")
    {
      buzzerSwitch = 0;
      MQTTclient.publish(buzzertopic, "0");
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("OFF");
      #endif
    }
    else 
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Failed");
      #endif
    }
    
    if (page == 0)
      drawBuzzer();    
  }
//------------------------------------------------------------------
  else if (String(topic) == String(LightCommandTopic)) 
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print("Changing light to ");
    #endif

    if(messageTemp == "1")
    {
      ledSwitch = 1;
      MQTTclient.publish(lighttopic, "1");

      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("ON");
      #endif
    }
    else if(messageTemp == "0")
    {
      ledSwitch = 0;
      MQTTclient.publish(lighttopic, "0");
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("OFF");
      #endif
    }
    else 
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Error");
      #endif
    }
    if (page == 0)
      drawBuzzer();
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
        drawConvFactor();
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
        drawAlarmThreshold();
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
        case 5: 
          integrationMode = 1;
          break;
        case 180: 
          integrationMode = 2;
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
