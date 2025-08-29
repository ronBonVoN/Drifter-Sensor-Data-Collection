/*
Veronika Davis
Kilroy was here
*/

#include "functions.h"
#include "data_structs.h"
#include "Adafruit_MCP9600.h"
#include <Wire.h> 
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include <Arduino.h>

#define MCP9600_STATUS_OPEN (0x01)
#define RELAY_PIN 4
#define LED_PIN 12

Adafruit_MCP9600 mcp; //thermocouple 

Data thermocouple_temp_hot = {{"Hot Junction", "C"}, -1.0, "-1.0", 1, 0x67, 0}; 
Data thermocouple_temp_cold = {{"Cold Junction", "C"}, -1.0, "-1.0", 1, 0x67, 0}; 
Data turbidity = {{"Turbidity", "volts"}, -1.0, "-1.0", 1, 0x00, A4};

// customizable vars
const char drifterName[20] = "Double"; 

// change for testing 
short displayCount = 2; //# of cycles display will run
short LEDcount = 1000; //# of cycles LED will run

char line[350]; // for general writing/printing
bool firstRun = 1; 

void blink(unsigned short num, unsigned int wait=3000, unsigned int pause = 300) {
  if (LEDcount>0) {
    for (int i=0; i<num; i++) {
      digitalWrite(LED_PIN, HIGH); 
      delay(pause); 
      digitalWrite(LED_PIN, LOW); 
      delay(pause); 
    }
    delay(wait);
  } 
}

bool initialize(bool x, const char* sensor) {
  snprintf(line, sizeof(line), "Initializing %s...", sensor);
  print_SerialDisplay(line);
  if(x) {
    snprintf(line, sizeof(line), "%s initialization done.\n", sensor);
    print_SerialDisplay(line);
    return 1; 
  }
  else {
    snprintf(line, sizeof(line), "%s failed\n", sensor);
    print_SerialDisplay(line, 10000); //moves on after 10secs if sensor failed 
    return 0; 
  }
}

void setup() { 
  delay(5000); 
  Serial.begin(115200);
  Wire.begin(); 
  Wire.setClock(100000); 

  pinMode(LED_PIN, OUTPUT);  
  pinMode(RELAY_PIN, OUTPUT);
  delay(2000); 
  digitalWrite(RELAY_PIN, HIGH); 
  delay(2000); 
  
  if (displayCount>0) {
    if(initialize(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS), "display")) blink(1);
    else displayCount = 0; 
  }
  if(initialize(mcp.begin(thermocouple_temp_hot.adress), "thermocouple")) blink(2); 
  if(initialize(SD.begin(SDcard_data.pin), "SD card")) blink(3); 
  
  setupGPS(); 
  setupCell(); 
  
  //file heading
  snprintf(line, sizeof(line), "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
  Air530_date.name, Air530_time.name, Air530_lat.name, Air530_lng.name, 
  SIM7600_date.name, SIM7600_time.name, SIM7600_lat.name, SIM7600_lng.name, 
  Air530_speed.name, Air530_heading.name, 
  thermocouple_temp_hot.name, thermocouple_temp_cold.name, 
  turbidity.name, 
  Air530_date.unit, Air530_time.unit, Air530_lat.unit, Air530_lng.unit, 
  SIM7600_date.unit, SIM7600_time.unit, SIM7600_lat.unit, SIM7600_lng.unit, 
  Air530_speed.unit, Air530_heading.unit, 
  thermocouple_temp_hot.unit, thermocouple_temp_cold.unit, 
  turbidity.unit);
  
  print_SerialFile(line);

  //heading message to send through cellular module 
  snprintf(line, sizeof(line), "{\"content\":\"^%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|(newline)%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|%s|\"}",
  SDcard_status.name, 
  Air530_date.name, Air530_time.name, Air530_lat.name, Air530_lng.name, 
  SIM7600_date.name, SIM7600_time.name, SIM7600_lat.name, SIM7600_lng.name, 
  Air530_speed.name, Air530_heading.name, 
  thermocouple_temp_hot.name, thermocouple_temp_cold.name, 
  turbidity.name, 
  Air530_date.unit, Air530_time.unit, Air530_lat.unit, Air530_lng.unit, 
  SIM7600_date.unit, SIM7600_time.unit, SIM7600_lat.unit, SIM7600_lng.unit, 
  Air530_speed.unit, Air530_heading.unit, 
  thermocouple_temp_hot.unit, thermocouple_temp_cold.unit, 
  turbidity.unit) 
} 

void loop() {
/************************************************************ relay on ************************************************************/
  print_SerialDisplay("Initializing cellular module...\n");
  digitalWrite(LED_PIN, HIGH); 
  if (setupCell()) print_SerialDisplay("\ncellular module initialization done.\n");
  else print_SerialDisplay("\ncellular module failed.\n"); 
  digitalWrite(LED_PIN, LOW); 
  
  if (!firstRun) {
    pinMode(RELAY_PIN, OUTPUT); 
    SD.begin(SDCHIP_SELECT);  
    sendCell(line, "heading"); 
  }  
  else firstRun = 0;
  
  print_SerialDisplay("Reading gps...\n"); 
  readGPS(180000); 
  print_SerialDisplay("\ngps reading done.\n", 3000);

  print_SerialDisplay("\nParsing data...");   
  
  snprintf(Air530_date.str_date, sizeof(Air530_date.str_date), "%02d-%02d-%04d",
  Air530_date.month, Air530_date.day, Air530_date.year); 
  snprintf(Air530_time.str_time, sizeof(Air530_time.str_time), "%02d:%02d:%02d", 
  Air530_time.hr, Air530_time.min, Air530_time.sec; 
  
  dtostrf(gps.location.lat(),     10, 6, Air530_lat.str_val); // -90.000000 -> _90.000000
  dtostrf(gps.location.lng(),     10, 6, Air530_lat.str_val); // -90.000000 -> _90.000000

  snprintf(SIM7600_date.str_date, sizeof(SIM7600_date.str_date), "%02d-%02d-%04d",
  SIM7600_date.month, SIM7600_date.day, SIM7600_date.year); 
  snprintf(SIM7600_time.str_time, sizeof(SIM7600_time.str_time), "%02d:%02d:%02d", 
  SIM7600_time.hr, SIM7600_time.min, SIM7600_time.sec);
  
  dtostrf(SIM7600_lat.val,        10, 6, SIM7600_lat.str_val); // -90.000000 -> _90.000000
  dtostrf(SIM7600_lng.val,        10, 6, SIM7600_lng.str_val); // -90.000000 -> _90.000000
  
  dtostrf(gps.speed.mps(),        5,  2, Air530_speed.str_val);   // 00.00
  dtostrf(gps.course.deg(),       6,  2, Air530_heading.str_val); // __0.00 -> 360.00
  
  dtostrf(mcp.readThermocouple(), 6,  2, thermocouple_temp_hot.str_val);  // -00.00 -> _00.00
  dtostrf(mcp.readAmbient(),      6,  2, thermocouple_temp_cold.str_val); // -00.00 -> _00.00
 
  dtostrf(turbidity.val,          4,  2, turbidity.str_val); // 0.00

  print_SerialDisplay("data parsing done.\n"); 

  //printing data to file 
  snprintf(line, sizeof(line), "%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
  Air530_date.str_date, Air530_time.str_time, Air530_lat.str_val, Air530_lng.str_val, 
  SIM7600_date.str_date, SIM7600_time.str_time, SIM7600_lat.str_val, SIM7600_lng.str_lng, 
  Air530_speed.str_val, Air530_heading.str_heading, 
  thermocouple_temp_hot.str_val, thermocouple_temp_cold.str_val, 
  turbidity.str_val)
  print_SerialFile(line); 
  
  //data message to send through cellular module 
  snprintf(line, sizeof(line), "{\"content\":\"~%s %d %s %s %s %s %s %s %s %s %s %s %s %s %s\"}",
  drifterName,
  SDcard.status, 
  Air530_date.str_date, Air530_time.str_time, Air530_lat.str_val, Air530_lng.str_val, 
  SIM7600_date.str_date, SIM7600_time.str_time, SIM7600_lat.str_val, SIM7600_lng.str_lng, 
  Air530_speed.str_val, Air530_heading.str_heading, 
  thermocouple_temp_hot.str_val, thermocouple_temp_cold.str_val, 
  turbidity.str_val); 
  sendCell(line, "data");
  
  print_SerialDisplay("\nCellular module shutdown...\n");
  if (commandCell("AT+CPOF")) print_SerialDisplay("\ncellular module shutdown successfully.\n", 5000); 
  else print_SerialDisplay("\ncellular module shutdown failed.\n", 10000);

  if (displayCount==1) {
    print_SerialDisplay("Sleeping display....\n", 2000); 
    display.clearDisplay(); 
    display.display(); 
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
  
  displayCount -= 1; 
  LEDcount -= 1; 

  print_SerialDisplay("Sleep mode..."); 
  digitalWrite(RELAY_PIN, LOW);
  blink(2,0,2000); 
  sleepArduino(87); //relay off, Arduino sleep
  blink(2,0,2000); 
  digitalWrite(RELAY_PIN, HIGH);  
  print_SerialDisplay("out of sleep mode.\n"); 
}










  

