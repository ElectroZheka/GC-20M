// ========================================
// ==== Settings LEVEL 1 (required) =======
// ========================================
#ifndef SETTINGS_INCLUDED
#define SETTINGS_INCLUDED

#define DEBUG_BAUD 921600
#define DEBUG_MODE true // change "true" to "false" to disable
//#define DEBUG_MODE false  // change "false" to "true" to enable
// Next, logs levels for comfortable deallbugging, 
// if DEBUG_MODE == false, logs level are not important 
#define DEBUG_TS true         // print touchscreen logs
#define DEBUG_WiFi true       // print WiFi logs
#define DEBUG_AP true         // print WiFi logs
#define DEBUG_MQTT true       // print MQTT logs
#define DEBUG_EEPROM true     // print EEPROM logs
#define DEBUG_BATT true       // print BATT logs
#define DEBUG_TEST true       // print BATT logs

#define BUZZER_PIN D0         // 
#define ACT_LED D3            // 

#define CS_PIN D2             // Touchscreen CS Pin

#define BATT_PIN A0           // ADC Battery input Pin

#define TFT_DC D4             // + builtin led
#define TFT_CS D8

#define BLACK 0x0000
#define BLUE 0x001F
#define RED 0xF800
#define GREEN 0x07E0
#define CYAN 0x07FF
#define MAGENTA 0xF81F
#define YELLOW 0xFFE0
#define WHITE 0xFFFF
#define DOSEBACKGROUND 0x0455

//#define TS_MINX 250
//#define TS_MINY 200 // calibration points for touchscreen
//#define TS_MAXX 3800
//#define TS_MAXY 3750

#define TS_MINX 450
#define TS_MINY 350 // calibration points for touchscreen
#define TS_MAXX 3300
#define TS_MAXY 3700

#define Dead_Time_Geiger 150  // Dead time Geiger sensor in uS 

#define MSG_BUFFER_SIZE	(50)

#endif