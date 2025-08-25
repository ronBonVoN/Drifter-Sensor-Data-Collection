/*
Veronika Davis
Kilroy was here
*/

/*to-do
 improve cell cmds
 fix 0 msgs
 change cellCommand name
*/

//change for testing 
#define SLEEP_LOOPS 2 // 86 = ~10mins (87) (93)
#define GPS_READ_MILLIS 60000 //180000 = 3mins 

#define I2C_ADDRESS (0x67)
#define SCREEN_ADDRESS (0x3C)
#define MCP9600_STATUS_OPEN (0x01)
#define TURBIDITY_PIN A4
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 2
#define RELAY_PIN 4
#define LED_PIN 12
#define SDCHIP_SELECT 53
#define CELL_PWK 9 
#define CELL_RST 7 
#define GPS_SERIAL Serial1
#define CELL_SERIAL Serial2
#define TINY_GSM_MODEM_SIM7600

#include <SD.h>
#include <Wire.h> 
#include <Adafruit_I2CDevice.h>
#include <Adafruit_I2CRegister.h>
#include "Adafruit_MCP9600.h"
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <TinyGPSPlus.h>
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h> 
#include <avr/power.h>
#include <avr/interrupt.h>
#include <TinyGsmClient.h>

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MCP9600 mcp;       //thermocouple
TinyGPSPlus gps;   
TinyGsm modem(CELL_SERIAL); //cellular communication module

// customizable vars
const char drifterName[20] = "Double"; 
const char fileName[13] = "DATA.txt"; 
const char url[200] = "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj";
      // for testing -> "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj"
      // for field   -> "https://discordapp.com/api/webhooks/1403412309438627851/RdeuTCbLB5Ul039usaiiCX5YhAeQGqdQAo4tfQz-Igut3GvUrKP22cAfCnrDrNmX5mA6"
short timeAdjust = -4; //num hrs time difference to utc time, pos/neg if utc is behind/ahead

// change for testing 
short displayCount = 200; //# of cycles display will run
short LEDcount = 1000; //# of cycles LED will run
const bool serialEnable = 0;  

short m[2] = {-1.0,-1.0}, d[2] = {-1.0,-1.0}, y[2] = {-1.0,-1.0}, hr[2] = {-1.0,-1.0}, min[2] = {-1.0,-1.0}, sec[2] = {-1.0,-1.0}; 
short i;                  //for indexing
unsigned long start, now; //for millis() while loops
float lat[2] = {-1.0,-1.0},  lng[2] = {-1.0,-1.0}, speed = -1.0, heading = -1.0, tempHot = -1.0, tempCold = -1.0, turbidity = -1.0; 
char c;            // for reading cellular module output
char line[300];    // for general writing/printing
char cmd[200];     // for building cellular module commands 
char msg[350];     // for building cellular module message command 
char data[9][15];  // for float data
char outputCell[512]; 
char gpsCell[10][15]; //output from cellular module gps 
short monthDays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}; //for adjusting timezone
bool firstRun = 1; 
bool SDstatus = 1; 
volatile bool shouldWake = false;

//for interrupt watchdog
ISR(WDT_vect) {
  shouldWake = true;
}

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

void blink(unsigned short num, unsigned int wait=3000, unsigned int pause = 300) {
  if (LEDcount>0) {
    for (i=0; i<num; i++) {
      digitalWrite(LED_PIN, HIGH); 
      delay(pause); 
      digitalWrite(LED_PIN, LOW); 
      delay(pause); 
    }
    delay(wait);
  } 
}

//for sending cellular module commands
bool commandCell(const char* cmd, const char* res = "OK", unsigned int wait = 10000) {
  CELL_SERIAL.print(cmd);
  CELL_SERIAL.print('\r');
  outputCell[0] = '\0'; 
  start = millis(); 
  while (millis() - start < wait) {    
    while (CELL_SERIAL.available()) {
      c =  CELL_SERIAL.read();
      if (serialEnable) Serial.write(c);
      i = strlen(outputCell); 
      if (i < sizeof(outputCell)-1) {
        outputCell[i] = c; 
        outputCell[i + 1] = '\0'; 
      }
    } 
     if (strstr(outputCell, res)) return 1; 
  }
  
  if (serialEnable) { 
    Serial.print(cmd); 
    Serial.print(" timed out waiting for "); 
    Serial.println(res);
  }
  return 0; 
}

void setup() {
  delay(5000); //time for arduino to adjust if program resets 
  if (serialEnable) Serial.begin(115200);
  GPS_SERIAL.begin(9600); 
  CELL_SERIAL.begin(115200); 
  Wire.begin(); //for i2c com
  Wire.setClock(100000); //make i2c com slower  
  delay(5000); 

  pinMode(LED_PIN, OUTPUT); 
  
  pinMode(RELAY_PIN, OUTPUT);
  delay(2000); 
  digitalWrite(RELAY_PIN, HIGH); 
  delay(2000); 

  if (displayCount>0) {
    if(initialize(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS), "display")) blink(1);
    else displayCount = 0; 
  }
  if(initialize(mcp.begin(I2C_ADDRESS), "thermocouple")) blink(2); 
  if(initialize(SD.begin(SDCHIP_SELECT), "SD card")) blink(3); 
  
  //file heading
  snprintf(line, sizeof(line), "Air530 GPS Date\tAir530 GPS Time\tAir530 Latitude\tAir530 Longitude\tSIM7600 GPS Date\tSIM7600 GPS Time\tSIM7600 Latitude\tSIM7600 Longitude\tSpeed\tHeading\tHot Junction\tCold Junction\tTurbidity\nUTC%hdhrs\tUTC%hdhrs\tGCS\tGCS\tUTC%hdhrs\tUTC%hdhrs\tGCS\tGCS\tmph\tdeg\tC\tC\tvolts\n",
  timeAdjust, timeAdjust, timeAdjust, timeAdjust);
  print_SerialFile(line);

  //creating heading message to send through cellular module 
  snprintf(msg, sizeof(msg), "{\"content\":\"^%s SD+Card+Status Air530+GPS+Date Air530+GPS+Time Air530+Latitude Air530+Longitude SIM7600+GPS+Date SIM7600+GPS+Time SIM7600+Latitude SIM7600+Longitude Speed Heading Hot+Junction Cold+Junction Turbidity(newline)bool UTC%+hdhrs UTC%+hdhrs GCS GCS UTC%+hdhrs UTC%+hdhrs GCS GCS mph deg C C volts\"}",
  drifterName, timeAdjust, timeAdjust, timeAdjust, timeAdjust);
  
  setupCell(); 
  if (sendCell(msg, "heading")) blink(4);
} 

void loop() {
/************************************************************ relay on ************************************************************/
  pinMode(RELAY_PIN, OUTPUT); 
  
  if (!firstRun) { //need to re-initialize sd card after relay shuts off, but dangerous to do twice during start up.
    SD.begin(SDCHIP_SELECT); 
    setupCell();
  }  
  else firstRun = 0;

  
  print_SerialDisplay("Reading gps...\n"); 
  commandCell("AT+CGPS=1"); 
  start = millis(); 
  now = millis(); 
  while (millis() - start < GPS_READ_MILLIS) {  //3mins
    while (GPS_SERIAL.available()) gps.encode(GPS_SERIAL.read());
    if (millis() - now >= 300) { // need to use while() instead of delay() for
        now += 300;              // blinking during reading gps to not pause reading
        digitalWrite(LED_PIN, !digitalRead(LED_PIN)); // togel LED
    }
  }
  if (digitalRead(LED_PIN)) digitalWrite(LED_PIN, LOW); 
  if (commandCell("AT+CGPSINFO") && outputCell[27] != ',') {
    strncpy(gpsCell[0],  outputCell+27,11); gpsCell[0][13] = '\0'; 
    gpsCell[1][0] = outputCell[39];         
    strncpy(gpsCell[2],  outputCell+41,12); gpsCell[2][14] = '\0';   
    gpsCell[3][0] = outputCell[54];  
    strncpy(gpsCell[4],  outputCell+56, 2); gpsCell[4][ 2] = '\0';
    strncpy(gpsCell[5],  outputCell+58, 2); gpsCell[5][ 2] = '\0';
    strncpy(gpsCell[6],  outputCell+60, 2); gpsCell[6][ 2] = '\0';
    strncpy(gpsCell[7],  outputCell+63, 2); gpsCell[7][ 2] = '\0';
    strncpy(gpsCell[8],  outputCell+65, 2); gpsCell[8][ 2] = '\0';
    strncpy(gpsCell[9],  outputCell+67, 2); gpsCell[9][ 2] = '\0';
    lat[1] = atof(gpsCell[0]); lat[1] = floor(lat[1]/100) + (lat[1] - floor(lat[1]/100)*100)/60; 
    lng[1] = atof(gpsCell[2]); lng[1] = floor(lng[1]/100) + (lng[1] - floor(lng[1]/100)*100)/60;
    if (gpsCell[1][0] == 'S')  lat[1] *= -1; 
    if (gpsCell[3][0] == 'W')  lng[1] *= -1; 
    m[1] = atoi(gpsCell[5]); d[1] = atoi(gpsCell[4]); y[1] = atoi(gpsCell[6]); 
    y[1] += 2000; 
    hr[1] = atoi(gpsCell[7]); min[1] = atoi(gpsCell[8]); sec[1] = atoi(gpsCell[9]);
  } 
  else {
    lat[1] = 0.0, lng[1]= 0.0; 
    m[1] = 0, d[1] = 0, sec[1] = 0; 
    hr[1] = 0, min[1] = 0, y[1] = 2000; 
  }
  commandCell("AT+CGPS=0"); 
  print_SerialDisplay("\ngps reading done.\n", 3000);
 
  print_SerialDisplay("\nReading sensors...");   
  m[0] = gps.date.month(); d[0] = gps.date.day(); y[0] = gps.date.year(); 
  hr[0] = gps.time.hour(); min[0] = gps.time.minute(); sec[0] = gps.time.second(); 
  if (!(m[0]<=0 && d[0]<=0 && y[0]<=2000 && hr[0]<=0 && min[0]<=0 && sec[0]<=0)) /*<--------------->*/ blink(1);
  
  adjustTime(m[0], d[0], y[0], hr[0], min[0], sec[0]); //adjust time zone  
  adjustTime(m[1], d[1], y[1], hr[1], min[1], sec[1]);
  
  lat[0] = gps.location.lat(); lng[0] = gps.location.lng(); 
  if (lat[0]!=0 && lng[0]!=0 && lat[0]!=-1 && lng[0]!=-1) /*<-------------------->*/ blink(2); 
  speed = gps.speed.mps(); heading = gps.course.deg();  
  tempHot = mcp.readThermocouple(); tempCold = mcp.readAmbient();
  if ((tempHot == tempHot) && (tempCold == tempCold) && tempCold!=-1 && tempHot!=-1) blink(3);
  turbidity = analogRead(TURBIDITY_PIN) * (5.0/1024.0); //reads voltage  
  if (!(turbidity<=0) && !(turbidity>=5)) /*<------------------------------------>*/ blink(4); 
  print_SerialDisplay("sensor reading done.\n"); 

  dtostrf(lat[0],    10, 6, data[0]); // -90.000000 -> _90.000000
  dtostrf(lng[0],    10, 6, data[1]); // -90.000000 -> _90.000000
  dtostrf(lat[1],    10, 6, data[2]); // -90.000000 -> _90.000000
  dtostrf(lng[1],    10, 6, data[3]); // -90.000000 -> _90.000000
  dtostrf(speed,     5,  2, data[4]); // 00.00
  dtostrf(heading,   6,  2, data[5]); // __0.00 -> 360.00
  dtostrf(tempHot,   6,  2, data[6]); // -00.00 -> _00.00
  dtostrf(tempCold,  6,  2, data[7]); // -00.00 -> _00.00
  dtostrf(turbidity, 4,  2, data[8]); // 0.00
  
  //printing data to file 
  snprintf(line, sizeof(line), "%02d-%02d-%04d\t%02d:%02d:%02d\t%s\t%s\t%02d-%02d-%04d\t%02d:%02d:%02d\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
  m[0], d[0], y[0], hr[0], min[0], sec[0], data[0], data[1],
  m[1], d[1], y[1], hr[1], min[1], sec[1], data[2], data[3], 
  data[4], data[5], data[6], data[7], data[8]);
  print_SerialFile(line); 
  
  //creating data message to send through cellular module 
  snprintf(msg, sizeof(msg), "{\"content\":\"~%s %d %02d-%02d-%04d %02d:%02d:%02d %s %s %02d-%02d-%04d %02d:%02d:%02d %s %s %s %s %s %s %s\"}",
  drifterName, SDstatus, 
  m[0], d[0], y[0], hr[0], min[0], sec[0], data[0], data[1],
  m[1], d[1], y[1], hr[1], min[1], sec[1], data[2], data[3], 
  data[4], data[5], data[6], data[7], data[8]);

  sendCell(msg, "data");
  
  //prep cellular module for shutdown
  print_SerialDisplay("\nCellular module shutdown...\n");
  commandCell("AT+HTTPTERM", "AT+HTTPTERM");
  if (commandCell("AT+CPOF")) print_SerialDisplay("\ncellular module shutdown successfully.\n", 5000); 
  else print_SerialDisplay("\ncellular module shutdown failed.\n", 10000);

  if (displayCount>0) {
    display.clearDisplay(); 
    display.setTextColor(WHITE); 
    display.setTextSize(1); 
    snprintf(line, sizeof(line), "%s-%s-%s %s:%s:%s", m[0], d[0], y[0], hr[0], min[0], sec[0]); 
    display.setCursor(0,0);  display.print(line);                                                            
    snprintf(line, sizeof(line), "lat:%s", data[0]);                                                         
    display.setCursor(0,10); display.print(line);                                          
    snprintf(line, sizeof(line), "lng:%s", data[1]);                                              
    display.setCursor(0,20); display.print(line); 
    snprintf(line, sizeof(line), "mph:%s deg:%s", data[4], data[5]);                                         
    display.setCursor(0,30); display.print(line);
    snprintf(line, sizeof(line), "temp:%s, %s", data[6], data[7]);
    display.setCursor(0,40); display.print(line);
    snprintf(line, sizeof(line), "turbidity: %s", data[8]);
    display.setCursor(0,50); display.print(line);
    display.display(); 
    delay(5000); 
    display.clearDisplay(); 
    snprintf(line, sizeof(line), "%s-%s-%s %s:%s:%s", m[1], d[1], y[1], hr[1], min[1], sec[1]); 
    display.setCursor(0,0);  display.print(line);                                                            
    snprintf(line, sizeof(line), "lat:%s", data[2]);                                                         
    display.setCursor(0,10); display.print(line);                                          
    snprintf(line, sizeof(line), "lng:%s", data[3]);                                              
    display.setCursor(0,20); display.print(line); 
    display.display(); 
    delay(5000); 
  }

  if (displayCount==1) {
    print_SerialDisplay("Sleeping display....\n", 2000); 
    display.clearDisplay(); 
    display.display(); 
    display.ssd1306_command(SSD1306_DISPLAYOFF);
  }
  displayCount -= 1; 
  LEDcount -= 1; 

/****************************************************** relay off, arduino sleep  ******************************************************/
  print_SerialDisplay("Sleep mode..."); 
  
  wdt_enable(WDTO_8S); //enabling watchdog, will reset arduino if wdt_reset() is not called in 8secs 
  power_spi_disable(); //turn off spi com
  power_twi_disable(); //turn off i2c com
  digitalWrite(RELAY_PIN, LOW);
  wdt_disable();

  blink(2,0,2000); 
  for(i=0; i<SLEEP_LOOPS; i++) {
    //puts arduino into a low power mode  
    //interupt watchdog 
    shouldWake = false; 
    MCUSR = 0;
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP3); //setting ~8sec sleep 
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
  }
  blink(2,0,2000); 
  
  wdt_enable(WDTO_8S);
  digitalWrite(RELAY_PIN, HIGH);  
  delay(2000); 
  power_spi_enable(); // turn on spi 
  power_twi_enable(); // turn on i2c
  wdt_disable();
  print_SerialDisplay("out of sleep mode.\n"); 
}

/************************************************************* functions *************************************************************/
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

void print_SerialFile(const char* message) { 
  if (serialEnable) Serial.print(message); 
  wdt_enable(WDTO_8S); //enabling watchdog, will reset arduino if wdt_reset() is not called in 8secs
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    SDstatus = 1; 
    dataFile.print(message);
    dataFile.close();
    wdt_disable();
  }
  else { 
    wdt_disable();
    SDstatus = 0; 
    snprintf(line, sizeof(line), "Error opening %s\n", fileName);
    print_SerialDisplay(line);
    if (LEDcount>0) blink(10); 
    else delay(10000); 
  }
}

void adjustTime(short &m, short &d, short &y, short &hr, short &min, short &sec) {
  //adjusts timezone based on adjust amount varibale 
  if (hr<=0 && min<=0 && sec<=0) return; 
  else hr += timeAdjust; 

  if (m<=0 && d<=0 && y<=2000) return; 
  else {
    if (hr<=0) {
      hr = 24 + hr;
      d -= 1 ;
    } 
    if (hr>=24) {
      hr -= 24; 
      d += 1; 
    }

    if ((y % 4 == 0) && (y % 100 != 0) || (y % 400 == 0)) monthDays[2-1] = 29;    
    
    if (d==0) {
      m -= 1;
      if (m>0) d = monthDays[m-1]; 
      if (m==0) {
        m = 12; 
        d = monthDays[12-1]; 
        y -= 1; 
      }
    }

    if (d>monthDays[m-1]) {
      m += 1;
      if (m<=12) d = 1;
      if (m==13) {
        m = 1; 
        d = 1; 
        y += 1; 
      }
    }
  }
}

bool setupCell() {
  //power-key button needed to start cellular module
  print_SerialDisplay("Initializing cellular module...\n");
  digitalWrite(LED_PIN, HIGH); 

  pinMode(CELL_PWK, OUTPUT);
  digitalWrite(CELL_PWK, LOW);
  delay(1200); 
  digitalWrite(CELL_PWK, HIGH); 
  delay(3000); 
  
  start = millis(); 
  //Basic communication test
  for (i=0; i<12; i++) {
    if (commandCell("AT", "OK", 5000)) {
      print_SerialDisplay("\ncellular module initialization done.\n");
      digitalWrite(LED_PIN, LOW); 
      return 1;
    }
  }
  print_SerialDisplay("\ncellular module failed.\n"); 
  digitalWrite(LED_PIN, LOW); 
  return 0;  
}

bool sendCell(const char* message, const char* reason) {
  //message cannot have tabs or newlines
  snprintf(line, sizeof(line), "\nSending %s...\n", reason);
  print_SerialDisplay(line);    
  
  //commands to prep for sending data to url 
  if (!(commandCell("AT") &&  //Basic communication test
        commandCell("AT+CSQ") &&  //Check signal quality
        commandCell("AT+CREG?") && //Network registration status 
        commandCell("AT+CGDCONT=1,\"IP\",\"m2mglobal\"") && //Set PDP context with APN
        commandCell("AT+CGATT=1") && //Attach to the GPRS network
        commandCell("AT+CGACT=1,1")) //Activate the PDP context
  ) goto fail; 
  
  commandCell("AT+HTTPTERM", "AT+HTTPTERM"); //Terminate HTTP service session
  snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"", url); //Set target URL
  if (!(commandCell("AT+HTTPINIT") &&  //Initialize HTTP
        commandCell("AT+HTTPPARA=\"CID\",1") && //Set bearer profile ID for HTTP
        commandCell(cmd) && 
        commandCell("AT+HTTPPARA=\"CONTENT\",\"application/json\"")) //Set content type for POST
  ) goto fail; 

  //sending message
  snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%d,10000", strlen(message)); //Prepare to send POST data
  if (!(commandCell(cmd, "DOWNLOAD") &&
        commandCell(message))
  ) goto fail; 
 
  delay(5000); 
  if (commandCell("AT+HTTPACTION=1", "+HTTP_PEER_CLOSED", 60000)) { //Execute HTTP POST
    commandCell("AT+HTTPTERM", "AT+HTTPTERM");
    snprintf(line, sizeof(line), "\n%s sent successfully.\n", reason);
    print_SerialDisplay(line, 5000);
    return 1; 
  }

  fail: 
    commandCell("AT+HTTPTERM", "AT+HTTPTERM");
    snprintf(line, sizeof(line), "\n%s sending failed.\n", reason);
    print_SerialDisplay(line, 10000);
    return 0; 
}








  

