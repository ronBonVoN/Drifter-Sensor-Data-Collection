#include <avr/sleep.h>
#include <avr/wdt.h> 
#include <avr/power.h>
#include <avr/interrupt.h>

volatile bool shouldWake = false;

//for interrupt watchdog
ISR(WDT_vect) {
  shouldWake = true;
}

void sleepArduino(const int loops) { 
  power_spi_disable(); //turn off spi com
  power_twi_disable(); //turn off i2c com

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
  
  power_spi_enable(); // turn on spi 
  power_twi_enable(); // turn on i2c
}