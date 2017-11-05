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
    Time.h - Arduino official library with parts Copyright (c) Michael Margolis 2009-2014 https://github.com/PaulStoffregen/Time
    Wire.h - Arduino official library TwoWire.h - TWI/I2C library for Arduino & Wiring  Copyright (c) 2006 Nicholas Zambetti.
    DS1307RTC.h  - Copyright (c) Michael Margolis 2009 https://github.com/PaulStoffregen/DS1307RTC

    At the time of this writing, those libraries were licensed as per the GNU Lesser General Public License. 

    Please check for any update on the licensing status of all software for compatibility and compliance to your intended usage. 

  1.0 05 Nov 2017 - Initial release 
 
*/

#include "Arduino.h"
#include "TimedDigitalIO.h"

// Class constructor
TimedDigitalInput::TimedDigitalInput() {
}

/*
  Must be called for each sensor 
    name: The name of the sensor, at it will appear on printouts, e.g. "Heater"
    pinCode: The Arduino pin that is associated with the sensor, e.g. 5
    logic: TDIO_LOGIC_POSITIVE or TDIO_LOGIC_NEGATIVE. Defines if the pin is active HIGH or active LOW
    pullup: true or false. If true, the pin mode is defined as INPUT_PULLUP, otherwise as INPUT
    eepromBlock: Defines the 48byte block of EEPROM whwre the monthly data of the sensor will be recorded
*/
int TimedDigitalInput::begin(const char *name, uint8_t pinCode, uint8_t logic, boolean pullup, uint8_t eepromBlock) {

  // Check pin validity. 
  if (pinValid(pinCode))
     sensorPin = pinCode;
    //setSensorPin(pinCode);
  else
    return -1;
      
  if (pullup)
    pinMode(sensorPin, INPUT_PULLUP);
  else  
    pinMode(sensorPin, INPUT);

  /*
    Set the logic of the sensor.
    TDIO_LOGIC_POSITIVE means 1 is ON and 0 is OFF
    TDIO_LOGIC_NEGATIVE means 0 is ON and 1 is OFF
  */
  if (logic == TDIO_LOGIC_POSITIVE)
    sensorLogic = TDIO_LOGIC_POSITIVE;
  else
    sensorLogic = TDIO_LOGIC_NEGATIVE;
  
  //setSensorLogic(logic);

  /*
    Set the EEPROM block where monthly data are written.
    Each sensor occupies 12 months * 4 bytes = 48 bytes.
    This value defines which 48 byte block will be used to
    read past data and write new data every month.
  */
  if ((eepromBlock >= 0) && (eepromBlock < TDI_MAX_SENSORS))
    EEPROMBlock = eepromBlock;
  else
    return -1;

  /*
    Set the sensor name
  */  
  strncpy(sensorName, name, sizeof(sensorName) -1); 
  sensorName[sizeof(sensorName)-1] = NULL;

  /*
    Set initial values for class variables
  */ 
  // Current state can be on or off, regardless of logic
  sensorState = TDIO_STATE_OFF;
  _previousState = TDIO_STATE_OFF;
  _previousMillis = 0;   

  _timeNow = now();

  // Month where time is counted. Months are 1-12
   currentMonth = month(_timeNow);
  // If something is stored in EEPROM for the current month, 
  // it means that we had a power outage, and we are restarting.
  // Since we have had recordings for that month
  // we will add to that.
  currentMonthOnDuration = readEEPROM(currentMonth);
  
  currentDay = day(_timeNow);  

  // Number of times the sensor came ON today and duration
  todayOnCounter = 0;
  todayOnDuration = 0;
  
  // Time that the sensor reported as on during the previous OFF - ON - OFF sequence
  previousOnDuration = 0;
  // Time when the state toggle to ON and is now OFF   
  previousOnStartDateTime = 0;
  previousOnStopDateTime = 0;
   
    // Time that the sensor is currently reporting as ON
  currentOnDuration = 0;
    
  // Time in unixtime when the pin started reporting state ON and is still ON
  currentOnStartDateTime = 0;

  // Use the function instead of direct assignment to handle the minimum recording time test
  setEEPROMRecordingInterval(EEPROM_DEFAULT_RECORDING_INTERVAL); 
  
  // Last time when we wrote data to EEPROM
  _previousEEPROMWriteMillis = 0;


}

    
//--------------------------------------------------------
void TimedDigitalInput::readSensor(void) {

    _timeNow = now();
    
    // Set the new state based on logic and pin value
    setState(digitalRead(sensorPin));
 
    if (sensorState == TDIO_STATE_ON) {
       
      if (_previousState == TDIO_STATE_OFF) {
        // Sensor is activated. Start counting time
        toggleStateOn();
        
      } else {
        // System was already active, continue timing       
        recordUpToNow();
        
      }

    } else { // state == TDIO_STATE_OFF

      if (_previousState == TDIO_STATE_ON) {
        // System is stopped
        recordUpToNow(); 
        // Stop timing
        toggleStateOff();
        
      } else {
        // System was already stopped
        
      }          
  }

  // Is it time to record current data to EEPROM?
  // We may write twice, immediately after or before the month crossing. I guess we cannot avoid it.
  uint32_t currentMillis = millis();
  if ( currentMillis - _previousEEPROMWriteMillis >= _EEPROMRecordingInterval) {  
    storeEEPROM(currentMonth, currentMonthOnDuration);
    _previousEEPROMWriteMillis = currentMillis;
  }
     
  // Check if we crossed day
  if (day(_timeNow) != currentDay) {
    currentDay = day(_timeNow);
    todayOnDuration = 0;
    #if TDIO_DEBUG
      Serial.print(F("Day change to "));
      Serial.println(currentDay);
    #endif
    
  } 
  
  // Check if we crossed month. 
  if (month(_timeNow) != currentMonth) { 
    
    storeEEPROM(currentMonth, currentMonthOnDuration); 
    // Set new current month as per clock 
    currentMonth = month(_timeNow);
    // Start counting the month data from zero
    currentMonthOnDuration = 0;
     
    // Prepare next month with a zero value, so that we will be ready to add time
    // after the month crossing
    uint8_t nextMonth = currentMonth + 1;
    if (nextMonth > 12)
      nextMonth = 1;
    storeEEPROM(nextMonth, 0);

    #if TDIO_DEBUG
      Serial.print(F("Month change to "));
      Serial.println(currentMonth);
    #endif

  }  
   
}

//--------------------------------------------------------
void TimedDigitalInput::toggleStateOn(void) {

  #if TDIO_DEBUG
    printStateChangeInfo();
  #endif
  _previousMillis = millis();      
  currentOnStartDateTime = _timeNow;
  ++todayOnCounter;
  _previousState = TDIO_STATE_ON;

}

//--------------------------------------------------------
void TimedDigitalInput::toggleStateOff(void) {

  #if TDIO_DEBUG
    printStateChangeInfo();
  #endif
  previousOnDuration = currentOnDuration;
  previousOnStartDateTime = currentOnStartDateTime;
  previousOnStopDateTime = _timeNow;
  currentOnDuration = 0;
  currentOnStartDateTime = 0;
  _previousState = TDIO_STATE_OFF;

}

//--------------------------------------------------------
void TimedDigitalInput::setState(uint8_t value) {
      
  if ((sensorLogic == TDIO_LOGIC_POSITIVE && value == HIGH) || 
       (sensorLogic == TDIO_LOGIC_NEGATIVE && value == LOW))
    sensorState = TDIO_STATE_ON;
  else 
    sensorState = TDIO_STATE_OFF; 
}

//--------------------------------------------------------
void TimedDigitalInput::recordUpToNow(void) {

  uint32_t millisPassed;
  uint32_t currentMillis;
  
  currentMillis = millis(); 
  millisPassed = currentMillis - _previousMillis;
  currentOnDuration += millisPassed;
  todayOnDuration += millisPassed;
  _previousMillis = currentMillis;
  // Register up to now to previous month
  // Depending on the loop period, this may mean that we may have 
  // a monthly on duration larger than the monthly millis              
  currentMonthOnDuration += millisPassed;
  
}

//--------------------------------------------------------
// For each "position" we have 12 values, one uint32_t for each month
void TimedDigitalInput::storeEEPROM(uint8_t month, uint32_t value) {

  #if TDIO_DEBUG
    Serial.print(F("##########  "));
    printHumanTime(now());
    Serial.print(F(" Storing to EEPROM "));
    Serial.print(F(" Sensor '")); Serial.print(sensorName);
    Serial.print(F("' block ")); Serial.print(EEPROMBlock);
    Serial.print(F(" for month ")); Serial.print(month);
    Serial.print(" value "); Serial.print(value);
  #endif

  uint32_t written_value;
  // Save EEPROM life. If value to be written is same as the already existing, do not perform a write operation
  EEPROM.get( EEPROM_OFFSET + (EEPROMBlock * 12 + (month - 1) )* sizeof(uint32_t), written_value); 
  if (written_value != value) {
    EEPROM.put( EEPROM_OFFSET + (EEPROMBlock * 12 + (month - 1)) * sizeof(uint32_t),  value);
    #if TDIO_DEBUG
      Serial.print(F(" #*#*#* Physical write #*#*#*"));
    #endif
  }
  #if TDIO_DEBUG
    Serial.println();
  #endif
}

//--------------------------------------------------------
uint32_t TimedDigitalInput::readEEPROM(uint8_t month) {

  uint32_t written_value;
 
  EEPROM.get( EEPROM_OFFSET + (EEPROMBlock * 12 + (month - 1)) * sizeof(uint32_t), written_value);
  return written_value;
  
}

//--------------------------------------------------------
void TimedDigitalInput::setEEPROMRecordingInterval(uint32_t interval) {

  if (interval < EEPROM_MINIMUM_RECORDING_INTERVAL) 
    interval = EEPROM_MINIMUM_RECORDING_INTERVAL;
  _EEPROMRecordingInterval = interval * 1000;
  
}

//--------------------------------------------------------
void TimedDigitalInput::printStateChangeInfo(void) {

      Serial.print(F("***** "));
      printHumanTime(now());
      Serial.print(F(" Sensor '")); Serial.print(sensorName);
      Serial.print(F("' State of pin ")); Serial.print(sensorPin);
      Serial.print(F(" changed to "));
    if (sensorState == TDIO_STATE_ON)
      Serial.println(F("ON"));
    else
      Serial.println(F("OFF"));  
 
} 

//--------------------------------------------------------
// Check pin validity
boolean TimedDigitalInput::pinValid(uint8_t mypin) {

  // You may designate specific pins to be used as outputs, referencing pins_arduino.h
/*
  if (mypin >= 8 && mypin <= 11)
    return true;
  else
    return false;
*/
  return true;
   
}

/////////////////////////////////////////////////////////////

//--------------------------------------------------------
static void InputSensorArray::printSensorData(TimedDigitalInput *s) {

    Serial.print(F("---------- "));
    Serial.print(s->sensorName);
     Serial.println(F("----------"));
    Serial.print(F("Pin: "));
    Serial.println(s->sensorPin);
    
    Serial.print(F("Logic: "));
    if (s->sensorLogic == TDIO_LOGIC_POSITIVE)
      Serial.print(F("POSITIVE"));
    else
      Serial.print(F("NEGATIVE"));
    Serial.println();
      
    Serial.print(F("State: "));
    if (s->sensorState == TDIO_STATE_ON)
      Serial.print(F("ON"));
    else
      Serial.print(F("OFF"));
    Serial.println();
      
    Serial.print(F("Currently ON duration: "));
    Serial.print(s->currentOnDuration);
    if (s->currentOnStartDateTime != 0) {
      Serial.print(F(" started at "));
      printHumanTime(s->currentOnStartDateTime);
    }
    Serial.println();  

    if (s->previousOnDuration != 0) {
      Serial.print(F("Previous ON duration: "));
      Serial.print(s->previousOnDuration); 
      Serial.print(F(" started at "));
      printHumanTime(s->previousOnStartDateTime);
      Serial.print(F(" ended at "));
      printHumanTime(s->previousOnStopDateTime); 
      Serial.println();   
    } 

    Serial.print(F("Today ON counter: "));
    Serial.println(s->todayOnCounter);

    Serial.print(F("Today ON duration: "));
    Serial.println(s->todayOnDuration);    

    Serial.print(F("Current month ON duration: "));
    Serial.println(s->currentMonthOnDuration);
    Serial.println(F("------------------------------"));
    Serial.println();
  
}

//--------------------------------------------------------
static void InputSensorArray::printMonthlyActivity(uint8_t eepromBlock) {

    uint32_t monthly_value;

     Serial.print(F("------ EEPROM BLOCK "));
     Serial.print(eepromBlock);
     Serial.println(F("-------------------"));    
    
    for (int i = 1; i <= 12; i++) {
  
      EEPROM.get( EEPROM_OFFSET + (eepromBlock * 12 + (i - 1)) * sizeof(uint32_t), monthly_value);

      Serial.print (F("    Month "));
      Serial.print(i);
      Serial.print(F(": "));
      Serial.print(monthly_value);
      Serial.println();
    }
    Serial.println();
    
}


////////////  Digital Output ////////////////////////

// Class constructor
TimedDigitalOutput::TimedDigitalOutput() {
}

/*
  Must be called for each sensor 
    name: The name of the sensor, at it will appear on printouts, e.g. "Heater"
    pinCode: The Arduino pin that is associated with the sensor, e.g. 5
    logic: TDIO_LOGIC_POSITIVE or TDIO_LOGIC_NEGATIVE. Defines if the pin is active HIGH or active LOW
    pullup: true or false. If true, the pin mode is defined as INPUT_PULLUP, otherwise as INPUT
    eepromBlock: Defines the 48byte block of EEPROM whwre the monthly data of the sensor will be recorded
*/
int TimedDigitalOutput::begin(const char *name, uint8_t pinCode, uint8_t logic) {

  // Check pin validity. 
  if (pinValid(pinCode))
    sensorPin = pinCode;
  else
    return -1;

  /*
    Set the logic of the sensor.
    TDIO_LOGIC_POSITIVE means 1 is ON and 0 is OFF
    TDIO_LOGIC_NEGATIVE means 0 is ON and 1 is OFF
 */ 
  if (logic == TDIO_LOGIC_POSITIVE)
    sensorLogic = TDIO_LOGIC_POSITIVE;
  else
    sensorLogic = TDIO_LOGIC_NEGATIVE;

  /*
    Set the sensor name
*/  
  strncpy(sensorName, name, sizeof(sensorName) -1); 
  sensorName[sizeof(sensorName)-1] = NULL;

  /*
    Set initial values for class variables
  */ 
  // Current state can be on or off, regardless of logic
  sensorState = TDIO_STATE_OFF;
  intervalMillis = 0;   
   
    // Time that the sensor is currently reporting as ON in millis
  currentOnDuration = 0;
    
  // Time in unixtime when the pin started reporting state ON and is still ON
  currentOnStartDateTime = 0;

}


//--------------------------------------------------------
// If timer is set to a non zero value, the system will turn on 
// and then off after timer millis
void TimedDigitalOutput::setOn(uint32_t timer) {

  _timeNow = now();

  setPin(TDIO_STATE_ON);
 
  _startMillis = millis();      
  currentOnStartDateTime = _timeNow;
  sensorState = TDIO_STATE_ON; 
  intervalMillis = timer;
   
  #if TDIO_DEBUG
    printStateChangeInfo();
  #endif
  
}


//--------------------------------------------------------
void TimedDigitalOutput::setOff(void) {

  setPin(TDIO_STATE_OFF);

  currentOnDuration = 0;
  currentOnStartDateTime = 0;
  sensorState = TDIO_STATE_OFF;
  #if TDIO_DEBUG
    printStateChangeInfo();
  #endif  
    
}

//--------------------------------------------------------
void TimedDigitalOutput::setPin(uint8_t state) {
      
  if ((sensorLogic == TDIO_LOGIC_POSITIVE && state == TDIO_STATE_ON)  || 
       (sensorLogic == TDIO_LOGIC_NEGATIVE && state == TDIO_STATE_OFF)) {
     digitalWrite(sensorPin, HIGH);
  
  } else { 
    digitalWrite(sensorPin, LOW);
    
  }   
    
}

//--------------------------------------------------------
void TimedDigitalOutput::checkTimer(void) {

  uint32_t millisPassed;

  if (intervalMillis > 0) {
    millisPassed = millis() - _startMillis;
    currentOnDuration = millisPassed;
    if (millisPassed >= intervalMillis) {
      setOff();
      intervalMillis = 0;
    }  
  }    
  
}


//--------------------------------------------------------
void TimedDigitalOutput::printStateChangeInfo(void) {

  Serial.print(F("***** "));
  printHumanTime(now());
  Serial.print(F(" Sensor '")); Serial.print(sensorName);
  Serial.print(F("' State of pin ")); Serial.print(sensorPin);
  Serial.print(F(" changed to "));
  if (sensorState == TDIO_STATE_ON)
    Serial.println(F("ON"));
  else
    Serial.println(F("OFF"));  
 
} 

//--------------------------------------------------------
// Check pin validity
boolean TimedDigitalOutput::pinValid(uint8_t mypin) {

  // You may designate specific pins to be used as outputs, referencing pins_arduino.h
/*
  if (mypin >= 4 && mypin <= 8) 
    return true;
  else
    return false;
*/
  return true;   
}

//--------------------------------------------------------
static void OutputSensorArray::printSensorData(TimedDigitalOutput *s) {

    Serial.print(F("---------- "));
    Serial.print(s->sensorName);
     Serial.println(F("----------"));
    Serial.print(F("Pin: "));
    Serial.println(s->sensorPin);
    
    Serial.print(F("Logic: "));
    if (s->sensorLogic == TDIO_LOGIC_POSITIVE)
      Serial.print(F("POSITIVE"));
    else
      Serial.print(F("NEGATIVE"));
    Serial.println();
      
    Serial.print(F("State: "));
    if (s->sensorState == TDIO_STATE_ON)
      Serial.print(F("ON"));
    else
      Serial.print(F("OFF"));
    Serial.println();

    if (s->sensorState == TDIO_STATE_ON && s->intervalMillis > 0) {  
      Serial.print(F("Currently ON duration: "));
      Serial.print(s->currentOnDuration/1000);
      if (s->currentOnStartDateTime != 0) {
        Serial.print(F(" sec started at "));
        printHumanTime(s->currentOnStartDateTime);
      }
      Serial.println();
     
      Serial.print(F("Remaining ON time: "));
      Serial.print((s->intervalMillis - s->currentOnDuration)/1000);
      Serial.print(F(" sec"));
      Serial.println();  
      
    }

    Serial.println(F("------------------------------"));
    Serial.println();
  
}

/////////////////////////////////////////////////////////////
// Some useful functions outside classes

//--------------------------------------------------------
// Prints a time from unix seconds to a human readable format
void printHumanTime(time_t t) {

  Serial.print(year(t)); 
  Serial.print("-"); 
  print2Digits(month(t));
  Serial.print("-");
  print2Digits(day(t)); 

  Serial.print(" ");
  print2Digits(hour(t));
  Serial.print(":");
  print2Digits(minute(t));
  Serial.print(":");
  print2Digits(second(t));
}

//--------------------------------------------------------
// Prints an integer with a leading zero. if necessary
void print2Digits(int digits) {
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

