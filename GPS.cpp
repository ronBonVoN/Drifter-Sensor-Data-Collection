#include "functions.h"
#include "data_structs.h"
#include <TinyGPS++.h>
#include <Arduino.h>

#define GPS_SERIAL Serial1

TinyGPSPlus gps;  

namespace GPS {
  void setup() {
    GPS_SERIAL.begin(9600); 
  }

  void init(Date &date, Time &time) {
    snprintf(date.header.unit, sizeof(date.header.unit), "UTC+%dhrs", timeAdjust); 
    snprintf(time.header.unit, sizeof(time.header.unit), "UTC+%dhrs", timeAdjust); 
  }

  void get(int wait) {
    Display::write("Reading gps...\n"); 
    Cellular::command("AT+CGPS=1"); 
    unsigned long start = millis(); 
    unsigned long now = millis(); 
    
    while (millis() - start < wait) {  
      while (GPS_SERIAL.available()) gps.encode(GPS_SERIAL.read());
      if (millis() - now >= 300) { // need to use while() instead of delay() for
          now += 300;              // blinking during reading gps to not pause reading
          digitalWrite(LED::pin, !digitalRead(LED::pin)); // togel LED
      }
    }
    if (digitalRead(LED::pin)) digitalWrite(LED::pin, LOW); 
    
    if (Cellular::command("AT+CGPSINFO") && Cellular::output[27] != ',') {    
      strncpy(SIM7600_lat.str_val,    Cellular::output+27,11);   SIM7600_lat.str_val[13] = '\0';    
      strncpy(SIM7600_lng.str_val,    Cellular::output+41,12);   SIM7600_lng.str_val[14] = '\0';   
      strncpy(SIM7600_date.str_day,   Cellular::output+56, 2);   SIM7600_date.str_day[2] = '\0';
      strncpy(SIM7600_date.str_month, Cellular::output+58, 2); SIM7600_date.str_month[2] = '\0';
      strncpy(SIM7600_date.str_year,  Cellular::output+60, 2);  SIM7600_date.str_year[2] = '\0';
      strncpy(SIM7600_time.str_hr,    Cellular::output+63, 2);    SIM7600_time.str_hr[2] = '\0';
      strncpy(SIM7600_time.str_min,   Cellular::output+65, 2);   SIM7600_time.str_min[2] = '\0';
      strncpy(SIM7600_time.str_sec,   Cellular::output+67, 2);   SIM7600_time.str_sec[2] = '\0';
      
      SIM7600_lat.val = atof(SIM7600_lat.str_val); 
      SIM7600_lng.val = atof(SIM7600_lng.str_val);

      SIM7600_lat.val = floor(SIM7600_lat.val/100) + (SIM7600_lat.val - floor(SIM7600_lat.val/100)*100)/60;
      SIM7600_lng.val = floor(SIM7600_lng.val/100) + (SIM7600_lng.val - floor(SIM7600_lng.val/100)*100)/60;
      
      if (Cellular::output[39] == 'S')  SIM7600_lat.val *= -1; 
      if (Cellular::output[54] == 'W')  SIM7600_lng.val *= -1; 
      
      SIM7600_date.month = atoi(SIM7600_date.str_month); 
      SIM7600_date.day = atoi(SIM7600_date.str_day); 
      SIM7600_date.year = atoi(SIM7600_date.str_year) + 2000; 
      SIM7600_time.hr = atoi(SIM7600_time.str_hr); 
      SIM7600_time.min = atoi(SIM7600_time.str_min); 
      SIM7600_time.sec = atoi(SIM7600_time.str_sec); 
    } 
    else {
      SIM7600_lat.val = 0.0; 
      SIM7600_lng.val = 0.0;
      SIM7600_date.month = 0; 
      SIM7600_date.day = 0; 
      SIM7600_date.year = 2000; 
      SIM7600_time.hr = 0; 
      SIM7600_time.min = 0; 
      SIM7600_time.sec = 0; 
    }

    Cellular::command("AT+CGPS=0"); 

    Air530_date.month = gps.date.month(); 
    Air530_date.day = gps.date.day(); 
    Air530_date.year = gps.date.year(); 
    Air530_time.hr = gps.time.hour(); 
    Air530_time.min = gps.time.minute();  
    Air530_time.sec = gps.time.second(); 
    Air530_lat.val = gps.location.lat(); 
    Air530_lng.val = gps.location.lng(); 

    Display::write("\ngps reading done.\n"); 
  }

  bool read(Date &date, Time &time, Data &lat, Data &lng) {
    snprintf(date.str_date, sizeof(date.str_date), "%02d-%02d-%04d",
    date.month, date.day, date.year); 

    snprintf(time.str_time, sizeof(time.str_time), "%02d:%02d:%02d", 
    time.hr, time.min, time.sec); 

    dtostrf(lat.val, 10, 6, lat.str_val); // -90.000000 -> _90.000000
    dtostrf(lng.val, 10, 6, lng.str_val); // -90.000000 -> _90.000000
    
    if (date.month<=0 && date.day<=0 && date.year <= 2000 &&
      time.hr<=0 && time.min <=0 && time.sec <=0 &&
      lat.val<=0 && lng.val <=0
    ) return 1; 
    
    return 0; 
  } 

  void timezone(Date &date, Time &time) {
    //adjusts timezone based on adjust amount varibale 
    int monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //for adjusting timezone
    
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
        if (date.month>0) date.day = monthDays[date.month-1]; 
        if (date.month==0) {
          date.month = 12; 
          date.day = monthDays[12-1]; 
          date.year -= 1; 
        }
      }

      if (date.day>monthDays[date.month-1]) {
        date.month += 1;
        if (date.month<=12) date.day = 1;
        if (date.month==13) {
          date.month = 1; 
          date.day = 1; 
          date.year += 1; 
        }
      }
    }
  }
}
