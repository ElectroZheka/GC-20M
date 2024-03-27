/*  SBM-20 based Geiger Counter
    Author: Prabhat    Email: pra22@pitt.edu
    Author: ElectroZheka
    Sketch for ESP8266 that counts clicks from the Geiger tube, calculates the counts per minute, displays information 
    on a TFT touchscreen and send data to MQTT server.
    Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
//#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WifiManager.h>
#include <EEPROM.h>
#include "SPI.h"
#include "Adafruit_GFX.h"
#include <Fonts/FreeSans9pt7b.h>
#include <Fonts/FreeSans12pt7b.h>
#include "Adafruit_ILI9341.h"
#include <XPT2046_Touchscreen.h>
#include <Wire.h>
#include <PubSubClient.h>
#include "settings/settings.h"
#include "Bitmap/Bitmap.h"
#include "battery/battery.h"

//#include "FontsRus/FreeSans9pt7b.h"
//#include "FontsRus/FreeSans12pt7b.h"

//=============================================================================================================================
// WiFi variables
unsigned long currentUploadTime;
unsigned long previousUploadTime;
int passwordLength;
int SSIDLength;
int MQTTserverLength;
int DeviceIDLength;
int MQTTportLength;
int MQTTloginLength;
int MQTTpassLength;
char ssid[20];
char password[20];
WiFiClient client;
//=============================================================================================================================
// MQTT variables
char MQTTdeviceID[20];                      // = "Esp_Dosimeter"; 
char MQTTserverIP[15];                      // = "192.168.1.1";
char MQTTport[5];    
char MQTTlogin[20];                         //  
char MQTTpassword[20];                      // 

int attempts;                               // number of connection attempts when device starts up in monitoring mode
int MQTTattempts;                           // number of MQTT connection attempts when device starts up in monitoring mode
bool previousMQTTstatus;
bool MQTTsend;                              // Флаг возможности отправки на MQTT сервер
unsigned int MQTTUpdateTime = 15;      // Интервал отправки данных на MQTT сервер

//String subscr_topic = "EspDosimeter/Control/";
//String prefix = "EspDosimeter/";
//String buf_recv;

PubSubClient MQTTclient(client);
//#define MSG_BUFFER_SIZE	(50)

char msg[MSG_BUFFER_SIZE];
char topic[MSG_BUFFER_SIZE];
char buzzertopic[MSG_BUFFER_SIZE];
char lighttopic[MSG_BUFFER_SIZE];
char iptopic[MSG_BUFFER_SIZE];
char RSSI_Topic[MSG_BUFFER_SIZE];
char batterytopic[MSG_BUFFER_SIZE];
char ConvFactorTopic[MSG_BUFFER_SIZE];
char CPMTopic[MSG_BUFFER_SIZE];
char Doserate_uSv_Topic[MSG_BUFFER_SIZE];
char Doserate_uR_Topic[MSG_BUFFER_SIZE];
char DoseLevelTopic[MSG_BUFFER_SIZE];
char AlarmThresholdCommandTopic[MSG_BUFFER_SIZE];
char IntTimeTopic[MSG_BUFFER_SIZE];
char MQTTUpdateTimeTopic[MSG_BUFFER_SIZE];
char CommandTopic[MSG_BUFFER_SIZE];
char BuzzerCommandTopic[MSG_BUFFER_SIZE];
char LightCommandTopic[MSG_BUFFER_SIZE];
char ConvFactorCommandTopic[MSG_BUFFER_SIZE];
char AlarmThresholdTopic[MSG_BUFFER_SIZE];
char IntTimeCommandTopic[MSG_BUFFER_SIZE];
char MQTTUpdateTimeCommandTopic[MSG_BUFFER_SIZE];
char LWTTopic[MSG_BUFFER_SIZE];
float value = 0;
//=============================================================================================================================
// Display variable
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

unsigned long count[61];
unsigned long fastCount[6];                   // arrays to store running counts
unsigned long slowCount[181];
int i = 0;                           // array elements
int j = 0;
int k = 0;

int page = 0;
//=============================================================================================================================
// Configuratin variable
long currentMillis;                  // Секундный таймер
long previousMillis;                 // Секундный таймер
unsigned long currentMicros;         // Таймер одновибратора
unsigned long previousMicros;        // Таймер одновибратора
bool deviceMode;                     // 0 = Portable, 1 = Station
bool ledSwitch = 1;
bool buzzerSwitch = 1;
unsigned int integrationMode = 0;             // 0 = medium, 1 = fast, 2 == slow;
unsigned int alarmThreshold = 5;
//=============================================================================================================================
// Geiger counter variable
unsigned long averageCount;
unsigned long previousaverageCount;
unsigned long currentCount;          // incremented by interrupt
unsigned long previousCount;         // to activate buzzer and LED
unsigned long cumulativeCount;
float doseRate;
float previousdoseRate;
float totalDose;

float doseRate_uSv;
float totalDose_uSv;
float doseRate_uR;
float totalDose_uR;

char dose[5];
int doseLevel;                       // determines home screen warning signs
int previousDoseLevel;
bool doseUnits = 0;                  // 0 = Sievert, 1 = Rem
//unsigned int conversionFactor = 575; //175;
//long conversionFactor = 575; //175;
unsigned long conversionFactor;// = 525; //175;
float GeigerDeadTime = 0.000190;
//unsigned int GeigerDeadTime_uS = 190;
long TestMicros;                      // Переменная для замера времени выполнения кода
//=============================================================================================================================
// Touchscreen variable
XPT2046_Touchscreen ts(CS_PIN);
bool wasTouched;
int x, y;                           // touch points
//=============================================================================================================================
// Battery indicator variables
//int batteryInput;
//unsigned int batteryInput;
//float batteryVoltage;
int batteryPercent;
int previousbatteryPercent = 150;    // >100 for first update display when boot
int batteryMapped = 212;            // pixel location of battery icon
int batteryUpdateCounter = 29;
//=============================================================================================================================
// EEPROM variables
const int saveUnits = 0;
const int saveAlarmThreshold = 1;   // Addresses for storing settings data in the EEPROM
const int saveCalibration = 2;
const int saveDeviceMode = 6;
const int saveLoggingMode = 7;
const int saveSSIDLen = 8;
const int savePWLen = 9;
const int saveIPLen = 10;
const int saveIDLen = 11;
const int savePortLen = 12;
const int saveMLoginLen = 13;
const int saveMPassLen = 14;
const int saveMQTTUpdateTime = 15;
//const int saveLedSwitch = 15;
//const int saveBuzzerSwitch = 16;
//const int saveIntegrationMode = 17;             // 0 = medium, 1 = fast, 2 == slow;
//const int saveAlarmThreshold = 18;
//const int saveDoseUnits = 0;                  // 0 = Sievert, 1 = Rem
//const int saveConversionFactor = 575;
//=============================================================================================================================
// Timed Count Variables:
int interval = 5;
unsigned long intervalMillis;
unsigned long startMillis;
unsigned long elapsedTime;
int progress;
float cpm;
bool completed = 0;
int intervalSize;                    // stores how many digits are in the interval
//=============================================================================================================================
// Logging variables
bool isLogging;
//=============================================================================================================================
// interrupt routine declaration
const int interruptPin = 5;
unsigned int previousIntMicros;              // timers to limit count increment rate in the ISR
void isr();
//=============================================================================================================================
#include "display/display.h"
#include "EEPROM/EEPROM.h"
#include "mqtt/mqtt.h"
//=============================================================================================================================
void IRAM_ATTR isr() // interrupt service routine
{
  if ((micros() - Dead_Time_Geiger) > previousIntMicros) 
  {   
    currentCount++;
    cumulativeCount++;
  }
  previousIntMicros = micros();
}
//=============================================================================================================================
//=============================================================================================================================
void setup()
{
  pinMode(ACT_LED, OUTPUT);            // LED
  pinMode(BUZZER_PIN, OUTPUT);         // Buzzer
  digitalWrite(ACT_LED, LOW);
  digitalWrite(BUZZER_PIN, LOW);
   
  Serial.begin(DEBUG_BAUD);

  delay(100);
 
  #if DEBUG_MODE
    Serial.println(" ");
    Serial.println(" ");
    Serial.println("==================================================================");
    Serial.println("GC-20M Starting...");
  #endif

  ts.begin();
  ts.setRotation(0);

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);

  EEPROM.begin(140);   // initialize emulated EEPROM sector 140 byte

  #if DEBUG_MODE && DEBUG_EEPROM
    Serial.print("Reading settings from EEPROM... ");
  #endif

  doseUnits = EEPROM.read(saveUnits);                                //Address = 0
  alarmThreshold = EEPROM.read(saveAlarmThreshold);                  //Address = 1
  conversionFactor = EEPROMReadlong(saveCalibration);                //Address = 2
  deviceMode = EEPROM.read(saveDeviceMode);                          //Address = 6
  isLogging = EEPROM.read(saveLoggingMode);                          //Address = 7
  SSIDLength = EEPROM.read(saveSSIDLen);                             //Address = 8
  passwordLength = EEPROM.read(savePWLen);                           //Address = 9
  MQTTserverLength = EEPROM.read(saveIPLen);                         //Address = 10
  DeviceIDLength = EEPROM.read(saveIDLen);                           //Address = 11
  MQTTportLength = EEPROM.read(savePortLen);                         //Address = 12
  MQTTloginLength = EEPROM.read(saveMLoginLen);                      //Address = 13
  MQTTpassLength = EEPROM.read(saveMPassLen);                        //Address = 14
  MQTTUpdateTime = EEPROM.read(saveMQTTUpdateTime);                  //Address = 15

  for (int i = 20; i < 20 + SSIDLength; i++)
  {
    ssid[i - 20] = EEPROM.read(i);
  }

  for (int i = 40; i < 40 + passwordLength; i++)
  {
    password[i - 40] = EEPROM.read(i);
  }

  for (int i = 60; i < 60 + DeviceIDLength; i++)
  {
    MQTTdeviceID[i - 60] = EEPROM.read(i);
  }

  for (int i = 80; i < 80 + MQTTserverLength; i++)
  {
    MQTTserverIP[i - 80] = EEPROM.read(i);
  }
 
  for (int i = 95; i < 95 + MQTTportLength; i++)
  {
    MQTTport[i - 95] = EEPROM.read(i);
  }

  for (int i = 100; i < 100 + MQTTloginLength; i++)
  {
    MQTTlogin[i - 100] = EEPROM.read(i);
  }

  for (int i = 120; i < 120 + MQTTpassLength; i++)
  {
    MQTTpassword[i - 120] = EEPROM.read(i);
  }

  #if DEBUG_MODE && DEBUG_EEPROM
    Serial.println("Complete");
    Serial.println("------------------------------------------------------------------");
    Serial.print("Device Mode:          ");  
    switch(deviceMode)
    {
      case 0: 
        Serial.println("Portable Dosimeter");
        break;
      case 1: 
        Serial.println("MQTT Monitoring Station"); 
        break;
    }     
    Serial.print("Dose Units:           ");
    switch(doseUnits)
    {
      case 0: 
        Serial.println("Sieverts (uSv/hr)");
        break;
      case 1: 
        Serial.println("Rems (uR/hr)"); 
        break;
    }     
    Serial.println("Alarm Threshold:      " + String(alarmThreshold));  
    Serial.println("Conversion Factor:    " + String(conversionFactor)); 
//    Serial.println("Logging:              " + String(isLogging));
    Serial.println("MQTT Update Interval: " + String(MQTTUpdateTime) + " second"); 
    Serial.println("SSID:                 " + String(ssid));// + "   L: "+ String(SSIDLength));
    Serial.println("Password:             " + String(password));// + "   L: "+ String(passwordLength));
    Serial.println("MQTT DeviceID:        " + String(MQTTdeviceID));// + "   L: "+ String(DeviceIDLength));
    Serial.println("MQTT Server:          " + String(MQTTserverIP));// + "   L: "+ String(MQTTserverLength));
    Serial.println("MQTT Port:            " + String(MQTTport));// + "   L: "+ String(MQTTportLength));
    Serial.println("MQTT Login:           " + String(MQTTlogin));// + "   L: "+ String(MQTTloginLength));
    Serial.println("MQTT Pass:            " + String(MQTTpassword));// + "   L: "+ String(MQTTpassLength));    
    Serial.println("==================================================================");
  #endif

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

  attachInterrupt(interruptPin, isr, FALLING);

  drawHomePage();

  if (!deviceMode)
  {
    #if DEBUG_MODE
      Serial.println("CG-20M in Portable Dosimeter Mode.");
    #endif

//    WiFi.mode( WIFI_OFF );                                     // turn off wifi
    WiFi.setSleepMode(WIFI_MODEM_SLEEP);
    WiFi.forceSleepBegin(); 
//    delay(1);
    MQTTsend = 0;
  }
  else
  {
    #if DEBUG_MODE
      Serial.println("CG-20M in MQTT Monitoring Station Mode.");
      Serial.print("Connecting to WiFi.. ");
    #endif

    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    drawBlankDialogueBox();
    tft.setTextSize(1);
    tft.setFont(&FreeSans9pt7b);
    tft.setTextColor(ILI9341_WHITE);
    tft.setCursor(38, 120);
    tft.println("Connecting to WiFi..");

    while ((WiFi.status() != WL_CONNECTED) && (attempts < 100))
    {
      delay(100);
      attempts ++;
    }
    if (attempts >= 100)
    {
      #if DEBUG_MODE && DEBUG_WiFi
        Serial.println("Failed to connect.");
      #endif
      deviceMode = 0; 
      tft.setCursor(45, 160);
      tft.println("Failed to connect.");
      delay(1000);
    }
    else
    {
      #if DEBUG_MODE && DEBUG_WiFi
        Serial.println("Connected!");
        Serial.print("Local IP: ");
        Serial.println(WiFi.localIP()); 
      #endif
      tft.setCursor(68, 160);
      tft.println("Connected!");
      tft.setCursor(55, 200);
      tft.println("IP:"); 
      tft.setCursor(78, 200);
      tft.println(WiFi.localIP());

      delay(1500);

      MQTTclient.setServer(MQTTserverIP, atoi(MQTTport));  
      MQTTclient.setCallback(callback);
      MQTTreconnect();
    }
    drawHomePage();
  }
  #if DEBUG_MODE
    Serial.println("Start Up Init Complete. Ready to Go ...");
    Serial.println("Go!!!");
    Serial.println("==================================================================");
  #endif
} //                                                            void setup()
//=============================================================================================================================
//=============================================================================================================================
//                                                              void loop())
void loop()
{
  MQTTclient.loop();

  if (deviceMode)                               // deviceMode is 1 when in monitoring station mode. 
  {
    if (WiFi.status() == WL_CONNECTED)
    {
      if (!MQTTclient.connected()) 
      MQTTreconnect();

      if (MQTTclient.connected()) 
      {
        MQTTsend = 1;
      }
      else
      {
        MQTTsend = 0;
      }
    }
  }
//=============================================================================================================================
  if (page == 0)                                                // homepage
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= 1000)                                       // 1 Second timer
    {
      previousMillis = currentMillis;

      batteryUpdateCounter ++;   
//------------------------------------------------------------------
      if (batteryUpdateCounter >= 61)                                                 // update battery level and RSSI every XX seconds. 60 
      {
        batteryUpdateCounter = 0;

        batteryPercent = getPercent();

        if (batteryPercent != previousbatteryPercent)
        {
          previousbatteryPercent = batteryPercent;

          batteryMapped = map(batteryPercent, 100, 0, 212, 233);
          tft.fillRect(212, 6, 22, 10, ILI9341_BLACK);
          if (batteryPercent < 10)
          {
            tft.fillRect(batteryMapped, 6, (234 - batteryMapped), 10, ILI9341_RED);
          }
          else
          {
            tft.fillRect(batteryMapped, 6, (234 - batteryMapped), 10, ILI9341_GREEN); // draws battery icon
          }
        }

        if (MQTTsend)
        {
          #if DEBUG_MODE && DEBUG_MQTT
            Serial.println("MQTT >: " + String(batterytopic) + ": " + (batteryPercent));
          #endif
          snprintf (msg, MSG_BUFFER_SIZE, "%i", batteryPercent);
          MQTTclient.publish(batterytopic, msg);

          snprintf (msg, MSG_BUFFER_SIZE, "%i", (WiFi.RSSI()));
          MQTTclient.publish(RSSI_Topic, msg);
        }
      }
//------------------------------------------------------------------
      if (MQTTclient.connected() != previousMQTTstatus)
      {
        previousMQTTstatus = MQTTclient.connected();
        //if (MQTTclient.connected())
        if (previousMQTTstatus)
        {
          tft.setTextSize(1);
          tft.setFont(&FreeSans9pt7b);
          tft.setCursor(157, 16);
          tft.println("M");
        }
        else
        {
          tft.fillRect(157, 2, 18, 18, ILI9341_BLACK);
        }
      }
//------------------------------------------------------------------ Расчёт
      count[i] = currentCount;
      i++;
      fastCount[j] = currentCount;    // keep concurrent arrays of counts. Use only one depending on user choice
      j++;
      slowCount[k] = currentCount;
      k++;

      if (i == 61)
        i = 0;

      if (j == 6)
        j = 0;

      if (k == 181)
        k = 0;

      switch(integrationMode)
      {
        case 0:     // 60
          averageCount = currentCount - count[i];                                 // count[i] stores the value from 60 seconds ago
        case 1:     // 5
          averageCount = (currentCount - fastCount[j]) * 12;
        case 2:     // 180
          averageCount = (currentCount - slowCount[k]) / 3;
        default:
          break;
      }

      averageCount = ((averageCount) / (1 - GeigerDeadTime * averageCount));  // Compensation dead time of the geiger tube. relevant at high count rates

      doseRate_uSv = float(averageCount) / conversionFactor;
      totalDose_uSv = float(cumulativeCount) / (60 * conversionFactor);

      doseRate_uR = doseRate_uSv * 100;
      totalDose_uR = totalDose_uSv * 100;

      // TestMicros = micros();

      // TestMicros = micros() - TestMicros;
      // #if DEBUG_MODE && DEBUG_TEST
      //   Serial.println("TEST: "+ String(TestMicros));
      // #endif

      if (doseUnits == 0)
      {
        doseRate = doseRate_uSv;
        totalDose = totalDose_uSv;
      }
      else if (doseUnits == 1)
      {
        doseRate = doseRate_uR;
        totalDose = totalDose_uR;    // 1 uSv = 100uR
      }
      
      tft.setFont();
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_WHITE, 0x630C);
      tft.setCursor(80, 192);
      tft.println(cumulativeCount);                               // display total counts since reset
      tft.setCursor(80, 222);
      tft.println(totalDose);                                     // display cumulative dose
//------------------------------------------------------------------
      if (doseRate != previousdoseRate)                           // Вывод на дисплей doserate
      {
        previousdoseRate = doseRate;
        if (doseRate < 10.0)
          dtostrf(doseRate, 4, 2, dose);                          // display two digits after the decimal point if value is less than 10
        else if ((doseRate >= 10) && (doseRate < 100))
          dtostrf(doseRate, 4, 1, dose);                          // display one digit after decimal point when dose is greater than 10
        else if ((doseRate >= 100))
          dtostrf(doseRate, 4, 0, dose);                          // whole numbers only when dose is higher than 100
        else 
          dtostrf(doseRate, 4, 0, dose);                          // covers the rare edge case where the dose rate is sometimes errorenously calculated to be negative
      
        tft.setFont();
        tft.setCursor(44, 52);
        tft.setTextSize(5);
        tft.setTextColor(ILI9341_WHITE, DOSEBACKGROUND);
        tft.println(dose);                                        // display effective dose rate
      }
//------------------------------------------------------------------
      if (averageCount != previousaverageCount)                   // Вывод на дисплей
      {      
        previousaverageCount = averageCount;
        tft.setFont();
        tft.setCursor(73, 122);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setTextSize(3);
        tft.println(averageCount);                                // Display CPM

        if (averageCount < 10)
        {
          tft.fillRect(90, 120, 144, 25, ILI9341_BLACK);          // erase numbers that may have been left from previous high readings
        }
        else if (averageCount < 100)
        {
          tft.fillRect(107, 120, 127, 25, ILI9341_BLACK);
        }
        else if (averageCount < 1000)
        {
          tft.fillRect(124, 120, 110, 25, ILI9341_BLACK);
        }
        else if (averageCount < 10000)
        {
          tft.fillRect(141, 120, 93, 25, ILI9341_BLACK);
        }
        else if (averageCount < 100000)
        {
          tft.fillRect(160, 120, 74, 25, ILI9341_BLACK);
        }
      }
//------------------------------------------------------------------
      if (averageCount < conversionFactor/2)                                 // 0.5 uSv/hr
        doseLevel = 0;                                                       // determines alert level displayed on homescreen
      else if (averageCount < alarmThreshold * conversionFactor)
        doseLevel = 1;
      else
        doseLevel = 2;

      if (doseLevel != previousDoseLevel)                       // only update alert level if it changed. This prevents flicker
      {
        switch(doseLevel)
        {
          case 0:
            tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_WHITE);
            tft.fillRoundRect(3, 94, 234, 21, 3, 0x2DC6);
            tft.setCursor(15, 104);
            tft.setFont(&FreeSans9pt7b);
            tft.setTextColor(ILI9341_WHITE);
            tft.setTextSize(1);
            tft.println("NORMAL BACKGROUND");
            break;
          case 1:
            tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_WHITE);
            tft.fillRoundRect(3, 94, 234, 21, 3, 0xCE40);
            tft.setCursor(29, 104);
            tft.setFont(&FreeSans9pt7b);
            tft.setTextColor(ILI9341_WHITE);
            tft.setTextSize(1);
            tft.println("ELEVATED ACTIVITY");
            break;
          case 2:
            tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_RED);
            tft.fillRoundRect(3, 94, 234, 21, 3, 0xB8A2);
            tft.setCursor(17, 104);
            tft.setFont(&FreeSans9pt7b);
            tft.setTextColor(ILI9341_WHITE);
            tft.setTextSize(1);
            tft.println("HIGH RADIATION LEVEL");          
            break;
          default:
            break;
        }
        previousDoseLevel = doseLevel;
      }
    } 
    // end of millis()-controlled block that runs once every second. The rest of the code on page 0 runs every loop
//----------------------------------------------------------------------------------
// Buzzer and LED reaction on Geiger event
    if (currentCount > previousCount)              // Если счётчиком зафиксирована новая частица
    {
      previousCount = currentCount;
      previousMicros = micros();                   // Начинаем отсчёт одновибратора      
      if (ledSwitch)
        digitalWrite(ACT_LED, HIGH);               // trigger buzzer and led if they are activated
      if (buzzerSwitch)
        digitalWrite(BUZZER_PIN, HIGH);
    }

    currentMicros = micros();

    if (currentMicros - previousMicros >= 500)     // если таймёр превышен, выключаем светодиод и пищалку
    {
      digitalWrite(ACT_LED, LOW);                  // LED off
      digitalWrite(BUZZER_PIN, LOW);               // Buzzer off
      previousMicros = currentMicros;              // 
    }
    //---------------------------------------------------------------------------------
    if (!ts.touched())
      wasTouched = 0;

    if (ts.touched() && !wasTouched)            // A way of "debouncing" the touchscreen. Prevents multiple inputs from single touch
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0);   // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 162 && x < 238) && (y > 259 && y < 318))
      {
        integrationMode ++;
        if (integrationMode == 3)
          integrationMode = 0;

        currentCount = 0;                     // reset counts and arrays when integration speed is changed
        previousCount = 0;
        for (int a = 0; a < 61; a++) 
        {
          count[a] = 0;
        }
        for (int b = 0; b < 6; b++)
        {
          fastCount[b] = 0;
        }
        for (int c = 0; c < 181; c++)
        {
          slowCount[c] = 0;
        }

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

        #if DEBUG_MODE && DEBUG_MQTT
          Serial.println("MQTT >: " + String(IntTimeTopic) + ": " + (value));
        #endif

        if (MQTTsend) 
        {
          snprintf (msg, MSG_BUFFER_SIZE, "%i", int(value));
          MQTTclient.publish(IntTimeTopic, msg, true);
        } 
      }
      else if ((x > 64 && x < 159) && (y > 259 && y < 318)) // timed count 
      {
        page = 6;
        drawTimedCountPage();
      }
      else if ((x > 190 && x < 238) && (y > 151 && y < 202)) // toggle LED
      {
        ledSwitch = !ledSwitch;
        if (ledSwitch)
        {
          tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 153, ledOnBitmap, 45, 45, ILI9341_WHITE);
          if (MQTTsend) 
          {
            #if DEBUG_MODE && DEBUG_MQTT
              Serial.println("MQTT >: " + String(lighttopic) + ": true");
            #endif
            MQTTclient.publish(lighttopic, "true", true);  
          }        
        }
        else
        {
          tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 153, ledOffBitmap, 45, 45, ILI9341_WHITE);
          if (MQTTsend) 
          {
            #if DEBUG_MODE && DEBUG_MQTT
              Serial.println("MQTT >: " + String(lighttopic) + ": false");
            #endif
            MQTTclient.publish(lighttopic, "false", true);
          }
        }
      }
      else if ((x > 190 && x < 238) && (y > 205 && y < 256)) // toggle buzzer
      {
        buzzerSwitch = !buzzerSwitch;
        if (buzzerSwitch)
        {
          tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 208, buzzerOnBitmap, 45, 45, ILI9341_WHITE);
          if (MQTTsend) 
          {
            #if DEBUG_MODE && DEBUG_MQTT
              Serial.println("MQTT >: " + String(buzzertopic) + ": true");
            #endif
            MQTTclient.publish(buzzertopic, "true", true);
          }
        }
        else
        {
          tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 208, buzzerOffBitmap, 45, 45, ILI9341_WHITE);
          if (MQTTsend) 
          {
            #if DEBUG_MODE && DEBUG_MQTT
              Serial.println("MQTT >: " + String(buzzertopic) + ": false");
            #endif
            MQTTclient.publish(buzzertopic, "false", true);
          }
        }
      }
      else if ((x > 3 && x < 61) && (y > 259 && y < 316)) // settings button pressed
      {
        page = 1;
        drawSettingsPage();
      }
    }
    if (MQTTsend)    // deviceMode is 1 when in monitoring station mode. Uploads CPM to mqtt every 15 sec    Сделать управляемый интервал
    {
      currentUploadTime = millis();

      if ((currentUploadTime - previousUploadTime) > (MQTTUpdateTime * 1000))
      {
        previousUploadTime = currentUploadTime;

        #if DEBUG_MODE && DEBUG_MQTT
          Serial.println("MQTT >: " + String(CPMTopic) + ": " + String(averageCount));
        #endif
        snprintf (msg, MSG_BUFFER_SIZE, "%li", averageCount);
        MQTTclient.publish(CPMTopic, msg);

        #if DEBUG_MODE && DEBUG_MQTT
          Serial.println("MQTT >: " + String(Doserate_uSv_Topic) + ": " + String(doseRate_uSv));
        #endif
        snprintf (msg, MSG_BUFFER_SIZE, "%f", doseRate_uSv);
        MQTTclient.publish(Doserate_uSv_Topic, msg);

        #if DEBUG_MODE && DEBUG_MQTT
          Serial.println("MQTT >: " + String(Doserate_uR_Topic) + ": " + String(doseRate_uR));
        #endif
        snprintf (msg, MSG_BUFFER_SIZE, "%f", doseRate_uR);
        MQTTclient.publish(Doserate_uR_Topic, msg);

        switch(doseLevel)
        {
          case 0: 
            snprintf (msg, MSG_BUFFER_SIZE, "NORMAL BACKGROUND");
            break;
          case 1: 
            snprintf (msg, MSG_BUFFER_SIZE, "ELEVATED ACTIVITY");
            break;
          case 2: 
            snprintf (msg, MSG_BUFFER_SIZE, "HIGH RADIATION LEVEL");
            break;
          default:
            break;
        }

        #if DEBUG_MODE && DEBUG_MQTT
          Serial.println("MQTT >: " + String(DoseLevelTopic) + ": " + String(msg));
        #endif

        MQTTclient.publish(DoseLevelTopic, msg);
      }
    }
  }
  //=============================================================================================================================
  else if (page == 1) // settings page. all display elements are drawn when drawSettingsPage() is called
  {
    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315)) // back button. draw homepage, reset counts and go back
      {
        currentCount = 0;
        previousCount = 0;
        for (int a = 0; a < 61; a++)
        {
          count[a] = 0;                // counts need to be reset to prevent errorenous readings
        }
        for (int b = 0; b < 6; b++)
        {
          fastCount[b] = 0;
        }
        for (int c = 0; c < 181; c++)
        {
          slowCount[c] = 0;
        }

        page = 0;
        drawHomePage();
      }
      else if ((x > 3 && x < 234) && (y > 64 && y < 108))
      {
        page = 2;
        drawUnitsPage();
      }
      else if ((x > 3 && x < 234) && (y > 114 && y < 158))
      {
        page = 3;
        drawAlertPage();
      }
      else if ((x > 3 && x < 234) && (y > 164 && y < 208))
      {
        page = 4;
        drawCalibrationPage();
      }
      else if ((x > 3 && x < 234) && (y > 214 && y < 268))
      {
        page = 5;
        drawWifiPage();
      }
    }
  }
  //=============================================================================================================================
  else if (page == 2) // units page
  {
    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315)) // back button
      {
        page = 1;
        if (EEPROM.read(saveUnits) != doseUnits) // check current EEPROM value and only write if new value is different
        {
          EEPROM.write(saveUnits, doseUnits); // save current units to EEPROM during exit. This will be retrieved at startup
          EEPROM.commit();
        }
        drawSettingsPage();
      }
      else if ((x > 4 && x < 234) && (y > 70 && y < 120))
      {
        doseUnits = 0;
        tft.fillRoundRect(4, 71, 232, 48, 4, 0x2A86);
        tft.setCursor(30, 103);
        tft.println("Sieverts (uSv/hr)");

        tft.fillRoundRect(4, 128, 232, 48, 4, ILI9341_BLACK);
        tft.setCursor(47, 160);
        tft.println("Rems (uR/hr)");
      }
      else if ((x > 4 && x < 234) && (y > 127 && y < 177))
      {
        doseUnits = 1;
        tft.fillRoundRect(4, 71, 232, 48, 4, ILI9341_BLACK);
        tft.setCursor(30, 103);
        tft.println("Sieverts (uSv/hr)");

        tft.fillRoundRect(4, 128, 232, 48, 4, 0x2A86);
        tft.setCursor(47, 160);
        tft.println("Rems (uR/hr)");
      }
    }
  }
  //=============================================================================================================================
  else if (page == 3)        // alert thresold page
  {
    tft.setFont();
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(151, 146);
    tft.println(alarmThreshold);
    if (alarmThreshold < 10)
      tft.fillRect(169, 146, 22, 22, ILI9341_BLACK);

    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315))
      {
        page = 1;
        if (EEPROM.read(saveAlarmThreshold) != alarmThreshold)
        {
          EEPROM.write(saveAlarmThreshold, alarmThreshold);
          EEPROM.commit(); // save to EEPROM to be retrieved at startup

          if (MQTTsend) 
          {
            snprintf (msg, MSG_BUFFER_SIZE, "%i", alarmThreshold);                             
            #if DEBUG_MODE && DEBUG_MQTT
              Serial.println("MQTT >: "+ String(AlarmThresholdTopic) + (": ") + msg);
            #endif
            MQTTclient.publish(AlarmThresholdTopic, msg, true);
          }
        } 

      drawSettingsPage();
      }
      else if ((x > 130 && x < 190) && (y > 70 && y < 120))
      {
        alarmThreshold++;
        if (alarmThreshold > 100)
          alarmThreshold = 100;
      }
      else if ((x > 130 && x < 190) && (y > 185 && y < 245))
      {
        alarmThreshold--;
        if (alarmThreshold <= 2)
          alarmThreshold = 2;
      }
    }
  }
  //=============================================================================================================================
  else if (page == 4)     // calibration page
  {
    tft.setFont();
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor(161, 146);
    tft.println(conversionFactor);
    if (conversionFactor < 100)
      tft.fillRect(197, 146, 22, 22, ILI9341_BLACK);

    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315))
      {
        page = 1;
        if ((EEPROMReadlong(saveCalibration)) != long(conversionFactor))
        {
          EEPROMWritelong(saveCalibration, conversionFactor);
          EEPROM.commit();

          if (MQTTsend) 
          {
            snprintf (msg, MSG_BUFFER_SIZE, "%li", conversionFactor);        //====!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//            snprintf (msg, MSG_BUFFER_SIZE, (conversionFactor.c_str()));
            #if DEBUG_MODE && DEBUG_MQTT
              Serial.println("MQTT >: " + String(ConvFactorTopic) + ": " + conversionFactor);
//              Serial.println("MQTT >: " + String(ConvFactorTopic) + ": " + msg);
            #endif
            MQTTclient.publish(ConvFactorTopic, msg, true); 
          }
        }
        drawSettingsPage();
      }
      else if ((x > 160 && x < 220) && (y > 70 && y < 120))
      {
        conversionFactor++;
      }
      else if ((x > 160 && x < 220) && (y > 185 && y < 245))
      {
        conversionFactor--;
        if (conversionFactor <= 1)
          conversionFactor = 1;
      }
    }
  }
  //=============================================================================================================================
  else if (page == 5)  // Logging and Wifi page
  {
    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315))        // Logging mode change
      {
        page = 1;
//        if (EEPROM.read(saveLoggingMode) != isLogging)      // check current EEPROM value and only write if new value is different
//        {
//          EEPROM.write(saveLoggingMode, isLogging); 
//          EEPROM.commit();
//        }
        drawSettingsPage();
      }
      else if ((x > 3 && x < 237) && (y > 64 && y < 108))   // wifi setup button
      {

        if (MQTTclient.connected()) 
        {
          MQTTclient.unsubscribe(CommandTopic);
          MQTTclient.disconnect();
        }
        
        tft.setFont(&FreeSans9pt7b);
        tft.setTextSize(1);

        tft.fillRoundRect(10, 30, 220, 260, 6, ILI9341_BLACK);
        tft.drawRoundRect(10, 30, 220, 260, 6, ILI9341_WHITE);

        tft.setCursor(50, 50);
        tft.println("AP SETUP MODE");
        tft.drawFastHLine(50, 53, 145, ILI9341_WHITE);
        tft.setCursor(20, 80);
        tft.println("With any WiFi capable");
        tft.setCursor(20, 100);
        tft.println("device, connect to");
        tft.setCursor(20, 120);
        tft.println("network \"GC20\" and ");
        tft.setCursor(20, 140);
        tft.println("browse to 192.168.4.1.");
        tft.setCursor(20, 160);
        tft.println("Enter credentials");
        tft.setCursor(20, 180);
        tft.println("of your WiFi network");
        tft.setCursor(20, 200);
        tft.println("and the IP address");
        tft.setCursor(20, 220);
        tft.println("MQTT server and write");
        tft.setCursor(20, 240);
        tft.println("MQTT device ID");

        #if DEBUG_MODE 
          Serial.println("==================================================================");
          Serial.println("WIFI AP: Change Mode to AP");
          Serial.println("WIFI AP: Connect to AP GC-20M and browse to IP adress 192.168.4.1.");
          Serial.println("WIFI AP: Enter credentials of your WiFi network and"); 
          Serial.println("WIFI AP: the IP address MQTT server and write MQTT device ID");
          Serial.println("==================================================================");
        #endif

        WiFiManager wifiManager;

        char ssidChar[20] = {0};
        char passwordChar[20] = {0};
        char AP_mqtt_clientid[20] = {0};
        char AP_mqtt_server[15] = {0};
        char AP_mqtt_port[5] = {0};
        char AP_mqtt_login[20] = {0};
        char AP_mqtt_pass[20] = {0};

        WiFiManagerParameter wm_mqtt_clientid("0", "MQTT Device ID", AP_mqtt_clientid, 20);
        WiFiManagerParameter wm_mqtt_server("1", "MQTT Server IP", AP_mqtt_server, 15); // create custom parameters for setup
        WiFiManagerParameter wm_mqtt_port("2", "MQTT Server port", AP_mqtt_port, 5);
        WiFiManagerParameter wm_mqtt_login("3", "MQTT Login", AP_mqtt_login, 20);
        WiFiManagerParameter wm_mqtt_pass("4", "MQTT Password", AP_mqtt_pass, 20);

        wifiManager.addParameter(&wm_mqtt_clientid);
        wifiManager.addParameter(&wm_mqtt_server);
        wifiManager.addParameter(&wm_mqtt_port);
        wifiManager.addParameter(&wm_mqtt_login); 
        wifiManager.addParameter(&wm_mqtt_pass);        

        wifiManager.startConfigPortal("GC-20M");             // put the esp in AP mode for wifi setup, create a network with name "GC20"

        String ssidString = WiFi.SSID();                     // retrieve ssid and password form the WifiManager library
        String passwordString = WiFi.psk();
        strcpy(AP_mqtt_clientid, wm_mqtt_clientid.getValue());
        strcpy(AP_mqtt_server, wm_mqtt_server.getValue());
        strcpy(AP_mqtt_port, wm_mqtt_port.getValue());
        strcpy(AP_mqtt_login, wm_mqtt_login.getValue());
        strcpy(AP_mqtt_pass, wm_mqtt_pass.getValue());

        size_t ssidLen = ssidString.length();
        size_t passLen = passwordString.length();
        size_t m_idLen = String(AP_mqtt_clientid).length();
        size_t m_ipLen = String(AP_mqtt_server).length();
        size_t m_portLen = String(AP_mqtt_port).length();
        size_t m_loginLen = String(AP_mqtt_login).length();
        size_t m_passLen = String(AP_mqtt_pass).length();

        #if DEBUG_MODE && DEBUG_AP
          Serial.println("WIFI AP: AP_wifi_ssid: " + (ssidString) + " ssidLen: " + String(ssidLen));
          Serial.println("WIFI AP: AP_wifi_pass: " + (passwordString) + " passLen: " + String(passLen));
          Serial.println("WIFI AP: AP_mqtt_clientid: " + String(AP_mqtt_clientid) + " m_idLen: "+ String(m_idLen));
          Serial.println("WIFI AP: AP_mqtt_server: " + String(AP_mqtt_server) + " m_ipLen: "+ String(m_ipLen));
          Serial.println("WIFI AP: AP_mqtt_port: "+ String(AP_mqtt_port) + " m_portLen: "+ String(m_portLen));
          Serial.println("WIFI AP: AP_mqtt_login: "+ String(AP_mqtt_login) + "m_loginLen: "+ String(m_loginLen));
          Serial.println("WIFI AP: AP_mqtt_pass: "+ String(AP_mqtt_pass) + "m_passLen: "+ String(m_passLen));
        #endif 

        ssidString.toCharArray(ssidChar, ssidLen + 1); 
        passwordString.toCharArray(passwordChar, passLen + 1);

        for (unsigned int a = 20; a < 20 + ssidLen; a++)
        {
          EEPROM.write((a), ssidChar[a - 20]);               // save ssid and ssid length to EEPROM
        }
        EEPROM.write(saveSSIDLen, ssidLen);
        
        for (unsigned int b = 40; b < 40 + passLen; b++)
        {    
          EEPROM.write((b), passwordChar[b - 40]);          // save password and password length to EEPROM
        }
        EEPROM.write(savePWLen, passLen);

        for (unsigned int b = 60; b < 60 + m_idLen; b++)
        {
          EEPROM.write((b), AP_mqtt_clientid[b - 60]);
        }
        EEPROM.write(saveIDLen, m_idLen);

        for (unsigned int a = 80; a < 80 + m_ipLen; a++)
        {
          EEPROM.write((a), AP_mqtt_server[a - 80]);
        }
        EEPROM.write(saveIPLen, m_ipLen);

        for (unsigned int a = 95; a < 95 + m_portLen; a++)
        {
          EEPROM.write((a), AP_mqtt_port[a - 95]);
        }
        EEPROM.write(savePortLen, m_portLen);

        for (unsigned int a = 100; a < 100 + m_loginLen; a++)
        {
          EEPROM.write((a), AP_mqtt_login[a - 100]);
        }
        EEPROM.write(saveMLoginLen, m_loginLen);

        for (unsigned int a = 120; a < 120 + m_passLen; a++)
        {
          EEPROM.write((a), AP_mqtt_pass[a - 120]);
        }
        EEPROM.write(saveMPassLen, m_passLen);

        EEPROM.commit();

        #if DEBUG_MODE && DEBUG_EEPROM
          Serial.println("EEPROM: WRITE_ssid: "+ String(ssidChar) + " WRITEidLen: "+ String(ssidLen));
          Serial.println("EEPROM: WRITE_password: "+ String(passwordChar) + " WRITEidLen: "+ String(passLen));
          Serial.println("EEPROM: WRITE_clientid: "+ String(AP_mqtt_clientid) + " WRITEidLen: "+ String(m_idLen));
          Serial.println("EEPROM: WRITE_mqtt_server: "+ String(AP_mqtt_server) + " WRITEipLen: "+ String(m_ipLen));
          Serial.println("EEPROM: WRITE_mqtt_port: "+ String(AP_mqtt_port) + " WRITEportLen: "+ String(m_portLen));
          Serial.println("EEPROM: WRITE_mqtt_login: "+ String(AP_mqtt_login) + " WRITEloginLen: "+ String(m_loginLen));
          Serial.println("EEPROM: WRITE_mqtt_pass: "+ String(AP_mqtt_pass) + " WRITEpassLen: "+ String(m_passLen));
        #endif

        tft.setCursor(16, 265);
        tft.println("Settings saved. Restarting");

        #if DEBUG_MODE 
          Serial.println("WIFI AP: Settings saved. Restarting");
        #endif

        delay(1000);
        
        ESP.reset();
      }
//=============================================================================================================================     
      else if ((x > 3 && x < 237) && (y > 162 && y < 206)) // upload data
      {
      /*  
        drawBlankDialogueBox();
        
          clearLogs();                 // erase logs and re-initialize the json buffer
          tft.setCursor(43, 260);
          tft.println("Resetting Device..");
          delay(1000);
          ESP.reset();                         
       */  
      delay(1);
      }
      else if ((x > 3 && x < 237) && (y > 114 && y < 158)) // logging 
      {
        isLogging = !isLogging;
        if (isLogging)
        {
          tft.fillRoundRect(3, 114, 234, 44, 4, 0x3B8F);
          tft.drawRoundRect(3, 114, 234, 44, 4, WHITE);
          tft.setCursor(38, 145);
          tft.println("LOGGING ON");
        }
        else
        {
          tft.fillRoundRect(3, 114, 234, 44, 4, 0xB9C7);
          tft.drawRoundRect(3, 114, 234, 44, 4, WHITE);
          tft.setCursor(33, 145);
          tft.println("LOGGING OFF");
        }
      }
      else if ((x > 3 && x < 237) && (y > 214 && y < 258))  // device mode
      {
        page = 8;
        drawDeviceModePage();
      }
    }
  }
  //=============================================================================================================================
  else if (page == 6) // timed count setup page
  {
    if (interval < 10)
    {
      intervalSize = 1;
    }
    else if (interval < 100)
    {
      intervalSize = 2;
    }
    else 
    {
      intervalSize = 3;
    }
    
    tft.setFont();
    tft.setTextSize(3);
    tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
    tft.setCursor((185 - (intervalSize - 1) * 11), 146);
    tft.println(interval);

    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315))
      {
        page = 0;
        drawHomePage();
        currentCount = 0;
        previousCount = 0;
        for (int a = 0; a < 60; a++)
        {
          count[a] = 0; // counts need to be reset to prevent errorenous readings
        }
        for (int b = 0; b < 5; b++)
        {
          fastCount[b] = 0;
        }
        for (int c = 0; c < 180; c++)
        {
          slowCount[c] = 0;
        }
      }
      else if ((x > 145 && x < 235) && (y > 271 && y < 315))
      {
        page = 7;
        drawTimedCountRunningPage(interval, intervalSize);
      }
      else if ((x > 160 && x < 220) && (y > 70 && y < 120))
      {
        interval += 5;
        if (interval >= 995)
        {
          interval = 995;
        }
        tft.fillRect(160, 130, 70, 40, ILI9341_BLACK);
      }
      else if ((x > 160 && x < 220) && (y > 185 && y < 245))
      {
        interval -= 5;
        if (interval <= 5)
        {
          interval = 5;
        }
        tft.fillRect(160, 130, 70, 40, ILI9341_BLACK);
      }
    }
  }
  //=============================================================================================================================
  else if (page == 7) // timed count running page
  {
    elapsedTime = millis() - startMillis;
    if(elapsedTime < intervalMillis)
    {
      if((millis() - previousMillis) >= 1000)
      {
        previousMillis = millis();

        tft.setFont();
        tft.setTextSize(3);
        tft.setTextColor(ILI9341_WHITE, ILI9341_BLACK);
        tft.setCursor(101, 181);
        tft.println(currentCount);

        cpm = float(currentCount) / float((1 + elapsedTime) / 60000.0);
        
        tft.setCursor(101, 226);
        tft.println(cpm);

        if(cpm < 10)
        {
          tft.fillRect(170, 225, 50, 40, ILI9341_BLACK);
        }
        else if(cpm < 100)
        {
          tft.fillRect(190, 225, 35, 40, ILI9341_BLACK);
        }
        else if(cpm < 1000)
        {
          tft.fillRect(209, 225, 18, 40, ILI9341_BLACK);
        }
      }
      progress = map(elapsedTime, 0, intervalMillis, 0, 217);
      tft.fillRect(12, 105, progress, 16, 0x25A6);
    }
    else 
    {
      if (completed == 0)
      {
        drawCloseButton();
        completed = 1;
      }
    }
    
    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 70 && x < 170) && (y > 271 && y < 315))
      {
        page = 0;
        drawHomePage();

        currentCount = 0;
        previousCount = 0;
        for (int a = 0; a < 60; a++)
        {
          count[a] = 0;                    // counts need to be reset to prevent errorenous readings
        }
        for (int b = 0; b < 5; b++)
        {
          fastCount[b] = 0;
        }
        for (int c = 0; c < 180; c++)
        {
          slowCount[c] = 0;
        }
      }
    }
  }
  //=============================================================================================================================
  else if (page == 8)          // device mode selection page
  {
    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched)
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 4 && x < 62) && (y > 271 && y < 315)) // back button
      {
        page = 5;
        if (EEPROM.read(saveDeviceMode) != deviceMode) // check current EEPROM value and only write if new value is different
        {
          EEPROM.write(saveDeviceMode, deviceMode); 
          EEPROM.commit();
        }
        drawWifiPage();
      }
      else if ((x > 4 && x < 234) && (y > 70 && y < 120))
      {
        deviceMode = 0;
        tft.setFont(&FreeSans12pt7b);
        tft.fillRoundRect(4, 71, 232, 48, 4, 0x2A86);
        tft.setCursor(13, 103);
        tft.println("GEIGER COUNTER");
        tft.fillRoundRect(4, 128, 232, 48, 4, ILI9341_BLACK);
        tft.setCursor(30, 160);
        tft.println("MON. STATION");

      }
      else if ((x > 4 && x < 234) && (y > 127 && y < 177))
      {
        deviceMode = 1;
        tft.setFont(&FreeSans12pt7b);
        tft.fillRoundRect(4, 71, 232, 48, 4, ILI9341_BLACK);
        tft.setCursor(13, 103);
        tft.println("GEIGER COUNTER");
        tft.fillRoundRect(4, 128, 232, 48, 4, 0x2A86);
        tft.setCursor(30, 160);
        tft.println("MON. STATION");
      }
    }
  }
}//                                                         void loop()
//=============================================================================================================================
/*
        if (MQTTclient.connected()) 
        {
          MQTTclient.unsubscribe(CommandTopic);
          MQTTclient.disconnect();
        }
        
        if (WiFi.status() == WL_CONNECTED)
        {
        WiFi.disconnect();
        WiFi.mode(WIFI_OFF);                                     // turn off wifi
        }
*/