/*
Veronika Davis
Kilroy was here
*/

/*to-do
*/

#include <SD.h>
//#include <SdFat.h> 
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

#define I2C_ADDRESS (0x67)
#define SCREEN_ADDRESS (0x3C)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET 7
#define RELAY_PIN 4
#define SDCHIP_SELECT 53
#define MCP9600_STATUS_OPEN (0x01)
#define SLEEP_LOOPS 86 // 86 = ~10mins
#define GPS_READ_MILLIS 180000 //180000 = 3mins 

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//SdFat SD;
Adafruit_MCP9600 mcp; //thermocouple
TinyGPSPlus gps;
short m = -1, d = -1, y = -1, hr = -1, min = -1, sec = -1; 
short displayCount = 2; //# of cycles display will run
float lat = -1.0, lng = -1.0, speed = -1.0, heading = -1.0, tempHot = -1.0, tempCold = -1.0, turbidity = -1.0; 
/*const*/ char fileName[9] = "data.txt"; 
char dateName[9];   //name fileName will be changed to depending on gps most recent date
char line[200];     // for writing/printing "lines"
char data[13][10];  // where sensor data strings will go 
bool firstRun = 1; 
const bool serialEnable = 0; 
volatile bool shouldWake = false; 

//for interrupt watchdog
ISR(WDT_vect) {
  shouldWake = true;
}

void setup() {
  delay(5000); //time for arduino to adjust if program resets 
  if (serialEnable) Serial.begin(9600);
  Serial1.begin(9600); //Serial for gps
  Wire.begin(); //for i2c com
  Wire.setClock(100000); //make i2c com slower 
  delay(5000); 

  pinMode(RELAY_PIN, OUTPUT);
  delay(2000); 
  digitalWrite(RELAY_PIN, HIGH); 
  delay(2000); 
  
  initialize(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS), "display");
  initialize(mcp.begin(I2C_ADDRESS), "thermocouple"); 
  initialize(SD.begin(SDCHIP_SELECT), "SD card"); 

  //file heading
  print_SerialFile("Date\tTime\tLatitude\tLongitude\tSpeed\tHeading\tHot Junction\tCold Junction\tTurbidity\n....\tEST/UTC\t....\t....\tmph\tdeg\tC\tC\tntu\n");
}

void loop() {
/************************************************************ relay on ************************************************************/
  pinMode(RELAY_PIN, OUTPUT); 
  if (!firstRun) { //need to re-initialize sd card after relay shuts off, but dangerous to do twice during start up.
    if (SD.begin(SDCHIP_SELECT)) {}
    else {
      print_SerialDisplay("SD card failed.\n"); 
      delay(10000); 
    }
  }  
  else {
    firstRun = 0;
  }

  print_SerialDisplay("Reading gps..."); 
  unsigned long start = millis();
  do{
      while (Serial1.available())
      gps.encode(Serial1.read()); 
  } while (millis()-start < GPS_READ_MILLIS); //3mins 
  print_SerialDisplay("gps reading done.\n");
 
  print_SerialDisplay("Reading sensors...");
  m = gps.date.month(); d = gps.date.day(); y = gps.date.year(); 
  hr = gps.time.hour(); min = gps.time.minute(); sec = gps.time.second(); 
  adjustTime(); //adjust time zone 
  lat = gps.location.lat(); lng = gps.location.lng(); 
  speed = gps.speed.mps(); heading = gps.course.deg(); 
  tempHot = mcp.readThermocouple(); 
  tempCold = mcp.readAmbient();
  turbidity = analogRead(A4) * (5.0/1024.0); //reads voltage  
  print_SerialDisplay("sensor reading done.\n");

  //writing floats to char array      ***size************ 
  snprintf(data[0],  3, "%02d", m);   // 00\0
  snprintf(data[1],  3, "%02d", d);   // 00\0
  snprintf(data[2],  5, "%04d", y);   // 0000\0
  snprintf(data[3],  3, "%02d", hr);  // 00\0
  snprintf(data[4],  3, "%02d", min); // 00\0
  snprintf(data[5],  3, "%02d", sec); // 00\0
  dtostrf(lat,       6, 2, data[6]);  // -90.00 -> _90.00
  dtostrf(lng,       6, 2, data[7]);  // -90.00 -> _90.00
  dtostrf(speed,     5, 2, data[8]);  // 00.00
  dtostrf(heading,   6, 2, data[9]);  // __0.00 -> 360.00
  dtostrf(tempHot,   6, 2, data[10]); // -00.00 -> _00.00
  dtostrf(tempCold,  6, 2, data[11]); // -00.00 -> _00.00
  dtostrf(turbidity, 4, 2, data[12]); // 0.00

  //printing data line to file 
  snprintf(line, sizeof(line), "%s-%s-%s\t%s:%s:%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
    data[0], data[1], data[2], data[3], data[4], data[5],
    data[6], data[7], data[8], data[9], data[10], data[11], data[12]);
  print_SerialFile(line); 

  if (displayCount>0) {
    display.clearDisplay(); 
    display.setTextColor(WHITE); 
    display.setTextSize(1); 
    snprintf(line, sizeof(line), "%s-%s-%s %s:%s:%s", data[0], data[1], data[2], data[3], data[4], data[5]); 
    display.setCursor(0,0);  display.print(line);                                                            
    snprintf(line, sizeof(line), "lat:%s lng:%s", data[6], data[7]);                                         
    display.setCursor(0,10); display.print(line); 
    snprintf(line, sizeof(line), "mph:%s deg:%s", data[8], data[9]);                                         
    display.setCursor(0,20); display.print(line);
    snprintf(line, sizeof(line), "temp:%s, %s", data[10], data[11]);
    display.setCursor(0,30); display.print(line);
    snprintf(line, sizeof(line), "turbidity: %s", data[12]);
    display.setCursor(0,40); display.print(line);
    display.display(); 
    delay(10000); 
  }

  if (displayCount==1) {
    print_SerialDisplay("Sleeping display....\n"); 
    delay(2000); 
    display.clearDisplay(); 
    display.display(); 
    display.ssd1306_command(SSD1306_DISPLAYOFF);
    delay(2000); 
  }
  displayCount -= 1; 

/****************************************************** relay off, arduino sleep  ******************************************************/
  //enabling watchdog, will reset arduino if wdt_reset() is not called in 8secs 
  wdt_enable(WDTO_8S);           
  
  print_SerialDisplay("Entering sleep mode..."); 
  wdt_reset(); 
  power_spi_disable(); power_twi_disable(); //turning off spi and i2c comunication 
  wdt_reset(); delay(2000); 

  digitalWrite(RELAY_PIN, LOW); 
  for(int i=0; i<SLEEP_LOOPS; i++) { //around 10mins (adjusted # of loops from testing)
    wdt_reset(); 
    sleepArduino(); //puts arduino into a low power mode (~8secs)
  }
  wdt_reset(); digitalWrite(RELAY_PIN, HIGH); wdt_reset(); 

  power_spi_enable(); power_twi_enable(); //turn spi and i2c back on 
  wdt_reset(); delay(2000); 
  print_SerialDisplay("out of sleep mode.\n"); wdt_disable(); delay(3000); 
}

/************************************************************* functions *************************************************************/
void initialize(bool x, const char* sensor) {
  snprintf(line, sizeof(line), "Initializing %s...", sensor);
  print_SerialDisplay(line);
  
  if(x) {
    snprintf(line, sizeof(line), "%s initialization done.\n", sensor);
    print_SerialDisplay(line);
  }
  else {
    snprintf(line, sizeof(line), "%s failed\n", sensor);
    print_SerialDisplay(line);
    delay(10000); //moves on after 10secs if sensor failed 
  }
}

void print_SerialDisplay(const char* message) {
  if (serialEnable) Serial.print(message); 
  if (displayCount>0) {
    display.clearDisplay(); 
    display.setTextColor(WHITE); 
    display.setTextSize(1); 
    display.setCursor(0,20);
    display.print(message); 
    display.display();
  }
}

void print_SerialFile(const char* message) {
  if (serialEnable) Serial.print(message); 
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
   dataFile.print(message);
   dataFile.close(); 
  }
  else { 
    snprintf(line, sizeof(line), "Error opening %s\n", fileName);
    print_SerialDisplay(line);
    delay(10000); //moves on ater 10secs if file can't be writen to 
  }
}

void adjustTime() {
  //shows utc time between 4/1->10/31 when utc time is 4hr difference. otherwise will show etc time. 
  if ((hr==0 && min==0 && sec==0) || (m<4 || m>10)) {}
  else {
    if (hr>=4) hr -= 4; 
    else {
      hr += 20;
      d -= 1; 
    }    
    if (d==0) {
    m -=1; 
    switch (m) {
      case 4: case 6: case 9:
        d = 30;
        break;
      default:
        d = 31;
        break;
      }
    }
  }
}

void sleepArduino() {
    //interupt watchdog 
    shouldWake = false; 
    MCUSR = 0;
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP3); //setting ~8sec sleep 
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN);
    sleep_enable();
    sleep_cpu();
    sleep_disable();
    
    wdt_enable(WDTO_8S);
}

/*void change_fileName_to_date() {
  if (!(m==0) && !(d==0)) {
    snprintf(dateName, sizeof(dateName), "%02d%02d.txt", m, d);
    if (!SD.exists(dateName) && SD.exists(fileName)) {
      SD.rename(fileName, dateName);
      strcpy(fileName, dateName);  
    }
  }
  else if ((m==0) && (d==0) && SD.exists(fileName) && ((fileName[0]-'0')*10 + (fileName[1]-'0'))<13 && ((fileName[2]-'0')*10 + (fileName[3]-'0'))<32) {
    snprintf(dateName, sizeof(dateName), "%02d%02d.txt", random(13,99), random(32,99));
    SD.rename(fileName, dateName);
    strcpy(fileName, dateName);
  }
}*/
  

