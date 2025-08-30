#ifndef FUNCTIONS_H
#define FUNCTIONS_H
#pragma once
#include "data_structs.h"

inline int serialEnable = 1; 
inline char line[400]; 

namespace SDcard {
  inline int pin = 53; 
  inline char fileName[10] = "DATA.txt"; 
  inline Data status = {{"SD Card Status", "bool"}, -1.0, "-1", 1}; 
  bool init(); 
  void write(const char* msg); 
}

namespace Display {
  inline short count = 200;  
  bool init(); 
  void write(const char* msg, unsigned int wait = 0); 
  void data(const char* data, int wait); 
  void sleep(); 
}

namespace GPS {
  inline int timeAdjust = -4; 
  inline Date Air530_date = {{"Air530 Date", "UTC"}, 0, 0, 2000, "00", "00", "2000"};
  inline Time Air530_time = {{"Air530 Time", "UTC"}, 0, 0, 0, "00", "00", "00", "00:00:00"};
  inline Data Air530_lat = {{"Air530 Latitude", "GCS"}, -1, "-1", 1}; 
  inline Data Air530_lng = {{"Air530 Longitude", "GCS"}, -1, "-1", 1}; 
  inline Date SIM7600_date = {{"SIM7600 Date", "UTC"}, 0, 0, 2000, "00", "00", "2000"};
  inline Time SIM7600_time = {{"SIM7600 Time", "UTC"}, 0, 0, 0, "00", "00", "00", "00:00:00"};
  inline Data SIM7600_lat = {{"SIM7600 Latitude", "GCS"}, -1, "-1", 1}; 
  inline Data SIM7600_lng = {{"SIM7600 Longitude", "GCS"}, -1, "-1", 1}; 
  void setup(); 
  void init(Date &date, Time &time); 
  void get(int wait); 
  void timezone(Date &date, Time &time); 
  bool read(Date &date, Time &time, Data &lat, Data &lng); 
}

namespace Cellular {
  inline const char url[200] = "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj";
  // for testing -> "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj"
  // for field   -> "https://discordapp.com/api/webhooks/1403412309438627851/RdeuTCbLB5Ul039usaiiCX5YhAeQGqdQAo4tfQz-Igut3GvUrKP22cAfCnrDrNmX5mA6"
  inline char output[512]; 
  void setup(); 
  bool command(const char* cmd, const char* res = "OK", unsigned int wait = 10000); 
  bool init(); 
  void send(const char* msg, const char* reason); 
  void sleep(); 
  void header(); 
  void data(); 
}

namespace LED {
  inline int pin = 12; 
  inline int count = 1000; 
  void blink(unsigned int num, unsigned int wait = 3000, unsigned int pause = 300); 
}

namespace Relay {
  inline int pin = 4;  
}

namespace Thermocouple {
  inline Data temp_hot = {{"Hot Junction", "C"}, -1.0, "-1", 1}; 
  inline Data temp_cold = {{"Cold Junction", "C"}, -1.0, "-1", 1};
  inline uint8_t adress = 0x67; 
  bool init(); 
  bool read(); 
}

namespace Turbidity {
  inline Data volts = {{"Turbidity", "volts"}, -1.0, "-1", 1};
  inline int pin = A4;  
  bool read(); 
}

namespace Arduino {
  bool initialize(bool device, const char* name); 
  void sleep(const int loops); 
}

#endif