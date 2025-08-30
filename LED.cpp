#include "data_structs.h"
#include "functions.h"
#include <Arduino.h>

namespace LED {
  void blink(unsigned int num, unsigned int wait, unsigned int pause) {
    if (count>0) {
      for (int i=0; i<num; i++) {
        digitalWrite(pin, HIGH); 
        delay(pause); 
        digitalWrite(pin, LOW); 
        delay(pause); 
      }
      delay(wait);
    } 
  }
}