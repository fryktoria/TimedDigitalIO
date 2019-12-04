# TimedDigitalIO
A library to monitor the time that devices are active, using the Arduino input pins. Output pins can be set to On and automatically Off after a specified interval.

## Introduction
We can monitor the state of devices using the digital inputs. But is that all a monitoring system can do? Just incorporate the time dimension to a sensor state and we can have tons of additional information regarding the usage of our devices!

There are dozens of devices in the home or in a factory which are operated automatically by electronic, electrical or mechanical controllers, such as thermostats, piezostats etc. For example, a pump operates for a few seconds at a time in order to maintain the pressure in the fresh water plumbing system. I wanted to measure the total amount of time that such devices operate throughout a year, in order to organize their maintenance schedule. In addition, I wanted to know if devices in unmanned locations are set into operation by their local controllers without any particular (meaning _intended_) reason, consuming costly power. Or maybe pumps are operating because there is a leak which nobody can visually identify. Or lamps are operating at night controlled by a light sensor, in spite of the fact that we have left our summer house for the winter, just because we forgot to turn of the mains switch. And for hundreds of other reasons....

This library keeps a log of the time that devices became active for any reason. Hardware that monitors the voltage applied to the device or the current that flows through and indicate that the device is active, is a prerequisite. 

For hobbyist projects, I assume that the user of this library must have adequate knowledge to wire a 0-5V signal to Arduino pins, using pull-down or pull-up resistors depending on the sensor logic (active-high or active-low). The electronic circuit for monitoring mains voltages will be published in another repository, because such a project which involves lethal voltages should be documented accordingly.    

## Digital Inputs
Digital Inputs which are monitored via this library keep a record of the time that were active.


Maintained information contains:

1. A name given to the input sensor
2. The logic of the sensor (active-low or active-high)
3. The current state of the pin
4. The duration since the sensor was last activated and the time of that last activation
5. The start time, end time and duration of the previous activation
6. The active duration throughout the current day 
7. The active duration throughout the current month
8. It maintains in EEPROM the monthly values for an entire 12-month period

## Digital Outputs
The state of Digital Outputs can be set using the `setOn()` and `setOff()` functions.

The `setOn(timer)` function will set the output immediately to On and will set it to Off after timer milliseconds. The library does not use interrupts, therefore the user application must incorporate the `checkTimer()` method in the loop().

Although time recording could be done for digital outputs in the exact same sense as with digital inputs, I do not consider it useful because the devices may be controlled additionally by manual methods, such as local power switches wired in parallel to the control relays. Our aim is to measure _the time that a device was really active_ and not only _the time we have set it active via the controller_.  Such devices should be monitored via a Digital Input instead. The drawback of course is that we need two pins per device.  

## Notes
1. I tried to implement the Arduino guidelines for creating libraries.
2. Code has tons of comments, sometimes redundant. Tried to explain things without having to read from start to end.
3. Code is written with the aim to help the reader (which could be myself, in a few hours from now) to follow exactly what is happening and not to shrink to a smaller tally of lines.
4. Have tried to avoid String (with capital S) objects and to keep RAM usage as short as possible.
5. The examples present most of the library functionality. If additional details must be explained, give me a call. 
6. The semantics of time, duration, interval and their usage in naming variables are a bit relaxed. I hope that the variable naming conveys both the every-day usage and the formal definition, in order to be easily understood.
7. I am a Physicist, Electronics and Telecommunications engineer. I have written millions of lines of code in C, PHP, Javascript, Fortran IV, Pascal, COBOL, Assembly etc. in order to build things but I do not consider myself a "programmer". I consider software to be just _a tool_, along with many others we use in Engineering. So, please be gentle in criticizing my programming style. Suggestions are welcome.
    
