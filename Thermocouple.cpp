#include "data_structs.h"
#include "functions.h"
#include "Adafruit_MCP9600.h"
#include <Wire.h> 

#define MCP9600_STATUS_OPEN (0x01)

Adafruit_MCP9600 mcp; //thermocouple 

namespace Thermocouple {
  bool init() {
    mcp.begin(adress); 
  }

  bool read() {
    temp_hot.val = mcp.readThermocouple(); 
    temp_cold.val = mcp.readAmbient(); 
    dtostrf(temp_hot.val,  7,  2, temp_hot.str_val);  // -00.00 -> _00.00
    dtostrf(temp_cold.val, 7,  2, temp_cold.str_val); // -00.00 -> _00.00
    if ((temp_hot.val == temp_hot.val) && (temp_cold.val == temp_cold.val)) {
      return 1; 
    }
    return 0; 
  }
}