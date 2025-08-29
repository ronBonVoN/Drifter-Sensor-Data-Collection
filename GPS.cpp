#define TINY_GSM_MODEM_SIM7600 //needed for cellular module to work
#define GPS_SERIAL Serial1

#include <TinyGPS++.h>
#include "functions.h"
#include "data_structs.h"

TinyGPSPlus gps;  

Date Air530_date; Time Air530_time; 
Data Air530_lat; Data Air530_lng; Data Air530_speed; Data Air530_heading;  

short timeAdjust = -4;

void setupGPS() {
  GPS_SERIAL.begin(9600); 

  snprintf(Air530_date.header.name, sizeof(Air530_date.header.name), "Air530 Date"); 
  snprintf(Air530_date.units, sizeof(Air530_date.units), "UCT+%hdhrs", timeAdjust); 
  
  snprintf(Air530_time.header.name, sizeof(Air530_time.header.name), "Air530 Time"); 
  snprintf(Air530_time.units, sizeof(Air530_time.units), "UCT+%hdhrs", timeAdjust); 
  
  snprintf(Air530_lat.header.name, sizeof(Air530_lat.header.name), "Air530 Latitude"); 
  snprintf(Air530_lat.header.units, sizeof(Air530_lat.header.units), "GCS"); 
  
  snprintf(Air530_lng.header.name, sizeof(Air530_lng.header.name), "Air530 Longitude"); 
  snprintf(Air530_lat.header.units, sizeof(Air530_lat.header.units), "GCS"); 

  snprintf( Air530_speed.header.name, sizeof( Air530_speed.header.name), "speed");
  snprintf( Air530_speed.header.units, sizeof( Air530_speed.header.units), "mph");

  snprintf( Air530_heading.header.name, sizeof( Air530_heading.header.name), "heading");
  snprintf( Air530_heading.header.units, sizeof( Air530_heading.header.units), "deg");
} 

void readGPS(int wait) {
  commandCell("AT+CGPS=1"); 
  unsigned long start = millis(); 
  unsigned long now = millis(); 
  
  while (millis() - start < wait) {  
    while (GPS_SERIAL.available()) gps.encode(GPS_SERIAL.read());
    if (millis() - now >= 300) { // need to use while() instead of delay() for
        now += 300;              // blinking during reading gps to not pause reading
        digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // togel LED
    }
  }
  if (digitalRead(LED_PIN)) digitalWrite(LED_PIN, LOW); 
  
  getCellGPS(); 
  commandCell("AT+CGPS=0"); 

  Air530_date.month = gps.date.month(); 
  Air530_date.day = gps.date.day(); 
  Air530_date.year = gps.date.year(); 
  Air530_time.hr = gps.time.hour(); 
  Air530_time.min = gps.time.minute();  
  Air530_time.sec = gps.time.second(); 
  
  adjustTime(Air530_date, Air530_time); 
}

void adjustTime(Date date, Time time) {
  //adjusts timezone based on adjust amount varibale 
  short monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //for adjusting timezone
  
  if (time.hr<=0 && time.min<=0 && time.sec<=0) return; 
  else time.hr += timeAdjust; 

  if (date.month<=0 && date.day<=0 && date.year<=2000) return; 
  else {
    if (time.hr<=0) {
      time.hr += 24;
      date.day -= 1 ;
    } 
    if (time.hr>=24) {
      time.hr -= 24; 
      date.day += 1; 
    }

    if ((date.year % 4 == 0) && (date.year % 100 != 0) || (date.year % 400 == 0)) monthDays[2-1] = 29;    
    
    if (date.day==0) {
      date.month -= 1;
      if (date.month>0) d = monthDays[m-1]; 
      if (date.month==0) {
        date.month = 12; 
        date.day = monthDays[12-1]; 
        date.year -= 1; 
      }
    }

    if (date.day>monthDays[m-1]) {
      date.month += 1;
      if (date.month<=12) d = 1;
      if (date.month==13) {
        date.month = 1; 
        date.day = 1; 
        date.year += 1; 
      }
    }
  }
}