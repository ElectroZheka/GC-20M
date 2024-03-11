#ifndef DISPLAY_INCLUDED
#define DISPLAY_INCLUDED

#include "../settings/settings.h"

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
  //tft.println(utf8rus("Фон в норме")); //====================================!!!!!!!!!!!!!====================================

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
/*
  if (isLogging)
  {
    tft.setCursor(175, 16);
    tft.println("L");
  }
  else
  {
    tft.fillRect(175, 2, 18, 18, ILI9341_BLACK);
  }
 */ 
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
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(ACT_LED, LOW);

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

#endif