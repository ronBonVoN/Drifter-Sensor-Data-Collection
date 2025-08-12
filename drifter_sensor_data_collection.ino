/*
Veronika Davis
Kilroy was here
*/

/*to-do
 ask abt units
 add bools display and led 
*/

//change for testing 
#define SLEEP_LOOPS 86 // 86 = ~10mins
#define GPS_READ_MILLIS 180000 //180000 = 3mins 

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
#define MODEM_PWK 9 
#define MODEM_RST 7 
#define GPS_SERIAL Serial1
#define MODEM_SERIAL Serial2
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
Adafruit_MCP9600 mcp; //thermocouple
TinyGPSPlus gps;
TinyGsm modem(MODEM_SERIAL); //cellular communication module

// customizable vars
const char drifterName[20] = "Kilroy"; 
const char fileName[13] = "DATA.txt"; 
const char url[200] = "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj";
    // for testing -> "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj"
    // for field   -> "https://discordapp.com/api/webhooks/1403412309438627851/RdeuTCbLB5Ul039usaiiCX5YhAeQGqdQAo4tfQz-Igut3GvUrKP22cAfCnrDrNmX5mA6"

// change for testing 
short displayCount = 100; //# of cycles display will run
const bool serialEnable = 0; 

short m = -1, d = -1, y = -1, hr = -1, min = -1, sec = -1; 
short i;             //for intexing Modem output 
unsigned long start; //for millis() while loops
float lat = -1.0, lng = -1.0, speed = -1.0, heading = -1.0, tempHot = -1.0, tempCold = -1.0, turbidity = -1.0; 
char c;             //for reading Modem output
char line[150];     // for general writing/printing
char cmd[200];      // for building modem commands 
char msg[200];      // for building modem message command 
char data[13][15];  // where sensor data will go
char outputModem[512];
bool firstRun = 1; 
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

void setup() {
  delay(5000); //time for arduino to adjust if program resets 
  if (serialEnable) Serial.begin(115200);
  GPS_SERIAL.begin(9600); 
  MODEM_SERIAL.begin(115200); 
  Wire.begin(); //for i2c com
  Wire.setClock(100000); //make i2c com slower  
  delay(5000); 

  pinMode(RELAY_PIN, OUTPUT);
  delay(2000); 
  digitalWrite(RELAY_PIN, HIGH); 
  delay(2000); 
  
  pinMode(LED_PIN, OUTPUT); 

  if (displayCount>0) initialize(display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS), "display");
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
      print_SerialDisplay("SD card failed.\n", 10000); 
    }
  }  
  else {
    firstRun = 0;
  }

  print_SerialDisplay("Reading gps..."); 
  start = millis();
  do{
      while (GPS_SERIAL.available())
      gps.encode(GPS_SERIAL.read()); 
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
  turbidity = analogRead(TURBIDITY_PIN) * (5.0/1024.0); //reads voltage  
  print_SerialDisplay("sensor reading done.\n"); 

  //writing floats to char array      ***size************ 
  snprintf(data[0],  3, "%02d", m);   // MM\0
  snprintf(data[1],  3, "%02d", d);   // DD\0
  snprintf(data[2],  5, "%04d", y);   // YYYY\0
  snprintf(data[3],  3, "%02d", hr);  // 00\0
  snprintf(data[4],  3, "%02d", min); // 00\0
  snprintf(data[5],  3, "%02d", sec); // 00\0
   dtostrf(lat,      10, 8, data[6]); // -90.000000 -> _90.000000
  dtostrf(lng,       10, 8, data[7]); // -90.000000 -> _90.000000
  dtostrf(speed,     5,  2, data[8]);  // 00.00
  dtostrf(heading,   6,  2, data[9]);  // __0.00 -> 360.00
  dtostrf(tempHot,   6,  2, data[10]); // -00.00 -> _00.00
  dtostrf(tempCold,  6,  2, data[11]); // -00.00 -> _00.00
  dtostrf(turbidity, 4,  2, data[12]); // 0.00

  //printing data to file 
  snprintf(line, sizeof(line), "%s-%s-%s\t%s:%s:%s\t%s\t%s\t%s\t%s\t%s\t%s\t%s\n",
    data[0], data[1], data[2], data[3], data[4], data[5],
    data[6], data[7], data[8], data[9], data[10], data[11], data[12]);
  print_SerialFile(line); 

  sendData(); //send data to url through modem

  if (displayCount>0) {
    display.clearDisplay(); 
    display.setTextColor(WHITE); 
    display.setTextSize(1); 
    snprintf(line, sizeof(line), "%s-%s-%s %s:%s:%s", data[0], data[1], data[2], data[3], data[4], data[5]); 
    display.setCursor(0,0);  display.print(line);                                                            
    snprintf(line, sizeof(line), "lat:%s", data[6]);
    display.setCursor(0,10); display.print(line);                                          
    snprintf(line, sizeof(line), "lng:%s", data[7]);                                              
    display.setCursor(0,20); display.print(line); 
    snprintf(line, sizeof(line), "mph:%s deg:%s", data[8], data[9]);                                         
    display.setCursor(0,30); display.print(line);
    snprintf(line, sizeof(line), "temp:%s, %s", data[10], data[11]);
    display.setCursor(0,40); display.print(line);
    snprintf(line, sizeof(line), "turbidity: %s", data[12]);
    display.setCursor(0,50); display.print(line);
    display.display(); 
    delay(10000); 
  }

  if (displayCount==1) {
    print_SerialDisplay("Sleeping display....\n", 2000); 
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
    print_SerialDisplay(line, 10000); //moves on after 10secs if sensor failed 
  }
}

void status(const char* msg, unsigned short blink = 0, bool condition = 1) {
  if (serialEnable) Serial.print(message); 
  if (displayCount>0) {
    display.clearDisplay(); 
    display.setTextColor(WHITE); 
    display.setTextSize(1); 
    display.setCursor(0,20);
    display.print(message); 
    display.display();
    delay(wait); 
  if (condition) {
    for (int i=0; i<blink; i++) {
      digitalWrite(LED_PIN, HIGH); 
      delay(200); 
      digitalWrite(LED_PIN, LOW); 
    } 
  }
}

}

void print_SerialFile(const char* message) {
  if (serialEnable) Serial.print(message); 
  wdt_enable(WDTO_8S); //enabling watchdog, will reset arduino if wdt_reset() is not called in 8secs
  File dataFile = SD.open(fileName, FILE_WRITE);
  if (dataFile) {
    dataFile.print(message);
    dataFile.close();
    wdt_disable();
  }
  else { 
    wdt_disable();
    snprintf(line, sizeof(line), "Error opening %s\n", fileName);
    print_SerialDisplay(line, 10000); //moves on ater 10secs if file can't be writen to 
  }
}

void adjustTime() {
  //shows etc time between 4/1->10/31 when utc time is 4hr difference. otherwise will show utc time. 
  if (!(hr==0 && min==0 && sec==0) || !(m<4 || m>10)) {
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

//for sending modem commands
bool sendCommand(const char* cmd, const char* res = "OK", int wait = 10000) {
  MODEM_SERIAL.print(cmd);
  outputModem[0] = '\0'; 
  start = millis(); 
  while (millis() - start < wait) {    
    while (MODEM_SERIAL.available()) {
      c =  MODEM_SERIAL.read();
      if (serialEnable) Serial.write(c);
      i = strlen(outputModem); 
      if (i < sizeof(outputModem)-1) {
        outputModem[i] = c; 
        outputModem[i + 1] = '\0'; 
      }
    } 
     if (strstr(outputModem, res)) return 1; 
  }
  if (serialEnable) {
    Serial.print(cmd); 
    Serial.print(" timed out waiting for "); 
    Serial.println(res);
  }
  return 0; 
}

void sendData() {
  print_SerialDisplay("Initializing modem...\n\n");  
  //power-key button needed to start modem
  pinMode(MODEM_PWK, OUTPUT);
  digitalWrite(MODEM_PWK, LOW);
  delay(3000); 
  digitalWrite(MODEM_PWK, HIGH); 
  delay(3000); 
  
  start = millis(); 
  while (millis()-start < 60000) {
    if (sendCommand("AT\r")) break; 
  } 
  if (!sendCommand("AT\r")) {
    print_SerialDisplay("\nmodem initialization failed.\n", 10000);
    goto shutdown;  
  } 
  print_SerialDisplay("\nmodem initialization done.\n", 2000); 
  print_SerialDisplay("Sending data...\n");
  
  //commands to prep for sending data to url 
  if (!sendCommand("AT+CSQ\r")) goto shutdown;   
  if (!sendCommand("AT+CREG?\r")) goto shutdown;    
  if (!sendCommand("AT+CGDCONT=1,\"IP\",\"m2mglobal\"\r")) goto shutdown;    
  if (!sendCommand("AT+CGATT=1\r")) goto shutdown;    
  if (!sendCommand("AT+CGACT=1,1\r")) goto shutdown;    
       sendCommand("AT+HTTPTERM\r", "AT+HTTPTERM");  
  if (!sendCommand("AT+HTTPINIT\r")) goto shutdown;     
  if (!sendCommand("AT+HTTPPARA=\"CID\",1\r")) goto shutdown;     
       snprintf(cmd, sizeof(cmd), "AT+HTTPPARA=\"URL\",\"%s\"\r", url);
  if (!sendCommand(cmd)) goto shutdown;
  if (!sendCommand("AT+HTTPPARA=\"CONTENT\",\"application/json\"\r")) goto shutdown;    
  
  //creating message to send
  snprintf(msg, sizeof(msg), "{\"content\":\"~%s %s-%s-%s %s:%s:%s %s %s %s %s %s %s %s\"}",
    drifterName, 
    data[0], data[1], data[2], data[3], data[4], data[5],
    data[6], data[7], data[8], data[9], data[10], data[11], data[12]);
  
  //sending message
  snprintf(cmd, sizeof(cmd), "AT+HTTPDATA=%d,10000\r", strlen(msg));
  if (!sendCommand(cmd, "DOWNLOAD")) goto shutdown;    
  if (!sendCommand(msg)) goto shutdown; 
  delay(5000); 
  if (sendCommand("AT+HTTPACTION=1\r", "+HTTP_PEER_CLOSED", 60000)) {
    delay(5000);
    print_SerialDisplay("\ndata sent succesfully.\n", 5000);
  }
  else print_SerialDisplay("\ndata sending failed.\n", 10000);
  
  //prep for shutdown 
  shutdown:  
    print_SerialDisplay("\nModem shutdown...\n");
    delay(5000);
    sendCommand("AT+HTTPTERM\r");   
    if (sendCommand("AT+CPOF\r")) print_SerialDisplay("\nmodem shutdown successfully.\n, 5000");
    else print_SerialDisplay("\nmodem shutdown failed.\n", 10000);
}






  

