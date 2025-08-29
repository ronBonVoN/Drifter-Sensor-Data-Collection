#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#pragma once
#include "data_structs.h"

void blink(unsigned short num, unsigned int wait=3000, unsigned int pause = 300);

void print_SerialDisplay(const char* message, int wait = 0); 
void print_SerialFile(const char* message); 

void setupGPS(); 
void readGPS(int wait);
void adjustTime(Date date, Time time);

bool commandCell(const char* cmd, const char* res = "OK", unsigned int wait = 10000);
bool setupCell();
bool sendCell(const char* message, const char* reason);
void getCellGPS();

void sleepArduino(const int loops); 

#endif