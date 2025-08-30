#include "functions.h"
#include "data_structs.h"
#include <Arduino.h>
#include <SD.h>

namespace SDcard {
  bool init() {
    return ::SD.begin(pin); 
  }

  void write(const char* msg) { 
    Display::write("Writing to SD card...\n"); 
    if (serialEnable) Serial.print(msg); 
    File dataFile = ::SD.open(fileName, FILE_WRITE);
    if (dataFile) {
      status.status = 1; 
      dataFile.print(msg);
      dataFile.close(); 
    }
    else { 
      status.status = 0; 
      snprintf(line, sizeof(line), "Error opening %s\n", fileName); 
      Display::write(line); 
      if (LED::count>0) LED::blink(10, 5000); 
      else delay(10000);
    }
  } 
}
