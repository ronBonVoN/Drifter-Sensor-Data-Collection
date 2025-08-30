/*
Veronika Davis
Kilroy was here
*/

#include "functions.h"
#include "data_structs.h"
#include <Arduino.h>
#include <Wire.h> 
 
char drifterName[20] = "Hans"; 
bool firstRun = 1; 

void setup() { 
  delay(5000); 
  if (serialEnable) Serial.begin(115200);
  GPS::setup(); 
  Cellular::setup(); 
  
  Wire.begin(); 
  Wire.setClock(100000); 

  pinMode(LED::pin, OUTPUT);  
  pinMode(Relay::pin, OUTPUT);
  digitalWrite(Relay::pin, HIGH); 
  delay(5000); 
  
  if (Arduino::initialize(Display::init(), "display")) LED::blink(1); 
  if (Arduino::initialize(Thermocouple::init(), "thermocouple")) LED::blink(2); 
  if (Arduino::initialize(SDcard::init(), "SD card")) LED::blink(3); 

  GPS::init(GPS::Air530_date, GPS::Air530_time); 
  GPS::init(GPS::SIM7600_date, GPS::SIM7600_time); 
  
  //file heading
  snprintf(line, sizeof(line), "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
    GPS::Air530_date.header.name, GPS::Air530_time.header.name, GPS::Air530_lat.header.name, GPS::Air530_lng.header.name, 
    GPS::SIM7600_date.header.name, GPS::SIM7600_time.header.name, GPS::SIM7600_lat.header.name, GPS::SIM7600_lng.header.name, 
    Thermocouple::temp_hot.header.name, Thermocouple::temp_cold.header.name, 
    Turbidity::volts.header.name, 
    GPS::Air530_date.header.unit, GPS::Air530_time.header.unit, GPS::Air530_lat.header.unit, GPS::Air530_lng.header.unit, 
    GPS::SIM7600_date.header.unit, GPS::SIM7600_time.header.unit, GPS::SIM7600_lat.header.unit, GPS::SIM7600_lng.header.unit,  
    Thermocouple::temp_hot.header.unit, Thermocouple::temp_cold.header.unit, 
    Turbidity::volts.header.unit
  ); 
  
  SDcard::write(line); 

  //cellular module heading 
  snprintf(line, sizeof(line), "{\"content\":\"^%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s(newline)%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s\"}",
    drifterName, 
    SDcard::status.header.name, 
    GPS::Air530_date.header.name, GPS::Air530_time.header.name, GPS::Air530_lat.header.name, GPS::Air530_lng.header.name, 
    GPS::SIM7600_date.header.name, GPS::SIM7600_time.header.name, GPS::SIM7600_lat.header.name, GPS::SIM7600_lng.header.name, 
    Thermocouple::temp_hot.header.name, Thermocouple::temp_cold.header.name, 
    Turbidity::volts.header.name, 
    SDcard::status.header.unit, 
    GPS::Air530_date.header.unit, GPS::Air530_time.header.unit, GPS::Air530_lat.header.unit, GPS::Air530_lng.header.unit, 
    GPS::SIM7600_date.header.unit, GPS::SIM7600_time.header.unit, GPS::SIM7600_lat.header.unit, GPS::SIM7600_lng.header.unit,  
    Thermocouple::temp_hot.header.unit, Thermocouple::temp_cold.header.unit, 
    Turbidity::volts.header.unit
  ); 

  Cellular::init(); 
  delay(60000); 
  Cellular::send(line, "heading"); 
} 

void loop() {
  if (!firstRun) {
    pinMode(Relay::pin, OUTPUT); 
    SDcard::init();  
    Cellular::init();
  }  
  else firstRun = 0;
  
  GPS::get(180000); //180000 = 3mins read
  
  Display::write("Parsing data..."); 
  GPS::timezone(GPS::SIM7600_date, GPS::SIM7600_time); 
  GPS::timezone(GPS::Air530_date, GPS::Air530_time); 
  if (GPS::read(GPS::SIM7600_date, GPS::SIM7600_time, GPS::SIM7600_lng, GPS::SIM7600_lat)) LED::blink(1); 
  if (GPS::read(GPS::Air530_date, GPS::Air530_time, GPS::Air530_lng, GPS::Air530_lat)) LED::blink(2); 
  if (Thermocouple::read()) LED::blink(3); 
  if (Turbidity::read()) LED::blink(4); 
  Display::write("parsing data done\n");

  //data to file 
  snprintf(line, sizeof(line), "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
    GPS::Air530_date.str_date, GPS::Air530_time.str_time,GPS:: Air530_lat.str_val, GPS::Air530_lng.str_val, 
    GPS::SIM7600_date.str_date, GPS::SIM7600_time.str_time, GPS::SIM7600_lat.str_val, GPS::SIM7600_lng.str_val, 
    Thermocouple::temp_hot.str_val, Thermocouple::temp_cold.str_val, 
    Turbidity::volts.str_val
  );
  
  SDcard::write(line); 
  
  //data to cellular module
  snprintf(line, sizeof(line), "{\"content\":\"~%s %d %s %s %s %s %s %s %s %s %s %s %s\"}",
    drifterName,
    SDcard::status.status, 
    GPS::Air530_date.str_date, GPS::Air530_time.str_time,GPS:: Air530_lat.str_val, GPS::Air530_lng.str_val, 
    GPS::SIM7600_date.str_date, GPS::SIM7600_time.str_time, GPS::SIM7600_lat.str_val, GPS::SIM7600_lng.str_val, 
    Thermocouple::temp_hot.str_val, Thermocouple::temp_cold.str_val, 
    Turbidity::volts.str_val
  );

  Cellular::send(line, "data"); 
  Cellular::sleep(); 

  if (Display::count==1) Display::sleep(); 
  
  Display::count -= 1; 
  LED::count -= 1; 

  Arduino::sleep(87); //relay off, Arduino sleep (~87 = 7mins sleep)
}










  

