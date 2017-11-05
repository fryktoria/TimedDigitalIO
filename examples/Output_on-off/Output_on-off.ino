/*
  TimedDigitalIO - Library for timing Digital inputs and Outputs.
  Created by Ilias Iliopoulos, October 23, 2017.

  Example sketch
*/
// Use the Time library for time manipulation (numer to time and vice versa)
#include <TimeLib.h>
// Use the Wire library to communicate with RTC via I2C
#include <Wire.h>
// The library for the RealTimeClock
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

// Our library
#include "TimedDigitalIO.h"

TimedDigitalOutput tdo;

void setup()  {
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println(F("****** Timed Digital Output Example *******"));
   
  setSyncProvider(RTC.get);   // the function to sync the time from the RTC  
  setSyncInterval(300); // set the number of seconds between re-sync of the RTC

  // Creates a sensor named "Pump" on pin 4
  tdo.begin("Pump", 4, TDIO_LOGIC_POSITIVE);
  
}

void loop() {

/*
 *  Simple example to demonstrate setting to ON and OFF
 *  and create a pulse
*/
  tdo.setOn(0); 
  delay(1000);
  tdo.setOff();
  delay(1000);
      
}

