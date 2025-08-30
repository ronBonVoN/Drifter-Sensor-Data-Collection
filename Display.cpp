#include "data_structs.h"
#include "functions.h"
#include <Arduino.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Wire.h> 

#define SCREEN_ADDRESS (0x3C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 2

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

namespace Display {

  bool init() {
    if (count>0) {
      return display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS); 
    }
  }
  
  void write(const char* msg, unsigned int wait) {
    if (serialEnable) Serial.print(msg); 
    if (count>0) {
      display.clearDisplay(); 
      display.setTextColor(WHITE); 
      display.setTextSize(1); 
      display.setCursor(0,20);
      display.print(msg); 
      display.display();
      delay(wait); 
    }
  }

  void data(const char* data, int wait) {
    if (serialEnable) Serial.print(data); 
    if (count>0) {
      display.clearDisplay(); 
      display.setTextColor(WHITE); 
      display.setTextSize(1); 
      display.setCursor(0,0);
      display.print(data); 
      display.display();
      delay(wait); 
    }
  }

  void sleep() {
    write("Sleeping display....\n", 2000); 
    display.clearDisplay(); 
    display.display(); 
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }

}