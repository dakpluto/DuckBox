DuckBox
=======

Arduino Mega-based data logger for bird and bat boxes

Arduino Mega-based data logger for bird and bat boxes

Code from Spring 2013 Physical Computing class. Reads temperature and light sensors, 
has logic for direction determination via precedence detection on photosensors. 
Logs data on SD card shield.

Needs the SD card and RTC libraries. Here's where you can get the RTC lib:

https://github.com/jcw/rtclib

And here’s how/where to place it on a Mac:

http://www.arduino.cc/en/Hacking/Libraries

The stuff I said about having to put the libraries in the application folder 
applied to versions prior to 0017, apparently. 
Since 0017 the libraries can be installed in the Documents/Arduino/libraries folder, 
which should be easier.

When using the Mega (at least the old one I have) with the SD card shield, it is necessary
to hack the SD Card library to select SOFTWARE_SPI, since this works around the pinout
difference between the Mega and other Arduinos.
