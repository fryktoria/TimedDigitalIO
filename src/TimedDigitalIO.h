/*
  TimedDigitalInput.cpp - Library for timing Digital inputs and outputs

    Copyright (C) 2017  Ilias Iliopoulos

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

    This library is using the following other libraries:
    
    EEPROM.h  - Arduino official library - Original Copyright (c) 2006 David A. Mellis.  New version by Christopher Andrews 2015.
    Time.h/Timelib.h - Arduino official library with parts Copyright (c) Michael Margolis 2009-2014 https://github.com/PaulStoffregen/Time
    Wire.h - Arduino official library TwoWire.h - TWI/I2C library for Arduino & Wiring  Copyright (c) 2006 Nicholas Zambetti.
    DS1307RTC.h  - Copyright (c) Michael Margolis 2009 https://github.com/PaulStoffregen/DS1307RTC

    At the time of this writing, those libraries were licensed as per the GNU Lesser General Public License. 

    Please check for any update on the licensing status of all software for compatibility and compliance to your intended usage. 


  1.0 05 Nov 2017 - Initial release 
 
*/

#ifndef TIMED_DIGITAL_IO_H
#define TIMED_DIGITAL_IO_H

#if ARDUINO >= 100
  #include "Arduino.h"
#else
#include <WProgram.h> 
#endif

// Set TDIO_DEBUG 0 for production environment.
// Allows the printing of sensor information in a sketch debugging phase
#define TDIO_DEBUG 1

// The maximum number of input sensors that can be defined.
// The library instantiates all these sensors, which are configured later with begin()
// but occupy memory regardless if a sensor is used or not. 
// Use the lowest possible value, in order to save memory.
#define TDI_MAX_SENSORS 4

// The maximum number of output sensors that can be defined.
#define TDO_MAX_SENSORS 4

// Starting location of the storage space within the EEPROM.
// The data occupy (4 bytes per month * 12 months) * TDI_MAX_SENSORS
#define EEPROM_OFFSET 0

// Set the period for recording data to EEPROM in SECONDS
// Try to avoid writing data too often because EEPROM has a lifetime
// of 100.000 writes. Once every 12 hours is OK. Will allow more than 100 years of life.
// Must be higher than EEPROM_MINIMUM_RECORDING_INTERVAL. If not, the code makes a correction and 
// sets to EEPROM_MINIMUM_RECORDING_INTERVAL 
#define EEPROM_DEFAULT_RECORDING_INTERVAL 43200 // Every 12 hours 12*3600=43200
//#define EEPROM_DEFAULT_RECORDING_INTERVAL 10 // for test purposes

// To avoid killing the EEPROM due to a possible error in setting a low value for recording interval
// that may cause too frequent writes, we have this safety measure.
// If the user tries to set a lower value, the value will be overriden to this minimum
#define EEPROM_MINIMUM_RECORDING_INTERVAL 3600 * 6 // 6 hours

// Maximum size of the sensor name in bytes
#define MAX_SENSOR_NAME 10

// The logic of the sensor. 
// TDIO_LOGIC_POSITIVE means ON is logic level 1
// TDIO_LOGIC_NEGATIVE means ON is logic level 0
#define TDIO_LOGIC_POSITIVE 1
#define TDIO_LOGIC_NEGATIVE 0

// Sensor states ON and OFF
#define TDIO_STATE_ON 1
#define TDIO_STATE_OFF 0

////////// Dependency on other libraries /////////

// To write in NVRAM monthly data of on time for digital inputs
#include <EEPROM.h>

// Use the Time library for time manipulation (numer to time and vice versa)
#include <TimeLib.h>

// Use the Wire library to communicate with RTC via I2C
#include <Wire.h>

// The library for the RealTimeClock
#include <DS1307RTC.h>  // a basic DS1307 library that returns time as a time_t


////////// Class Definitions/////////

class TimedDigitalInput {

  // Durations are stored in millis.
  // DateTimes are stored as unix seconds
  
  private:
    uint8_t _previousState = TDIO_STATE_OFF;
    uint32_t _previousMillis = 0; 
    time_t _timeNow; // current unix time
    // Set how often data will be stored to EEPROM
    uint32_t _EEPROMRecordingInterval;   
    // Variable to store the time when data were written lastly to EEPROM
    uint32_t _previousEEPROMWriteMillis;
        
    //////////////////////////////////////////////////////////////////
    // Private Functions
    //////////////////////////////////////////////////////////////////
    void printStateChangeInfo(void);
    void setState(uint8_t value); 
    void toggleStateOn(void);
    void toggleStateOff(void);    
    void recordUpToNow(void);
    void storeEEPROM(uint8_t month, uint32_t value);
    uint32_t readEEPROM(uint8_t month);
    boolean pinValid(uint8_t mypin);
       
  public:

    // Public name of sensor
    // Longer names are truncated to this size
    char sensorName[MAX_SENSOR_NAME];

    // Pin associated with the digital input sensor
    uint8_t sensorPin;

    // Current state can be on or off, regardless of logic
    byte sensorState;
    
    // Positive logic means sensor value equal 1 is on. Negative means 1 is off
    byte sensorLogic;

    // Each sensor is assigned to an EEPROM block where data are logged
    uint8_t EEPROMBlock;

    // Month and Day where time is counted. 
    uint8_t currentMonth;
    uint8_t currentDay;

    // ON duration of current month
    uint32_t currentMonthOnDuration;

    // Number of times the sensor came ON today and duration
    uint32_t todayOnCounter;
    uint32_t todayOnDuration;
    
    // Time that the sensor has reported as On during the previous OFF - ON - OFF sequence
    uint32_t previousOnDuration;    
    time_t previousOnStartDateTime;   
    time_t previousOnStopDateTime;
    
    // Time that the sensor is currently reporting as ON
    uint32_t currentOnDuration;
    
    // Time in unixtime when the pin started reporting state ON
    time_t currentOnStartDateTime;

    //////////////////////////////////////////////////////////////////
    // Public Functions
    //////////////////////////////////////////////////////////////////
    TimedDigitalInput(void);
    int begin(const char *name, uint8_t a_pin, uint8_t sensor_logic, boolean pullup, uint8_t eepromBlock);
    void readSensor(void); 
    void setEEPROMRecordingInterval(uint32_t interval);
 
};

//--------------------------------------------------------
// The inputSensorArray class instantiates an array of 
// TimedDigitalInput classes, tdi[TDI_MAX_SENSORS]
// e.g. s.tdi[0], s.tdi[1] etc.
// It also contains some useful functions which would
// occupy a lot of memory space if instantiated at the 
// TimedDigitalInput class. Here, they are defined only once.

class InputSensorArray {

  public:
    TimedDigitalInput tdi[TDI_MAX_SENSORS];
    static void printSensorData(TimedDigitalInput *s);
    static void printMonthlyActivity(uint8_t eepromBlock);
      
};

//--------------------------------------------------------
class TimedDigitalOutput {

  // Durations are stored in millis.
  // DateTimes are stored as unix seconds
  
  private:
    
    uint32_t _startMillis = 0;       
    time_t _timeNow; //  unix time
        
    //////////////////////////////////////////////////////////////////
    // Private Functions
    //////////////////////////////////////////////////////////////////
    void printStateChangeInfo(void);
    void setPin(uint8_t state);
    boolean pinValid(uint8_t mypin);
       
  public:

    // Public name of sensor
    // Longer names are truncated to this size
    char sensorName[MAX_SENSOR_NAME];

    // Pin associated with the digital output sensor
    uint8_t sensorPin;

    // Current state can be on or off, regardless of logic
    byte sensorState;
    
    // Positive logic means sensor value equal 1 is on. Negative means 1 is off
    byte sensorLogic;

    // Time that the sensor is currently reporting as ON (in millis)
    uint32_t currentOnDuration;
    
    // Time in unixtime when the pin started reporting state ON
    time_t currentOnStartDateTime;

    // Time interval the sensor will remain ON
    uint32_t intervalMillis = 0;

    //////////////////////////////////////////////////////////////////
    // Public Functions
    //////////////////////////////////////////////////////////////////
    TimedDigitalOutput(void);
    int begin(const char *name, uint8_t a_pin, uint8_t sensor_logic);
    // timer = 0 means permanently On. Other values mean set now to on and go back to off after timer millis
    void setOn(uint32_t timer); 
    void setOff(void); 
    void checkTimer(void);
 
};

//--------------------------------------------------------
// The outputSensorArray class instantiates an array of 
// TimedDigitalOutput classes, tdo[TDO_MAX_SENSORS]
// e.g. s.tdo[0], s.tdo[1] etc.

class OutputSensorArray {

  public:
    TimedDigitalOutput tdo[TDO_MAX_SENSORS];
    static void printSensorData(TimedDigitalOutput *s);
 
};

//--------------------------------------------------------
// General utility functions
//--------------------------------------------------------
void printHumanTime(time_t t);
void print2Digits(int digits);

#endif // TIMED_DIGITAL_IO_H

