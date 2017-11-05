/*
  TimedDigitalIO - Library for timing Digital inputs.
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

// The SensorArray class instantiates an array of TimedDigitalInput classes, tdi[TDI_MAX_SENSORS]
// e.g. s.tdi[0], s.tdi[1] etc
InputSensorArray s;

void setup()  {
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println(F("****** Timed Digital Input Example *******"));
  Serial.println(F("For production environment, remember to set #define TDI_DEBUG 0 in TimedDigitalIO.h, "));
   
  setSyncProvider(RTC.get);   // the function to sync the time from the RTC  
  setSyncInterval(300); // set the number of seconds between re-sync of the RTC

  // Creates a sensor named "Pump" using tdi[0], which reads arduino digital pin 8, is active-ON, 
  // pinMode is INPUT and uses EEPROM memory block 0 
  s.tdi[0].begin("Pump", 8, TDIO_LOGIC_POSITIVE, false, 0);


  // Creates a sensor named "Heater"  using tdi[1], which reads arduino digital pin 9, is active-ON, 
  // pinMode is INPUT and uses EEPROM memory block 1   
  s.tdi[1].begin("Heater", 9, TDIO_LOGIC_POSITIVE, false, 1);
  // Set the EEPROM recording interval in seconds
  // Deliberately setting a value lower than EEPROM_MINIMUM_RECORDING_INTERVAL
  // to show the fail-safe
  s.tdi[1].setEEPROMRecordingInterval(20);

  // Two more sensors
  s.tdi[2].begin("Boiler", 10, TDIO_LOGIC_POSITIVE, false, 2);
  s.tdi[3].begin("Lamp", 11, TDIO_LOGIC_POSITIVE, false, 3); 

  // Presents what has been recorded so far in the EEPROM for memory blocks 0 and 1
  s.printMonthlyActivity(0);
  s.printMonthlyActivity(1);  

  Serial.print(F("Time now is "));
  printHumanTime(now());
  Serial.println();
  
}

void loop() {

  // The loop() function must be designed to run as quicly as possible,
  // in order to record the state of pins with millisecond accuracy. 
  // Do not use delay(). If you wish to do things at frequent intervals,
  // do it as per the example below which prints sensor data every 5 seconds.

  static uint32_t previousMillis;
  
  s.tdi[0].readSensor(); 
  s.tdi[1].readSensor();
  s.tdi[2].readSensor();
  s.tdi[3].readSensor();

  // Report current status every so often.
  // Not too fast to allow quick reading of 
  // the pins.  

  if (millis() - previousMillis > 5000) {
    previousMillis = millis();
    s.printSensorData(&s.tdi[0]);
    s.printSensorData(&s.tdi[1]);
    //s.printSensorData(&s.tdi[2]);
    //s.printSensorData(&s.tdi[3]);
    
  }
    
}





