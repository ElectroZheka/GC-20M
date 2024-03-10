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

    //String clientId = "ESP_Dosimeter";
    String clientId = String(MQTTdeviceID);
    if (MQTTclient.connect(clientId.c_str(), MQTTlogin, MQTTpassword))    // Attempt to connect
    {                                                          
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("Connected");
      #endif
//      MQTTclient.subscribe(buzzertopic);                                // ... and resubscribe
//      MQTTclient.subscribe(lighttopic);                                 // ... and resubscribe
      MQTTclient.subscribe(ConvFactorTopic);                              // ... and resubscribe
      MQTTclient.publish(iptopic, (WiFi.localIP().toString().c_str()));
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
    Serial.print("Message arrived [");
    Serial.print(topic);
    Serial.print("] ");
  #endif

  String messageTemp;
  
  for (unsigned int i = 0; i < length; i++)
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print((char)payload[i]);
    #endif
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == String(buzzertopic)) 
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print("Changing buzzer to ");
    #endif
    if(messageTemp == "true")
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("on");
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
        Serial.println("off");
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
  else if (String(topic) == String(lighttopic)) 
  {
    #if DEBUG_MODE && DEBUG_MQTT
      Serial.print("Changing light to ");
    #endif

    if(messageTemp == "true")
    {
      #if DEBUG_MODE && DEBUG_MQTT
        Serial.println("on");
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
        Serial.println("off");
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
  else if (String(topic) == String(ConvFactorTopic)) 
  {
    if (atoi(messageTemp.c_str()) != int(conversionFactor))
    {
      EEPROMWritelong(saveCalibration, conversionFactor);
      EEPROM.commit();
      conversionFactor = atoi(messageTemp.c_str());

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
}                                                        //void callback
#endif