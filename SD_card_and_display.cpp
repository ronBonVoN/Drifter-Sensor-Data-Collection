#include "data_structs.h"
#include "functions.h"
#include <Arduino.h>
#include <SD.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_ADDRESS (0x3C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Data SDcard = {{"SD Card Status", "bool"}, -1.0, "-1", 1, 0x00, 53}; 

const char fileName[13] = "DATA.txt"; 
short displayCount = 2; //# of cycles display will run
const bool serialEnable = 0;  

void print_SerialDisplay(const char* message, int wait = 0) {
  if (serialEnable) Serial.print(message); 
  if (displayCount>0) {
    display.clearDisplay(); 
    display.setTextColor(WHITE); 
    display.setTextSize(1); 
    display.setCursor(0,20);
    display.print(message); 
    display.display();
    delay(wait); 
  }
}

bool print_SerialFile(const char* message, const char* filName) { 
  char error[50]; 
  if (serialEnable) Serial.print(message); 
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    SDcard.status = 1; 
    dataFile.print(message);
    dataFile.close();
  }
  else { 
    SDcard.status = 0; 
    snprintf(error, sizeof(error), "Error opening %s\n", fileName);
    print_SerialDisplay(error);
  }
}