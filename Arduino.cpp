#include "data_structs.h"
#include "functions.h"
#include <Arduino.h>
#include <avr/sleep.h>
#include <avr/wdt.h> 
#include <avr/power.h>
#include <avr/interrupt.h>

volatile bool shouldWake = false;

//for interrupt watchdog
ISR(WDT_vect) {
  shouldWake = true;
}

namespace Arduino {
  bool initialize(bool device, const char* name) {
    snprintf(line, sizeof(line), "Initializing %s...", name);
    Display::write(line);
    if(device) {
      snprintf(line, sizeof(line), "%s initialization done.\n", name);
      Display::write(line);
      return 1; 
    }
    else {
      snprintf(line, sizeof(line), "%s failed.\n", name);
      Display::write(line, 10000); //moves on after 10secs if sensor failed 
      return 0; 
    }
  }
  
  void sleep(int loops) {
    Display::write("Sleep mode...\n"); 
    wdt_enable(WDTO_8S); //enabling watchdog, will reset arduino if wdt_reset() is not called in 8secs 
    power_spi_disable(); //turn off spi com
    power_twi_disable(); //turn off i2c com
    digitalWrite(Relay::pin, LOW);
    wdt_disable();

    LED::blink(2,0,2000); 
    for(int i=0; i<loops; i++) {
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
    LED::blink(2,0,2000); 
    
    wdt_enable(WDTO_8S);
    digitalWrite(Relay::pin, HIGH);  
    power_spi_enable(); // turn on spi 
    power_twi_enable(); // turn on i2c
    wdt_disable();
    Display::write("out of sleep mode.\n"); 
  }
}