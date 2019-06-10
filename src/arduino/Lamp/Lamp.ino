#include <EEPROM.h>
#include <Wire.h>
#include "RTClib.h"
#include <LiquidCrystal_I2C.h>
#define MP3_OUTPUT_PIN 4   // connect to Rx pin of DFPlayer
#define MP3_BUSY_PIN 8     // connect to BUSY pin of DFPlayer
#define RED 9
#define GREEN 10 
#define BLUE 11
#define SOUND_GND 16
#define LOOK_PERIOD 500
#define ANALOG_NOISE 12

#define MAX_MENU_DEPTH 3
#define MENU_IDLE_LIMIT 30
#define TURN_ANALOG 220
#define TOUCH_ANALOG 80
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS1307 RTC;

extern uint8_t initialTimesCnt;
extern uint8_t initialTimes [][4];
extern uint8_t  color1[][3];
extern uint8_t color2[][3];
extern int8_t atMode[][4];
extern int8_t  afterMode[][4];
extern int8_t alarm [];
extern int8_t turn [];
extern int8_t touch [];
extern int8_t loop3 [];
extern uint8_t earlyAlarm;

const int okButton = 12;
int okButtonState = 1;
int prevOkButtonState = 1;

int currentMode = 0;
long currentModeStarted;
uint8_t modeChangesColor;
long timeFinishColorTransition;

uint8_t currentR, currentG, currentB;

//state of menu
int currentMenu = 0;
int menuStack[MAX_MENU_DEPTH];
int menuSP = 0;
int menuItem = 0;

uint8_t checkState = 0;
long nextTimeCheck;
int prevAnalogState = 0;

//alarm
bool alarmOn = false;
uint8_t hourAlarm;
uint8_t minuteAlarm;
uint8_t hourAlarm2 = -1;
uint8_t minuteAlarm2 = -1;
bool todayAlarm = false;
int volume = 30;
int timesAlarmPosponed = -1;

uint8_t hourTime;
uint8_t minuteTime;
uint8_t secondTime;
int recursiveChangeMode= 0;

//music - current song
uint8_t song_number = 1;

#define ROOT_MENU 999
#define BACK_MENU -999

// configuration od menu
const char* menu [] = {
  "Intensity",                       //0 -1
  "Color",                           //1     1
  "Mode",                            //2     2
  "Alarm & Time",                    //3     3

  "White light",                     //4 -3
  "White light withblue filter",     //5 -4
  "Red Green Blue",                  //6 -5
  "Blue",                            //7 -10
  "Green",                           //8 -11
  "Red",                             //9 -12
  "Pink",                            //10 -13
  "Violet",                          //11 -14

  "Circadian  mode",                 //12 -6               //7
  "Focusing mode",                   //13 -7
  "Reading mode",                    //14 -8
  "Relaxing mode",                   //15 -9

  "Set Alarm Time",                  //16 -15
  "ON / OFF",                        //17 -16
  "Show Alarm Time",                 //18 -17
  "Set Time",                        //19 -18
  "Set Music",                        //20 -19
  "Set Volume" ,                       //21 -20
  "Play Song"
};
int menuStartItem[] = {
  0, 4, 12, 16
};
int  menuLen[] = {
  4, 8, 4, 7
};
int menuCmd [] = {
  -1, 1, 2, 3, -3, -4, -5, -10, -11, -12, -13, -14, -6, -7, -8, -9, -15, -16, -17, -18, -19, -20, -21
};

void setup() {
  //eeprom 
  alarmOn = EEPROM.read(0);    //0 - 1023          0 - vypnuty, 1 - zapnuty    = adresa 0 ...adresa 1 = hodina alarmu ad2- minuta alarmu
  hourAlarm = EEPROM.read(1);
  minuteAlarm =  EEPROM.read(2);
  
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(okButton, INPUT);
  pinMode(SOUND_GND, OUTPUT);
  digitalWrite(SOUND_GND, 0);
  setupLCD();
  setupRTC();
  setupButton();
  currentModeStarted = RTC.now().secondstime();
  changeMode(getInitMode());
  mainMenu();
}

uint8_t getInitMode(){
  DateTime now = RTC.now();
  int index = 0;
  while(true){
    if(now.hour() < initialTimes[index][0])
      return initialTimes[index][3];
    if(now.hour() == initialTimes[index][0]){
      if(now.minute() < initialTimes[index][1])
        return initialTimes[index][3];
         
      if(now.minute() == initialTimes[index][1])
        if(now.second() <= initialTimes[index][2])
          return initialTimes[index][3];
      }
      index++;
      if(index == initialTimesCnt)
        return initialTimes[0][3];
  }
}
void setRGB(uint8_t r, uint8_t g, uint8_t b) {
  analogWrite(RED, r);
  analogWrite(GREEN, g);
  analogWrite(BLUE, b);
  currentR = r;
  currentG = g;
  currentB = b;
}
void doMenuCmd(int cmd) {
  switch (cmd) {
    case -1: setIntensity(); break;
   // case -2: setAlarm(); break;

    case -3: setColor(cmd); break;
    case -4: setColor(cmd); break;
    case -5: setColor(cmd); break;

    case -6: setMode(cmd); break;
    case -7: setMode(cmd); break;
    case -8: setMode(cmd); break;
    case -9: setMode(cmd); break;

    case -10: setColor(cmd); break;
    case -11: setColor(cmd); break;
    case -12: setColor(cmd); break;
    case -13: setColor(cmd); break;
    case -14: setColor(cmd); break;

    case -15: setAlarmTime(); break;
    case -16: setAlarmOnOff(); break;
    case -17: showAlarmTime(); break;
    case -18: setClockTime(); break;
    case -19: setMusic(); break;
    case -20: setVolume(); break;
    case -21: playSong(); break;
  }
}
void setMusic(){
  uint8_t num;
    lcd.clear();
  while (!readButton()) {
    num = map(analogRead(A0), 0, 1023, 0, 5);
    lcd.setCursor(0,0);
    lcd.print(num);
    lcd.print("  ");
    delay(200);
  }
  song_number = num;
//analgove zobrazenie...
  //
  lcd.clear();
  lcd.print("Song number is: ");
  lcd.setCursor(1,1);
  lcd.print(song_number);
  delay(4000);
}
void setVolume(){
  while(!readButton()){
    volume = analogRead(A0) * 0.06;
    lcd.setCursor(0,0);
    lcd.print("Volume: ");
    lcd.print(volume);
    lcd.print("  ");
    delay(50);
  }
}
void playSong(){
  playMusic();
}
void setIntensity() {
    // currentR, currentG, currentB;
//  uint32_t q = 1;
//  while (!readButton()) {
//    uint8_t r = map(analogRead(A0), 0, 1023, 0, 255);
//   // if(r > analogRead(RED)){
//      analogWrite(RED, );
//      analogWrite(GREEN, r);
//      analogWrite(BLUE, r);
//    //}
//  }
}

void selectRGB() {
  lcd.clear();
  lcd.setCursor(1, 0);
  lcd.print("***Red***");
  delay(1000);
  uint8_t r;
  analogWrite(RED, 0);
  analogWrite(GREEN, 0);
  analogWrite(BLUE, 0);
  while (!readButton()) {
    r = map(analogRead(A0), 0, 1023, 0, 255);
    analogWrite(RED, r);
  }
  analogWrite(RED, 0);
  lcd.clear();
  lcd.print("***Green***");
  delay(1000);
  uint8_t g;
  while (!readButton()) {
    g = map(analogRead(A0), 0, 1023, 0, 255);
    analogWrite(GREEN, g);
  }
  analogWrite(GREEN, 0);
  lcd.clear();
  lcd.print("***Blue***");
  delay(1000);
  uint8_t b;
  while (!readButton()) {
    b = map(analogRead(A0), 0, 1023, 0, 255);
    analogWrite(BLUE, b);
  }
  analogWrite(RED, r);
  analogWrite(GREEN, g);
}
void setColor(int colorNum) {
  //analofWrite pre kazdy z troch pinov
  switch (colorNum) {
    case -3: setRGB(255, 255, 255); break;
    case -4: setRGB(255, 255, 0x95); break;
    case -5: selectRGB(); break;

    case -10: setRGB(0, 0, 255); break;
    case -11: setRGB(0, 255, 0); break;
    case -12: setRGB(255, 0, 0); break;
    case -13: setRGB(255, 180, 180); break;
    case -14: setRGB(255, 0, 255); break;
  }
}

void setMode(int modeNum) {
  int newMode = 0;
  switch(modeNum){
    case -6: newMode = 0; break;    //circadian
    case -7: newMode = 0; break;    //focusing
    case -8: newMode = 11; break;   //reading
    case -9: newMode = 12; break;    //realxing
  }
  changeMode(newMode);
}

void alarmControler(){
  if(alarmOn){
    DateTime nowTime = RTC.now();
    if((nowTime.hour() == hourAlarm && nowTime.minute() == minuteAlarm && nowTime.second() == 0) ||
    (nowTime.hour() == hourAlarm2 && nowTime.minute() == minuteAlarm2 && nowTime.second() == 0)){
      if(!todayAlarm){
        if (alarm[currentMode] != -1) {  
          changeMode(alarm[currentMode]);
        }
        //alarmSTART  
        //alarmOFF
        if(timesAlarmPosponed < 3) {
          minuteAlarm2 = nowTime.minute();
          hourAlarm2 = nowTime.hour();
          plusXmin(&hourAlarm2, &minuteAlarm2, earlyAlarm / 2);
          timesAlarmPosponed++;
        } else {
          minuteAlarm2 = -1;
          hourAlarm2 = -1;
          timesAlarmPosponed = 0;
        }        
        
        todayAlarm = true;
        lcd.clear();
        lcd.setCursor(1,0);
        lcd.print("Good Morning!");
        delay(2000);
        playMusic();     
      }
   } else {
    
     todayAlarm = false;
   }
  }
}

void showAlarmTime(){
   lcd.setCursor(4,1);
   lcd.print(hourAlarm);
   lcd.print(":");
   lcd.print(minuteAlarm);
   while(!readButton());
   delay(50);
   while(readButton());
   delay(50);   
}
void setClockTime(){
   lcd.clear();
   uint8_t day = inputNum(2,0, 31, 1);
   lcd.print(".");
   uint8_t month = inputNum(5,0, 12, 1);
   lcd.print(".");
   uint16_t year = 2000 + inputNum(8,0, 99, 0);
   setTime();
   RTC.adjust(DateTime(year, month, day, hourTime, minuteTime, 0));
}

void setTime(){
  hourTime = inputNum(4,1, 23, 0);
  lcd.print(":");
  minuteTime = inputNum(7,1, 59, 0);
}
void setAlarmTime() {
    setTime();
    hourAlarm = hourTime;
    minuteAlarm = minuteTime;
    EEPROM.write(0, 1);
    EEPROM.write(1, hourAlarm);
    EEPROM.write(2, minuteAlarm);
//    EEPROM.write(3, secondAlarm);
  //  RTC.equals(DateTime(__DATE__, __TIME__));

  // RTC.adjust(DateTime(__DATE__, __TIME__));
  
}

int inputNum(int x, int y, int max, int min){
  int option;
  while(!readButton()){
    option = map(analogRead(A0), 0, 1023, min, max);
    lcd.setCursor(x,y);
    lcd.print(option / 10);
    lcd.print(option % 10);
  }
  while(readButton());     //kym ho nepusti
  return option;
}
void setAlarmOnOff(){
  alarmOn = !alarmOn;
  EEPROM.write(0, alarmOn);
}

bool readButton() {
  okButtonState = digitalRead(okButton);
  if (prevOkButtonState != okButtonState)
  {
    prevOkButtonState = okButtonState;
    delay(50);
    if (okButtonState == LOW)
      return true;
  }
  return false;
}
int checkAction() {
  if (readButton())  {
    while (readButton())
      delay(50);
    return 0;
  }
  if(checkState == 0){
     int wheel = analogRead(A0);
      if(abs(wheel - prevAnalogState) > ANALOG_NOISE){
        nextTimeCheck = millis() + LOOK_PERIOD;
        checkState = 1;
        prevAnalogState = wheel;
      }
  } else {
     long tm = millis();
     if(tm > nextTimeCheck){
      int wheel = analogRead(A0);
        if (abs(prevAnalogState - wheel) > TURN_ANALOG) {
            tone(13, 220, 20);        //buzzer
            checkState = 0;
            prevAnalogState = wheel;
            return 4;
        }
          if ((prevAnalogState + TOUCH_ANALOG) <  wheel) {
            tone(13, 880, 20);        //buzzer
            checkState = 0;
            prevAnalogState = wheel;
            return 1;
         }
         if ((prevAnalogState - TOUCH_ANALOG) >  wheel) {
            tone(13, 440, 20);        //buzzer
            checkState = 0;
            prevAnalogState = wheel;
            return 2;
         }
     }
  }  
  return 3;
}
/*
 int checkAction() {
  if (readButton())  {
    while (readButton())
      delay(50);
    return 0;
  }
  if(checkState == 0){
     int wheel = analogRead(A0);
      if(abs(wheel - prevAnalogState) > ANALOG_NOISE){
        nextTimeCheck = millis() + LOOK_PERIOD;
        checkState = 1;
        prevAnalogState = wheel;
      }
  } else {
     long tm = millis();
     if(tm > nextTimeCheck){
      int wheel = analogRead(A0);
        if (abs(prevAnalogState - wheel) > TURN_ANALOG) {
            tone(13, 220, 20);        //buzzer
            checkState = 0;
            prevAnalogState = wheel;
            return 4;
        }
          if ((prevAnalogState + TOUCH_ANALOG) <  wheel) {
            tone(13, 880, 20);        //buzzer
            checkState = 0;
            prevAnalogState = wheel;
            return 1;
         }
         if ((prevAnalogState - TOUCH_ANALOG) >  wheel) {
            tone(13, 440, 20);        //buzzer
            checkState = 0;
            prevAnalogState = wheel;
            return 2;
         }
     }
  }  
  return 3;
} 
 * */

//void printMenu(int num) {
//  lcd.clear();
//  lcd.print(menu[num]);
//}
void printCurrentMenuItem() {
  lcd.clear();
  if (menuItem == menuLen[currentMenu]) {
    lcd.print("Back");
  } else {
    if (menuStartItem[currentMenu] + menuItem == 5) {
      for (int i = 0; i < 16; i++) {
        lcd.setCursor(i, 0);
        lcd.print(menu[menuStartItem[currentMenu] + menuItem][i]);
      }
      int j = 0;
      for (int i = 16; i < strlen(menu[menuStartItem[currentMenu] + menuItem]); i++) {
        lcd.setCursor(j, 1);
        lcd.print(menu[menuStartItem[currentMenu] + menuItem][i]);
        j++;
      }
      //    lcd.print(menu[menuStartItem[currentMenu] + menuItem].);
      lcd.setCursor(1, 0);
    } else {
      lcd.print(menu[menuStartItem[currentMenu] + menuItem]);
    }
  }
  printAlarmIndicator();
  printCurrentModeNumber();
}
bool enterToChoosenMenuOption(int num) {
  if (num == 4)
    return false;
  lcd.clear();
  delay(50);
  lcd.print("enter: ");

  return true;
}

int enterMenuItem() {
  Serial.print("menuitem:");
  Serial.print(menuItem);
  Serial.print(" menu:");
  Serial.print(currentMenu);
  Serial.print(" len:");
  Serial.println(menuLen[currentMenu]);

  if (menuItem == menuLen[currentMenu]) {
    Serial.print("menusp:");
    Serial.println(menuSP);
    if (menuSP > 0) {
      menuItem = 0;
      currentMenu = menuStack[--menuSP];
    }
  } else {
    int n = menuCmd[menuStartItem[currentMenu] + menuItem];
    if (n < 0) {
      Serial.print("doMenuCmd: ");
      Serial.println(n);
      doMenuCmd(n);
    } else {
      menuStack[menuSP++] = currentMenu;
      currentMenu = n;
      menuItem = 0;
      Serial.print("menusp:");
      Serial.println(menuSP);
    }
  }
  printCurrentMenuItem();
}
int nextMenuItem() {
  int optionalBack = currentMenu > 0;
  menuItem++;
  menuItem %= menuLen[currentMenu] + optionalBack;
  printCurrentMenuItem();
}
int prevMenuItem() {
  int optionalBack = currentMenu > 0;
  menuItem--;
  if (menuItem < 0)
    menuItem = menuLen[currentMenu] + optionalBack - 1;
  printCurrentMenuItem();
}
void changeMode(int newMode) {
  if(currentMode == newMode && loop3[currentMode] != -1){
    recursiveChangeMode++;
    if(recursiveChangeMode == 4)
    newMode = loop3[currentMode];
  } else {
    recursiveChangeMode = 0;
  }
  Serial.print("mode:");
  Serial.print(currentMode);
  Serial.print("->");
  Serial.println(newMode);
  currentMode = newMode;
  DateTime nowTime = RTC.now();
  currentModeStarted = nowTime.secondstime();
  setRGB(color1[currentMode][0], color1[currentMode][1], color1[currentMode][2]);
  if (color1[currentMode][0] == color2[currentMode][0] && color1[currentMode][1] == color2[currentMode][1] && color1[currentMode][2] == color2[currentMode][2]) {
    modeChangesColor = 0;
  } else {
    modeChangesColor = 1;
    if (afterMode[currentMode][3] != -1)
      timeFinishColorTransition = afterMode[currentMode][0] * 3600 + afterMode[currentMode][1] * 60 + afterMode[currentMode][2];
  }
}
bool isBefore(DateTime *time1, DateTime *time2) {
  if (time1->hour() < time2->hour())
    return true;
  if (time1->hour() > time2->hour())
    return false;
  if (time1->minute() < time2->minute())
    return true;
  if (time1->minute() > time2->minute())
    return false;
  if (time1->second() < time2->second())
    return true;
  return false;
}

void trans(){
  DateTime nowTime = RTC.now();

  long fromStartMode = nowTime.secondstime() - currentModeStarted;
    Serial.print(fromStartMode);
    Serial.print(" - ");
    Serial.println(timeFinishColorTransition);
  delay(200);
    int r = map(fromStartMode, 0, timeFinishColorTransition, color1[currentMode][0], color2[currentMode][0]);
    int g = map(fromStartMode, 0, timeFinishColorTransition, color1[currentMode][1], color2[currentMode][1]);
    int b = map(fromStartMode, 0, timeFinishColorTransition, color1[currentMode][2], color2[currentMode][2]);
    setRGB(r, g, b);
}
void idle() {
  //Serial.println("idle");
  long lastTimeShown = RTC.now().secondstime();
  while (true) {
    DateTime nowTime = RTC.now();
    long nowSec = nowTime.secondstime();
    if (afterMode[currentMode][3] != -1) {
      if (nowSec - currentModeStarted >= afterMode[currentMode][0] * 3600 + afterMode[currentMode][1] * 60 + afterMode[currentMode][2])
        changeMode(afterMode[currentMode][3]);
    }
    if (atMode[currentMode][3] != -1) {
      if (nowTime.hour() == atMode[currentMode][0] && nowTime.minute() == atMode[currentMode][1] &&  nowTime.second() >= atMode[currentMode][2])
        changeMode(atMode[currentMode][3]);
    }
    if (alarm[currentMode] != -1) {
       int m = nowTime.minute();
       int h = nowTime.hour();
       plusXmin(&h,&m, earlyAlarm);
       if(h == hourAlarm && m == minuteAlarm && nowTime.second() == 0){
        changeMode(alarm[currentMode]);
       }
    }
    if(modeChangesColor)
      trans();
    int action = checkAction();
    switch(action){
      case 1: 
      case 2: if(digitalRead(MP3_BUSY_PIN) == 0){ 
          mp3_set_volume(0); 
          minuteAlarm2 = nowTime.minute();
          hourAlarm2 = nowTime.hour();
          plusXmin(&hourAlarm2, &minuteAlarm2, earlyAlarm / 2);
        }
        if(touch[currentMode] != -1){
           changeMode(touch[currentMode]);
        }
        break;
      case 4: if(digitalRead(MP3_BUSY_PIN) == 0) mp3_set_volume(0);
        hourAlarm2 = -1;
        minuteAlarm2 = -1;
        timesAlarmPosponed = 0;
        if(turn[currentMode] != -1 ){
          changeMode(turn[currentMode]); 
        }
        break; 
      case 0: if(digitalRead(MP3_BUSY_PIN) == 0) mp3_set_volume(0); while (readButton()) delay(50);  return;
    
    if (nowSec - lastTimeShown >= 1) {
      displayTime();
      lastTimeShown = RTC.now().secondstime();
    }
    alarmControler();
  }
  Serial.println("back to menu");
}

void plusXmin(int *h, int *m, int x) {
  int dayMin = *h * 60 + *m + x;
  *h = (dayMin / 60) % 24;
  *m = dayMin % 60;    
}

void mainMenu() {
  prevAnalogState = analogRead(A0);
  printCurrentMenuItem();
  long menuStarted = RTC.now().secondstime();
  while (true) {
    long nowSec = RTC.now().secondstime();
    if (nowSec - menuStarted > MENU_IDLE_LIMIT)
    {
      idle();
      printCurrentMenuItem();
      menuStarted = RTC.now().secondstime();
    }
    uint8_t action = checkAction();
    if(action != 3)
    Serial.println(action);
    if (action != 3) menuStarted = RTC.now().secondstime();

    switch (action) {
      case 0: enterMenuItem(); break;
      case 1: nextMenuItem(); break;
      case 2: prevMenuItem(); break;
    }
  }
}
void displayTime() {
  DateTime now = RTC.now();
  lcd.clear();
  lcd.setCursor(3, 0);
  lcd.print(now.day(), DEC);
  lcd.print(".");
  lcd.print(now.month(), DEC);
  lcd.print(".");
  lcd.print(now.year(), DEC);
  lcd.setCursor(4, 1);
  lcd.print(now.hour(), DEC);
  lcd.print(':');
  lcd.print(now.minute()/10, DEC);
  lcd.print(now.minute()%10, DEC);
  lcd.print(':');
  lcd.print(now.second()/10, DEC);
  lcd.print(now.second()%10, DEC);

  printAlarmIndicator();
  printCurrentModeNumber();
}

void printAlarmIndicator(){
   if(alarmOn){
    lcd.setCursor(0,1);
    lcd.print("*");
  }
}
void printCurrentModeNumber(){
    lcd.setCursor(14,1);
    lcd.print(currentMode);
    lcd.print(" ");
}
void setupButton() {
  pinMode(okButton, INPUT);
  digitalWrite(okButton, HIGH);
}

void playMusic() {
   mp3_set_volume(volume);
  delay(10);
  mp3_play();
  //while (digitalRead(MP3_BUSY_PIN) == 0) {
   // Serial.println(digitalRead(MP3_BUSY_PIN));    // shows 0 while playing, 1 when idle
    //delay(200);
  //}
}

void setupRTC() {
  Wire.begin();
  RTC.begin();
  // Check to see if the RTC is keeping time.  If it is, load the time from your computer.
  if (! RTC.isrunning()) {
    Serial.println("RTC is NOT running!");
    // This will reflect the time that your sketch was compiled
    RTC.adjust(DateTime(__DATE__, __TIME__));
  }
 // RTC.adjust(DateTime(__DATE__, __TIME__));
}

void setupLCD() {
  lcd.init();                      // initialize the lcd
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("Welcome!");
  lcd.setCursor(0, 1);
  lcd.print("Have a nice day!");
  delay(1000); //5000
  lcd.clear();
}

//START: https://github.com/Robotics-DAI-FMFI-UK/cu-lIllI/blob/master/src/arduino/test-dfplayer/test-dfplayer.ino

// volume 0-30
void mp3_set_volume(uint8_t volume)
{
  mp3_send_packet(0x06, volume);
}

void mp3_play()
{
  mp3_send_packet(0x03, song_number);
}

void mp3_reset()
{
  mp3_send_packet(0x0C, 0);
}
void mp3_send_byte(uint8_t pin, uint8_t val)
{
  pinMode(MP3_OUTPUT_PIN, OUTPUT);
  float start_transmission = micros();
  float one_bit = 1000000 / 9600.0;
  float next_change = start_transmission + one_bit;
  digitalWrite(pin, LOW);
  while (micros() < next_change);

  for (int i = 2; i < 10; i++)
  {
    if (val & 1) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
    next_change = start_transmission + one_bit * i;
    val >>= 1;
    while (micros() < next_change);
  }

  digitalWrite(pin, HIGH);
  next_change = micros() + 2 * one_bit;
  while (micros() < next_change);
  pinMode(MP3_OUTPUT_PIN, INPUT);
}

void mp3_send_packet(uint8_t cmd, uint16_t param)
{
  mp3_send_byte(MP3_OUTPUT_PIN, 0x7E);
  mp3_send_byte(MP3_OUTPUT_PIN, 0xFF);
  mp3_send_byte(MP3_OUTPUT_PIN, 0x06);
  mp3_send_byte(MP3_OUTPUT_PIN, cmd);
  mp3_send_byte(MP3_OUTPUT_PIN, 0x00);
  mp3_send_byte(MP3_OUTPUT_PIN, (uint8_t)(param >> 8));
  mp3_send_byte(MP3_OUTPUT_PIN, (uint8_t)(param & 0xFF));
  uint16_t chksm = 0xFF + 0x06 + cmd + (param >> 8) + (param & 0xFF);
  chksm = -chksm;
  mp3_send_byte(MP3_OUTPUT_PIN, (uint8_t)(chksm >> 8));
  mp3_send_byte(MP3_OUTPUT_PIN, (uint8_t)(chksm & 0xFF));
  mp3_send_byte(MP3_OUTPUT_PIN, 0xEF);
}
//END: https://github.com/Robotics-DAI-FMFI-UK/cu-lIllI/blob/master/src/arduino/test-dfplayer/test-dfplayer.ino
