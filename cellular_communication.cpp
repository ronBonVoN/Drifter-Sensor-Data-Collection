#define TINY_GSM_MODEM_SIM7600 //needed for cellular module to work
#define CELL_SERIAL Serial2
#define CELL_PWK 9 
#define CELL_RST 7 

#include <TinyGsmClient.h>
#include "data_structs.h"
#include "functions.h"

TinyGsm modem(CELL_SERIAL); //cellular communication module

Date SIM7600_date; Time SIM7600_time; 
Data SIM7600_lat; Data SIM7600_lng;

char outputCell[512]; 
const char url[200] = "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj";
      // for testing -> "https://discordapp.com/api/webhooks/1401923116128669707/G7_utp4Gbo1fE5foKBWAxCOe12AhQyyvDfCFF5wA0-suP81QI6LCd_ErrZr5gcm_D0Rj"
      // for field   -> "https://discordapp.com/api/webhooks/1403412309438627851/RdeuTCbLB5Ul039usaiiCX5YhAeQGqdQAo4tfQz-Igut3GvUrKP22cAfCnrDrNmX5mA6"

bool commandCell(const char* cmd, const char* res = "OK", unsigned int wait = 10000) {
  char c; // for reading cellular module output
  CELL_SERIAL.print(cmd);
  CELL_SERIAL.print('\r');
  outputCell[0] = '\0'; 
  unsigned long start = millis(); 
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

bool setupCell() {
  //power-key button needed to start cellular module
  CELL_SERIAL.begin(115200); 

  pinMode(CELL_PWK, OUTPUT);
  digitalWrite(CELL_PWK, LOW);
  delay(1200); 
  digitalWrite(CELL_PWK, HIGH); 
  delay(3000); 
  
  unsigned long start = millis(); 
  while (millis()-start < 120000) {
    if (commandCell("AT", "OK", 5000)) {
      digitalWrite(LED_PIN, LOW); 
      return 1;
    }
  }
  return 0;  
}

bool sendCell(const char* message, const char* reason) {
  //message cannot have tabs or newlines
  char status[50]; 
  char url_cmd[200]; 
  char size_cmd[50]; 

  snprintf(status, sizeof(status), "\nSending %s...\n", reason);
  print_SerialDisplay(line);    
  
  //commands to prep for sending data to url 
  snprintf(url_cmd, sizeof(url_cmd), "AT+HTTPPARA=\"URL\",\"%s\"", url); //Set target URL
  snprintf(size_cmd, sizeof(size_cmd), "AT+HTTPDATA=%d,10000", strlen(message)); //Prepare to send POST data
  if (
    commandCell("AT+CSQ") &&  //Check signal quality, output +CSQ: <10-31>,<0-3> is good 
    commandCell("AT+CREG?") && //Network registration status, output +CREG: 0,1 or +CREG: 0,5 is good
    commandCell("AT+CGDCONT=1,\"IP\",\"m2mglobal\"") && //Set PDP context with APN
    commandCell("AT+CGATT=1") && //Attach to the GPRS network
    commandCell("AT+CGACT=1,1") && //Activate the PDP context
    commandCell("AT+HTTPINIT") &&  //Initialize HTTP
    commandCell("AT+HTTPPARA=\"CID\",1") && //Set bearer profile ID for HTTP
    commandCell(cmd) && 
    commandCell("AT+HTTPPARA=\"CONTENT\",\"application/json\"") && //Set content type for POST
    commandCell(size_cmd, "DOWNLOAD") &&
    commandCell(message)) &&
    commandCell("AT+HTTPACTION=1", "+HTTP_PEER_CLOSED", 60000) //Execute HTTP POST
  ) {
    commandCell("AT+HTTPTERM"); //Terminate HTTP session 
    snprintf(status, sizeof(status), "\n%s sent successfully.\n", reason);
    print_SerialDisplay(line, 5000);
    return 1; 
  }    
  commandCell("AT+HTTPTERM");
  snprintf(status, sizeof(status), "\n%s sending failed.\n", reason);
  print_SerialDisplay(status, 10000);
  return 0; 
}

void getCellGPS() {
  SIM7600_date.header.name = "SIM7600 Date"; 
  snprintf(SIM7600_date.units, sizeof(SIM7600_date.units), "UCT+%hdhrs", timeAdjust); 
  SIM7600_time.header.name = "SIM7600 Time"; 
  snprintf(SIM7600_time.units, sizeof(SIM7600_time.units), "UCT+%hdhrs", timeAdjust); 
  
  SIM7600_lat.header.name = "SIM7600 Latitude"; SIM7600_lat.header.units = "GCS";
  SIM7600_lng.header.name = "SIM7600 Longitude"; SIM7600_lng.header.units = "GCS";
  
  if (commandCell("AT+CGPSINFO") && outputCell[27] != ',') {    
    strncpy(SIM7600_lat.str_val,    outputCell+27,11);   SIM7600_lat.str_val[13] = '\0';    
    strncpy(SIM7600_lng.str_val,    outputCell+41,12);   SIM7600_lng.str_val[14] = '\0';   
    strncpy(SIM7600_date.str_day,   outputCell+56, 2);   SIM7600_date.str_day[2] = '\0';
    strncpy(SIM7600_date.str_month, outputCell+58, 2); SIM7600_date.str_month[2] = '\0';
    strncpy(SIM7600_date.str_year,  outputCell+60, 2);  SIM7600_date.str_year[2] = '\0';
    strncpy(SIM7600_time.str_hr,    outputCell+63, 2);    SIM7600_time.str_hr[2] = '\0';
    strncpy(SIM7600_time.str_min,   outputCell+65, 2);   SIM7600_time.str_min[2] = '\0';
    strncpy(SIM7600_time.str_sec,   outputCell+67, 2);   SIM7600_time.str_sec[2] = '\0';
    
    SIM7600_lat.val = atof(SIM7600_lat.str_val); 
    SIM7600_lng.val = atof(SIM7600_lng.str_val);

    SIM7600_lat.val = floor(SIM7600_lat.val/100) + (SIM7600_lat.val - floor(SIM7600_lat.val/100)*100)/60;
    SIM7600_lng.val = floor(SIM7600_lng.val/100) + (SIM7600_lng.val - floor(SIM7600_lng.val/100)*100)/60;
    
    if (outputCell[39] == 'S')  SIM7600_lat.val *= -1; 
    if (outputCell[54] == 'W')  SIM7600_lng.val *= -1; 
    
    SIM7600_date.month = atoi(SIM7600_date.str_month); 
    SIM7600_date.day = atoi(SIM7600_date.str_day); 
    SIM7600_date.year = atoi(SIM7600_date.str_year) + 2000; 
    SIM7600_time.hr = atoi(SIM7600_time.str_hr); 
    SIM7600_time.min = atoi(SIM7600_time.str_min); 
    SIM7600_time.sec = atoi(SIM7600_time.str_sec); 
  } 
  else {
    SIM7600_lat.val = 0.0; 
    SIM7600_lng.val = 0.0;
    SIM7600_date.month = 0; 
    SIM7600_date.day = 0; 
    SIM7600_date.year = 2000; 
    SIM7600_time.hr = 0; 
    SIM7600_time.min = 0; 
    SIM7600_time.sec = 0; 
  }
}