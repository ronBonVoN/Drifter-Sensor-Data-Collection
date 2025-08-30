#include "data_structs.h"
#include "functions.h"

#define MCP9600_STATUS_OPEN (0x01)

namespace Turbidity {
  bool read() {
    volts.val = analogRead(pin) * (5.0/1024.0);
    dtostrf(volts.val, 4,  2, volts.str_val); // 0.00
    if (volts.val>0 && volts.val<5) {
      return 1; 
    }
    return 0; 
  }
}