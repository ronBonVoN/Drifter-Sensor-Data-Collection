#include <SPI.h>
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

#define I2C_ADDRESS (0x67)
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET     4
#define SCREEN_ADDRESS (0x3C)

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Adafruit_MCP9600 mcp;
TinyGPSPlus gps;

const int chipSelect = 53;
int runs = 0; 
int m = -1, d = -1, y = -1, hr = -1, min = -1, sec = -1; 
//String date = "-1", time = "-1"; 
float lat = -1.0, lng = -1.0, speed = -1.0, heading = -1.0, tempHot = -1.0, tempCold = -1.0, turbidity = -1.0, oxygen = -1.0; 
const char fileName[10] = "testing8.txt"; 
char line[150];
char data[14][10];
volatile bool shouldWake = false;
bool serialEnable = 0; 

/*ISR(WDT_vect) {
  shouldWake = true;
}*/

void setup() {

  delay(10000); 
  if (serialEnable) Serial.begin(9600);
  Serial1.begin(9600);
  Wire.begin(); 
  pinMode(4, OUTPUT);
  delay(2000); 
  digitalWrite(4, HIGH); 
  delay(2000); 

  initialize(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS), "display");
  initialize(SD.begin(chipSelect), "SD card");
  initialize(mcp.begin(I2C_ADDRESS), "thermocouple");

  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    println_SerialFile("Date\tTime\tLatitude\tLongitude\tSpeed\tHeading\tHot Junction\tCold Junction\tTurbidity\tOxygen", dataFile);
  }
  else {
    snprintf(line, sizeof(line), "error opening %s", fileName);
    print_SerialDisplay(line, 1);
    while(1); 
  }
}

void loop() {
  if (runs<2) {
    runs++;  
    
    pinMode(4, OUTPUT);
    SD.begin(chipSelect);   
      
    unsigned long start = millis();
    do{
        while (Serial1.available())
        gps.encode(Serial1.read()); 
    } while (millis()-start < 5000);

    m = gps.date.month(); d = gps.date.day(); y = gps.date.year(); 
    hr = gps.time.hour(); min = gps.time.minute(); sec = gps.time.second(); 
      String(m)+"-"+String(d)+"-"+String(y)+"\t"+
      String(hr)+":"+String(min)+":"+String(sec)+"\t"+
  else {
  runs = 0; 
  wdt_enable(WDTO_8S); 
  print_SerialDisplay("Entering sleep mode...", 0); 
  wdt_reset(); 
  
  digitalWrite(4, LOW); 
  delay(5000);  //sleep_timed(16);
  digitalWrite(4,HIGH);
  
  wdt_reset(); 
  delay(3000); 
  print_SerialDisplay("out of sleep mode.", 1); 
  wdt_disable();
  delay(3000); 
  }

}

void initialize(bool x, const char* sensor) {
  snprintf(line, sizeof(line), "Initializing %s", sensor, "...");
  print_SerialDisplay(line, 0);
  if(!x) {
    snprintf(line, sizeof(line), "%s failed", sensor);
    print_SerialDisplay(line, 1);
    while (1); 
  }
  snprintf(line, sizeof(line), "%s initialization done.", sensor);
  print_SerialDisplay(line, 1);
}

void print_SerialDisplay(const char* message, bool end) {
  if (serialEnable) {
  if (end) Serial.println(message); 
  else Serial.print(message); 
  }
  display.clearDisplay(); 
  display.setTextColor(WHITE); 
  display.setTextSize(1); 
  display.setCursor(0,20);
  display.print(message); 
  display.display();
}

/*void print_SerialDisplay(String s, bool end) {
  if (end) Serial.println(s); 
  else Serial.print(s); 
  display.clearDisplay(); 
  display.setTextColor(WHITE); 
  display.setTextSize(1); 
  display.setCursor(0,20);
  display.print(s); 
  display.display();
}*/

void println_SerialFile(const char* message, File dataFile) {
  if (serialEnable) Serial.println(message); 
  dataFile.println(message);
  dataFile.close(); 
}

void sleep_timed(int secs) {
  int loops = secs/8; 
  for (int i=0; i<loops; i++) {
    shouldWake = false;
    
    MCUSR = 0;
    WDTCSR |= (1 << WDCE) | (1 << WDE);
    WDTCSR = (1 << WDIE) | (1 << WDP3); 
    
   // power_adc_disable();
   // power_spi_disable();
   // power_twi_disable();
   // power_timer1_disable(); 
   // power_timer2_disable(); 
    
    set_sleep_mode(SLEEP_MODE_PWR_DOWN); //SLEEP_MODE_IDLE or SLEEP_MODE_PWR_DOWN
    sleep_enable();

    // only in SLEEP_MODE_IDLE
   // while (!shouldWake) sleep_mode(); 
    
  //only in SLEEP_MODE_PWR_DOWN
  /*  noInterrupts(); 
  #if defined(BODS) && defined(BODSE)
    MCUCR |= (1 << BODS) | (1 << BODSE);
    MCUCR &= ~(1 << BODSE);
  #endif
    interrupts();*/
    
    sleep_cpu();

    sleep_disable();
    wdt_disable(); 

  //  power_adc_enable();
   // power_spi_enable();
   // power_twi_enable();
   // power_timer1_enable();
   // power_timer2_enable(); 
  }
}

/*void delayWithWDT(unsigned long ms) {
  unsigned long start = millis();
  while (millis() - start < ms) {
    wdt_reset();       // Feed the watchdog
    delay(50);         // Short delay to avoid CPU hammering
  }
}*/
