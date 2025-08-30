#define TINY_GSM_MODEM_SIM7600 //needed for cellular module to work
#define CELL_SERIAL Serial2
#define CELL_PWK 9 
#define CELL_RST 7 

#include <TinyGsmClient.h>
#include <Arduino.h>
#include "data_structs.h"
#include "functions.h"

TinyGsm modem(CELL_SERIAL); 

namespace Cellular {
  void setup() {
    CELL_SERIAL.begin(115200);
  }
  
  bool command(const char* cmd, const char* res = "OK", unsigned int wait = 10000) {
    char c; 
    int i; 
    CELL_SERIAL.print(cmd);
    CELL_SERIAL.print('\r');
    output[0] = '\0'; 
    
    unsigned long start = millis(); 
    while (millis() - start < wait) {    
      while (CELL_SERIAL.available()) {
        c =  CELL_SERIAL.read();
        if (serialEnable) Serial.write(c);
        i = strlen(output); 
        if (i < sizeof(output)-1) {
          output[i] = c; 
          output[i + 1] = '\0'; 
        }
      } 
      if (strstr(output, res)) return 1; 
    }   
    if (serialEnable) { 
      Serial.println(); 
      Serial.print(cmd); 
      Serial.print(" timed out waiting for "); 
      Serial.println(res);
    }
    return 0; 
  }

  bool init() {
    //power-key button needed to start cellular module
    Display::write("Initializing cellular module..."); 
    digitalWrite(LED::pin, HIGH); 
    pinMode(CELL_PWK, OUTPUT);
    digitalWrite(CELL_PWK, LOW);
    delay(1200); 
    digitalWrite(CELL_PWK, HIGH); 
    delay(3000); 
    
    unsigned long start = millis(); 
    while (millis()-start < 120000) {
      if (command("AT", "OK", 5000)) {
        digitalWrite(LED::pin, LOW); 
        Display::write("\ncellular module initialization done.\n"); 
        return 1;
      }
    }
    digitalWrite(LED::pin, LOW);
    Display::write("\ncellular module initialization failed.\n"); 
    return 0;  
  }

  void send(const char* msg, const char* reason) {
    //message cannot have tabs or newlines
    char status[50]; 
    snprintf(status, sizeof(status), "\nSending %s...\n", reason);
    Display::write(status);    
    if (serialEnable) Serial.println(msg); 
    if (
      command("AT+CSQ") &&  //Check signal quality, output +CSQ: <10-31>,<0-3> is good 
      command("AT+CREG?") && //Network registration status, output +CREG: 0,1 or +CREG: 0,5 is good
      command("AT+CGDCONT=1,\"IP\",\"m2mglobal\"") && //Set PDP context with APN
      command("AT+CGATT=1") && //Attach to the GPRS network
      command("AT+CGACT=1,1") && //Activate the PDP context
      command("AT+HTTPINIT") &&  //Initialize HTTP
      command("AT+HTTPPARA=\"CID\",1") && //Set bearer profile ID for HTTP
      snprintf(line, sizeof(line), "AT+HTTPPARA=\"URL\",\"%s\"", url) && //Set target URL
      command(line) && 
      command("AT+HTTPPARA=\"CONTENT\",\"application/json\"") && //Set content type for POST
      snprintf(line, sizeof(line), "AT+HTTPDATA=%d,10000", strlen(msg)) && //Prepare to send POST data
      command(line, "DOWNLOAD") &&
      command(msg) &&
      command("AT+HTTPACTION=1", "+HTTP_PEER_CLOSED", 60000) //Execute HTTP POST
    ) {
      command("AT+HTTPTERM"); //Terminate HTTP session 
      snprintf(status, sizeof(status), "\n%s sent successfully.\n", reason);
      Display::write(status, 5000);
      return; 
    }    
    command("AT+HTTPTERM");
    snprintf(status, sizeof(status), "\n%s sending failed.\n", reason);
    Display::write(status, 10000);
  } 

  void sleep() {
    Display::write("\nCellular module shutdown...\n");
    if (command("AT+CPOF")) Display::write("\ncellular module shutdown successfully.\n", 5000); 
    else Display::write("\ncellular module shutdown failed.\n", 10000);
  }
}



