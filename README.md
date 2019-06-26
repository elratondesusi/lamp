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
4. load src/arduino/lamp/lamp.ino to Arduino IDE and upload sketch to lamp's arduino
5. test your schedule

Requirements:

* Real Time Clock Arduino library RTClib, https://github.com/adafruit/RTClib
* LiquidCrystal_I2C Arduino library, https://github.com/johnrickman/LiquidCrystal_I2C
* JRE and JDK need to be installed on the computer, please edit the first line of compile-java.bat to contain the path to the javac.exe

Example schedule is in schedules/modesQuickPresentation.txt

Set your Alarm 4 minutes after you turn the lamp on (or depending on current time of day).

You should see the following scenario:

* depending on the time of the day, the lamp will start in one of the main modes (circadian, afternoon, evening, night, sleep)
* it advances to the next mode (in the order mentioned above) in 30 seconds, except of the night -> sleep, which takes 1 minute, always changing colours:
  * white in circadian,
  * yellow in the afternoon,
  * white in the evening,
  * yellow -> red, and then red -> black in the night,
  * black in the sleep mode.
* in the sleep mode:
  * turnining the dial slightly ("touch") opens red light until touched again, or 15 seconds pass
  * turning the dial more ("turn") starts the wakeup procedure
  * EARLYALARM minutes before the alarm (if alarm is ON) starts the wakeup procedure
* wakeup procedure:
  * always change from dark red to bright red the first 30 seconds
  * always change from red to yellow the next 30 seconds
  * in case the wakeup procedure was initiated by "turn" (not by alarm), change from yellow to white in the next 15 seconds and advance to the circadian mode
  * otherwise, start alarm music while yellow, and allow postponing alarm by "touch", or closing alarm by "turn" action
  * postponing the alarm three times, or no action in one minute (3 x 20 seconds) finally close the alarm 
  * closing the alarm means change yellow to white in 15 seconds, and then advancing to circadian mode
