/*  SBM-20 based Geiger Counter
    Author: Prabhat    Email: pra22@pitt.edu
    Author: ElectroZheka
    Sketch for ESP8266 that counts clicks from the Geiger tube, calculates the counts per minute, displays information 
    on a TFT touchscreen and send data to MQTT server.
    Attribution-ShareAlike 4.0 International (CC BY-SA 4.0)
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
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
char ssid[20];
char password[20];
WiFiClient client;
//=============================================================================================================================
// MQTT variables
char MQTTserverIP[15];                      // = "192.168.1.1";
char MQTTdeviceID[20];                      // = "Esp_Dosimeter"; 
//char MQTTuser[20];                        //  
//char MQTTpass[20];                        // = "Esp_Dosimeter";

int attempts;                               // number of connection attempts when device starts up in monitoring mode
int MQTTattempts;                           // number of MQTT connection attempts when device starts up in monitoring mode
// PubSubClient variables
const int mqtt_port = 1883;                 // Порт для подключения к серверу MQTT
const char *mqtt_user = "hamqtt";
const char *mqtt_pass = "hamqtt";

//String subscr_topic = "EspDosimeter/Control/";
//String prefix = "EspDosimeter/";
//String buf_recv;

PubSubClient MQTTclient(client);
#define MSG_BUFFER_SIZE	(50)

char msg[MSG_BUFFER_SIZE];
char topic[MSG_BUFFER_SIZE];
char buzzertopic[MSG_BUFFER_SIZE];
char lighttopic[MSG_BUFFER_SIZE];
char iptopic[MSG_BUFFER_SIZE];
char batterytopic[MSG_BUFFER_SIZE];
char ConvFactorTopic[MSG_BUFFER_SIZE];
char IntTimeTopic[MSG_BUFFER_SIZE];
float value = 0;
//=============================================================================================================================
// Display variable
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

long count[61];
long fastCount[6];                   // arrays to store running counts
long slowCount[181];
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
bool deviceMode;
bool ledSwitch = 1;
bool buzzerSwitch = 1;
int integrationMode = 0;             // 0 = medium, 1 = fast, 2 == slow;
unsigned int alarmThreshold = 5;
//=============================================================================================================================
// Geiger counter variable
unsigned long averageCount;
unsigned long currentCount;          // incremented by interrupt
unsigned long previousCount;         // to activate buzzer and LED
unsigned long cumulativeCount;
float doseRate;
float totalDose;
char dose[5];
int doseLevel;                       // determines home screen warning signs
int previousDoseLevel;
bool doseUnits = 0;                  // 0 = Sievert, 1 = Rem
unsigned int conversionFactor = 575; //175;
//=============================================================================================================================
// Touchscreen variable
XPT2046_Touchscreen ts(CS_PIN);
bool wasTouched;
int x, y;                           // touch points
//=============================================================================================================================
// Battery indicator variables
int batteryInput;
int batteryPercent;
int batteryMapped = 212;            // pixel location of battery icon
int batteryUpdateCounter = 29;
//=============================================================================================================================
// EEPROM variables
const int saveUnits = 0;
const int saveAlertThreshold = 1;   // Addresses for storing settings data in the EEPROM
const int saveCalibration = 2;
/*const int saveDeviceMode = 3;
const int saveLoggingMode = 4;
const int saveSSIDLen = 5;
const int savePWLen = 6;
const int saveIDLen = 7;
const int saveAPILen = 8; */
const int saveDeviceMode = 6;
const int saveLoggingMode = 7;
const int saveSSIDLen = 8;
const int savePWLen = 9;
const int saveIDLen = 10;
const int saveAPILen = 11;
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
#include "graphic/graphic.h"
#include "EEPROM/EEPROM.h"
#include "mqtt/mqtt.h"
//=============================================================================================================================
// SubRoutine
void drawHomePage();                                      // page 0
void drawSettingsPage();                                  // page 1
void drawUnitsPage();                                     // page 2
void drawAlertPage();                                     // page 3
void drawCalibrationPage();                               // page 4
void drawWifiPage();                                      // page 5
void drawTimedCountPage();                                // page 6
void drawTimedCountRunningPage(int duration, int size);   // page 7 
void drawDeviceModePage();                                // page 8

void drawFrame();
void drawBackButton();
void drawCancelButton();
void drawCloseButton();
void drawBlankDialogueBox();

long EEPROMReadlong(long address);                        // EEPRON Read
void EEPROMWritelong(int address, long value);            // EEPRON Write
//=============================================================================================================================
//=============================================================================================================================
void setup()
{
  pinMode(D0, OUTPUT); // buzzer switch
  pinMode(D3, OUTPUT); // LED
  digitalWrite(D3, LOW);
  digitalWrite(D0, LOW);
  
  Serial.begin(115200);

  delay(500);

  #if DEBUG_MODE
    Serial.println(" ");
    Serial.println(" ");
    Serial.println("====================================");
    Serial.println("GC-20M Starting...");
  #endif

  tft.begin();
  tft.setRotation(0);

  tft.begin();
  tft.setRotation(2);
  tft.fillScreen(ILI9341_BLACK);

  //EEPROM.begin(4096);   // initialize emulated EEPROM sector with 4 kb
  EEPROM.begin(128);   // initialize emulated EEPROM sector 128 byte

  #if DEBUG_MODE && DEBUG_EEPROM
    Serial.println("Reading contants from EEPROM...");
  #endif

  doseUnits = EEPROM.read(saveUnits);                                //Address = 0
  alarmThreshold = EEPROM.read(saveAlertThreshold);                  //Address = 1
  conversionFactor = EEPROMReadlong(saveCalibration);                //Address = 2
  deviceMode = EEPROM.read(saveDeviceMode);                          //Address = 6
  isLogging = EEPROM.read(saveLoggingMode);                          //Address = 7
  SSIDLength = EEPROM.read(saveSSIDLen);                             //Address = 8
  passwordLength = EEPROM.read(savePWLen);                           //Address = 9
  MQTTserverLength = EEPROM.read(saveIDLen);                         //Address = 10
  DeviceIDLength = EEPROM.read(saveAPILen);                          //Address = 11

  for (int i = 12; i < 12 + SSIDLength; i++)
  {
    ssid[i - 12] = EEPROM.read(i);
  }

  for (int j = 32; j < 32 + passwordLength; j++)
  {
    password[j - 32] = EEPROM.read(j);
  }

  for (int k = 52; k < 52 + MQTTserverLength; k++)
  {
    MQTTserverIP[k - 52] = EEPROM.read(k);
  }

  for (int l = 72; l < 72 + DeviceIDLength; l++)
  {
    MQTTdeviceID[l - 72] = EEPROM.read(l);
  }

  #if DEBUG_MODE && DEBUG_EEPROM
    Serial.println("doseUnits: "+ String(doseUnits));
    Serial.println("alarmThreshold: "+ String(alarmThreshold));  
    Serial.println("conversionFactor: " + String(conversionFactor)); 
    Serial.println("deviceMode: " + String(deviceMode));
    Serial.println("isLogging: " + String(isLogging));
    Serial.println("SSID: "+ String(ssid) + " Length: "+ String(SSIDLength));
    Serial.println("Password: "+ String(password) + " Length: "+ String(passwordLength));
    Serial.println("MQTTserver: "+ String(MQTTserverIP) + " Length: "+ String(MQTTserverLength));
    Serial.println("DeviceID: "+ String(MQTTdeviceID) + " Length: "+ String(DeviceIDLength));
    Serial.println("====================================");
  #endif

  snprintf (buzzertopic, MSG_BUFFER_SIZE, "%s/Control/Buzzer", MQTTdeviceID );
  snprintf (lighttopic, MSG_BUFFER_SIZE, "%s/Control/Light", MQTTdeviceID );
  snprintf (iptopic, MSG_BUFFER_SIZE, "%s/System/IP", MQTTdeviceID );
  snprintf (batterytopic, MSG_BUFFER_SIZE, "%s/System/Battery", MQTTdeviceID );
  snprintf (ConvFactorTopic, MSG_BUFFER_SIZE, "%s/System/ConversionFactor", MQTTdeviceID );
  snprintf (IntTimeTopic, MSG_BUFFER_SIZE, "%s/System/Integration_Time", MQTTdeviceID );

  attachInterrupt(interruptPin, isr, FALLING);

  drawHomePage();

  if (!deviceMode)
  {
    #if DEBUG_MODE
      Serial.println("CG-20M in Portable Dosimeter Mode.");
    #endif
    WiFi.mode( WIFI_OFF );                                     // turn off wifi
    WiFi.forceSleepBegin();
    delay(1);
  }
  else
  {
    #if DEBUG_MODE
      Serial.println("CG-20M in Monitoring Station Mode.");
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

      MQTTclient.setServer(MQTTserverIP, mqtt_port);
      MQTTclient.setCallback(callback);
      MQTTreconnect();
    }
    drawHomePage();
  }
} //                                                            void setup()
//=============================================================================================================================
void loop()
{
MQTTclient.loop();

  if (page == 0)                                                // homepage
  {
    currentMillis = millis();
    if (currentMillis - previousMillis >= 1000)
    {
      previousMillis = currentMillis;

      batteryUpdateCounter ++;     

      if (batteryUpdateCounter == 30){         // update battery level every 30 seconds. Prevents random fluctations of battery level.

        batteryUpdateCounter = 0;

        batteryInput = analogRead(BATT_PIN);
        batteryInput = constrain(batteryInput, 590, 800);
        batteryPercent = map(batteryInput, 590, 800, 0, 100);
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

        #if DEBUG_MODE && DEBUG_BATT
//          Serial.println("BATT: ADC: " + String(batteryInput));
          Serial.println("BATT: " + String(batteryPercent) + "%");
        #endif

        snprintf (msg, MSG_BUFFER_SIZE, "%i", batteryPercent);
        MQTTclient.publish(batterytopic, msg);
      }

      if (MQTTclient.connected())
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

      count[i] = currentCount;
      i++;
      fastCount[j] = currentCount;    // keep concurrent arrays of counts. Use only one depending on user choice
      j++;
      slowCount[k] = currentCount;
      k++;

      if (i == 61)
      {
        i = 0;
      }

      if (j == 6)
      {
        j = 0;
      }

      if (k == 181)
      {
        k = 0;
      }

      if (integrationMode == 2)
      {
        averageCount = (currentCount - slowCount[k]) / 3;
      }

      if (integrationMode == 1)
      {
        averageCount = (currentCount - fastCount[j]) * 12;
      }

      else if (integrationMode == 0)
      {
        averageCount = currentCount - count[i];                              // count[i] stores the value from 60 seconds ago
      }

      averageCount = ((averageCount) / (1 - 0.00000333 * float(averageCount))); // accounts for dead time of the geiger tube. relevant at high count rates

      if (doseUnits == 0)
      {
        doseRate = averageCount / float(conversionFactor);
        totalDose = cumulativeCount / (60 * float(conversionFactor));
      }
      else if (doseUnits == 1)
      {
        doseRate = averageCount / float(conversionFactor * 10.0);
        totalDose = cumulativeCount / (60 * float(conversionFactor * 10.0)); // 1 mRem == 10 uSv
      }

      if (averageCount < conversionFactor/2)                                 // 0.5 uSv/hr
        doseLevel = 0;                                                       // determines alert level displayed on homescreen
      else if (averageCount < alarmThreshold * conversionFactor)
        doseLevel = 1;
      else
        doseLevel = 2;

      if (doseRate < 10.0)
      {
        dtostrf(doseRate, 4, 2, dose);                          // display two digits after the decimal point if value is less than 10
      }
      else if ((doseRate >= 10) && (doseRate < 100))
      {
        dtostrf(doseRate, 4, 1, dose);                          // display one digit after decimal point when dose is greater than 10
      }
      else if ((doseRate >= 100))
      {
        dtostrf(doseRate, 4, 0, dose);                          // whole numbers only when dose is higher than 100
      }
      else {
        dtostrf(doseRate, 4, 0, dose);                          // covers the rare edge case where the dose rate is sometimes errorenously calculated to be negative
      }
      
      tft.setFont();
      tft.setCursor(44, 52);
      tft.setTextSize(5);
      tft.setTextColor(ILI9341_WHITE, DOSEBACKGROUND);
      tft.println(dose);                                        // display effective dose rate
//      tft.setTextSize(1);

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
      tft.setCursor(80, 192);
      tft.setTextSize(2);
      tft.setTextColor(ILI9341_WHITE, 0x630C);
      tft.println(cumulativeCount);                             // display total counts since reset

      tft.setCursor(80, 222);
      tft.println(totalDose);                                   // display cumulative dose

      if (doseLevel != previousDoseLevel)                       // only update alert level if it changed. This prevents flicker
      {
        if (doseLevel == 0)
        {
          tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_WHITE);
          tft.fillRoundRect(3, 94, 234, 21, 3, 0x2DC6);
          tft.setCursor(15, 104);
          tft.setFont(&FreeSans9pt7b);
          tft.setTextColor(ILI9341_WHITE);
          tft.setTextSize(1);
          tft.println("NORMAL BACKGROUND");
          //tft.println("Фон в норме");

         // previousDoseLevel = doseLevel;
        }
        else if (doseLevel == 1)
        {
          tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_WHITE);
          tft.fillRoundRect(3, 94, 234, 21, 3, 0xCE40);
          tft.setCursor(29, 104);
          tft.setFont(&FreeSans9pt7b);
          tft.setTextColor(ILI9341_WHITE);
          tft.setTextSize(1);
          tft.println("ELEVATED ACTIVITY");

          //previousDoseLevel = doseLevel;
        }
        else if (doseLevel == 2)
        {
          tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_RED);
          tft.fillRoundRect(3, 94, 234, 21, 3, 0xB8A2);
          tft.setCursor(17, 104);
          tft.setFont(&FreeSans9pt7b);
          tft.setTextColor(ILI9341_WHITE);
          tft.setTextSize(1);
          tft.println("HIGH RADIATION LEVEL");

          //previousDoseLevel = doseLevel;
        }
        previousDoseLevel = doseLevel;
      }
      //Serial.println(currentCount);
    } 
    // end of millis()-controlled block that runs once every second. The rest of the code on page 0 runs every loop
//----------------------------------------------------------------------------------
// Buzzer and LED reaction on Geiger event
    if (currentCount > previousCount)      // Если счётчиком зафиксирована новая частица
    {
      if (ledSwitch)
      {
        //digitalWrite(D3, LOW);             // LED off
        digitalWrite(D3, HIGH);            // trigger buzzer and led if they are activated
      } 
      if (buzzerSwitch){
        digitalWrite(D0, LOW);             // Buzzer off
        delay(1);
        digitalWrite(D0, HIGH);
      }  
      previousCount = currentCount;
      previousMicros = micros();           // Начинаем отсчёт одновибратора
    }

    currentMicros = micros();

    if (currentMicros - previousMicros >= 200)
    {
      digitalWrite(D3, LOW);               // LED off
      digitalWrite(D0, LOW);               // Buzzer off
      previousMicros = currentMicros;      // 
    }
//---------------------------------------------------------------------------------
    if (!ts.touched())
      wasTouched = 0;
    if (ts.touched() && !wasTouched) // A way of "debouncing" the touchscreen. Prevents multiple inputs from single touch
    {
      wasTouched = 1;
      TS_Point p = ts.getPoint();
      x = map(p.x, TS_MINX, TS_MAXX, 240, 0); // get touch point and map to screen pixels
      y = map(p.y, TS_MINY, TS_MAXY, 320, 0);

      #if DEBUG_MODE && DEBUG_TS
        Serial.println("TS: Px: "+ String(p.x) + " Py: "+ String(p.y));
        Serial.println("TS: X: "+ String(x) + " Y: "+ String(y));
      #endif

      if ((x > 162 && x < 238) && (y > 259 && y < 318))
      {
        integrationMode ++;
        if (integrationMode == 3)
        {
          integrationMode = 0;
        }
        currentCount = 0;
        previousCount = 0;
        for (int a = 0; a < 61; a++) // reset counts when integretation speed is changed
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
        if (deviceMode)    // deviceMode is 1 when in monitoring station mode. Uploads CPM to thingspeak every 5 minutes
        {
        if (WiFi.status() == WL_CONNECTED)
          {
            if (!MQTTclient.connected()) 
            {
              MQTTreconnect();
            }
            if (MQTTclient.connected()) 
            {
              if (integrationMode == 0) // change button based on touch and previous state
              {
                value = 60;
              }
              else if (integrationMode == 1)
              {
                value = 5;
              }
              else if (integrationMode == 2)
              {
                value = 180;
              }

              #if DEBUG_MODE && DEBUG_MQTT
                Serial.print("MQTT: Topic: ");
                Serial.print(IntTimeTopic);                                                        //!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
                Serial.print(": ");
                Serial.println(value);
              #endif
              snprintf (msg, MSG_BUFFER_SIZE, "%f", value);
              MQTTclient.publish(IntTimeTopic, msg); 
            }
          } 
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
          MQTTclient.publish(lighttopic, "true");          
        }
        else
        {
          tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 153, ledOffBitmap, 45, 45, ILI9341_WHITE);
          MQTTclient.publish(lighttopic, "false");
        }
      }
      else if ((x > 190 && x < 238) && (y > 205 && y < 256)) // toggle buzzer
      {
        buzzerSwitch = !buzzerSwitch;
        if (buzzerSwitch)
        {
          tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 208, buzzerOnBitmap, 45, 45, ILI9341_WHITE);
          MQTTclient.publish(buzzertopic, "true");
        }
        else
        {
          tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
          tft.drawBitmap(190, 208, buzzerOffBitmap, 45, 45, ILI9341_WHITE);
          MQTTclient.publish(buzzertopic, "false");
        }
      }
      else if ((x > 3 && x < 61) && (y > 259 && y < 316)) // settings button pressed
      {
        page = 1;
        drawSettingsPage();
      }
    }
    /*
    if (isLogging)
    {
      if(addr < 2100)
      {
        currentLogTime = millis();
        if ((currentLogTime - previousLogTime) >= 600000)   // log every 10 minutes
        {
          EEPROMWritelong(addr, averageCount);
          addr += 4;
          EEPROMWritelong(96, addr); // write current address number to an adress just before the logged data
          previousLogTime = currentLogTime;
          EEPROM.commit();
        }
      }
    }
   */
    if (deviceMode)    // deviceMode is 1 when in monitoring station mode. Uploads CPM to thingspeak every 5 minutes
    {
      currentUploadTime = millis();
      if ((currentUploadTime - previousUploadTime) > 15000)
      {
        previousUploadTime = currentUploadTime;
        if (WiFi.status() == WL_CONNECTED)
        {
          if (!MQTTclient.connected()) 
          {
            MQTTreconnect();
          }
          if (MQTTclient.connected()) 
          {
            value = doseRate;
            snprintf (msg, MSG_BUFFER_SIZE, "DoseRate: %fuSv/hr", value);
            Serial.print("MQTT: ");
            Serial.println(msg);
            snprintf (msg, MSG_BUFFER_SIZE, "%f", value);
            snprintf (topic, MSG_BUFFER_SIZE, "%s/Doserate/uSv_hr", MQTTdeviceID );
            MQTTclient.publish(topic, msg);

            if (doseLevel == 0)
            {
              snprintf (msg, MSG_BUFFER_SIZE, "NORMAL BACKGROUND");
            }
            else if (doseLevel == 1)
            {
              snprintf (msg, MSG_BUFFER_SIZE, "ELEVATED ACTIVITY");
            }
            else if (doseLevel == 2)
            {
              snprintf (msg, MSG_BUFFER_SIZE, "HIGH RADIATION LEVEL");
            }
            Serial.print("MQTT: DoseLevel: ");
            Serial.println(msg);
            snprintf (topic, MSG_BUFFER_SIZE, "%s/Doserate/DoseLevel", MQTTdeviceID );
            MQTTclient.publish(topic, msg);
          }
        } 
      }
    }
  }
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
          count[a] = 0; // counts need to be reset to prevent errorenous readings
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
        tft.println("Rems (mR/hr)");
      }
      else if ((x > 4 && x < 234) && (y > 127 && y < 177))
      {
        doseUnits = 1;
        tft.fillRoundRect(4, 71, 232, 48, 4, ILI9341_BLACK);
        tft.setCursor(30, 103);
        tft.println("Sieverts (uSv/hr)");

        tft.fillRoundRect(4, 128, 232, 48, 4, 0x2A86);
        tft.setCursor(47, 160);
        tft.println("Rems (mR/hr)");
      }
    }
  }
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
        if (EEPROM.read(saveAlertThreshold) != alarmThreshold)
        {
          EEPROM.write(saveAlertThreshold, alarmThreshold);
          EEPROM.commit(); // save to EEPROM to be retrieved at startup

          if (deviceMode)    // deviceMode is 1 when in monitoring station mode. Uploads CPM to thingspeak every 5 minutes
          {
            if (WiFi.status() == WL_CONNECTED)
            {
              if (!MQTTclient.connected()) 
              {
                MQTTreconnect();
              }
              if (MQTTclient.connected()) 
              {
                snprintf (msg, MSG_BUFFER_SIZE, "%i", alarmThreshold);
                snprintf (topic, MSG_BUFFER_SIZE, "%s/System/AlertThreshold", MQTTdeviceID );
                Serial.print("MQTT: Topic: ");
                Serial.print(topic);
                Serial.print(": ");
                Serial.println(msg);
                MQTTclient.publish(topic, msg); 
              }
            } 
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
        if (uint32(EEPROMReadlong(saveCalibration)) != conversionFactor)
        {
          EEPROMWritelong(saveCalibration, conversionFactor);
          EEPROM.commit();

          if (deviceMode)    // deviceMode is 1 when in monitoring station mode. Uploads data
          {
            if (WiFi.status() == WL_CONNECTED)
              {
              if (!MQTTclient.connected()) 
              {
                MQTTreconnect();
              }
              if (MQTTclient.connected()) 
              {
                snprintf (msg, MSG_BUFFER_SIZE, "%i", conversionFactor);
                Serial.print("MQTT: Topic: ");
                Serial.print(ConvFactorTopic);
                Serial.print(": ");
                Serial.println(msg);
                MQTTclient.publish(ConvFactorTopic, msg); 
              }
            } 
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
  else if (page == 5)  // Wifi page
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

      if ((x > 4 && x < 62) && (y > 271 && y < 315))
      {
        page = 1;
        if (EEPROM.read(saveLoggingMode) != isLogging) // check current EEPROM value and only write if new value is different
        {
          EEPROM.write(saveLoggingMode, isLogging); 
          EEPROM.commit();
        }
        drawSettingsPage();
      }
      else if ((x > 3 && x < 237) && (y > 64 && y < 108))  // wifi setup button
      {
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

        delay(100);
        WiFiManager wifiManager;

        char channelIDSt[20] = {0};
        char writeAPISt[20] = {0};
///*
        for (int k = 52; k < 52 + MQTTserverLength; k++)
        {
          channelIDSt[k - 52] = EEPROM.read(k);
        }
//        Serial.println("READchannelIDLength: "+ String(MQTTserverLength));
//        Serial.println("READchannelIDSt: "+ String(channelIDSt));

        for (int l = 72; l < 72 + DeviceIDLength; l++)
        {
          writeAPISt[l - 72] = EEPROM.read(l);
        }
//        Serial.println("READDeviceIDLength: "+ String(DeviceIDLength));
//        Serial.println("READwriteAPISt: "+ String(writeAPISt));
//*/
        WiFiManagerParameter channel_id("0", "MQTT Server IP", channelIDSt, 20); // create custom parameters for setup
        
        WiFiManagerParameter write_api("1", "MQTT Device ID", writeAPISt, 20);
        wifiManager.addParameter(&channel_id);
        wifiManager.addParameter(&write_api);

        wifiManager.startConfigPortal("GC-20M");            // put the esp in AP mode for wifi setup, create a network with name "GC20"

        strcpy(channelIDSt, channel_id.getValue());
        strcpy(writeAPISt, write_api.getValue());
//       Serial.println("channelIDSt: "+ String(channelIDSt));
//        Serial.println("writeAPISt: "+ String(writeAPISt));

        size_t idLen = String(channelIDSt).length();

        size_t apiLen = String(writeAPISt).length();

//       Serial.println("idLen: "+ String(idLen));
//       Serial.println("apiLen: "+ String(apiLen));

        char channelInit = EEPROM.read(4001);  // first character of channelID is stored in EEPROM address 4001
//       Serial.println("channelInit: "+ String(channelInit));
        char apiKeyInit = EEPROM.read(4002);   // Only overwrite channelIDSt and writeAPISt if new value of the first character is different from what was saved.
//       Serial.println("apiKeyInit: "+ String(apiKeyInit));
        if (channelInit != channelIDSt[0])   
        {
          for (unsigned int a = 52; a < 52 + idLen; a++)
          {
            EEPROM.write((a), channelIDSt[a - 52]);
          }
//          Serial.println("WRITEchannelIDSt: "+ String(channelIDSt));
          EEPROM.write(saveIDLen, idLen);
//          Serial.println("WRITEidLen: "+ String(idLen));
        }

        if(apiKeyInit != writeAPISt[0])
        {
          for (unsigned int b = 72; b < 72 + apiLen; b++)
          {
            EEPROM.write((b), writeAPISt[b - 72]);
          }
//          Serial.println("WRITEwriteAPISt: "+ String(writeAPISt));
          EEPROM.write(saveAPILen, apiLen);
//          Serial.println("WRITEapiLen: "+ String(apiLen));
        }

        String ssidString = WiFi.SSID();      // retrieve ssid and password form the WifiManager library
        String passwordString = WiFi.psk();

        size_t ssidLen = ssidString.length();
        size_t passLen = passwordString.length();

        Serial.println(ssidLen);
        Serial.println(passLen);

        char ssidChar[20];
        char passwordChar[20];

        ssidString.toCharArray(ssidChar, ssidLen + 1); 
        passwordString.toCharArray(passwordChar, passLen + 1);

        for (unsigned int a = 12; a < 12 + ssidLen; a++)
        {
          EEPROM.write((a), ssidChar[a - 12]);             // save ssid and ssid length to EEPROM
        }
        EEPROM.write(saveSSIDLen, ssidLen);
        
        for (unsigned int b = 32; b < 32 + passLen; b++)
        {    
          EEPROM.write((b), passwordChar[b - 32]);          // save password and password length to EEPROM
        }
        EEPROM.write(savePWLen, passLen);

        EEPROM.write(4001, channelIDSt[0]);                 // save first characters of channel ID and api key to EEPROM
        EEPROM.write(4002, writeAPISt[0]);

        EEPROM.commit();

        tft.setCursor(16, 265);
        tft.println("Settings saved. Restarting");

        delay(1000);
        
        ESP.reset();
      }
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
void drawHomePage()
{
  tft.fillRect(1, 21, 237, 298, ILI9341_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_WHITE);

  tft.drawRoundRect(210, 4, 26, 14, 3, ILI9341_WHITE);
  tft.drawLine(209, 8, 209, 13, ILI9341_WHITE); // Battery symbol
  tft.drawLine(208, 8, 208, 13, ILI9341_WHITE);
  tft.fillRect(212, 6, 22, 10, ILI9341_BLACK);

  tft.fillRect(batteryMapped, 6, (234 - batteryMapped), 10, ILI9341_GREEN);
  
  tft.setTextSize(1);
  tft.setTextColor(ILI9341_CYAN);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(2, 16);
  tft.println("GC-20M");
  tft.setTextColor(ILI9341_WHITE);
  
  tft.setFont();
  tft.setTextSize(2);
  tft.setCursor(118, 4);
  tft.println("+");
  tft.setTextSize(1);
  tft.setFont(&FreeSans9pt7b);

  tft.drawBitmap(103, 2, betaBitmap, 18, 18, ILI9341_WHITE);
  tft.drawBitmap(128, 2, gammaBitmap, 12, 18, ILI9341_WHITE);

  tft.drawLine(1, 20, 238, 20, ILI9341_WHITE);
  tft.fillRoundRect(3, 23, 234, 69, 3, DOSEBACKGROUND);
  tft.setCursor(16, 40);
  tft.println("EFFECTIVE DOSE RATE:");
  tft.setCursor(165, 85);
  tft.setFont(&FreeSans12pt7b);
  
  if (doseUnits == 0)
  {
    tft.println("uSv/hr");
  }
  else if (doseUnits == 1)
  {
    tft.println("mR/hr");
  }

  tft.fillRoundRect(3, 94, 234, 21, 3, 0x2DC6);
  tft.setCursor(15, 110);
  tft.setFont(&FreeSans9pt7b);
  tft.setTextColor(ILI9341_WHITE);
  tft.setTextSize(1);
  tft.println("NORMAL BACKGROUND");
  //tft.println(utf8rus("Фон в норме")); //====================================Тут прикол!!!!!!!!!!!!!====================================

  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(7, 141);
  tft.println("CPM:");
  tft.drawRoundRect(3, 117, 234, 32, 3, DOSEBACKGROUND);

  tft.fillRoundRect(3, 151, 185, 105, 4, 0x630C);
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(9, 171);
  tft.println("CUMULATIVE DOSE");
  tft.setCursor(7, 205);
  tft.println("Counts:");
  
  if (doseUnits == 0)
  {
    tft.setCursor(34, 235);
    tft.println("uSv:");
  }
  else if (doseUnits == 1)
  {
    tft.setCursor(37, 235);
    tft.println("mR:");
  }

  tft.fillRoundRect(3, 259, 58, 57, 3, 0x3B8F);
  tft.drawBitmap(1, 257, settingsBitmap, 60, 60, ILI9341_WHITE);

  tft.fillRoundRect(64, 259, 95, 57, 3, 0x6269);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(1);
  tft.setCursor(74, 284);
  tft.println("TIMED");
  tft.setCursor(70, 309);
  tft.println("COUNT");

  if (integrationMode == 0)
  {
    tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
    tft.setCursor(180, 283);
    tft.println("INT");
    tft.setCursor(177, 309);
    tft.println("60 s");
  }
  else if (integrationMode == 1)
  {
    tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
    tft.setCursor(180, 283);
    tft.println("INT");
    tft.setCursor(184, 309);
    tft.println("5 s");
  }
  else if (integrationMode == 2)
  {
    tft.fillRoundRect(162, 259, 74, 57, 3, 0x2A86);
    tft.setCursor(180, 283);
    tft.println("INT");
    tft.setCursor(169, 309);
    tft.println("180 s");
  }

  if (ledSwitch)
  {
    tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
    tft.drawBitmap(190, 153, ledOnBitmap, 45, 45, ILI9341_WHITE);
  }
  else if (!ledSwitch)
  {
    tft.fillRoundRect(190, 151, 46, 51, 3, 0x6269);
    tft.drawBitmap(190, 153, ledOffBitmap, 45, 45, ILI9341_WHITE);
  }
  
  if (buzzerSwitch)
  {
    tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
    tft.drawBitmap(190, 208, buzzerOnBitmap, 45, 45, ILI9341_WHITE);
  }
  else if (!buzzerSwitch)
  {
    tft.fillRoundRect(190, 205, 46, 51, 3, 0x6269);
    tft.drawBitmap(190, 208, buzzerOffBitmap, 45, 45, ILI9341_WHITE);
  }

  tft.setFont(&FreeSans9pt7b);

  if (MQTTclient.connected())
  {
  //  tft.setTextSize(1);
  //  tft.setFont(&FreeSans9pt7b);
    tft.setCursor(157, 16);
    tft.println("M");
  }
  else
  {
    tft.fillRect(157, 2, 18, 18, ILI9341_BLACK);
  }

  if (isLogging)
  {
    tft.setCursor(175, 16);
    tft.println("L");
  }
  else
  {
    tft.fillRect(175, 2, 18, 18, ILI9341_BLACK);
  }
  
  if (deviceMode)
  {
    tft.drawBitmap(188, 1, wifiBitmap, 19, 19, ILI9341_WHITE);
  }
  else
  {
    tft.fillRect(188, 1, 19, 19, ILI9341_BLACK);
  }
}
//                                                         void drawHomePage()
//=============================================================================================================================
void drawSettingsPage()
{
  digitalWrite(D3, LOW);
  digitalWrite(D0, LOW);

  drawFrame();
  drawBackButton();

  tft.fillRoundRect(3, 23, 234, 35, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(57, 48);
  tft.println("SETTINGS");
  tft.drawFastHLine(59, 51, 117, WHITE);

  tft.fillRoundRect(3, 64, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 64, 234, 44, 4, WHITE);
  tft.setCursor(44, 94);
  tft.println("DOSE UNITS");

  tft.fillRoundRect(3, 114, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 114, 234, 44, 4, WHITE);
  tft.setCursor(5, 145);
  tft.println("ALERT THRESHOLD");

  tft.fillRoundRect(3, 164, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 164, 234, 44, 4, WHITE);
  tft.setCursor(37, 194);
  tft.println("CALIBRATION");

  tft.fillRoundRect(3, 214, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 214, 234, 44, 4, WHITE);
  tft.setCursor(8, 244);
  tft.println("LOGGING AND WIFI");
}
//=============================================================================================================================
void drawUnitsPage()
{
  drawFrame();
  drawBackButton();

  tft.fillRoundRect(3, 23, 234, 40, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(84, 51);
  tft.println("UNITS");
  tft.drawFastHLine(86, 55, 71, WHITE);

  tft.drawRoundRect(3, 70, 234, 50, 4, WHITE);
  if (doseUnits == 0)
    tft.fillRoundRect(4, 71, 232, 48, 4, 0x2A86);
  tft.setCursor(30, 103);
  tft.println("Sieverts (uSv/hr)");

  tft.drawRoundRect(3, 127, 234, 50, 4, WHITE);
  if (doseUnits == 1)
    tft.fillRoundRect(4, 128, 232, 48, 4, 0x2A86);
  tft.setCursor(47, 160);
  tft.println("Rems (mR/hr)");
}
//=============================================================================================================================
void drawAlertPage()
{
  drawFrame();
  drawBackButton();

  tft.fillRoundRect(3, 23, 234, 40, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(4, 51);
  tft.println("ALERT THRESHOLD");
  tft.drawFastHLine(5, 55, 229, WHITE);
  
  tft.setCursor(30, 164);
  tft.println("uSv/hr:");

  tft.drawRoundRect(130, 70, 60, 60, 4, ILI9341_WHITE);
  tft.fillRoundRect(131, 71, 58, 58, 4, 0x2A86);
  tft.drawRoundRect(130, 185, 60, 60, 4, ILI9341_WHITE);
  tft.fillRoundRect(131, 186, 58, 58, 4, 0x2A86);

  tft.setCursor(140, 113);
  tft.setTextSize(3);
  tft.println("+");
  tft.setCursor(148, 232);
  tft.println("-");
  tft.setTextSize(1);
}
//=============================================================================================================================
void drawCalibrationPage()
{
  drawFrame();
  drawBackButton();

  tft.fillRoundRect(3, 23, 234, 40, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(47, 51);
  tft.println("CALIBRATE");
  tft.drawFastHLine(48, 55, 133, WHITE);

  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(8, 154);
  tft.println("Conversion Factor");
  tft.setCursor(8, 174);
  tft.println("(CPM per uSv/hr)");

  tft.drawRoundRect(160, 70, 60, 60, 4, ILI9341_WHITE);
  tft.fillRoundRect(161, 71, 58, 58, 4, 0x2A86);
  tft.drawRoundRect(160, 185, 60, 60, 4, ILI9341_WHITE);
  tft.fillRoundRect(161, 186, 58, 58, 4, 0x2A86);

  tft.setCursor(170, 113);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(3);
  tft.println("+");
  tft.setCursor(178, 232);
  tft.println("-");
  tft.setTextSize(1);
}
//=============================================================================================================================
void drawWifiPage()
{
  drawFrame();
  drawBackButton();

  tft.fillRoundRect(3, 23, 234, 35, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(7, 48);
  tft.println("LOGGING AND WIFI");
  tft.drawFastHLine(8, 51, 222, WHITE);

  tft.fillRoundRect(3, 64, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 64, 234, 44, 4, WHITE);
  tft.setCursor(48, 94);
  tft.println("WIFI SETUP");

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
  
  tft.fillRoundRect(3, 164, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 164, 234, 44, 4, WHITE);
  tft.setCursor(31, 194);
  tft.println("UPLOAD DATA");

  tft.fillRoundRect(3, 214, 234, 44, 4, 0x2A86);
  tft.drawRoundRect(3, 214, 234, 44, 4, WHITE);
  tft.setCursor(35, 244);
  tft.println("DEVICE MODE");

/*
  if (addr > 2000)
  {
    tft.setFont(&FreeSans9pt7b);
    tft.setCursor(80, 297);
    tft.println("Log memory full"); 
  }
*/
}
//=============================================================================================================================
void drawTimedCountPage()
{
  drawFrame();
  drawBackButton();

  tft.fillRoundRect(145, 271, 92, 45, 3, 0x3B8F);
  tft.drawRoundRect(145, 271, 92, 45, 3, ILI9341_WHITE);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(149, 302);
  tft.println("BEGIN!");

  tft.fillRoundRect(3, 23, 234, 40, 3, 0x3B8F);
  tft.setCursor(34, 51);
  tft.println("TIMED COUNT");
  tft.drawFastHLine(35, 55, 163, WHITE);

  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(5, 162);
  tft.println("Duration (minutes):");

  tft.drawRoundRect(160, 70, 60, 60, 4, ILI9341_WHITE);
  tft.fillRoundRect(161, 71, 58, 58, 4, 0x2A86);
  tft.drawRoundRect(160, 185, 60, 60, 4, ILI9341_WHITE);
  tft.fillRoundRect(161, 186, 58, 58, 4, 0x2A86);

  tft.setCursor(170, 113);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(3);
  tft.println("+");
  tft.setCursor(178, 232);
  tft.println("-");
  tft.setTextSize(1);

  cpm = 0;
  progress = 0;
}
//=============================================================================================================================
void drawTimedCountRunningPage(int duration, int size)
{
  drawFrame();
  drawCancelButton();

  tft.fillRoundRect(3, 23, 234, 40, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setTextSize(1);
  tft.setCursor(34, 51);
  tft.println("TIMED COUNT");
  tft.drawFastHLine(35, 55, 163, WHITE);

  tft.drawRoundRect(3, 66, 234, 95, 4, ILI9341_WHITE);
  tft.drawRect(10, 103, 220, 20, ILI9341_WHITE);
  tft.drawRoundRect(3, 164, 234, 103, 4, ILI9341_WHITE);

  tft.setCursor(58, 90);
  tft.println("Progress:");
  tft.setCursor(13, 150);
  tft.println("Duration:");
  tft.setCursor(115, 150);
  tft.println(duration);
  tft.setCursor((135 + (size - 1)*15), 150);
  tft.println("min");
  tft.setCursor(15, 200);
  tft.println("Counts:");
  tft.setCursor(37, 245);
  tft.println("CPM:");

  currentCount = 0;
  startMillis = millis();
  intervalMillis = duration * 60000;
  completed = 0;
}
//=============================================================================================================================
void drawDeviceModePage()
{
  drawFrame();
  drawBackButton();

  tft.fillRoundRect(3, 23, 234, 40, 3, 0x3B8F);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(34, 51);
  tft.println("DEVICE MODE");
  tft.drawFastHLine(35, 57, 160, WHITE);

  tft.drawRoundRect(3, 70, 234, 50, 4, WHITE);
  if (deviceMode == 0)
  tft.fillRoundRect(4, 71, 232, 48, 4, 0x2A86);
  tft.setCursor(13, 103);
  tft.println("GEIGER COUNTER");

  tft.drawRoundRect(3, 127, 234, 50, 4, WHITE);
  if (deviceMode == 1)
  tft.fillRoundRect(4, 128, 232, 48, 4, 0x2A86);
  tft.setCursor(30, 160);
  tft.println("MON. STATION");

  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(20, 200);
  tft.println("Press Back button and");
  tft.setCursor(20, 220);
  tft.println("reset device for changes");
  tft.setCursor(20, 240);
  tft.println("to take effect");
}
//=============================================================================================================================
void IRAM_ATTR isr() // interrupt service routine
{
  if ((micros() - 200) > previousIntMicros)                           //!!!!!!!!!!!!!! Вытащить в переменную Dead_Time_Geiger!!!!!!!!!!!!!!!!!!!!!!!!
  {
    currentCount++;
    cumulativeCount++;
  }
  previousIntMicros = micros();
}
//=============================================================================================================================
void drawBackButton()
{
  tft.fillRoundRect(4, 271, 62, 45, 3, 0x3B8F);
  tft.drawRoundRect(4, 271, 62, 45, 3, ILI9341_WHITE);
  tft.drawBitmap(4, 271, backBitmap, 62, 45, ILI9341_WHITE);
}
//=============================================================================================================================
void drawFrame(){
  tft.fillRect(2, 21, 236, 298, ILI9341_BLACK);
  tft.drawRect(0, 0, tft.width(), tft.height(), ILI9341_WHITE);

  tft.drawRoundRect(210, 4, 26, 14, 3, ILI9341_WHITE);
  tft.drawLine(209, 8, 209, 13, ILI9341_WHITE); // Battery symbol
  tft.drawLine(208, 8, 208, 13, ILI9341_WHITE);
  tft.fillRect(212, 6, 22, 10, ILI9341_BLACK);
  tft.fillRect(batteryMapped, 6, (234 - batteryMapped), 10, ILI9341_GREEN);
  
  tft.setFont(&FreeSans9pt7b);
  tft.setCursor(2, 16);
  tft.setTextColor(ILI9341_CYAN);
  
  tft.setTextSize(1);
  tft.println("GC-20M");
  tft.setTextColor(ILI9341_WHITE);

  tft.setFont();
  tft.setTextSize(2);
  tft.setCursor(118, 4);
  tft.println("+");
  tft.setTextSize(1);
  tft.setFont(&FreeSans9pt7b);

  tft.drawBitmap(103, 2, betaBitmap, 18, 18, ILI9341_WHITE);
  tft.drawBitmap(128, 2, gammaBitmap, 12, 18, ILI9341_WHITE);

  tft.drawLine(1, 20, 238, 20, ILI9341_WHITE);
  tft.setFont(&FreeSans9pt7b);

  if (MQTTclient.connected())
  {
  //  tft.setTextSize(1);
  //  tft.setFont(&FreeSans9pt7b);
    tft.setCursor(157, 16);
    tft.println("M");
  }
  else
  {
    tft.fillRect(157, 2, 18, 18, ILI9341_BLACK);
  }

  if (isLogging)
  {
    tft.setCursor(175, 16);
    tft.println("L");
  }
  else
  {
    tft.fillRect(175, 2, 18, 18, ILI9341_BLACK);
  }
  
  if (deviceMode)
  {
    tft.drawBitmap(188, 1, wifiBitmap, 18, 18, ILI9341_WHITE);
  }
  else
  {
    tft.fillRect(188, 1, 19, 19, ILI9341_BLACK);
  }
}//                                                          void drawFrame()
//=============================================================================================================================
void drawCancelButton()
{
  tft.fillRoundRect(70, 271, 100, 45, 3, 0xB9C7);
  tft.drawRoundRect(70, 271, 100, 45, 3, ILI9341_WHITE);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(72, 302);
  tft.println("CANCEL");
}
//=============================================================================================================================
void drawCloseButton()
{
  tft.fillRoundRect(70, 271, 100, 45, 3, 0x3B8F);
  tft.drawRoundRect(70, 271, 100, 45, 3, ILI9341_WHITE);
  tft.setTextSize(1);
  tft.setFont(&FreeSans12pt7b);
  tft.setCursor(79, 302);
  tft.println("CLOSE");
}
//=============================================================================================================================
void drawBlankDialogueBox()
{
  tft.setFont(&FreeSans9pt7b);
  tft.setTextSize(1);
  tft.fillRoundRect(20, 50, 200, 220, 6, ILI9341_BLACK);
  tft.drawRoundRect(20, 50, 200, 220, 6, ILI9341_WHITE);
}
//=============================================================================================================================