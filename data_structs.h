#ifndef DATA_STRUCTS_H
#define DATA_STUCTS_H
#include <Arduino.h>
#pragma once

struct Header {
  char name[20]; 
  char unit[10]; 
};

struct Data {
  Header header; 
  float val; 
  char str_val[20]; 
  bool status; 
  uint8_t adress; 
  short pin; 
};

struct Date {
  Header header; 
  short month; 
  short day; 
  short year; 
  char str_month[3]; 
  char str_day[3]; 
  char str_year[5];
  char str_date[11];
}; 

struct Time {
  Header header; 
  short hr = -1.0; 
  short min = -1.0; 
  short sec = -1.0; 
  char str_hr[3] = "00"; 
  char str_min[3] = "00"; 
  char str_sec[3] = "00";
  char str_time[9] = "00:00:00"; 
};

#endif