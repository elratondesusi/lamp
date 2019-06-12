# lamp
Intelligent Lamp Project

* src/java/       sources of compiler of lamp schedules to arduino sources
* src/arduino/    arduino source code for lamp
* schedules/      example lamp schedules
* docs/           detailed documentation

system was developed in Windows environment, for Linux, change "\\" in paths to "/"

Check: http://dai.fmph.uniba.sk/courses/dtv/index.php/ILampa

Usage:

1. edit your lamp schedule and place it into schedules/ folder
2. run compile-java  (make sure javac and java are in system path)
3. run translate-schedule 
4. load src/arduino/lamp/lamp.ino to arduino and upload sketch to lamp's arduino
5. test your schedule

Requirements:

* Real Time Clock Arduino library RTClib, https://github.com/adafruit/RTClib
* LiquidCrystal_I2C Arduino library, https://github.com/johnrickman/LiquidCrystal_I2C
* JRE and JDK need to be installed on the computer, please edit the first line of compile-java.bat to contain the path to the javac.exe

Example schedule is in schedules/modesQuickPresentation

Set your Alarm 4 minutes after you turn the lamp on.

You should see the following scenario:


