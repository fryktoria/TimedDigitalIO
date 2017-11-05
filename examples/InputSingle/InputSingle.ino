/*
  TimedDigitalIO - Library for timing Digital inputs.
  Created by Ilias Iliopoulos, October 23, 2017.

  Example sketch
  Presents the TimedDigitalInput class, where each sensor
  is instantiated manualy. For production usage, base your sketches 
  to the SensorArray example
*/
// Use the Time library for time manipulation (numer to time and vice versa)
#include <TimeLib.h>
// Use the Wire library to communicate with RTC via I2C
#include <Wire.h>
// The library for the RealTimeClock
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t

// Our library
#include "TimedDigitalIO.h"

TimedDigitalInput tdi_0;
TimedDigitalInput tdi_1;

void setup()  {
  
  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  Serial.println(F("****** Timed Digital Input Example *******"));
  Serial.println(F("For production environment, remember to comment line #define TDI_DEBUG in TimedDigitalInput.h, "));
   
  setSyncProvider(RTC.get);   // the function to sync the time from the RTC  
  setSyncInterval(300); // set the number of seconds between re-sync of the RTC

  // Creates a sensor named "Pump", which reads arduino digital pin 8, is active-ON, 
  // pinMode is INPUT and uses EEPROM memory block 0 
  tdi_0.begin("Pump", 8, TDIO_LOGIC_POSITIVE, false, 0);


  // Creates a sensor named "Heater", which reads arduino digital pin 9, is active-ON, 
  // pinMode is INPUT and uses EEPROM memory block 1   
  tdi_1.begin("Heater", 9, TDIO_LOGIC_POSITIVE, false, 1);
  // Set the EEPROM recording interval in seconds
  tdi_1.setEEPROMRecordingInterval(3600*24);


  // Presents what has been recorded so far in the EEPROM for memory blocks 0 and 1
  InputSensorArray::printMonthlyActivity(0);
  InputSensorArray::printMonthlyActivity(1);  

  Serial.print(F("Time now is "));
  printHumanTime(now());
  Serial.println();
  
}

void loop() {

  static uint32_t previousMillis;
  
  tdi_0.readSensor(); 
  tdi_1.readSensor();

  // Report current status every so often.
  // Not too fast to allow quick reading of 
  // the pins.  

  if (millis() - previousMillis > 5000) {
    previousMillis = millis();
    InputSensorArray::printSensorData(&tdi_0);
    InputSensorArray::printSensorData(&tdi_1);

  }
    
}





