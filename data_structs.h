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
  short hr ; 
  short min; 
  short sec ; 
  char str_hr[3]; 
  char str_min[3]; 
  char str_sec[3];
  char str_time[9]; 
};  

#endif