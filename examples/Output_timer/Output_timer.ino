/*
  TimedDigitalIO - Library for timing Digital inputs and Outputs.
  Created by Ilias Iliopoulos, October 23, 2017.

 * Example to demonstrate an output with a timer 
 * 
*/
// Use the Time library for time manipulation (numer to time and vice versa)
#include <TimeLib.h>
// Use the Wire library to communicate with RTC via I2C
#include <Wire.h>
// The library for the RealTimeClock
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

// Our library
#include "TimedDigitalIO.h"

OutputSensorArray s;

uint32_t previousMillis = 0;

void setup()  {
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println(F("****** Timed Digital Output Example *******"));
   
  setSyncProvider(RTC.get);   // the function to sync the time from the RTC  
  setSyncInterval(300); // set the number of seconds between re-sync of the RTC

  // Creates a sensor named "Pump" using tdo[0] on pin 4
  s.tdo[0].begin("Pump", 4, TDIO_LOGIC_POSITIVE);

  // Set state to ON and start timer to 20 seconds
  s.tdo[0].setOn(20000);
  
}

void loop() {

  // This method must be in the loop and run as quicly as possible 
  // depending on the application
  s.tdo[0].checkTimer();

  // Normally, long intervals in the order of several seconds will be set
  // You can print sensor data every so often to check the remaining time
  // before going back to OFF
  if (millis() - previousMillis > 2000) {

    s.printSensorData(&s.tdo[0]);
    previousMillis = millis();
  }  
   
     
}

